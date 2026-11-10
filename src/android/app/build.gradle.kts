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
