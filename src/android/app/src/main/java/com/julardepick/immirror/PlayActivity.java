package com.julardepick.immirror;

import android.app.NativeActivity;
import android.os.Bundle;
import android.view.WindowManager;

import com.julardepick.immirror.bridge.NativeBridge;
import com.julardepick.immirror.device.DeviceProfile;

/*
 * 游戏界面入口. 继承 NativeActivity, 由 android_native_app_glue 在独立线程上
 * 驱动 C++ 主循环; Java 侧只在进入与退出时各介入一次, 不参与逐帧逻辑.
 *
 * 原生库由 NativeActivity 依据 manifest 中的 android.app.lib_name 自行加载,
 * 因此这里不需要手动调用加载接口, 但仍通过 NativeBridge 下发设备画像.
 */
public final class PlayActivity extends NativeActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        NativeBridge.onDeviceProfile(DeviceProfile.obtain(this).toNativePayload());
        NativeBridge.start();
    }

    @Override
    protected void onDestroy() {
        NativeBridge.stop();
        super.onDestroy();
    }
}
