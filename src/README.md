# 客户端源码

> 《我之镜界》(ImMirror) 的 Android 客户端源码目录。
> 包含 Gradle 工程、C++ 原生主循环、GLES 清屏渲染与设备分级展示。

## 一、目录结构

```
src/
├── .gitignore                  # 源码目录级忽略规则
├── README.md                   # 本文件
├── assets/                     # 随包资源 (方块贴图等, 从原版资源复制的部分在 blocks/)
├── android/                    # Gradle 工程 (Java 平台桥接层)
│   ├── settings.gradle.kts     # 模块声明与仓库配置
│   ├── build.gradle.kts        # 插件版本声明
│   ├── gradle.properties       # Gradle 与 Android 构建开关
│   └── app/
│       ├── build.gradle.kts    # 应用模块配置 (SDK 版本, ABI, 打包)
│       └── src/main/
│           ├── AndroidManifest.xml
│           ├── java/com/julardepick/immirror/
│           │   ├── LauncherActivity.java     # 启动界面, 展示版本与设备分级
│           │   ├── PlayActivity.java         # NativeActivity 游戏入口
│           │   ├── bridge/NativeBridge.java  # 原生库加载与 JNI 声明
│           │   └── device/DeviceProfile.java # 设备画像采集与分级
│           └── res/values/strings.xml
├── native/                     # C++ 引擎 (CMake, C++17)
│   ├── CMakeLists.txt          # 原生库目标与依赖声明
│   ├── core/                   # 主循环, 帧计时, 帧率统计, 日志与断言
│   ├── platform/               # 胶水层封装, 输入快照, 应用装配根
│   ├── render/                 # 渲染抽象 Device 与 GLES 后端
│   ├── jni/                    # JNI 导出与 Java/原生共享状态
│   └── third_party/            # 随 NDK 分发的 android_native_app_glue
└── scripts/
    └── build-native.sh         # 外置构建三个 ABI 的原生库
```

## 二、构建环境

| 项 | 取值 |
|:---:|:---|
| Gradle | 8.14.3 (本机全局安装, 不使用 wrapper) |
| JDK | 21 (Gradle 与 AGP 的编译运行时) |
| Android Gradle Plugin | 8.13.2 |
| compileSdk | `android-36.1` |
| buildToolsVersion | 36.1.0 |
| minSdk / targetSdk | 24 / 36 |
| NDK | r30 (30.0.16248370) |
| CMake | 系统 4.4.0, 生成器固定 `MinGW Makefiles` |
| 目标 ABI | arm64-v8a, armeabi-v7a, x86_64 |
| C++ 标准 | C++17 |
| STL | c++_static (原生库自包含) |

Gradle 需要可写的用户目录, 本机按下面的环境变量隔离, 避免污染全局缓存:

```bash
export JAVA_HOME="<JDK 21 安装路径>"
export GRADLE_USER_HOME="<工作区>/temp/gradle-home"
export ANDROID_USER_HOME="<工作区>/temp/android-user-home"
```

Android SDK 与 NDK 的路径由本机环境变量或本机 Gradle 配置提供, 不随仓库分发。

## 三、构建步骤

### 1. 构建原生库

```bash
cd single-branch/client/src
bash scripts/build-native.sh
```

脚本逐个 ABI 调用外部 CMake (NDK 工具链 + NDK 自带 make) 生成 `libimmirror_core.so`,
再复制到 Android 工程的打包目录。脚本顶部集中了可改常量:

| 常量 | 含义 | 默认值 |
|:---:|:---|:---|
| `NDK_ROOT` | NDK 根目录 | `D:/AppWithoutInstall/android-ndk-r30` |
| `CMAKE_BIN` | CMake 主程序 | `cmake` (取自 PATH) |
| `MAKE_PROGRAM` | 生成器使用的 make | `$NDK_ROOT/prebuilt/windows-x86_64/bin/make.exe` |
| `CMAKE_GENERATOR` | CMake 生成器 | `MinGW Makefiles` |
| `BUILD_TYPE` | 构建类型 | `Release` |
| `ANDROID_PLATFORM` | 目标平台级别 | `android-24` |
| `ANDROID_STL` | C++ 运行时链接方式 | `c++_static` |
| `DEFAULT_ABIS` | 默认 ABI 列表 | `arm64-v8a armeabi-v7a x86_64` |
| `JOBS` | 并行编译任务数 | `8` |

常用变体:

```bash
bash scripts/build-native.sh arm64-v8a   # 只构建单个 ABI
CLEAN=1 bash scripts/build-native.sh     # 先清理各 ABI 构建目录
```

### 2. 构建 APK

```bash
cd single-branch/client/src/android
gradle :app:assembleDebug --no-daemon --console=plain
```

产物位于 `app/build/outputs/apk/debug/app-debug.apk`, 内含三个 ABI 的原生库。

### 3. 安装与运行

```bash
adb -s <设备序列号> install -r app/build/outputs/apk/debug/app-debug.apk
adb -s <设备序列号> shell am start -n com.julardepick.immirror/.LauncherActivity
```

## 四、运行时行为

### 4.1 启动链路

1. `LauncherActivity` 采集设备画像 (ABI, API, 内存, CPU 核数, GPU, Vulkan, 屏幕), 计算分级并展示
2. 点击"进入游戏"后跳转 `PlayActivity`
3. `PlayActivity` 继承 `NativeActivity`, 由 `android_native_app_glue` 在独立线程上调用 `android_main`
4. 原生侧组装平台桥接与渲染后端, 进入固定步长主循环; 每帧只做清屏与缓冲交换

### 4.2 Java 与原生边界

| 时机 | 调用 | 频率 |
|:---:|:---|:---:|
| 进入游戏 | `nativeOnDeviceProfile(payload)` | 一次 |
| 进入游戏 | `nativeStart()` | 一次 |
| 退出游戏 | `nativeStop()` | 一次 |

设备画像以扁平键值文本下发, 每行一个 `key=value`, 原生侧无需引入 JSON 解析依赖。
逐帧路径上没有任何 JNI 调用: 输入由胶水层直接解析为原生快照, 渲染与统计全部在原生线程内完成。

### 4.3 日志

```bash
adb logcat -s ImMirrorCore ImMirrorPlatform ImMirrorRender ImMirrorHost ImMirrorJni ImMirrorLauncher
```

| 标签 | 来源 |
|:---:|:---|
| `ImMirrorCore` | 应用装配根, 帧率汇报与设备画像 |
| `ImMirrorPlatform` | 胶水层封装, 窗口与生命周期事件 |
| `ImMirrorRender` | GLES 后端, 上下文创建与释放 |
| `ImMirrorHost` | Java 与原生共享状态 |
| `ImMirrorJni` | JNI 导出入口 |
| `ImMirrorLauncher` | 启动界面的设备采集结果 |

每 60 帧输出一行帧率日志, 同时带上设备信息:

```
frame fps=119.9 avg=8.34ms max=9.85ms frames=4360 size=2560x1440 gpu=Adreno (TM) 750 | OpenGL ES 3.1 v1 abi=x86_64 grade=high
```

### 4.4 生命周期

- 切到后台: 暂停上报, 释放 EGL 表面与上下文, 写入最小存档状态
- 回到前台: 重建 EGL 上下文, 恢复渲染与帧率统计
- 返回键: 原生侧拦截并结束游戏 Activity, 主循环退出后由胶水层回收线程

## 五、已知限制

1. 原生库由 `scripts/build-native.sh` 先行构建, 不使用 AGP 的内置原生构建
2. 渲染仅验证清屏与缓冲交换, 尚无场景绘制与材质管线
3. 只有 GLES 后端; Vulkan 仅在 Java 侧做能力探测, 后端实现留待后续版本
4. 启动界面当前没有登录与账号功能, 账号相关界面在后续版本接入
5. 设备分级评分为经验值, 阈值需结合真实机型校准
6. 游戏界面固定横屏, 未做竖屏适配
7. APK 同时打包三个 ABI 并使用静态 C++ 运行时, 包体偏大, 后续按 ABI 拆分或开启裁剪

## 版权信息

Copyright &copy; 2026 JularDepick, 详见 [COPYRIGHT](../COPYRIGHT)。

## 许可证

本仓库采用 [Apache-2.0 许可证](../LICENSE)。

## 六、体素世界

| 模块 | 职责 |
|:---:|:---|
| `native/voxel/` | 方块注册表, 16x256x16 区块列, 值噪声地形生成, 面剔除网格构建 |
| `native/world/` | 区块流式加载与卸载, 方块读写, 相邻列网格修补 |
| `native/render/` | 纹理图集加载, 区块顶点缓冲增量同步, 区块绘制 |
| `native/player/` | 第一人称相机, 触摸输入映射, AABB 碰撞, 视线射线拾取 |
| `native/game/` | 装配层, 把世界与玩家接到主循环 |

### 6.1 资源与图集

方块贴图来自 `src/assets/blocks/` (从原版资源复制的部分), 由 `scripts/build_atlas.py` 合成为
`android/app/src/main/assets/generated/atlas.rgba` (1024x1024 RGBA8, 16x16 格, 每格 64 像素)。
图集产物不进入版本库, 构建前先执行脚本。

### 6.2 操作

| 手势 | 行为 |
|:---:|:---|
| 左半屏按下拖动 | 移动摇杆 |
| 左半屏双击 | 跳跃 |
| 右半屏拖动 | 转动视角 |
| 右半屏短按 | 破坏准星命中的方块 |
| 右半屏长按 | 在命中面外侧放置方块 |

### 6.3 当前限制

1. 世界不落盘, 退出即重置
2. 水面按不透明渲染, 未做透明排序
3. 只有一种方块材质, 尚未接入物品栏与合成
