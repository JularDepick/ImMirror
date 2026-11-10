import org.gradle.api.tasks.compile.JavaCompile

plugins {
    id("com.android.application")
}

android {
    namespace = "com.julardepick.immirror"
    /* 本机只安装了 android-36.1 平台与 36.1.0 构建工具, 必须显式指定 */
    compileSdkVersion("android-36.1")
    buildToolsVersion = "36.1.0"
    ndkVersion = "30.0.16248370"

    defaultConfig {
        applicationId = "com.julardepick.immirror"
        minSdk = 24
        targetSdk = 36
        versionCode = 1
        versionName = "0.1.0"

        /* 三个 ABI 全覆盖: 真机常用 arm64/armv7, 模拟器用 x86_64 */
        ndk {
            abiFilters += listOf("arm64-v8a", "armeabi-v7a", "x86_64")
        }
    }

    buildFeatures {
        buildConfig = true
    }

    androidResources {
        /* 音效与纹理需要按未压缩形式读取, 否则无法用 openFd 打开. */
        noCompress += listOf("ogg", "ktx", "png", "json")
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    buildTypes {
        getByName("debug") {
            isMinifyEnabled = false
        }
        getByName("release") {
            isMinifyEnabled = false
        }
    }

    /*
     * 原生库由外置脚本构建后放入 jniLibs, 因此这里不声明 externalNativeBuild.
     * 保留自有库的调试符号, 避免 AGP 依赖 NDK 的 strip 工具链.
     */
    packaging {
        jniLibs {
            keepDebugSymbols.add("**/libimmirror_core.so")
        }
    }
}

/* 源码内含中文注释, 显式固定 UTF-8 编译编码 */
tasks.withType<JavaCompile>().configureEach {
    options.encoding = "UTF-8"
}

/*
 * 界面素材参数表: 构建期从 assets/ui/ui_manifest.json 生成 Java 常量类, 原生头文件
 * 与 APK 内的素材副本, 让 Java 与 C++ 两侧共用同一份尺寸与九宫边界.
 * 生成物在 .gitignore 里忽略; Python 解释器可用 -PimmirrorPython=<路径> 指定.
 */
val genUiResources = tasks.register<Exec>("genUiResources") {
    val script = layout.projectDirectory.file("../../scripts/gen_ui_resources.py").asFile
    val manifest = layout.projectDirectory.file("../../assets/ui/ui_manifest.json").asFile
    val python = (findProperty("immirrorPython") as String?) ?: "python"
    workingDir = script.parentFile.parentFile
    commandLine(python, script.absolutePath)
    inputs.file(manifest)
    outputs.dir(layout.projectDirectory.dir("src/main/java/com/julardepick/immirror/generated"))
    outputs.dir(layout.projectDirectory.dir("../../native/ui/generated"))
    outputs.dir(layout.projectDirectory.dir("src/main/assets/ui"))
}

tasks.named("preBuild") {
    dependsOn(genUiResources)
}