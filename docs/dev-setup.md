# 开发环境

本文给出构建客户端, 服务端与素材管线所需的工具链, 环境变量与命令。命令中的相对路径以各仓库根目录为基准。

## 一、工具链

| 工具 | 版本要求 | 用途 | 说明 |
|:---:|:---:|:---:|:---:|
| JDK | 21 | 客户端构建 | 与 Android Gradle Plugin 配套; 系统默认 JDK 版本不同时必须显式指定 |
| Gradle | 8.14.3 | 客户端构建 | 使用开发环境安装的 Gradle, 仓库不含 wrapper |
| Android Gradle Plugin | 8.13.2 | 客户端构建 | 由根构建脚本声明, 构建时解析 |
| Android SDK Platform | `android-36.1` | 客户端编译平台 | 与 build-tools 36.1.0 配套 |
| Android NDK | r30 (30.0.16248370) | 原生库交叉编译 | 路径由环境变量指定 |
| CMake | 3.22 及以上 | 原生库构建 | Windows 下生成器固定为 MinGW Makefiles |
| Node.js | 24 LTS | 服务端与构建工具 | 服务端与 Nuxt 运行时 |
| pnpm | 10 及以上 | 服务端依赖管理 | 需允许原生模块执行构建脚本 |
| Python | 3.x | 图集与素材脚本 | 生成图集需要 Pillow |
| adb | 随 platform-tools | 安装与调试 | 模拟器与真机联调 |

## 二、环境变量

| 变量 | 作用 | 建议取值 |
|:---:|:---:|:---:|
| `JAVA_HOME` | 指定 JDK 21 | 系统默认 JDK 不是 21 时必需 |
| `GRADLE_USER_HOME` | Gradle 缓存与守护进程目录 | 指向工作区内可写目录, 该目录不纳入版本管理 |
| `ANDROID_USER_HOME` | Android 工具的用户目录 | 同上 |
| `NDK_ROOT` | NDK 根目录 | 指向 r30 安装位置, 覆盖构建脚本内置默认值 |
| `ANDROID_SDK_ROOT` | Android SDK 根目录 | Android 工程也可通过本地属性文件指定, 该文件不纳入版本管理 |

构建缓存与产物一律落在工作区内, 避免写入用户主目录或系统盘。

## 三、服务端

```bash
cd single-branch/server/src
pnpm install      # 安装依赖
pnpm dev          # 开发模式启动, 默认监听 61110
pnpm build        # 生产构建
pnpm start        # 运行生产产物
pnpm test         # 运行接口用例
```

配置项与默认值见 `.env.example`。开发模式会读取工程根目录下的本地配置文件 (不纳入版本管理) 以存放邮件凭据; 生产运行只从进程环境注入配置。常用键: `NITRO_PORT`, `DB_FILE`, `SESSION_TTL_DAYS`, `CODE_TTL_SECONDS`, `MAIL_TRANSPORT`, `SMTP_*`, `APP_TIME_ZONE`。

## 四、客户端

```bash
cd single-branch/client/src
python scripts/build_atlas.py     # 方块贴图合成图集
python scripts/pack_assets.py     # 资源子集复制进安装包资源目录
bash scripts/build-native.sh      # 三个 ABI 的原生库

cd android
gradle :app:assembleDebug --no-daemon --console=plain
```

三个脚本的顺序有意义: 图集与音频副本是运行期资源, 缺失时应用可安装但资源加载失败。

### 4.1 原生构建脚本常量

| 常量 | 含义 | 默认值 |
|:---:|:---:|:---:|
| `NDK_ROOT` | NDK 根目录 | 由环境变量指定 |
| `CMAKE_BIN` | CMake 主程序 | 从 `PATH` 解析 `cmake` |
| `CMAKE_GENERATOR` | CMake 生成器 | `MinGW Makefiles` |
| `BUILD_TYPE` | 构建类型 | `Release` |
| `ANDROID_PLATFORM` | 目标平台级别 | `android-24` |
| `ANDROID_STL` | C++ 运行时链接方式 | `c++_static` |
| `DEFAULT_ABIS` | 默认 ABI 列表 | `arm64-v8a armeabi-v7a x86_64` |
| `JOBS` | 并行编译任务数 | `8` |
| `CLEAN` | 置 1 时先清理各 ABI 构建目录 | `0` |

变体用法:

```bash
bash scripts/build-native.sh arm64-v8a   # 只构建单个 ABI
CLEAN=1 bash scripts/build-native.sh     # 清理后重建
```

安装包产物为 Android 工程构建输出目录下的 `app-debug.apk`。

## 五、素材管线

```bash
cd single-branch/assets
python -m forge.cli formats                  # 列出编解码器
python -m forge.cli census                   # 全量清点
python -m forge.cli unpack                   # 解码到工程树
python -m forge.cli pack                     # 编码出运行时二进制
python -m forge.cli verify                   # 校验索引与工程树
python -m unittest discover -s tests         # 契约测试
```

管线只依赖 Python 标准库; 客户端侧的图集合成脚本额外需要 Pillow。

## 六、模拟器联调

| 步骤 | 命令 |
|:---:|:---:|
| 连接设备 | `adb connect 127.0.0.1:5555` |
| 安装 | `adb -s 127.0.0.1:5555 install -r app-debug.apk` |
| 启动 | `adb -s 127.0.0.1:5555 shell am start -n com.julardepick.immirror/.LauncherActivity` |
| 端口反向映射 | `adb reverse tcp:61110 tcp:61110` |
| 查看日志 | `adb logcat -s ImMirrorLauncher ImMirrorBridge ImMirrorPlay ImMirrorDevice` |

服务端地址规则:

| 场景 | 地址 |
|:---:|:---:|
| 模拟器访问宿主机 | `http://10.0.2.2:61110` (客户端默认值) |
| 模拟器走回环地址 | `http://127.0.0.1:61110`, 需先执行 `adb reverse` |
| 真机访问开发机 | 开发机的局域网地址加端口 |

地址可在登录界面的设置区修改并即时生效, 修改后无需重新安装。

### 6.1 日志标签

| 标签 | 来源 |
|:---:|:---:|
| `ImMirrorLauncher` | 启动界面与账号资料页 |
| `ImMirrorBridge` | 原生库加载 |
| `ImMirrorPlay` | 游戏界面, 音效加载 |
| `ImMirrorDevice` | 设备画像采集 |
| `ImMirrorCore` | 原生装配层与帧率汇报 |
| `ImMirrorPlatform` | 胶水层与生命周期事件 |
| `ImMirrorRender` | GLES 后端 |
| `ImMirrorHost` | Java 与原生共享状态 |
| `ImMirrorJni` | JNI 导出入口 |

## 七、环境自检

```bash
java -version
gradle -v
node -v
pnpm -v
python --version
cmake --version
adb version
```

并确认 NDK 目录下存在 `build/cmake/android.toolchain.cmake`, 以及 Android SDK 内已安装 `android-36.1` 平台与 36.1.0 构建工具。

## 八、常见问题

| 现象 | 处理 |
|:---:|:---:|
| AGP 内置原生构建卡在配置阶段 | 使用外置脚本构建原生库, 工程不声明内置原生构建 |
| SDK 组件安装失败 | SDK 管理器无法写入缓存目录时, 由开发者手动安装对应组件 |
| Gradle 守护进程报端口占用 | 重试一次; 或使用 `--no-daemon` 单次运行 |
| 图集脚本报缺少 Pillow | 执行 `python -m pip install Pillow` |
| 安装包体积偏大 | 三个 ABI 与静态 C++ 运行时导致, 发布期可按 ABI 拆分 |
| 服务端端口被占用 | 用 `NITRO_PORT` 换端口, 并同步修改客户端服务端地址 |
