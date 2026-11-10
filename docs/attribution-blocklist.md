# 素材署名排除清单

本清单列出原版素材中确认含原厂署名, 第三方渠道标识或外部 IP 授权文字的文件。这些文件不得导出到客户端, 也不得作为界面素材复用。清单由图片文字识别实测得出, 判据与证据见工作区的 OCR 排查记录。

## 一、含原厂版权声明的图片

| 文件 | 识别文字 |
|:---:|:---|
| `assets/ui/mobile/texture0/bigtex/arblueprintcn.png` | Copyright 2015-2018 深圳市迷你玩科技有限公司 All Rights Reserved. |

## 二、含原厂品牌文字的图片

| 文件 | 识别文字 |
|:---:|:---|
| `assets/ui/login_logo_alpha.ktx` | 迷你世界 |
| `assets/ui/splash_screen_bg.ktx` | 迷你玩 / MINIWAN |
| `assets/ui/splash_screen_bg_en.ktx` | 迷你玩 / MINIWAN |
| `res/drawable-xxhdpi-v4/splashscreen.png` | MINIWAN |
| `assets/ui/mobile/texture0/minilobby.ktx` | 4399 账号 / 4399 游戏 |
| `assets/ui/mobile/texture0/bigtex/dy_img_1.ktx` | 公众号 迷你世界粉丝阵地 |
| `assets/ui/mobile/texture0/bigtex/dy_img_2.ktx` | 公众号 迷你宇宙 |
| `assets/ui/mobile/texture0/bigtex/dy_img_1.ktx` | 周礼包 / 背后的秘密 / 限定皮肤 |
| `assets/ui/mobile/texture0/bigtex/fx_font.ktx` | 一起玩迷你世界 |
| `assets/ui/mobile/texture0/bigtex/fx_font_alpha.ktx` | 一起玩迷你世界 |
| `assets/ui/roleicons/avatar_icon1000_405.ktx` | MINI WORLD |

## 三、第三方渠道与外部 IP

| 文件 | 识别文字 |
|:---:|:---|
| `assets/ui/wechat_alipay.ktx` | 微信支付 / 打开微信, 扫一扫继续付款 |
| `assets/ui/mobile/texture0/bigtex/transformers_icon1.gif` | TRANSFORMERS / 正版授权 |
| `assets/ui/mobile/texture0/bigtex/transformers_icon2.gif` | TRANSFORMERS / 正版授权 |
| `assets/ui/mobile/texture0/bigtex/transformers_icon1_hw.gif` | Authorized |
| `assets/ui/mobile/texture0/bigtex/transformers_icon2_hw.gif` | Authorized |
| `assets/ui/mobile/texture0/bigtex/common.ktx` | 正版授权 / TRANSFORMERS |
| `assets/ui/mobile/texture0/bigtex/4399_acc01.ktx` | 4399 |
| `assets/ui/mobile/texture0/bigtex/img_adv_kuaishou.ktx` | 快手 |
| `assets/ui/mobile/texture0/bigtex/giflogo_70.gif` | 正版授权 |
| `assets/IdreamskyLogo.png` | 乐逗游戏 / iDREAMSKY GAMES |
| `res/drawable-hdpi-v4/mintegral_reward_end_pager_logo.png` | Mintegral |
| `res/drawable-xhdpi-v4/tt_playable_l_logo.png` | 穿山甲 |

## 四、非图片署名

| 位置 | 内容 |
|:---:|:---|
| `src/lua/ui/mobile/setting.lua` 关于页文案 | 原厂版权与备案信息, 明文可改, 接入时整体替换 |
| `heiti.ttf` 字体 name 表 | 内嵌商业字库版权声明, 不接入, 改用开源字体或自绘点阵 |

## 五、处理规则

1. 上述图片一律不进入 `client/src/assets`, 需要同类视觉元素时用自绘或从非署名页重制
2. 管线导出界面素材时按本清单过滤, 命中即跳过并记日志
3. 新增素材入库前做一次文字识别抽查, 命中署名关键词的按本流程处理