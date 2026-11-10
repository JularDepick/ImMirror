package com.julardepick.immirror.settings;

import android.content.Context;
import android.view.Gravity;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.julardepick.immirror.R;
import com.julardepick.immirror.account.AccountTheme;

/*
 * 设置行的公共骨架.
 *
 * 标题行依次放名称, 暂未生效标记, 弹性留白与取值, 控件本体由子类追加到标题行或下一行.
 * 本类只负责展示与取值, 写入原生设置与联动刷新由 SettingsActivity 统一处理,
 * 因此三个子类都不直接接触 NativeBridge.
 */
public abstract class SettingControl {

    /* 用户改动回调, 页面收到后写入原生设置并刷新关联项. */
    public interface Listener {
        void onValueEdited(SettingControl control, int value);
    }

    protected final Context context;
    protected final SettingsItem item;
    protected final LinearLayout root;
    protected final LinearLayout header;
    protected final TextView valueView;
    private final Listener listener;
    private boolean binding;

    protected SettingControl(Context context, SettingsItem item, Listener listener) {
        this.context = context;
        this.item = item;
        this.listener = listener;

        root = new LinearLayout(context);
        root.setOrientation(LinearLayout.VERTICAL);
        int vertical = AccountTheme.dp(context, 10f);
        root.setPadding(0, vertical, 0, vertical);

        header = new LinearLayout(context);
        header.setOrientation(LinearLayout.HORIZONTAL);
        header.setGravity(Gravity.CENTER_VERTICAL);

        TextView label = AccountTheme.body(context, item.label(), AccountTheme.COLOR_TEXT, 14f);
        label.setSingleLine(false);
        header.addView(label);

        if (!item.wired()) {
            header.addView(buildBadge(context));
        }

        View spacer = new View(context);
        header.addView(spacer, new LinearLayout.LayoutParams(0, 1, 1f));

        valueView = AccountTheme.body(context, "", AccountTheme.COLOR_ACCENT, 13f);
        valueView.setGravity(Gravity.END);
        header.addView(valueView);

        root.addView(header, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT));
    }

    public final View view() {
        return root;
    }

    public final SettingsItem item() {
        return item;
    }

    /* 外部同步: 记录取值并刷新显示, 不回调监听者, 避免与写入形成回环. */
    public final void update(int value) {
        item.setValue(value);
        binding = true;
        try {
            showValue(value);
        } finally {
            binding = false;
        }
    }

    /* 子类构造结尾调用, 按当前值完成首次显示. */
    protected final void bindInitial() {
        update(item.value());
    }

    /* 用户改动: 记录取值后交给页面写入. */
    protected final void emit(int value) {
        if (binding) {
            return;
        }
        item.setValue(value);
        if (listener != null) {
            listener.onValueEdited(this, value);
        }
    }

    protected abstract void showValue(int value);

    private TextView buildBadge(Context context) {
        TextView badge = AccountTheme.body(context,
                context.getString(R.string.settings_badge_unwired), AccountTheme.COLOR_TEXT_FAINT, 10f);
        int horizontal = AccountTheme.dp(context, 6f);
        int vertical = AccountTheme.dp(context, 2f);
        badge.setPadding(horizontal, vertical, horizontal, vertical);
        badge.setBackground(AccountTheme.stroked(context, AccountTheme.COLOR_FIELD,
                AccountTheme.FIELD_RADIUS_DP, AccountTheme.COLOR_FIELD_STROKE, 1f));
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        params.leftMargin = AccountTheme.dp(context, 8f);
        badge.setLayoutParams(params);
        return badge;
    }
}
