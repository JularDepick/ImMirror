/* 界面数据绑定: 脚本通过宿主函数读取游戏状态. */
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "ui/script/widget_host.h"

using namespace immirror::ui;
using namespace immirror::ui::script;

static int failures = 0;

static void check(const char* label, bool ok) {
    std::printf(ok ? "ok   %s\n" : "FAIL %s\n", label);
    if (!ok) {
        failures += 1;
    }
}

/* 假数据源: 固定三个背包格与一组状态. */
class FakeSource : public UiDataSource {
public:
    ItemDefView ItemDef(int32_t id) const override {
        ItemDefView view;
        if (id != 11012) {
            return view;
        }
        view.found = true;
        view.id = id;
        view.displayName = "石矿镐";
        view.englishName = "StonePickaxe";
        view.icon = "icon11012";
        view.category = 3;
        view.maxStack = 1;
        return view;
    }

    int32_t BagSlotCount() const override { return 3; }
    std::string BagSlotName(int32_t slot) const override {
        static const char* const kNames[] = {"StonePickaxe", "Torch", "Bread"};
        return slot >= 0 && slot < 3 ? kNames[slot] : "";
    }
    int32_t BagSlotCountAt(int32_t slot) const override {
        return slot >= 0 && slot < 3 ? (slot + 1) * 5 : 0;
    }
    int32_t BagSlotDurability(int32_t slot) const override { return slot == 0 ? 42 : -1; }
    float Health() const override { return 0.75f; }
    float Hunger() const override { return 0.5f; }
    float Stamina() const override { return 0.25f; }
    std::string WorldName() const override { return "TESTWORLD"; }
    float WorldTimeSeconds() const override { return 720.0f; }
    int64_t ProgressValue(const std::string& key) const override {
        return key == "breakBlocks" ? 123 : 0;
    }
};

static bool ReadFile(const char* path, std::string& out) {
    FILE* file = std::fopen(path, "rb");
    if (file == nullptr) {
        return false;
    }
    char buffer[8192];
    size_t got = 0;
    while ((got = std::fread(buffer, 1, sizeof(buffer), file)) > 0) {
        out.append(buffer, got);
    }
    std::fclose(file);
    return true;
}

int main(int argc, char** argv) {
    const char* layoutPath =
        argc > 1 ? argv[1] : "../../assets/project/data/xml/ui/mobile/account.xml";
    std::string layout;
    if (!ReadFile(layoutPath, layout)) {
        std::printf("FAIL 读不到布局 %s\n", layoutPath);
        return 1;
    }
    const LayoutRect screen{0.0f, 0.0f, 1280.0f, 720.0f};

    FakeSource source;
    WidgetHost host;
    check("载入布局", host.Load(layout.data(), layout.size()));
    host.BindDataSource(&source);

    const char* script =
        "Slots = BagSlotCount() "
        "FirstName = BagSlotName(0) "
        "FirstCount = BagSlotCountAt(0) "
        "FirstDurability = BagSlotDurability(0) "
        "Hp = PlayerHealth() "
        "Hunger = PlayerHunger() "
        "Stamina = PlayerStamina() "
        "World = WorldName() "
        "Clock = WorldTime() "
        "Broken = ProgressValue(\"breakBlocks\") "
        "Missing = ProgressValue(\"noSuchStat\") ";
    check("执行读取脚本", host.RunScript(script, std::strlen(script), screen));

    check("背包格数", host.Vm().GetGlobal("Slots").number == 3.0);
    check("首格名称", host.Vm().GetGlobal("FirstName").text == "StonePickaxe");
    check("首格数量", host.Vm().GetGlobal("FirstCount").number == 5.0);
    check("首格耐久", host.Vm().GetGlobal("FirstDurability").number == 42.0);
    check("生命比例", host.Vm().GetGlobal("Hp").number > 0.74 && host.Vm().GetGlobal("Hp").number < 0.76);
    check("饥饿比例", host.Vm().GetGlobal("Hunger").number > 0.49 && host.Vm().GetGlobal("Hunger").number < 0.51);
    check("体力比例", host.Vm().GetGlobal("Stamina").number > 0.24 && host.Vm().GetGlobal("Stamina").number < 0.26);
    check("世界名", host.Vm().GetGlobal("World").text == "TESTWORLD");
    check("世界时间", host.Vm().GetGlobal("Clock").number == 720.0);
    check("进度统计", host.Vm().GetGlobal("Broken").number == 123.0);
    check("未收录统计为零", host.Vm().GetGlobal("Missing").number == 0.0);

    /* 数据写进控件: 把背包首格名称设到某个文本控件上. */
    const char* writeScript =
        "local c = getglobal(\"DropdownBoxNickname\") "
        "if c ~= nil then c:SetText(BagSlotName(1)) end";
    check("执行写入脚本", host.RunScript(writeScript, std::strlen(writeScript), screen));
    Widget* label = host.Tree().Find("DropdownBoxNickname");
    check("数据写入控件", label != nullptr && label->text == "Torch");

    /* 物品定义查询: 脚本按字段读取. */
    const char* defScript =
        "local d = GetInst(\"DefMgr\"):getItemDef(11012) "
        "DefName = d.EnglishName "
        "DefIcon = d.Icon "
        "DefStack = d.MaxStack "
        "DefMissing = GetInst(\"DefMgr\"):getItemDef(999999) "
        "DefMissingOk = (DefMissing.ID == nil) and 1 or 0 ";
    check("执行定义查询", host.RunScript(defScript, std::strlen(defScript), screen));
    check("定义英文名", host.Vm().GetGlobal("DefName").text == "StonePickaxe");
    check("定义图标", host.Vm().GetGlobal("DefIcon").text == "icon11012");
    check("定义堆叠", host.Vm().GetGlobal("DefStack").number == 1.0);
    check("未收录定义为空", host.Vm().GetGlobal("DefMissingOk").number == 1.0);

    std::printf("failures=%d\n", failures);
    return failures == 0 ? 0 : 1;
}
