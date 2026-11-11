# 编辑器命令协议 COMMANDS

本文档是 `EditorBridge.nativeEditorCommand(String)` 与原生命令层 `editor_command.cpp` 之间的协议契约。
Java 侧入口见 `editor/EditorBridge.java`, 回复解析见 `editor/EditorReply.java`。

## 一、总体格式

| 项 | 约定 |
|:---:|:---|
| 请求 | 一行文本, 首个空白前是动词, 动词不区分大小写, 其后是参数 |
| 响应 | 多行文本, 每行 `key=value`, 行序稳定 |
| 首行 | 恒为 `code=...` |
| `code=0` | 成功 |
| `code=1` | 命令被拒绝, `message` 给出中文原因 |
| `code=2` | 未知命令或空命令 |
| `message` | 每个响应都携带, 供界面提示条直接展示 |
| 列表 | 同一个键重复多行, Java 侧用 `EditorReply.all` 读取 |
| 三元组 | 逗号分隔的 `x,y,z`, Java 侧用 `EditorReply.triple` 读取 |
| 字段转义 | 值内的 `|` 与换行替换为 `/`, 保证按行按竖线切分不错位 |
| 空值 | 缺失的键按空串处理, Java 侧 `first` 返回空串 | |

## 二、状态存储设计

命令层状态保存在 `editor_command.cpp` 的进程内单例 `CommandState`, Java 主线程串行调用, 不加锁:

| 字段 | 说明 |
|:---:|:---|
| `initialized` 与 `dataPath` | `init` 记录的应用内部存储目录 |
| `project` | 打开的 `Project` 对象, 命令层唯一持有, `open` 时整体替换 |
| `tool` | `place` 或 `remove` |
| `paletteIndex` 与 `paletteKey` | 当前调色板序号与键, `open` 时按工程描述恢复 |
| `cursorX/Y/Z` | 命令层光标, `apply` 与 `sel` 的目标 |
| `selectionA/B` 与 `selectionSet` | 框选两角点, `sel b` 时置位 |
| `undo` | 命令层自己的撤销栈, 与编辑器外壳同一套 `UndoStack` 类型 |

与编辑器外壳 (`EditorShell`) 的关系: 外壳服务世界内编辑, 拥有世界与渲染对象;
命令层服务 Java 界面的工程管理型编辑, 只写工程数据。两条路径共享 `Project` 的磁盘格式,
不共享内存对象。同一工程先经命令层修改再从外壳进入时, 外壳会重新加载工程目录, 数据一致。

## 三、命令清单

| 命令 | 作用 | 需要工程 |
|:---:|:---|:---:|
| `ping` | 探针, 验证符号已接线 | 否 |
| `init <dataPath>` | 记录存储目录, 初始化命令层 | 否 |
| `status` | 回填界面全部状态字段 | 否 |
| `projects` | 列出工程根目录下全部工程 | 否 |
| `new <name>` | 在工程根目录创建工程 | 否 |
| `open <dir>` | 加载工程目录 | 否 |
| `save` | 保存当前工程 | 是 |
| `tool <token>` | 切换工具, `place` 或 `remove` | 是 |
| `block <key>` | 按键选中方块 | 是 |
| `cursor <x> <y> <z>` | 移动命令层光标 | 是 |
| `apply` | 在光标处执行当前工具 | 是 |
| `sel a` / `sel b` / `sel clear` | 标记选区角点或清空 | 是 |
| `fill` | 框选范围填充当前方块 | 是 |
| `undo` / `redo` | 撤销与重做一批编辑 | 是 |
| `export <target>` | 导出可玩世界, 目前只支持 `playtest` | 是 |
| `palette` | 方块调色板清单 | 否 |
| `triggers` | 触发器清单 | 是 |
| `trigger_add <event>` | 新建触发器 | 是 |
| `trigger_remove <id>` | 删除触发器 | 是 |
| `trigger_run <id>` | 回放触发器动作摘要 | 是 |

## 四、输入输出明细

### ping

输入: `ping`

输出:

```
code=0
message=pong
```

### init

输入: `init <dataPath>`, 参数为应用内部存储目录, 允许为空串。

输出: `code`, `message`, `dataPath`, `root` (`<dataPath>/editor`)。

### status

输入: `status`。

输出字段:

| 键 | 说明 |
|:---:|:---|
| `open` | 已打开工程为 1 |
| `project` | 工程目录, 未打开时缺失 |
| `name` / `seed` / `chunks` / `triggers` / `edits` / `dirty` | 工程元信息, 未打开时缺失 |
| `tool` | `place` 或 `remove` |
| `block` | 当前方块键, 供 `EditorPaletteView.setSelected` |
| `blockLabel` | 当前方块中文名, 供属性面板 |
| `blockId` | 当前方块客户端 id |
| `cursor` | `x,y,z` 三元组 |
| `selectionSet` | 已框选为 1 |
| `selectionA` / `selectionB` / `selectionSize` | 选区三元组, `selectionSet=1` 时携带 |
| `undo` / `redo` | 撤销与重做深度 |
| `dataPath` | 当前存储目录 | |

### projects

输入: `projects`。

输出: 每个 `project` 行为 `目录|名称|种子|区块数|触发器数|更新时间`, 按更新时间倒序。
工程清单由 `ProjectRoot` 下的 `project.json` 识别, 损坏的目录跳过。

### new

输入: `new <name>`, 显示名允许中文, 目录名取安全字符加序号去重。

输出: `code`, `message`, `project` (工程目录)。同名不冲突, 每次新建独立目录。

### open

输入: `open <dir>`, 参数为工程目录路径。

输出: `code`, `message`, `project`, `name`, `seed`, `chunks`, `triggers`, `edits`。
成功后命令层状态重置为工程描述记录的选中方块, 工具回到 `place`, 选区清空。

### save

输入: `save`。

输出: `code`, `message`, `chunks`, `edits`。保存会重算差异表, 与地形一致的改动不再落盘。

### tool

输入: `tool place` 或 `tool remove`。

输出: `code`, `message`。未知工具拒绝。

### block

输入: `block <key>`, 键取 `palette` 输出的第一列, 例如 `block stone`。

输出: `code`, `message`。未知键拒绝。

### cursor

输入: `cursor <x> <y> <z>`, 三个整数。`y` 限制在 `0` 到 255。

输出: `code`, `message`。

### apply

输入: `apply`。在光标处执行当前工具: `place` 放置当前方块, `remove` 写入空气。
目标与当前方块相同时成功且无改动, 撤销深度不变。

输出: `code`, `message`, `changed` (本次改动格数, 单格恒为 1), `cursor` (执行位置三元组)。

### sel

输入: `sel a` 记 A 点, `sel b` 记 B 点, `sel clear` 清空。角点取当前光标。

输出: `code`, `message`。记 A 点后 `selectionSet` 归零, 记 B 点后置一。

### fill

输入: `fill`。把选区范围 (A 与 B 的轴对齐包围盒) 内与当前方块不同的格全部替换。
体积上限 `limits::kMaxFillBlocks`, 超出拒绝。选区未设置拒绝。

输出: `code`, `message`, `changed` (本次实际改动格数), `selectionSize` (包围盒尺寸三元组)。

### undo 与 redo

输入: `undo` 或 `redo`。无可撤销或重做内容时拒绝, 深度不变。

输出: `code`, `message`, `undo`, `redo`。

`edits` 语义说明: 工程内存差异表只在保存时按地形重算, 因此撤销后的 `edits` 计数不回退,
界面应以 `undo`/`redo` 深度呈现历史状态, 保存后计数归真。

### export

输入: `export playtest`。目标目前只支持 `playtest`, 写入 `<dataPath>/worlds/<安全名>.world`。
工程没有任何改动时拒绝, 工程格式层没有可导出内容。

输出: `code`, `message`, `world` (导出文件路径)。

### palette

输入: `palette`。

输出: 每个 `block` 行为 `键|中文名|客户端方块id`, 顺序取自生成的参数表, 不含空气。

### triggers

输入: `triggers`。

输出: 每个 `trigger` 行为 `id|名称|事件中文名|动作数|启用`, 顺序为工程内存储顺序。

### trigger_add

输入: `trigger_add <event>`, 事件取 `enter_region`, `block_broken`, `timer` 三者之一。

输出: `code`, `message`, `id` (新触发器 id)。动作默认为一条日志, 上限 `limits::kMaxTriggers`。

### trigger_remove

输入: `trigger_remove <id>`。

输出: `code`, `message`。id 不存在拒绝。

### trigger_run

输入: `trigger_run <id>`。

输出: `code`, `message` (动作摘要), `id`, `script` (生成的脚本源码)。
命令层不接入脚本运行时, 事件驱动执行由编辑器外壳的 `TriggerRuntime` 承担。

## 五、错误与边界行为

| 场景 | 行为 |
|:---:|:---|
| 空命令或纯空白 | `code=2` |
| 未知动词 | `code=2`, message 带动词原文 |
| 未 init 就调用业务命令 | `code=1`, 提示先执行 init |
| 未打开工程就调用编辑命令 | `code=1`, 提示尚未打开工程 |
| 参数缺失或非法整数 | `code=1`, message 说明期望格式 |
| `cursor` 的 y 越界 | `code=1` |
| 工程目录不存在或 project.json 损坏 | `code=1`, message 携带格式层原因 |
| 任何情况下 | 不抛异常, 不输出半行, 失败也有完整 code 与 message |

## 六、单元测试

测试位于 `native/tests/`, 主机构建与运行:

```bash
cmake -S src/native/tests -B src/native/temp/editor-tests-build
cmake --build src/native/temp/editor-tests-build
ctest --test-dir src/native/temp/editor-tests-build --output-on-failure
```

覆盖用例: 探针与未知命令, 未初始化拒绝, init 记录目录, 调色板中文名, 新建与清单,
打开元信息与目录缺失拒绝, `init -> new -> open -> apply -> save -> projects` 全链,
apply 的放置删除与空操作, cursor 校验, 选区与填充, 填充撤销重做, 工具与方块校验,
触发器增删与回放, 导出写盘。
