# 界面素材命名与来源

本目录的 PNG 全部由 `single-branch/client/temp/ui_extract/ui_extract.py` 从原版界面图集导出,
文件名按本项目用途重新拟定, 不使用原版文件名与专有词。索引数据在同目录的 `ui_manifest.json`,
其中 `insets` 是渲染九宫时四边不参与拉伸的像素宽度。

## 一. 命名规则

| 规则 | 说明 |
|:---:|:---|
| 类别目录 | 按控件用途分 `button` `panel` `dialog` `tab` `slot` `icon` `misc` 七类 |
| 文件名 | 全小写蛇形, 形如 `<语义>_<变体>.png`, 语义取本项目用语 |
| 禁止词 | 原版专有词与品牌词 (原版迷你前缀, 渠道名, 联名 IP 名) 一律不出现 |
| 状态后缀 | `_on` 表示选中或按下, `_off` 表示未选, `_blue` `_gold` `_red` 一类的色名表示同一控件的配色变体 |

## 二. 素材清单

| 文件 | 类别 | 尺寸 | 拉伸模式 | 九宫边界 (左/上/右/下) | 原版图集页 | 原版精灵 | 用途 |
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---|
| `button/alert_orange.png` | 按钮 | 158x61 | 三片横向 | 45/0/46/0 | `texture2/common.ktx` | `btn_big_orange` | 提醒按钮 158x61, 橙底黑边 |
| `button/circle_blue.png` | 按钮 | 50x53 | 整体拉伸 | - | `texture2/common.ktx` | `btn_circle_blue` | 圆形按钮 50x53 |
| `button/circle_gray.png` | 按钮 | 41x44 | 整体拉伸 | - | `texture2/common.ktx` | `btn_circle_gray` | 圆形按钮灰态 |
| `button/compact_blue.png` | 按钮 | 40x43 | 三片横向 | 15/0/15/0 | `texture2/common.ktx` | `btn_mini_blue` | 小号按钮 40x43, 蓝底 |
| `button/compact_orange.png` | 按钮 | 40x43 | 三片横向 | 15/0/15/0 | `texture2/common.ktx` | `btn_mini_orange` | 小号按钮 40x43, 橙底黑边 |
| `button/danger_red.png` | 按钮 | 182x61 | 三片横向 | 45/0/46/0 | `texture2/common.ktx` | `btn_big_red` | 危险操作按钮 182x61, 红底黑边 |
| `button/ghost_white.png` | 按钮 | 96x61 | 三片横向 | 35/0/35/0 | `texture2/common.ktx` | `btn_big_empty` | 空底按钮 96x61, 灰白描边 |
| `button/medium_blue.png` | 按钮 | 130x50 | 三片横向 | 57/0/57/0 | `texture2/common.ktx` | `btn_middle_blue` | 中号按钮 130x50, 蓝底 |
| `button/medium_gold.png` | 按钮 | 130x50 | 三片横向 | 57/0/57/0 | `texture2/common.ktx` | `btn_middle_yellow` | 中号按钮 130x50, 金黄底 |
| `button/medium_red.png` | 按钮 | 130x50 | 三片横向 | 57/0/57/0 | `texture2/common.ktx` | `btn_middle_red` | 中号按钮 130x50, 红底 |
| `button/primary_blue.png` | 按钮 | 96x61 | 三片横向 | 35/0/35/0 | `texture2/common.ktx` | `btn_big_blue` | 主行动按钮 182x61, 蓝底黑边 |
| `button/primary_gold.png` | 按钮 | 182x61 | 三片横向 | 45/0/46/0 | `texture2/common.ktx` | `btn_big_yellow` | 次级行动按钮 182x61, 金黄底黑边 |
| `button/wide_blue.png` | 按钮 | 70x81 | 三片横向 | 30/0/30/0 | `texture2/common.ktx` | `btn_superbig_blue` | 超宽按钮 70x81 九宫 |
| `dialog/body_white.png` | 弹窗与提示 | 300x90 | 九宫 | 100/30/100/30 | `texture2/messagebox.ktx` | `board_clip` | 对话框主体 300x90, 九宫 100x30 |
| `dialog/tips_arrow.png` | 弹窗与提示 | 10x17 | 整体拉伸 | - | `texture2/messagebox.ktx` | `board_tips_arrow` | 提示条指向箭头 10x17 |
| `dialog/tips_body.png` | 弹窗与提示 | 30x30 | 九宫 | 10/10/10/10 | `texture2/messagebox.ktx` | `board_tips_icon` | 提示条底 30x30, 九宫 10 |
| `dialog/tips_explain.png` | 弹窗与提示 | 30x30 | 九宫 | 10/10/10/10 | `texture2/messagebox.ktx` | `board_tips_explain` | 悬浮说明底 30x30 |
| `dialog/title_bar.png` | 弹窗与提示 | 330x67 | 三片横向 | 110/0/110/0 | `texture2/messagebox.ktx` | `board_clip_title` | 对话框标题条 330x67 |
| `icon/arrow_down.png` | 图标 | 89x91 | 整体拉伸 | - | `texture0/operate.ktx` | `icon_down` | 向下 89x91 |
| `icon/arrow_left.png` | 图标 | 45x32 | 整体拉伸 | - | `texture0/operate.ktx` | `icon_last` | 上一页 45x32 |
| `icon/arrow_right.png` | 图标 | 45x32 | 整体拉伸 | - | `texture0/operate.ktx` | `icon_next` | 下一页 45x32 |
| `icon/arrow_up.png` | 图标 | 89x91 | 整体拉伸 | - | `texture0/operate.ktx` | `icon_up` | 向上 89x91 |
| `icon/backpack.png` | 图标 | 54x51 | 整体拉伸 | - | `texture0/operate.ktx` | `icon_backpack` | 背包入口 54x51 |
| `icon/chat.png` | 图标 | 37x37 | 整体拉伸 | - | `texture0/operate.ktx` | `icon_chat` | 聊天入口 37x37 |
| `icon/close.png` | 图标 | 36x46 | 整体拉伸 | - | `texture2/common.ktx` | `btn_close` | 关闭按钮 36x46 |
| `icon/create.png` | 图标 | 38x38 | 整体拉伸 | - | `texture2/common_icon.ktx` | `icon_create_b` | 新建 38x38 |
| `icon/download.png` | 图标 | 38x38 | 整体拉伸 | - | `texture2/common_icon.ktx` | `icon_download_b` | 下载 38x38 |
| `icon/dropdown.png` | 图标 | 20x20 | 整体拉伸 | - | `texture2/common_icon.ktx` | `icon_dropdown_blue` | 下拉箭头 20x20 |
| `icon/expression.png` | 图标 | 51x49 | 整体拉伸 | - | `texture0/operate.ktx` | `icon_expression` | 表情入口 51x49 |
| `icon/female.png` | 图标 | 25x25 | 整体拉伸 | - | `texture2/common_icon.ktx` | `icon_female` | 性别女 25x25 |
| `icon/heart.png` | 图标 | 20x16 | 整体拉伸 | - | `texture0/operate.ktx` | `icon_heart` | 血量图标 20x16 |
| `icon/help.png` | 图标 | 26x37 | 整体拉伸 | - | `texture2/common.ktx` | `btn_help` | 帮助按钮 |
| `icon/horn.png` | 图标 | 31x33 | 整体拉伸 | - | `texture0/operate.ktx` | `icon_horn` | 喇叭 31x33 |
| `icon/joystick.png` | 图标 | 92x92 | 整体拉伸 | - | `texture0/operate.ktx` | `icon_rocker` | 摇杆底盘 92x92 |
| `icon/joystick_core.png` | 图标 | 74x74 | 整体拉伸 | - | `texture0/operate.ktx` | `icon_rocker_circle` | 摇杆杆头 74x74 |
| `icon/jump.png` | 图标 | 117x119 | 整体拉伸 | - | `texture0/operate.ktx` | `icon_jump` | 跳跃键 117x119 |
| `icon/like.png` | 图标 | 38x38 | 整体拉伸 | - | `texture2/common_icon.ktx` | `icon_like_b` | 点赞 38x38 |
| `icon/link.png` | 图标 | 44x44 | 整体拉伸 | - | `texture2/common_icon.ktx` | `icon_link` | 链接 44x44 |
| `icon/male.png` | 图标 | 25x25 | 整体拉伸 | - | `texture2/common_icon.ktx` | `icon_male` | 性别男 25x25 |
| `icon/more.png` | 图标 | 36x10 | 整体拉伸 | - | `texture0/operate.ktx` | `icon_more` | 更多 36x10 |
| `icon/picture.png` | 图标 | 44x44 | 整体拉伸 | - | `texture2/common_icon.ktx` | `icon_picture` | 图片 44x44 |
| `icon/record.png` | 图标 | 38x38 | 整体拉伸 | - | `texture2/common_icon.ktx` | `icon_record_b` | 录制 38x38 |
| `icon/settings.png` | 图标 | 35x34 | 整体拉伸 | - | `texture0/operate.ktx` | `icon_set` | 设置 35x34 |
| `icon/steering_wheel.png` | 图标 | 193x193 | 整体拉伸 | - | `texture0/operate.ktx` | `operating_steering_wheel` | 方向盘 193x193 |
| `icon/switch.png` | 图标 | 45x32 | 整体拉伸 | - | `texture0/operate.ktx` | `icon_switch` | 切换箭头 45x32 |
| `icon/voice.png` | 图标 | 25x34 | 整体拉伸 | - | `texture0/operate.ktx` | `icon_voice` | 语音 25x34 |
| `misc/bar_fill_green.png` | 通用件 | 3x3 | 九宫 | 1/1/1/1 | `texture2/modlib.ktx` | `img_bar_green` | 绿色进度条 3x3 |
| `misc/bar_fill_orange.png` | 通用件 | 12x12 | 九宫 | 4/4/4/4 | `texture0/common.ktx` | `img_bar_orange` | 橙色进度条 12x12 |
| `misc/bar_hp.png` | 通用件 | 15x16 | 整体拉伸 | - | `texture0/operate.ktx` | `img_blood_strip_red` | 血条红色段 15x16 |
| `misc/bar_hp_track.png` | 通用件 | 15x15 | 九宫 | 5/5/5/5 | `texture0/operate.ktx` | `img_blood_strip_board` | 血条底槽 15x15 |
| `misc/bar_track.png` | 通用件 | 6x6 | 九宫 | 2/2/2/2 | `texture0/common.ktx` | `img_board_bar` | 进度条轨道 6x6 |
| `misc/hud_pad.png` | 通用件 | 88x88 | 整体拉伸 | - | `texture0/operate.ktx` | `operating_board_square` | HUD 方形底座 88x88 |
| `misc/line_dark.png` | 通用件 | 6x1 | 三片横向 | 1/0/1/0 | `texture2/common.ktx` | `img_line_drak` | 深色横线 6x1 |
| `misc/line_light.png` | 通用件 | 6x2 | 三片横向 | 2/0/2/0 | `texture0/common.ktx` | `img_line_gary` | 浅灰横线 |
| `misc/line_sink.png` | 通用件 | 6x3 | 三片横向 | 2/0/2/0 | `texture2/common.ktx` | `img_line_sink` | 内凹横线 6x2 |
| `misc/line_vertical.png` | 通用件 | 2x6 | 三片横向 | 2/0/2/0 | `texture0/common.ktx` | `img_line_gary_v` | 竖直分隔线 |
| `misc/progress_step.png` | 通用件 | 9x8 | 三片横向 | 3/0/3/0 | `texture0/operate.ktx` | `img_progress_bar` | 分段进度 9x8 |
| `misc/solid_white.png` | 通用件 | 16x16 | 整体拉伸 | - | `texture0/operate.ktx` | `white` | 纯白 16x16, 纯色与遮罩用 |
| `panel/surface_blue.png` | 面板 | 30x30 | 九宫 | 10/10/10/10 | `texture2/common.ktx` | `board_bottom_blue` | 蓝色底板 30x30 |
| `panel/surface_blue_large.png` | 面板 | 60x60 | 九宫 | 20/20/20/20 | `texture2/common.ktx` | `board_bottom_blue_big` | 大号蓝色底板 60x60, 九宫 20 |
| `panel/surface_gray.png` | 面板 | 30x30 | 九宫 | 10/10/10/10 | `texture0/common.ktx` | `board_more` | 白色底板 30x30, 九宫 10 |
| `panel/surface_list_row.png` | 面板 | 30x30 | 九宫 | 10/10/10/10 | `texture0/common.ktx` | `board_map_info` | 列表条目底色 |
| `panel/surface_round_blue.png` | 面板 | 30x30 | 九宫 | 10/10/10/10 | `texture2/common.ktx` | `board_blue_r` | 圆角蓝色板 30x30 |
| `panel/surface_scrim.png` | 面板 | 15x15 | 九宫 | 5/5/5/5 | `texture0/common.ktx` | `img_bd_black50` | 半透明黑底, 弹窗遮罩 |
| `panel/surface_white.png` | 面板 | 30x30 | 九宫 | 10/10/10/10 | `texture2/common.ktx` | `board_white` | 白色底板, 黑边 3 像素 |
| `slot/avatar_frame.png` | 槽位与勾选 | 30x30 | 九宫 | 10/10/10/10 | `texture0/common.ktx` | `sel_icon_custom` | 头像选择框 |
| `slot/check_off.png` | 槽位与勾选 | 24x24 | 九宫 | 8/8/8/8 | `texture2/common.ktx` | `board_check` | 勾选框 24x24 未选 |
| `slot/check_on.png` | 槽位与勾选 | 24x24 | 九宫 | 8/8/8/8 | `texture2/common.ktx` | `board_check_select_` | 勾选框 24x24 已选 |
| `slot/item_frame.png` | 槽位与勾选 | 30x30 | 九宫 | 10/10/10/10 | `texture0/operate.ktx` | `img_item_box` | 物品槽 30x30 |
| `slot/item_frame_on.png` | 槽位与勾选 | 30x30 | 九宫 | 10/10/10/10 | `texture0/operate.ktx` | `img_item_box_selected` | 物品槽选中 30x30 |
| `tab/bar_blue.png` | 标签页 | 30x30 | 九宫 | 10/10/10/10 | `texture2/common.ktx` | `board_tab_blue` | 页签条底色 30x30 |
| `tab/leaf_active.png` | 标签页 | 30x30 | 九宫 | 10/10/10/10 | `texture2/common.ktx` | `btn_mini_gray` | 页签选中 30x30, 九宫 10 |
| `tab/leaf_flat.png` | 标签页 | 30x30 | 九宫 | 10/10/10/10 | `texture2/common.ktx` | `btn_mini_gray_n_flat` | 页签常态无高光 30x30 |
| `tab/leaf_hover.png` | 标签页 | 40x43 | 三片横向 | 15/0/15/0 | `texture2/common.ktx` | `btn_mini_gray_b` | 页签高亮 30x30 |
| `tab/leaf_normal.png` | 标签页 | 30x30 | 九宫 | 10/10/10/10 | `texture2/common.ktx` | `btn_mini_gray_n` | 页签常态 30x30, 九宫 10 |

## 三. 未导出的素材与原因

| 对象 | 数量 | 原因 |
|:---:|:---:|:---|
| 原厂标识与启动图 | 12 | 启动图, logo, 动画 logo 组, 属原厂品牌标识, 按复用政策直接排除 |
| 渠道与第三方标志 | 12 | 渠道图标, 微信支付宝标识, 快手广告位, 联名 IP 图形, 权利方不属于本项目 |
| 广告 SDK 图形 | 19 | 位于原版 res 目录, 属运营资产, 不进客户端 |
| 界面字体 heiti.ttf | 1 | 商业字库 (方正粗圆简体), 未取得授权, 改用系统字体 |
| 标题字图 font_* | 21 | 字图内嵌品牌文字, 无法通过改名去除, 改用文本渲染 |
| alpha 遮罩页 | 26 | 与同名图集页成对的通道图, 本项目按单张 RGBA 使用, 不需要单独导出 |

逐条清单见 `.agent/ASSET_COVERAGE.md` 第七章。

## 四. 复现

```bash
cd single-branch/assets && python -m forge.cli unpack --include ui
python single-branch/client/temp/ui_extract/ui_extract.py
```

脚本只读原版资源与管线工程树, 输出覆盖本目录的 PNG 与 ui_manifest.json。
本目录原有的 login_bg.jpg 由历史提交带入, 不在导出清单内。
