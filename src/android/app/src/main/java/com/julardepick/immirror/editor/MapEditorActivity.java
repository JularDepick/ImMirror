package com.julardepick.immirror.editor;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Typeface;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.julardepick.immirror.PlayActivity;
import com.julardepick.immirror.account.AccountTheme;
import com.julardepick.immirror.account.NoticeBar;
import com.julardepick.immirror.ui.MobilePage;

import java.util.ArrayList;
import java.util.EnumMap;
import java.util.List;
import java.util.Map;

/*
 * 地图编辑器主界面.
 *
 * 页面由四块组成: 工程卡片负责新建, 打开与切换工程; 工具选择栏决定放置, 删除与
 * 框选填充三种手势; 属性面板显示光标, 选区与撤销深度; 底部动作行提供保存, 载入,
 * 试玩与返回. 触发器卡片列出工程内全部触发器, 支持新增模板, 试跑与删除.
 *
 * 编辑动作全部走"一条命令一次刷新": 每次点击把命令发给原生编辑器, 随后重新拉一次
 * status 回填界面, 因此界面不缓存任何编辑状态, 原生侧始终是唯一事实来源.
 * 全部提示使用页面内提示条, 不使用 Toast 与系统弹窗; 删除触发器采用按钮二次确认,
 * 三秒内没有再点即回到初始文案.
 */
public final class MapEditorActivity extends Activity implements
        EditorPaletteView.Listener,
        EditorPropertyPanel.Listener {

    private static final String TAG = "ImMirrorEditorUi";

    /* 坐标步进一次挪动的格子数, 单格步进配合长按即可覆盖建筑常用范围. */
    private static final int CURSOR_STEP = 1;

    private final Handler handler = new Handler(Looper.getMainLooper());

    private final Map<EditorTool, Button> toolButtons = new EnumMap<>(EditorTool.class);
    private final List<Button> projectButtons = new ArrayList<>();
    private final List<Button> triggerButtons = new ArrayList<>();

    private TextView summaryView;
    private TextView projectHintView;
    private TextView triggerHintView;
    private TextView logView;
    private LinearLayout projectHost;
    private LinearLayout triggerHost;
    private LinearLayout toolRow;
    private EditText projectNameField;
    private EditorPaletteView paletteView;
    private EditorPropertyPanel propertyPanel;
    private NoticeBar noticeBar;
    private Button saveButton;
    private Button loadButton;
    private Button playtestButton;

    private EditorReply lastStatus = EditorReply.unavailable("");
    private EditorTool currentTool = EditorTool.PLACE;
    private int[] cursor = new int[]{0, 64, 0};
    private String openedProject = "";

    private String pendingDeleteId = "";
    private Button pendingDeleteButton;

    /* 最近一次工程清单的条目数, 摘要行与工程卡片共用. */
    private int projectCount;

    private final Runnable deleteTimeout = new Runnable() {
        @Override
        public void run() {
            resetPendingDelete();
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(buildPage());

        boolean available = EditorBridge.available();
        Log.i(TAG, "editor ui opened, native=" + available);
        if (available) {
            EditorBridge.init(getFilesDir().getAbsolutePath());
            refreshAll();
        } else {
            showNotice(EditorText.NATIVE_UNAVAILABLE, AccountTheme.COLOR_ERROR);
            bindStatus(EditorReply.unavailable(EditorText.NATIVE_UNAVAILABLE));
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (EditorBridge.available() && !openedProject.isEmpty()) {
            /* 从试玩返回后重新拉一次状态, 工程可能被游戏侧改过存档目录. */
            refreshStatusOnly();
        }
    }

    @Override
    protected void onDestroy() {
        handler.removeCallbacks(deleteTimeout);
        if (noticeBar != null) {
            noticeBar.clear();
        }
        super.onDestroy();
    }

    /* EditorPaletteView.Listener */
    @Override
    public void onBlockSelected(EditorPaletteView.Entry entry) {
        EditorReply reply = EditorBridge.selectBlock(entry.key());
        if (!reply.ok()) {
            showNotice(EditorText.commandFailed(entry.label(), reply.message()),
                    AccountTheme.COLOR_ERROR);
            return;
        }
        paletteView.setSelected(entry.key());
        showNotice(entry.label(), AccountTheme.COLOR_SUCCESS);
        refreshStatusOnly();
    }

    /* EditorPropertyPanel.Listener */
    @Override
    public void onStepCursor(int axis, int delta) {
        int[] next = new int[]{cursor[0], cursor[1], cursor[2]};
        next[axis] += delta * CURSOR_STEP;
        if (next[1] < 0) {
            next[1] = 0;
        }
        if (next[1] > 255) {
            next[1] = 255;
        }
        applyCommand(EditorBridge.setCursor(next[0], next[1], next[2]), EditorText.PANEL_TITLE);
    }

    @Override
    public void onMarkPoint(boolean second) {
        applyCommand(EditorBridge.markSelection(second),
                second ? EditorText.POINT_B : EditorText.POINT_A);
    }

    @Override
    public void onClearSelection() {
        applyCommand(EditorBridge.clearSelection(), EditorText.SELECTION_CLEAR);
    }

    @Override
    public void onFillSelection() {
        applyCommand(EditorBridge.fillSelection(), EditorText.SELECTION_FILL);
    }

    @Override
    public void onUndo() {
        applyCommand(EditorBridge.undo(), EditorText.ACTION_UNDO);
    }

    @Override
    public void onRedo() {
        applyCommand(EditorBridge.redo(), EditorText.ACTION_REDO);
    }

    /* 一次编辑动作的统一收尾: 失败提示原因, 成功后重新拉状态. */
    private void applyCommand(EditorReply reply, String action) {
        if (!reply.ok()) {
            showNotice(EditorText.commandFailed(action, reply.message()),
                    AccountTheme.COLOR_ERROR);
            return;
        }
        if (reply.message() != null && !reply.message().isEmpty()) {
            showNotice(reply.message(), AccountTheme.COLOR_SUCCESS);
        }
        refreshStatusOnly();
    }

    private void refreshAll() {
        /* 先刷工程列表, 摘要里的工程数量与状态回填共用同一次结果. */
        rebuildProjects(EditorBridge.listProjects());
        rebuildPalette(EditorBridge.palette());
        bindStatus(EditorBridge.status());
        rebuildTriggers(EditorBridge.triggers());
    }

    private void refreshStatusOnly() {
        bindStatus(EditorBridge.status());
        rebuildTriggers(EditorBridge.triggers());
    }

    /* 把原生状态回填到页面的各块区域. */
    private void bindStatus(EditorReply status) {
        lastStatus = status == null ? EditorReply.unavailable("") : status;
        boolean open = lastStatus.intValue("open", 0) != 0;
        openedProject = open ? lastStatus.first("project") : "";

        int[] reportedCursor = lastStatus.triple("cursor");
        if (reportedCursor != null) {
            cursor = reportedCursor;
        }
        currentTool = EditorTool.fromToken(lastStatus.first("tool"), currentTool);
        paletteView.setSelected(lastStatus.first("block"));
        paletteView.setVisibility(open ? View.VISIBLE : View.GONE);
        propertyPanel.setVisibility(open ? View.VISIBLE : View.GONE);
        propertyPanel.bind(lastStatus);

        rebindToolButtons(open);
        saveButton.setEnabled(open);
        loadButton.setEnabled(open);
        playtestButton.setEnabled(open);

        String title = open
                ? lastStatus.first("title", openedProject)
                : "未打开";
        summaryView.setText(String.format(EditorText.SUMMARY_FORMAT, projectCount, title));
        updateLog(lastStatus);
    }

    private void rebindToolButtons(boolean open) {
        for (Map.Entry<EditorTool, Button> entry : toolButtons.entrySet()) {
            Button button = entry.getValue();
            button.setEnabled(open);
            boolean selected = entry.getKey() == currentTool;
            button.setTextColor(selected ? AccountTheme.COLOR_ACCENT_TEXT
                    : AccountTheme.COLOR_TEXT_MUTED);
            button.setBackground(selected
                    ? AccountTheme.stroked(this, AccountTheme.COLOR_ACCENT, 12f,
                            AccountTheme.COLOR_ACCENT, 1f)
                    : AccountTheme.stroked(this, AccountTheme.COLOR_FIELD, 12f,
                            AccountTheme.COLOR_FIELD_STROKE, 1f));
        }
    }

    private void selectTool(EditorTool tool) {
        EditorReply reply = EditorBridge.selectTool(tool);
        if (!reply.ok()) {
            showNotice(EditorText.commandFailed(tool.label(), reply.message()),
                    AccountTheme.COLOR_ERROR);
            return;
        }
        currentTool = tool;
        refreshStatusOnly();
    }

    private void rebuildProjects(EditorReply reply) {
        projectHost.removeAllViews();
        projectButtons.clear();
        List<String> rows = reply.all("project");
        projectCount = rows.size();
        projectHintView.setVisibility(rows.isEmpty() ? View.VISIBLE : View.GONE);
        for (String row : rows) {
            String[] fields = EditorReply.fields(row);
            if (fields.length < 5) {
                continue;
            }
            final String directory = fields[0];
            String label = String.format(EditorText.PROJECT_ROW_FORMAT, fields[1],
                    parseInt(fields[2], 0), parseInt(fields[3], 0));
            Button button = AccountTheme.outlineButton(this, label, AccountTheme.COLOR_TEXT);
            button.setOnClickListener(view -> openProject(directory));
            projectHost.addView(button, new LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.MATCH_PARENT,
                    AccountTheme.dp(this, AccountTheme.BUTTON_HEIGHT_DP)));
            projectButtons.add(button);
        }
    }

    private void rebuildPalette(EditorReply reply) {
        List<EditorPaletteView.Entry> entries = new ArrayList<>();
        for (String row : reply.all("block")) {
            String[] fields = EditorReply.fields(row);
            if (fields.length < 3) {
                continue;
            }
            entries.add(new EditorPaletteView.Entry(fields[0], fields[1],
                    parseInt(fields[2], 0)));
        }
        paletteView.setEntries(entries);
    }

    private void rebuildTriggers(EditorReply reply) {
        triggerHost.removeAllViews();
        triggerButtons.clear();
        List<String> rows = reply.all("trigger");
        triggerHintView.setVisibility(rows.isEmpty() ? View.VISIBLE : View.GONE);
        for (String row : rows) {
            String[] fields = EditorReply.fields(row);
            if (fields.length < 5) {
                continue;
            }
            final String id = fields[0];
            String label = String.format(EditorText.TRIGGER_ROW_FORMAT, fields[1], fields[2],
                    parseInt(fields[3], 0));
            addTriggerRow(id, label);
        }
        if (pendingDeleteId.isEmpty()) {
            pendingDeleteButton = null;
        }
    }

    private void addTriggerRow(String id, String label) {
        LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setGravity(Gravity.CENTER_VERTICAL);

        TextView name = AccountTheme.body(this, label, AccountTheme.COLOR_TEXT, 16f);
        row.addView(name, new LinearLayout.LayoutParams(0,
                LinearLayout.LayoutParams.WRAP_CONTENT, 1f));

        Button run = AccountTheme.outlineButton(this, EditorText.TRIGGER_RUN,
                AccountTheme.COLOR_ACCENT);
        run.setOnClickListener(view -> runTrigger(id));
        LinearLayout.LayoutParams runParams = new LinearLayout.LayoutParams(
                AccountTheme.dp(this, 62f), AccountTheme.dp(this, 38f));
        runParams.leftMargin = AccountTheme.dp(this, 6f);
        row.addView(run, runParams);

        Button remove = AccountTheme.outlineButton(this, EditorText.TRIGGER_DELETE,
                AccountTheme.COLOR_DANGER);
        remove.setOnClickListener(view -> requestDeleteTrigger(id, remove));
        LinearLayout.LayoutParams removeParams = new LinearLayout.LayoutParams(
                AccountTheme.dp(this, 82f), AccountTheme.dp(this, 38f));
        removeParams.leftMargin = AccountTheme.dp(this, 6f);
        row.addView(remove, removeParams);

        triggerHost.addView(row, AccountTheme.block(this, 4f));
        triggerButtons.add(run);
        triggerButtons.add(remove);
    }

    /* 删除触发器: 第一次点击把按钮换成确认文案, 三秒内再点才真正删除. */
    private void requestDeleteTrigger(String id, Button button) {
        if (!pendingDeleteId.equals(id)) {
            resetPendingDelete();
            pendingDeleteId = id;
            pendingDeleteButton = button;
            button.setText(EditorText.TRIGGER_DELETE + "?");
            button.setTextColor(AccountTheme.COLOR_ERROR);
            handler.postDelayed(deleteTimeout, EditorText.CONFIRM_TIMEOUT_MS);
            return;
        }
        handler.removeCallbacks(deleteTimeout);
        pendingDeleteId = "";
        pendingDeleteButton = null;
        button.setText(EditorText.TRIGGER_DELETE);
        applyCommand(EditorBridge.removeTrigger(id), EditorText.TRIGGER_DELETE);
    }

    private void resetPendingDelete() {
        if (pendingDeleteButton != null) {
            pendingDeleteButton.setText(EditorText.TRIGGER_DELETE);
            pendingDeleteButton.setTextColor(AccountTheme.COLOR_DANGER);
        }
        pendingDeleteId = "";
        pendingDeleteButton = null;
    }

    private void runTrigger(String id) {
        EditorReply reply = EditorBridge.runTrigger(id);
        String outcome = reply.message();
        showNotice(String.format(EditorText.TRIGGER_RUN_FORMAT, id,
                        outcome == null || outcome.isEmpty() ? "无返回" : outcome),
                reply.ok() ? AccountTheme.COLOR_SUCCESS : AccountTheme.COLOR_ERROR);
        refreshStatusOnly();
    }

    private void addTrigger(String eventType) {
        EditorReply reply = EditorBridge.addTrigger(eventType);
        applyCommand(reply, EditorText.TRIGGER_ADDED);
    }

    private void createProject() {
        String name = projectNameField.getText().toString().trim();
        if (name.isEmpty()) {
            showNotice(EditorText.PROJECT_NAME_EMPTY, AccountTheme.COLOR_ERROR);
            return;
        }
        EditorReply reply = EditorBridge.createProject(name);
        if (!reply.ok()) {
            showNotice(String.format(EditorText.PROJECT_CREATE_FAILED, reply.message()),
                    AccountTheme.COLOR_ERROR);
            return;
        }
        projectNameField.setText("");
        showNotice(String.format(EditorText.PROJECT_CREATED, name), AccountTheme.COLOR_SUCCESS);
        refreshAll();
    }

    private void openProject(String directory) {
        EditorReply reply = EditorBridge.openProject(directory);
        if (!reply.ok()) {
            showNotice(String.format(EditorText.PROJECT_OPEN_FAILED, reply.message()),
                    AccountTheme.COLOR_ERROR);
            return;
        }
        showNotice(String.format(EditorText.PROJECT_OPENED, directory),
                AccountTheme.COLOR_SUCCESS);
        resetPendingDelete();
        refreshAll();
    }

    private void saveProject() {
        if (openedProject.isEmpty()) {
            showNotice(EditorText.NO_PROJECT, AccountTheme.COLOR_ERROR);
            return;
        }
        EditorReply reply = EditorBridge.saveProject();
        if (!reply.ok()) {
            showNotice(String.format(EditorText.SAVE_FAILED, reply.message()),
                    AccountTheme.COLOR_ERROR);
            return;
        }
        showNotice(EditorText.SAVE_OK, AccountTheme.COLOR_SUCCESS);
        refreshAll();
    }

    private void loadProject() {
        if (openedProject.isEmpty()) {
            showNotice(EditorText.NO_PROJECT, AccountTheme.COLOR_ERROR);
            return;
        }
        EditorReply reply = EditorBridge.openProject(openedProject);
        if (!reply.ok()) {
            showNotice(String.format(EditorText.LOAD_FAILED, reply.message()),
                    AccountTheme.COLOR_ERROR);
            return;
        }
        showNotice(EditorText.LOAD_OK, AccountTheme.COLOR_SUCCESS);
        refreshAll();
    }

    /* 试玩: 先把工程导出成可玩世界, 再带标记进入游戏界面. */
    private void startPlaytest() {
        if (openedProject.isEmpty()) {
            showNotice(EditorText.NO_PROJECT, AccountTheme.COLOR_ERROR);
            return;
        }
        EditorReply reply = EditorBridge.exportWorld("playtest");
        if (!reply.ok()) {
            showNotice(String.format(EditorText.PLAYTEST_FAILED, reply.message()),
                    AccountTheme.COLOR_ERROR);
            return;
        }
        showNotice(EditorText.PLAYTEST_OK, AccountTheme.COLOR_SUCCESS);
        Intent intent = new Intent(this, PlayActivity.class);
        intent.putExtra(EditorBridge.EXTRA_PLAYTEST, openedProject);
        startActivity(intent);
    }

    private void updateLog(EditorReply status) {
        List<String> lines = status == null ? new ArrayList<String>() : status.all("log");
        if (lines.isEmpty()) {
            logView.setText(EditorText.LOG_EMPTY);
            return;
        }
        StringBuilder builder = new StringBuilder();
        for (String line : lines) {
            if (builder.length() > 0) {
                builder.append('\n');
            }
            builder.append(line);
        }
        logView.setText(builder.toString());
    }

    private void showNotice(CharSequence message, int color) {
        if (noticeBar != null) {
            noticeBar.show(message, color);
        }
    }

    private static int parseInt(String text, int fallback) {
        try {
            return Integer.parseInt(text.trim());
        } catch (NumberFormatException error) {
            return fallback;
        }
    }

    private View buildPage() {
        /*
         * 面板内纵向内容: 说明行, 摘要行, 工程卡片, 工具与调色板, 属性面板,
         * 触发器卡片, 日志卡片, 底部动作行与作者页脚.
         * 标题由面板标题行承担, 内容在面板内部滚动.
         */
        LinearLayout column = new LinearLayout(this);
        column.setOrientation(LinearLayout.VERTICAL);
        column.setGravity(Gravity.CENTER_HORIZONTAL);
        /* 底部留出提示条的高度, 滚到末尾时动作行不会被提示条盖住. */
        column.setPadding(0, 0, 0, AccountTheme.dp(this, 84f));

        column.addView(AccountTheme.subtitle(this, EditorText.PAGE_SUBTITLE),
                AccountTheme.block(this, 4f));

        summaryView = AccountTheme.body(this, "", AccountTheme.COLOR_TEXT_FAINT, 12f);
        summaryView.setGravity(Gravity.CENTER);
        column.addView(summaryView, AccountTheme.block(this, 8f));

        column.addView(buildProjectCard(), cardParams(10f));

        toolRow = buildCard(EditorText.TOOL_TITLE);
        buildToolButtons(toolRow);
        column.addView(toolRow, cardParams(12f));

        paletteView = new EditorPaletteView(this, this);
        paletteView.setVisibility(View.GONE);
        column.addView(paletteView, cardParams(12f));

        propertyPanel = new EditorPropertyPanel(this, this);
        propertyPanel.setVisibility(View.GONE);
        column.addView(propertyPanel, cardParams(12f));

        column.addView(buildTriggerCard(), cardParams(12f));
        column.addView(buildLogCard(), cardParams(12f));
        column.addView(buildActionRow(), cardParams(18f));

        TextView author = AccountTheme.body(this,
                String.format(EditorText.AUTHOR_FORMAT, AccountTheme.AUTHOR_NAME),
                AccountTheme.COLOR_TEXT_FAINT, 11f);
        author.setGravity(Gravity.CENTER);
        column.addView(author, AccountTheme.block(this, 14f));

        /* 外层: 场景底图 + 遮罩 + 居中面板, 标题与关闭按钮由面板标题行承担. */
        FrameLayout root = new FrameLayout(this);
        root.addView(MobilePage.create(this, EditorText.PAGE_TITLE,
                MobilePage.scrollable(this, column)), new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT));

        /* 提示条浮在面板底部, 不参与内容滚动. */
        TextView noticeView = AccountTheme.notice(this);
        noticeBar = new NoticeBar(noticeView);
        FrameLayout.LayoutParams noticeParams = new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.WRAP_CONTENT,
                Gravity.BOTTOM);
        int margin = AccountTheme.dp(this, 16f);
        noticeParams.setMargins(margin, 0, margin, margin);
        root.addView(noticeView, noticeParams);
        return root;
    }

    private View buildProjectCard() {
        LinearLayout card = buildCard(EditorText.PROJECT_TITLE);

        projectHintView = AccountTheme.body(this, EditorText.PROJECT_EMPTY,
                AccountTheme.COLOR_TEXT_FAINT, 13f);
        projectHintView.setVisibility(View.GONE);
        card.addView(projectHintView, AccountTheme.block(this, 4f));

        projectHost = new LinearLayout(this);
        projectHost.setOrientation(LinearLayout.VERTICAL);
        card.addView(projectHost, AccountTheme.block(this, 4f));

        LinearLayout newRow = new LinearLayout(this);
        newRow.setOrientation(LinearLayout.HORIZONTAL);
        newRow.setGravity(Gravity.CENTER_VERTICAL);

        projectNameField = AccountTheme.field(this, EditorText.PROJECT_NEW_HINT, 16f);
        newRow.addView(projectNameField, new LinearLayout.LayoutParams(0,
                LinearLayout.LayoutParams.WRAP_CONTENT, 1f));

        Button create = AccountTheme.primaryButton(this, EditorText.ACTION_CREATE);
        create.setOnClickListener(view -> createProject());
        LinearLayout.LayoutParams createParams = new LinearLayout.LayoutParams(
                AccountTheme.dp(this, 92f), AccountTheme.dp(this, AccountTheme.BUTTON_HEIGHT_DP));
        createParams.leftMargin = AccountTheme.dp(this, 8f);
        newRow.addView(create, createParams);
        card.addView(newRow, AccountTheme.block(this, 8f));

        Button refresh = AccountTheme.outlineButton(this, EditorText.ACTION_REFRESH,
                AccountTheme.COLOR_TEXT_MUTED);
        refresh.setOnClickListener(view -> refreshAll());
        card.addView(refresh, AccountTheme.block(this, 6f));
        return card;
    }

    private void buildToolButtons(LinearLayout card) {
        LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.HORIZONTAL);
        EditorTool[] tools = EditorTool.values();
        for (int index = 0; index < tools.length; index++) {
            EditorTool tool = tools[index];
            Button button = AccountTheme.outlineButton(this, tool.label(),
                    AccountTheme.COLOR_TEXT_MUTED);
            button.setOnClickListener(view -> selectTool(tool));
            LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(0,
                    AccountTheme.dp(this, AccountTheme.BUTTON_HEIGHT_DP), 1f);
            if (index > 0) {
                params.leftMargin = AccountTheme.dp(this, 8f);
            }
            row.addView(button, params);
            toolButtons.put(tool, button);
        }
        card.addView(row, AccountTheme.block(this, 4f));
    }

    private View buildTriggerCard() {
        LinearLayout card = buildCard(EditorText.TRIGGER_TITLE);

        triggerHintView = AccountTheme.body(this, EditorText.TRIGGER_EMPTY,
                AccountTheme.COLOR_TEXT_FAINT, 13f);
        triggerHintView.setVisibility(View.GONE);
        card.addView(triggerHintView, AccountTheme.block(this, 4f));

        triggerHost = new LinearLayout(this);
        triggerHost.setOrientation(LinearLayout.VERTICAL);
        card.addView(triggerHost, AccountTheme.block(this, 4f));

        LinearLayout addRow = new LinearLayout(this);
        addRow.setOrientation(LinearLayout.HORIZONTAL);
        String[] events = {"enter_region", "break_block", "timer"};
        String[] labels = {EditorText.TRIGGER_ADD_ENTER, EditorText.TRIGGER_ADD_BREAK,
                EditorText.TRIGGER_ADD_TIMER};
        for (int index = 0; index < events.length; index++) {
            final String eventType = events[index];
            Button button = AccountTheme.outlineButton(this, labels[index],
                    AccountTheme.COLOR_ACCENT);
            button.setOnClickListener(view -> addTrigger(eventType));
            LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(0,
                    AccountTheme.dp(this, AccountTheme.BUTTON_HEIGHT_DP), 1f);
            if (index > 0) {
                params.leftMargin = AccountTheme.dp(this, 8f);
            }
            addRow.addView(button, params);
        }
        card.addView(addRow, AccountTheme.block(this, 8f));
        return card;
    }

    private View buildLogCard() {
        LinearLayout card = buildCard(EditorText.LOG_TITLE);
        logView = AccountTheme.body(this, EditorText.LOG_EMPTY, AccountTheme.COLOR_TEXT_MUTED,
                13f);
        logView.setLineSpacing(AccountTheme.dp(this, 3f), 1f);
        card.addView(logView, AccountTheme.block(this, 4f));
        return card;
    }

    private View buildActionRow() {
        LinearLayout column = new LinearLayout(this);
        column.setOrientation(LinearLayout.VERTICAL);

        LinearLayout first = new LinearLayout(this);
        first.setOrientation(LinearLayout.HORIZONTAL);
        saveButton = AccountTheme.primaryButton(this, EditorText.ACTION_SAVE);
        saveButton.setOnClickListener(view -> saveProject());
        first.addView(saveButton, new LinearLayout.LayoutParams(0,
                AccountTheme.dp(this, AccountTheme.BUTTON_HEIGHT_DP), 1f));

        loadButton = AccountTheme.outlineButton(this, EditorText.ACTION_LOAD,
                AccountTheme.COLOR_ACCENT);
        loadButton.setOnClickListener(view -> loadProject());
        LinearLayout.LayoutParams loadParams = new LinearLayout.LayoutParams(0,
                AccountTheme.dp(this, AccountTheme.BUTTON_HEIGHT_DP), 1f);
        loadParams.leftMargin = AccountTheme.dp(this, 10f);
        first.addView(loadButton, loadParams);
        column.addView(first);

        LinearLayout second = new LinearLayout(this);
        second.setOrientation(LinearLayout.HORIZONTAL);
        playtestButton = AccountTheme.outlineButton(this, EditorText.ACTION_PLAYTEST,
                AccountTheme.COLOR_SUCCESS);
        playtestButton.setOnClickListener(view -> startPlaytest());
        second.addView(playtestButton, new LinearLayout.LayoutParams(0,
                AccountTheme.dp(this, AccountTheme.BUTTON_HEIGHT_DP), 1f));

        Button back = AccountTheme.outlineButton(this, EditorText.ACTION_BACK,
                AccountTheme.COLOR_TEXT_MUTED);
        back.setOnClickListener(view -> finish());
        LinearLayout.LayoutParams backParams = new LinearLayout.LayoutParams(0,
                AccountTheme.dp(this, AccountTheme.BUTTON_HEIGHT_DP), 1f);
        backParams.leftMargin = AccountTheme.dp(this, 10f);
        second.addView(back, backParams);

        column.addView(second, AccountTheme.block(this, 10f));
        return column;
    }

    /* 卡片外壳: 白底面板 + 加粗标题, 与设置界面同一套卡片样式. */
    private LinearLayout buildCard(String titleText) {
        LinearLayout card = new LinearLayout(this);
        card.setOrientation(LinearLayout.VERTICAL);
        int horizontal = AccountTheme.dp(this, 18f);
        int vertical = AccountTheme.dp(this, 14f);
        card.setPadding(horizontal, vertical, horizontal, vertical);
        card.setBackground(AccountTheme.panelBackground(this));

        TextView title = AccountTheme.body(this, titleText, AccountTheme.COLOR_ACCENT, 15f);
        title.setTypeface(title.getTypeface(), Typeface.BOLD);
        card.addView(title);
        return card;
    }

    /* 卡片宽度由面板内容区决定, 这里只给与上一块之间的间距. */
    private LinearLayout.LayoutParams cardParams(float topMarginDp) {
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        params.topMargin = AccountTheme.dp(this, topMarginDp);
        return params;
    }
}
