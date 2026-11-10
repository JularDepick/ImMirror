<div align="center">

# ImMirror 我之镜界

[![Version](https://img.shields.io/badge/Version-v0.1.0-red)](https://github.com/JularDepick/ImMirror/)
[![Copyright](https://img.shields.io/badge/Copyright-JularDepick-0066AA)](./COPYRIGHT)
[![License](https://img.shields.io/badge/License-Apache--2.0-yellow)](./LICENSE)

</div>

新世界, 新花样: 从心所欲, 不逾矩。

《我之镜界》(ImMirror) 是一款体素沙盒游戏。玩家在由方块构成的世界中自由探索, 破坏与放置方块, 用镜头记录自己的世界, 并通过每日签到积累镜币。

## 项目简介

本项目包含 Android 客户端与纯接口服务端两部分。客户端负责体素世界, 渲染, 玩法与本地存档; 服务端只负责账号, 会话与货币数据, 不提供网页, 也不托管任何美术资源。全部美术与音频资源随客户端安装包下发且不加密。

## 特性

- 体素沙盒世界: 连续地形由方块构成, 可自由破坏与放置方块
- 第一人称操作: 屏幕左半区移动, 右半区转动视角, 准星命中方块即可交互
- 账号体系: 邮箱注册, 邮箱或 UID 加密码登录, 邮箱验证码登录, 忘记密码重置, 邮箱换绑
- 镜币与签到: 每个自然日可签到一次获得镜币, 当月累计签到达到一定次数另有里程碑奖励
- 无充值体系: 不含付费充值, 内购与广告, 货币只能通过签到获得
- 资源随包: 美术与音频随安装包下发, 游玩过程不需要下载资源
- 纯接口服务端: 只提供 JSON 接口, 不提供在线网页

## 运行环境

| 项 | 要求 |
|:---:|:---:|
| 操作系统 | Android 7.0 (API 24) 及以上 |
| 处理器架构 | arm64-v8a 或 armeabi-v7a |
| 图形接口 | 支持 OpenGL ES 3.0 |
| 网络 | 需要可访问服务端接口的联网环境 |

## 项目文档

| 文档 | 内容 |
|:---:|:---:|
| [文档索引](docs/README.md) | 全部项目文档的入口与阅读顺序 |
| [仓库结构规范](docs/repo-layout.md) | 四个仓库的结构与目录约定 |
| [需求基线](docs/requirements.md) | R1 至 R18 需求条目与明确剔除项 |
| [架构](docs/architecture.md) | 客户端与服务端分层, 双端边界与数据流 |
| [素材管线](docs/assets-pipeline.md) | 素材工程文件与运行时二进制的转换流程 |
| [里程碑](docs/roadmap.md) | M0 至 M10 的目标与验收标准 |
| [决策记录](docs/decisions.md) | 关键技术决策及其依据 |
| [开发环境](docs/dev-setup.md) | 工具链要求, 构建命令与模拟器联调 |

## 版权信息

Copyright &copy; 2026 JularDepick

详见 [COPYRIGHT](./COPYRIGHT) 。

## 许可证

本仓库采用 [Apache-2.0 许可证](./LICENSE) 。
