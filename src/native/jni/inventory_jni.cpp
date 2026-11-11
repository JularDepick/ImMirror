#include <jni.h>

#include <string>

#include "game/inventory.h"
#include "platform/android_runtime.h"
#include "game/item/item_def.h"
#include "game/item/item_table.h"

namespace {

using immirror::game::Inventory;
using immirror::game::ItemId;
using immirror::game::item::ItemTable;

/* 取宿主侧物品栏; 尚未装配时返回空. */
Inventory* HostInventory() {
    return immirror::platform::RuntimeHostShell() != nullptr
               ? &immirror::platform::RuntimeHostShell()->InventoryRef()
               : nullptr;
}

int32_t SelectedSlotIndex() {
    return immirror::platform::RuntimeHostShell() != nullptr
               ? immirror::platform::RuntimeHostShell()->SelectedSlot()
               : 0;
}

void SelectSlotByIndex(int32_t index) {
    if (immirror::platform::RuntimeHostShell() != nullptr) {
        immirror::platform::RuntimeHostShell()->SelectSlotPublic(index);
    }
}

std::string EscapeJson(const std::string& value) {
    std::string out;
    for (const char ch : value) {
        if (ch == '"' || ch == '\\') {
            out += '\\';
        }
        out += ch;
    }
    return out;
}

/* 物品表 JSON: {"items":[{"id":..,"key":..,"name":..,"icon":..,"stack":..,"category":..,"block":..}]} */
std::string BuildCatalogJson() {
    const ItemTable& table = ItemTable::Instance();
    std::string json = "{\"items\":[";
    const int32_t count = table.Count();
    bool first = true;
    for (int32_t index = 1; index < count; ++index) {
        const auto& info = table.Info(static_cast<ItemId>(index));
        if (info.key == nullptr || info.key[0] == '\0') {
            continue;
        }
        if (!first) {
            json += ",";
        }
        first = false;
        json += "{\"id\":" + std::to_string(index);
        json += ",\"key\":\"" + EscapeJson(info.key) + "\"";
        json += ",\"name\":\"" + EscapeJson(info.displayName == nullptr ? "" : info.displayName) + "\"";
        json += ",\"icon\":\"" + EscapeJson(info.icon == nullptr ? "" : info.icon) + "\"";
        json += ",\"stack\":" + std::to_string(info.maxStack);
        json += ",\"category\":" + std::to_string(static_cast<int>(info.category));
        json += ",\"block\":" + std::to_string(info.block);
        json += "}";
    }
    json += "]}";
    return json;
}

/* 槽位快照 JSON: {"selected":..,"slots":[{"slot":..,"item":..,"count":..}]} */
std::string BuildSnapshotJson() {
    Inventory* inventory = HostInventory();
    const int32_t selected = SelectedSlotIndex();
    std::string json = "{\"selected\":" + std::to_string(selected) + ",\"slots\":[";
    bool first = true;
    for (int32_t slot = 0; inventory != nullptr && slot < Inventory::kSlotCount; ++slot) {
        const auto& stack = inventory->Slot(slot);
        if (stack.Empty()) {
            continue;
        }
        if (!first) {
            json += ",";
        }
        first = false;
        json += "{\"slot\":" + std::to_string(slot);
        json += ",\"item\":" + std::to_string(static_cast<int>(stack.item));
        json += ",\"count\":" + std::to_string(stack.count);
        json += "}";
    }
    json += "]}";
    return json;
}

}  /* namespace */

extern "C" JNIEXPORT jstring JNICALL
Java_com_julardepick_immirror_inventory_InventoryBridge_nativeInventoryCatalog(JNIEnv* env, jclass clazz) {
    (void)clazz;
    return env->NewStringUTF(BuildCatalogJson().c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_julardepick_immirror_inventory_InventoryBridge_nativeInventorySnapshot(JNIEnv* env, jclass clazz) {
    (void)clazz;
    return env->NewStringUTF(BuildSnapshotJson().c_str());
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_julardepick_immirror_inventory_InventoryBridge_nativeInventoryMove(JNIEnv* env, jclass clazz,
                                                                            jint from, jint to) {
    (void)env;
    (void)clazz;
    Inventory* inventory = HostInventory();
    if (inventory == nullptr || from < 0 || to < 0) {
        return JNI_FALSE;
    }
    return inventory->Move(from, to) ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_julardepick_immirror_inventory_InventoryBridge_nativeInventoryDrop(JNIEnv* env, jclass clazz,
                                                                            jint slot, jint count) {
    (void)env;
    (void)clazz;
    Inventory* inventory = HostInventory();
    if (inventory == nullptr || slot < 0 || count <= 0) {
        return JNI_FALSE;
    }
    return inventory->Take(slot, count).Empty() ? JNI_FALSE : JNI_TRUE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_julardepick_immirror_inventory_InventoryBridge_nativeInventorySelect(JNIEnv* env, jclass clazz,
                                                                              jint index) {
    (void)env;
    (void)clazz;
    SelectSlotByIndex(index);
}