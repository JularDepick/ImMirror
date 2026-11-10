package com.julardepick.immirror.device;

import android.app.ActivityManager;
import android.content.Context;
import android.content.pm.FeatureInfo;
import android.content.pm.PackageManager;
import android.graphics.Rect;
import android.opengl.EGL14;
import android.opengl.EGLConfig;
import android.opengl.EGLContext;
import android.opengl.EGLDisplay;
import android.opengl.EGLExt;
import android.opengl.EGLSurface;
import android.opengl.GLES20;
import android.os.Build;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.view.WindowManager;
import android.view.WindowMetrics;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

/*
 * 设备画像: 采集与游戏运行直接相关的硬件与系统能力, 给出粗粒度分级,
 * 并以扁平键值文本下发给原生侧.
 *
 * 分级只用于选择画质档位的初值, 不参与任何玩法判定;
 * 采集动作全部发生在进入原生主循环前后的低频路径上, 不进入逐帧逻辑.
 */
public final class DeviceProfile {

    private static final String TAG = "ImMirrorDevice";

    /* 采集结果只与设备有关, 进程内缓存一份, 避免重复做 EGL 探测. */
    private static volatile DeviceProfile cachedProfile;

    private final String abi;
    private final String abiList;
    private final int apiLevel;
    private final String releaseName;
    private final String model;
    private final String hardware;
    private final int cpuCores;
    private final long totalMemoryMb;
    private final long availableMemoryMb;
    private final String gpuVendor;
    private final String gpuRenderer;
    private final String gpuVersion;
    private final int vulkanMajor;
    private final int vulkanMinor;
    private final int vulkanLevel;
    private final int screenWidth;
    private final int screenHeight;
    private final int densityDpi;
    private final float density;
    private final int gradeScore;
    private final String gradeCode;

    /* 取缓存画像; 首次调用时完成一次完整采集. */
    public static DeviceProfile obtain(Context context) {
        DeviceProfile local = cachedProfile;
        if (local == null) {
            synchronized (DeviceProfile.class) {
                if (cachedProfile == null) {
                    cachedProfile = new DeviceProfile(context.getApplicationContext());
                }
                local = cachedProfile;
            }
        }
        return local;
    }

    private DeviceProfile(Context context) {
        final String[] abis = Build.SUPPORTED_ABIS;
        this.abi = abis.length > 0 ? abis[0] : "";
        this.abiList = joinAbis(abis);
        this.apiLevel = Build.VERSION.SDK_INT;
        this.releaseName = Build.VERSION.RELEASE == null ? "" : Build.VERSION.RELEASE;
        this.model = Build.MODEL == null ? "" : Build.MODEL;
        this.hardware = Build.HARDWARE == null ? "" : Build.HARDWARE;
        this.cpuCores = Math.max(1, Runtime.getRuntime().availableProcessors());

        long[] memory = readMemory(context);
        this.totalMemoryMb = memory[0];
        this.availableMemoryMb = memory[1];

        GpuProbe probe = GpuProbe.run();
        this.gpuVendor = probe.vendor;
        this.gpuRenderer = probe.renderer;
        this.gpuVersion = probe.version;

        int[] vulkan = readVulkan(context);
        this.vulkanMajor = vulkan[0];
        this.vulkanMinor = vulkan[1];
        this.vulkanLevel = vulkan[2];

        int[] metrics = readScreenMetrics(context);
        this.screenWidth = metrics[0];
        this.screenHeight = metrics[1];
        DisplayMetrics displayMetrics = context.getResources().getDisplayMetrics();
        this.densityDpi = displayMetrics.densityDpi;
        this.density = displayMetrics.density;

        this.gradeScore = scoreDevice();
        this.gradeCode = gradeCodeOf(gradeScore);
    }

    /* 主 ABI, 例如 arm64-v8a. */
    public String abi() {
        return abi;
    }

    /* 分级代码: high / mid / low, 供原生侧与档案使用. */
    public String gradeCode() {
        return gradeCode;
    }

    /* 分级中文标签, 仅用于界面展示. */
    public String gradeLabel() {
        if ("high".equals(gradeCode)) {
            return "高";
        }
        if ("mid".equals(gradeCode)) {
            return "中";
        }
        return "低";
    }

    public int gradeScore() {
        return gradeScore;
    }

    /*
     * 下发到原生侧的扁平键值文本, 每行一个 key=value.
     * 这里刻意不用 JSON, 原生侧无需引入解析库即可消费.
     */
    public String toNativePayload() {
        StringBuilder builder = new StringBuilder(768);
        appendPayloadLine(builder, "abi", abi);
        appendPayloadLine(builder, "abi_list", abiList);
        appendPayloadLine(builder, "api", Integer.toString(apiLevel));
        appendPayloadLine(builder, "release", releaseName);
        appendPayloadLine(builder, "model", model);
        appendPayloadLine(builder, "hardware", hardware);
        appendPayloadLine(builder, "cpu_cores", Integer.toString(cpuCores));
        appendPayloadLine(builder, "mem_total_mb", Long.toString(totalMemoryMb));
        appendPayloadLine(builder, "mem_avail_mb", Long.toString(availableMemoryMb));
        appendPayloadLine(builder, "gpu_vendor", gpuVendor);
        appendPayloadLine(builder, "gpu_renderer", gpuRenderer);
        appendPayloadLine(builder, "gpu_version", gpuVersion);
        appendPayloadLine(builder, "vulkan_version",
                String.format(Locale.US, "%d.%d", vulkanMajor, vulkanMinor));
        appendPayloadLine(builder, "vulkan_level", Integer.toString(vulkanLevel));
        appendPayloadLine(builder, "screen_w", Integer.toString(screenWidth));
        appendPayloadLine(builder, "screen_h", Integer.toString(screenHeight));
        appendPayloadLine(builder, "density_dpi", Integer.toString(densityDpi));
        appendPayloadLine(builder, "density", String.format(Locale.US, "%.2f", density));
        appendPayloadLine(builder, "grade", gradeCode);
        appendPayloadLine(builder, "grade_score", Integer.toString(gradeScore));
        return builder.toString();
    }

    /* 界面上逐行展示的可读文本. */
    public List<String> toDisplayLines() {
        List<String> lines = new ArrayList<>();
        lines.add(String.format(Locale.US, "系统: Android %s (API %d)", releaseName, apiLevel));
        lines.add(String.format(Locale.US, "机型: %s (%s)", model, hardware));
        lines.add(String.format(Locale.US, "主 ABI: %s", abi));
        lines.add(String.format(Locale.US, "ABI 列表: %s", abiList));
        lines.add(String.format(Locale.US, "处理器核心: %d", cpuCores));
        lines.add(String.format(Locale.US, "内存: 共 %d MB / 可用 %d MB", totalMemoryMb, availableMemoryMb));
        lines.add(String.format(Locale.US, "GPU 厂商: %s", emptyAs(gpuVendor, "未知")));
        lines.add(String.format(Locale.US, "GPU 型号: %s", emptyAs(gpuRenderer, "未知")));
        lines.add(String.format(Locale.US, "GPU 版本: %s", emptyAs(gpuVersion, "未知")));
        lines.add(String.format(Locale.US, "Vulkan: %d.%d (硬件等级 %d)", vulkanMajor, vulkanMinor, vulkanLevel));
        lines.add(String.format(Locale.US, "屏幕: %d x %d, %d dpi (x%.2f)", screenWidth, screenHeight,
                densityDpi, density));
        return lines;
    }

    private String emptyAs(String value, String fallback) {
        return value == null || value.isEmpty() ? fallback : value;
    }

    private static String joinAbis(String[] abis) {
        StringBuilder builder = new StringBuilder();
        for (int index = 0; index < abis.length; index++) {
            if (index > 0) {
                builder.append(',');
            }
            builder.append(abis[index]);
        }
        return builder.toString();
    }

    /* 兼容不同 API 级别的屏幕尺寸读取: 30 起用窗口度量, 更早用真实分辨率. */
    private static int[] readScreenMetrics(Context context) {
        WindowManager windowManager = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
        if (windowManager != null) {
            try {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                    WindowMetrics metrics = windowManager.getCurrentWindowMetrics();
                    Rect bounds = metrics.getBounds();
                    if (bounds.width() > 0 && bounds.height() > 0) {
                        return new int[]{bounds.width(), bounds.height()};
                    }
                } else {
                    Display display = windowManager.getDefaultDisplay();
                    DisplayMetrics displayMetrics = new DisplayMetrics();
                    display.getRealMetrics(displayMetrics);
                    if (displayMetrics.widthPixels > 0 && displayMetrics.heightPixels > 0) {
                        return new int[]{displayMetrics.widthPixels, displayMetrics.heightPixels};
                    }
                }
            } catch (Throwable error) {
                Log.w(TAG, "read screen metrics failed: " + error);
            }
        }
        DisplayMetrics fallback = context.getResources().getDisplayMetrics();
        return new int[]{fallback.widthPixels, fallback.heightPixels};
    }

    /* 返回 [总内存 MB, 可用内存 MB]; 读取失败时返回 0, 不阻断后续流程. */
    private static long[] readMemory(Context context) {
        try {
            ActivityManager manager = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
            if (manager == null) {
                return new long[]{0L, 0L};
            }
            ActivityManager.MemoryInfo info = new ActivityManager.MemoryInfo();
            manager.getMemoryInfo(info);
            return new long[]{info.totalMem / (1024L * 1024L), info.availMem / (1024L * 1024L)};
        } catch (Throwable error) {
            Log.w(TAG, "read memory failed: " + error);
            return new long[]{0L, 0L};
        }
    }

    /* 返回 [主版本, 次版本, 硬件等级]; 不支持 Vulkan 时全为 0. */
    private static int[] readVulkan(Context context) {
        int encodedVersion = 0;
        int hardwareLevel = 0;
        try {
            PackageManager manager = context.getPackageManager();
            FeatureInfo[] features = manager.getSystemAvailableFeatures();
            if (features != null) {
                for (FeatureInfo feature : features) {
                    if (feature.name == null) {
                        continue;
                    }
                    if (PackageManager.FEATURE_VULKAN_HARDWARE_VERSION.equals(feature.name)) {
                        encodedVersion = feature.version;
                    } else if (PackageManager.FEATURE_VULKAN_HARDWARE_LEVEL.equals(feature.name)) {
                        hardwareLevel = feature.version;
                    }
                }
            }
        } catch (Throwable error) {
            Log.w(TAG, "read vulkan features failed: " + error);
        }
        final int major = (encodedVersion >> 22) & 0x3ff;
        final int minor = (encodedVersion >> 12) & 0x3ff;
        return new int[]{major, minor, hardwareLevel};
    }

    /* 分级打分, 满分 90: 系统 10 + 内存 25 + 核心 20 + 图形接口 15 + Vulkan 20. */
    private int scoreDevice() {
        int score = 0;
        if (apiLevel >= 33) {
            score += 10;
        } else if (apiLevel >= 31) {
            score += 9;
        } else if (apiLevel >= 29) {
            score += 7;
        } else if (apiLevel >= 26) {
            score += 5;
        } else {
            score += 3;
        }

        if (totalMemoryMb >= 7600L) {
            score += 25;
        } else if (totalMemoryMb >= 5600L) {
            score += 21;
        } else if (totalMemoryMb >= 3600L) {
            score += 16;
        } else if (totalMemoryMb >= 2600L) {
            score += 10;
        } else {
            score += 5;
        }

        if (cpuCores >= 8) {
            score += 20;
        } else if (cpuCores >= 6) {
            score += 16;
        } else if (cpuCores >= 4) {
            score += 11;
        } else {
            score += 6;
        }

        score += scoreGraphicsApi();

        if (vulkanMajor > 1 || (vulkanMajor == 1 && vulkanMinor >= 2)) {
            score += 20;
        } else if (vulkanMajor == 1 && vulkanMinor >= 1) {
            score += 16;
        } else if (vulkanMajor == 1) {
            score += 12;
        }
        return score;
    }

    /* 按 EGL 探测到的 GLES 版本给图形接口打分. */
    private int scoreGraphicsApi() {
        if (gpuVersion == null || gpuVersion.isEmpty()) {
            return 6;
        }
        if (gpuVersion.contains("3.2")) {
            return 15;
        }
        if (gpuVersion.contains("3.1")) {
            return 13;
        }
        if (gpuVersion.contains("3.0")) {
            return 11;
        }
        return 6;
    }

    private static String gradeCodeOf(int score) {
        if (score >= 65) {
            return "high";
        }
        if (score >= 42) {
            return "mid";
        }
        return "low";
    }

    /* 追加一行键值; 值里的换行与回车替换为空格, 保证文本行格式不被破坏. */
    private void appendPayloadLine(StringBuilder builder, String key, String value) {
        builder.append(key).append('=');
        if (value != null) {
            builder.append(value.replace('\n', ' ').replace('\r', ' '));
        }
        builder.append('\n');
    }

    /*
     * 离屏 EGL 探测: 建立 1x1 的 pbuffer 表面并读取 GL 上报字符串.
     * 探测完成后只做上下文的解绑与销毁, 不调用 eglTerminate,
     * 避免影响原生渲染线程持有的同一个显示连接.
     */
    private static final class GpuProbe {
        String vendor = "";
        String renderer = "";
        String version = "";

        static GpuProbe run() {
            GpuProbe probe = new GpuProbe();
            EGLDisplay display = EGL14.EGL_NO_DISPLAY;
            EGLContext context = EGL14.EGL_NO_CONTEXT;
            EGLSurface surface = EGL14.EGL_NO_SURFACE;
            try {
                display = EGL14.eglGetDisplay(EGL14.EGL_DEFAULT_DISPLAY);
                if (display == EGL14.EGL_NO_DISPLAY) {
                    return probe;
                }
                int[] version = new int[2];
                if (!EGL14.eglInitialize(display, version, 0, version, 1)) {
                    return probe;
                }
                int[] configAttribs = {
                        EGL14.EGL_RENDERABLE_TYPE, EGLExt.EGL_OPENGL_ES3_BIT_KHR,
                        EGL14.EGL_SURFACE_TYPE, EGL14.EGL_PBUFFER_BIT,
                        EGL14.EGL_RED_SIZE, 8,
                        EGL14.EGL_GREEN_SIZE, 8,
                        EGL14.EGL_BLUE_SIZE, 8,
                        EGL14.EGL_ALPHA_SIZE, 8,
                        EGL14.EGL_NONE,
                };
                EGLConfig[] configs = new EGLConfig[1];
                int[] configCount = new int[1];
                boolean chosen = EGL14.eglChooseConfig(display, configAttribs, 0, configs, 0, 1,
                        configCount, 0);
                if (!chosen || configCount[0] < 1) {
                    return probe;
                }
                int[] contextAttribs = {EGL14.EGL_CONTEXT_CLIENT_VERSION, 3, EGL14.EGL_NONE};
                context = EGL14.eglCreateContext(display, configs[0], EGL14.EGL_NO_CONTEXT,
                        contextAttribs, 0);
                int[] surfaceAttribs = {
                        EGL14.EGL_WIDTH, 1,
                        EGL14.EGL_HEIGHT, 1,
                        EGL14.EGL_NONE,
                };
                surface = EGL14.eglCreatePbufferSurface(display, configs[0], surfaceAttribs, 0);
                if (context == null || context == EGL14.EGL_NO_CONTEXT
                        || surface == null || surface == EGL14.EGL_NO_SURFACE) {
                    return probe;
                }
                if (!EGL14.eglMakeCurrent(display, surface, surface, context)) {
                    return probe;
                }
                probe.vendor = glString(GLES20.GL_VENDOR);
                probe.renderer = glString(GLES20.GL_RENDERER);
                probe.version = glString(GLES20.GL_VERSION);
            } catch (Throwable error) {
                Log.w(TAG, "gpu probe failed: " + error);
            } finally {
                if (display != null && display != EGL14.EGL_NO_DISPLAY) {
                    EGL14.eglMakeCurrent(display, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_SURFACE,
                            EGL14.EGL_NO_CONTEXT);
                    if (surface != null && surface != EGL14.EGL_NO_SURFACE) {
                        EGL14.eglDestroySurface(display, surface);
                    }
                    if (context != null && context != EGL14.EGL_NO_CONTEXT) {
                        EGL14.eglDestroyContext(display, context);
                    }
                }
            }
            return probe;
        }

        private static String glString(int name) {
            String value = GLES20.glGetString(name);
            return value == null ? "" : value;
        }
    }
}
