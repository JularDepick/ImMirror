package com.julardepick.immirror.inventory;

import android.util.Log;

import com.julardepick.immirror.bridge.NativeBridge;

/*
 * 背包界面与原生物品栏之间的桥.
 *
 * 逐帧路径不经过这里: 打开背包时取一次物品表与槽位快照, 每次点击回写一次改动,
 * 都属于低频调用, 与设置界面的做法一致. 传给 Java 的只有两份最小 JSON 文本,
 * 原生侧始终是物品数量的唯一所有者, Java 不保存可写状态, 关掉页面不会丢数据.
 *
 * 桥接方法由原生侧在 jni 模块里注册, 尚未注册时调用会抛 UnsatisfiedLinkError,
 * 这里统一捕获并回落成 "原生未接线", 界面改用预览数据显示, 因此本包可以脱离原生
 * 单独联调. 原生侧接线约定:
 *   nativeInventoryCatalog()  -> 物品表 JSON, 见 InventoryCatalog 文件头
 *   nativeInventorySnapshot() -> 槽位 JSON, 见 InventorySnapshot 文件头
 *   nativeInventoryMove(from, to) -> 两格交换或合并
 *   nativeInventoryDrop(slot, count) -> 丢弃指定格若干数量
 *   nativeInventorySelect(index) -> 选中快捷栏格
 */
public final class InventoryBridge {

    private static final String TAG = "ImMirrorInventory";

    /* 原生方法是否可用; 一旦探测到可用就不再重复探测, 未接线时每次开页重试一次. */
    private static volatile boolean nativeReady;

    private InventoryBridge() {
    }

    /*
     * 探测原生桥接是否已经接线.
     * 原生库未加载, 或库已加载但没有注册这几个方法, 都视为未接线.
     */
    public static boolean isNativeReady() {
        if (nativeReady) {
            return true;
        }
        if (!NativeBridge.ensureLoaded()) {
            return false;
        }
        try {
            nativeInventoryCatalog();
            nativeReady = true;
            Log.i(TAG, "原生物品栏桥接已就绪");
        } catch (Throwable error) {
            Log.w(TAG, "原生物品栏桥接未接线, 使用预览数据: " + error);
        }
        return nativeReady;
    }

    /* 物品表; 原生未接线时返回空表, 由调用方决定是否退回预览数据. */
    public static InventoryCatalog catalog() {
        if (!isNativeReady()) {
            return new InventoryCatalog();
        }
        try {
            return InventoryCatalog.parse(nativeInventoryCatalog());
        } catch (Throwable error) {
            Log.w(TAG, "读取物品表失败", error);
            return new InventoryCatalog();
        }
    }

    /* 槽位快照; 原生未接线时返回空格快照. */
    public static InventorySnapshot snapshot() {
        if (!isNativeReady()) {
            return InventorySnapshot.empty();
        }
        try {
            return InventorySnapshot.parse(nativeInventorySnapshot());
        } catch (Throwable error) {
            Log.w(TAG, "读取物品栏快照失败", error);
            return InventorySnapshot.empty();
        }
    }

    /* 两格之间移动; 同种物品合并, 否则交换. 返回原生侧是否接受这次改动. */
    public static boolean moveSlot(int from, int to) {
        if (!isNativeReady()) {
            return false;
        }
        try {
            return nativeInventoryMove(from, to);
        } catch (Throwable error) {
            Log.w(TAG, "移动槽位失败", error);
            return false;
        }
    }

    /* 丢弃指定格的若干数量, 走原生物品栏的同一套扣减规则. */
    public static boolean dropSlot(int slot, int count) {
        if (!isNativeReady()) {
            return false;
        }
        try {
            return nativeInventoryDrop(slot, count);
        } catch (Throwable error) {
            Log.w(TAG, "丢弃槽位失败", error);
            return false;
        }
    }

    /* 选中快捷栏格, 由原生侧广播给底部 HUD. */
    public static boolean selectHotbar(int index) {
        if (!isNativeReady()) {
            return false;
        }
        try {
            return nativeInventorySelect(index);
        } catch (Throwable error) {
            Log.w(TAG, "选中快捷栏失败", error);
            return false;
        }
    }

    private static native String nativeInventoryCatalog();

    private static native String nativeInventorySnapshot();

    private static native boolean nativeInventoryMove(int from, int to);

    private static native boolean nativeInventoryDrop(int slot, int count);

    private static native boolean nativeInventorySelect(int index);
}
