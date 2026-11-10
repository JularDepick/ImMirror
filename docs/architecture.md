# 架构

本文描述四个仓库的运行期结构与代码分层。仓库级的目录规范见 [repo-layout.md](repo-layout.md), 需求约束见 [requirements.md](requirements.md)。

## 一、仓库视图

| 仓库 | 路径 | 运行期角色 |
|:---:|:---:|:---:|
| main | 根 | 不参与任何构建, 只承载 README 与项目文档 |
| client | `single-branch/client` | 产出 Android 安装包, 内含 C++ 引擎与随包资源 |
| server | `single-branch/server` | 产出 Node 服务, 对外只暴露 JSON 接口 |
| assets | `single-branch/assets` | 离线转码管线, 把素材工程文件转换为客户端可用的二进制 |

主仓库的忽略规则屏蔽了 `single-branch/`, 因此业务仓库的文件不会出现在主仓库的提交记录中。

## 二、运行期拓扑

```
客户端 APK                                 服务端进程
├── Java 界面层                            ├── Nitro 宿主 (进程与构建产物)
│   ├── LauncherActivity                   ├── Nitro 中间件 (请求转交给 Fastify)
│   ├── LoginActivity                      ├── Fastify 实例 (路由, 鉴权, 限流)
│   └── PlayActivity (NativeActivity)      ├── 服务层 (账号, 会话, 钱包, 邮件)
├── C++ 引擎主循环                          └── 数据层 (better-sqlite3, WAL) ── SQLite
│   ├── 平台适配 (android_native_app_glue)
│   ├── 体素世界与区块渲染
│   └── 玩家控制与 HUD
└── 随包资源 (图集, 音效, 界面贴图)
        │
        └────────────── HTTP/JSON  /api/v1  ─────────────->  服务端
```

数据流向是单向的: 账号与货币数据只在服务端, 资源只在客户端安装包内。两侧唯一的运行期通道是 HTTP/JSON 接口。

## 三、客户端分层

### 3.1 进程与线程模型

| 阶段 | 承载组件 | 说明 |
|:---:|:---:|:---:|
| 启动 | `LauncherActivity` | 采集设备画像, 判断本地会话; 有会话直接进入游戏, 否则跳转登录页 |
| 账号 | `LoginActivity` | 登录, 注册与找回密码共用一页, 支持切换服务端地址 |
| 游戏 | `PlayActivity` | 继承 `NativeActivity`, 由 `android_native_app_glue` 在独立线程调用 `android_main` |
| 返回 | `LauncherActivity` | 展示账号资料, 余额, 设备分级, 提供再次进入与退出登录 |

逐帧路径完全在原生线程内: 输入由胶水层解析为原生快照, 渲染与统计不回调 Java。Java 与原生只在进入与退出游戏, 以及事件触发的音效播放上交互。

### 3.2 目录结构

```
src/
├── .gitignore
├── README.md
├── android/                          # Gradle 工程与 Java 平台桥接层
│   ├── settings.gradle.kts
│   ├── build.gradle.kts
│   ├── gradle.properties
│   └── app/
│       ├── build.gradle.kts
│       └── src/main/
│           ├── AndroidManifest.xml
│           ├── java/com/julardepick/immirror/
│           │   ├── LauncherActivity.java
│           │   ├── PlayActivity.java
│           │   ├── account/          # 账号界面, 接口客户端与会话存储
│           │   ├── bridge/           # JNI 声明与库加载
│           │   └── device/           # 设备画像采集与分级
│           └── res/values/strings.xml
├── native/                           # C++ 引擎 (CMake, C++17)
│   ├── CMakeLists.txt
│   ├── core/                         # 主循环, 帧计时, 帧率统计, 日志
│   ├── platform/                     # 胶水层封装, 生命周期, 输入快照
│   ├── jni/                          # JNI 导出与 Java 原生共享状态
│   ├── voxel/                        # 方块注册表, 区块列, 地形生成, 网格构建
│   ├── world/                        # 区块流式加载, 方块读写, 存档
│   ├── render/                       # 渲染抽象, GLES 后端, 图集, 区块绘制
│   ├── player/                       # 第一人称相机, 输入映射, 碰撞, 射线拾取
│   ├── game/                         # 装配层, 把世界与玩家接到主循环
│   ├── audio/                        # 音效句柄持有与事件触发播放
│   ├── ui/                           # HUD 覆盖层
│   └── third_party/android_native_app_glue/
├── assets/                           # 运行时资源子集 (方块贴图, 音效, 界面贴图)
├── scripts/
│   ├── build-native.sh               # 外置 CMake 三 ABI 构建
│   ├── build_atlas.py                # 方块贴图合成图集
│   └── pack_assets.py                # 资源子集复制进安装包
└── shared/naming/resource-map.tsv    # 原版到本项目的资源改名映射
```

### 3.3 原生模块职责

| 模块 | 职责 | 关键类型 |
|:---:|:---:|:---:|
| `core` | 固定步长主循环, 帧计时, 帧率统计, 日志与向量 | `AppLoop`, `LoopHost`, `FrameClock`, `FrameStats` |
| `platform` | 封装胶水层的命令队列与输入队列, 分发生命周期回调 | `AppBridge`, `AppLifecycleListener`, `InputSnapshot` |
| `jni` | 导出 Java 可调用的五个入口, 保存 Java 与原生共享状态 | `EngineHost` |
| `voxel` | 方块静态属性, 16x256x16 区块列, 值噪声地形, 面剔除网格 | `BlockRegistry`, `ChunkColumn`, `TerrainGenerator`, `ChunkMesher` |
| `world` | 视距内区块的加载与卸载, 方块读写, 网格重建, 存档读写 | `World`, `ChunkView`, `SaveState` |
| `render` | 渲染后端抽象与 GLES 实现, 图集上传, 区块顶点缓冲增量同步 | `Device`, `GlesDevice`, `TextureAtlas`, `ChunkRenderer` |
| `player` | 第一人称相机, 触摸手势到移动的映射, AABB 碰撞, DDA 射线 | `PlayerController`, `RayHit` |
| `game` | 装配层: 画质档位, 手势动作映射, 自动存档, 帧率日志 | `GameShell` |
| `audio` | 持有 Java 侧 SoundPool 与音效句柄, 按事件触发播放 | `AudioEngine` |
| `ui` | 准星, 虚拟摇杆与跳跃按钮的覆盖层绘制 | `HudRenderer` |

方块 id 取值与注册表条目顺序绑定, 世界数据与存档直接引用这些数值, 因此只能追加, 不得重排或复用旧值。

### 3.4 主循环与渲染

| 项 | 约定 |
|:---:|:---:|
| 逻辑步长 | 固定 `1/60` 秒 |
| 单帧补步上限 | 4 步, 防止慢帧雪崩 |
| 渲染节奏 | 每帧一次, 与逻辑步解耦 |
| 后端抽象 | `render::Device` 统一资源创建与帧提交, 当前只有 GLES 实现 |
| 场景回调 | 渲染设备在清屏之后, 交换缓冲之前调用 `GameShell::RenderScene` |
| 绘制顺序 | 区块网格, 然后是 HUD; HUD 绘制前关闭深度测试与背面剔除并开启混合 |
| 视距档位 | `high` 4 列, `medium` 3 列, `low` 2 列, 由设备分级决定 |
| 图集 | 1024x1024 的 RGBA8 裸数据, 16x16 格, 每格 64 像素 |
| 顶点布局 | 位置 3 个 float, 图集坐标 2 个 float, 明暗 1 个 float, 共 24 字节 |

### 3.5 存档

| 项 | 约定 |
|:---:|:---:|
| 容器 | 文件头加区块记录, 魔数 `IMWF`, 版本 1 |
| 文件头 | 版本, 世界种子, 区块数量, 玩家坐标与朝向 |
| 区块记录 | 区块坐标, 游程数量, 其后为游程序列 |
| 方块压缩 | 每个区块按游程编码存储, 游程由数量与方块 id 组成 |
| 落盘时机 | 每 20 秒自动写盘, 前后台切换与退出时写盘 |
| 位置 | 应用内部存储目录下的单一存档文件 |

### 3.6 设备分级与账号接入

| 项 | 实现 |
|:---:|:---:|
| 采集内容 | ABI 列表, API 级别, 内存, CPU 核数, GPU 厂商与渲染器, Vulkan 版本, 屏幕尺寸与密度 |
| 分级方式 | 加权评分 0 至 90, 以 65 与 42 为分界划分 `high`, `mid`, `low` |
| 下发格式 | 扁平键值文本, 每行一个 `key=value`, 原生侧无需 JSON 解析 |
| 接口客户端 | `ApiClient` 只依赖 `HttpURLConnection` 与 `org.json`, 请求在后台线程, 回调投递回主线程 |
| 服务端地址 | `AccountSettings` 保存, 默认 `http://10.0.2.2:61110`, 改址立即生效 |
| 会话存储 | `SessionStore` 保存令牌与账号标识, 进入游戏前校验可用性 |

### 3.7 客户端已接入的接口

| 能力 | 客户端入口 | 服务端端点 |
|:---:|:---:|:---:|
| 注册验证码 | `sendRegisterCode` | `POST /auth/register/code` |
| 注册 | `register` | `POST /auth/register` |
| 密码登录 | `login` | `POST /auth/login` |
| 验证码登录 | `sendLoginCode` 与 `loginWithCode` | `POST /auth/login/code`, `POST /auth/login/code/verify` |
| 找回密码 | `sendResetCode` 与 `resetPassword` | `POST /auth/password/reset/code`, `POST /auth/password/reset` |
| 账号资料与余额 | `fetchProfile` | `GET /me` |
| 登出 | `logout` | `POST /auth/logout` |
| 服务可用性探测 | `ping` | `GET /meta/version` |

服务端另外提供换绑邮箱与签到接口, 客户端界面尚未接入。

## 四、服务端分层

### 4.1 进程组装

| 层 | 实现 | 说明 |
|:---:|:---:|:---:|
| 宿主 | Nitro | 只负责进程与构建产物, `ssr` 关闭, `ignore` 屏蔽 `api/**`, 不生成页面路由 |
| 启动插件 | `server/plugins/00-fastify.ts` | 构造唯一的 Fastify 实例并等待就绪, 关闭时释放数据库连接 |
| 请求转交 | `server/middleware/00-fastify-mount.ts` | 把所有请求原样交给 `fastify.routing`, Nitro 自身不注册业务路由 |
| 路由 | `server/api/v1/` | 按领域注册到同一个 Fastify 实例, 统一前缀 `/api/v1` |

该组合保证任意路径都只返回 JSON: 未匹配路径由 Fastify 的 404 处理并返回统一错误包络, 不会返回 HTML。

### 4.2 目录结构

```
src/
├── .env.example            # 配置样例
├── .gitignore
├── .npmrc                  # pnpm 链接方式等基础配置
├── nuxt.config.ts          # Nuxt 与 Nitro 配置
├── package.json
├── pnpm-workspace.yaml     # pnpm 设置与允许的构建脚本
├── tsconfig.json
├── vitest.config.ts
├── app/app.vue             # 空壳应用, 不渲染页面
├── docs/                   # 错误码, 会话与部署文档
├── server/
│   ├── api/v1/             # 路由层
│   ├── fastify/            # 实例组装, 上下文, 鉴权守卫
│   ├── services/           # 服务层: 账号, 会话, 钱包, 邮件, 限流
│   ├── db/                 # 数据层: 连接, 表结构, 各表查询封装
│   ├── utils/              # 配置, 错误, 响应, 校验, 时间, 随机, 密码
│   ├── middleware/         # Nitro 中间件: 请求转交 Fastify
│   └── plugins/            # Nitro 启动插件
└── tests/                  # 接口测试
```

### 4.3 分层职责

| 层 | 目录 | 职责边界 |
|:---:|:---:|:---:|
| 路由层 | `server/api/v1` | 参数提取, 鉴权前置, 响应封装; 不含业务规则 |
| 服务层 | `server/services` | 账号, 会话, 钱包, 签到, 邮件, 限流等业务规则 |
| 数据层 | `server/db` | 连接与建表, 各表的 SQL 封装; 业务代码不写裸 SQL |
| 工具层 | `server/utils` | 配置读取, 错误类型, 响应包络, 校验, 时间与随机 |
| 组装层 | `server/fastify` | Fastify 实例, 应用上下文, 鉴权守卫 |

### 4.4 数据模型

| 表 | 关键列 | 说明 |
|:---:|:---:|:---:|
| `users` | `id` `uid` `email` `password_hash` `email_verified_at` `status` | UID 与邮箱均唯一, 邮箱写入前转小写 |
| `verification_codes` | `email` `purpose` `code_hash` `expires_at` `attempts` `consumed_at` | 用途取注册, 登录, 重置, 换绑 |
| `sessions` | `user_id` `token_hash` `expires_at` `revoked_at` | 只存令牌摘要, 支持滑动续期与撤销 |
| `wallets` | `user_id` `balance` `updated_at` | 一用户一钱包, 余额不允许为负 |
| `wallet_ledger` | `delta` `balance_after` `reason` `ref` | 变动流水, 便于审计 |
| `checkins` | `user_id` `date` `month` `reward` | 唯一约束 `(user_id, date)` 兜底重复签到 |

### 4.5 接口分组

| 分组 | 端点数量 | 代表端点 |
|:---:|:---:|:---:|
| 账号认证 | 10 | `/auth/register`, `/auth/login`, `/auth/password/reset`, `/auth/email/rebind`, `/auth/logout` |
| 账号资料 | 1 | `GET /me` |
| 钱包与签到 | 3 | `GET /wallet`, `GET /wallet/checkin`, `POST /wallet/checkin` |
| 元信息 | 1 | `GET /meta/version` |

统一包络为 `{ ok: true, data }` 与 `{ ok: false, error: { code, message } }`, 认证头为 `Authorization: Bearer <token>`。错误码清单见服务端仓库的接口文档, 客户端按错误码映射为中文提示。

### 4.6 安全与限流

| 项 | 实现 |
|:---:|:---:|
| 密码 | `crypto.scrypt` (N=16384, r=8, p=1) 加 16 字节随机盐, 存储串带参数前缀 |
| 会话 | 32 字节随机令牌, 只存 SHA-256, 30 天滑动续期, 登出置撤销标记 |
| 验证码 | 6 位数字, 只存 SHA-256, 默认 600 秒有效, 同邮箱同用途 60 秒冷却, 校验上限 5 次 |
| 枚举防护 | 注册与重置的验证码接口对已注册与未注册邮箱返回同形态响应 |
| 请求体 | 上限 8192 字节, 非 JSON 直接拒绝 |
| 限流 | 同一 IP 每分钟 120 次; 验证码发送同 IP 每小时 30 次; 登录失败按账号退避; 重置同邮箱每小时 5 次 |
| 日志 | 不输出密码, 令牌与验证码明文 |

## 五、双端边界

| 边界 | 约定 | 落地位置 |
|:---:|:---:|:---:|
| 账号与会话 | 服务端是唯一真源, 客户端只持有令牌与展示用的资料副本 | `ApiClient`, `SessionStore` 与服务端 `services/auth.ts` |
| 货币 | 余额与流水只在服务端, 客户端只读展示 | `wallets`, `wallet_ledger` 与 `GET /me` |
| 资源 | 全部随安装包下发, 服务端不提供下载与静态路由 | `client/src/assets` |
| 配置 | 服务端读环境变量; 客户端只持久化服务端地址 | `utils/config.ts` 与 `AccountSettings` |
| 协议 | HTTP/JSON, Bearer 令牌, 统一响应包络 | 服务端路由层与 `ApiClient` |
| 版本协商 | 服务端返回协议版本与最低客户端版本 | `GET /meta/version` |
| 实时联机 | 尚未实现; 计划以自有协议承载房间同步, 不经过 HTTP 接口 | 后续里程碑 |

## 六、扩展点

| 目标 | 做法 |
|:---:|:---:|
| 新增渲染后端 | 实现 `render::Device` 并在 `CreateDefaultDevice` 中按设备能力选择 |
| 新增原生模块 | 在 `native` 下新建子目录, 自带 `CMakeLists.txt` 并把源文件追加到 `IMMIRROR_MODULE_SOURCES` |
| 新增业务接口 | 在 `server/api/v1` 注册路由, 规则写进 `services`, SQL 写进 `db/repositories` |
| 新增随包资源 | 资源进入 `client/src/assets` 后在 `pack_assets.py` 的清单登记一行 |
| 新增素材格式 | 在素材仓库新增编解码器并登记, 流程见 [assets-pipeline.md](assets-pipeline.md) |
