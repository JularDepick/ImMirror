#include "voxel/block_registry.h"

#include <cstring>

#include "game/generated/block_data_gen.h"
#include "voxel/generated/atlas_index_gen.h"
#include "voxel/generated/block_light_gen.h"

namespace immirror::voxel {
namespace {

/*
 * 生成表所在命名空间的别名: 生成表定义在 immirror::game::gen,
 * 体素侧按非限定名 gen:: 无法解析到它, 这里固定指向生成表命名空间.
 */
namespace gen = immirror::game::gen;

/* 图集格未指定时的回落标记, 与 BlockInfo 的约定一致. */
constexpr uint16_t kAtlasFallback = 0xFFFF;

/*
 * 兼容段条目, 条目顺序即方块 id 0 到 14. 图集格编号与渲染侧从资源合成的
 * 图集一致: 0 草顶, 1 草侧, 2 土, 3 石, 4 沙, 5 水, 6 木侧, 7 木顶,
 * 8 叶, 9 基岩, 10 砖, 11 圆石, 12 雪, 13 冰, 14 砂砾, 15 木板.
 * 除草与木需要区分顶侧底之外, 其余方块三个面共用 atlasIndex.
 *
 * 硬度, 采集工具与采集材质族取自 blockdef.csv 的实测值:
 * 草 3.5 铲, 石 8 镐, 沙 3 铲, 雪 1.5 铲, 冰 5 镐, 木板 6 斧, 树叶 0.4 徒手.
 * 表里没有单列的方块按数据表归族: 土同草, 圆石与砖同石, 原木同木板,
 * 砂砾的音效取砂砾族, 用它与沙区分开才是数据表的材质反馈.
 * 水与基岩硬度取 -1, 即不可破坏.
 * 本段字段是存档兼容面, 只能保持, 不得随数据表更新而改动.
 */
/*
 * 兼容段光照三列与生成表 kLightEntries 前十五条一致, 由 block_light_gen.h
 * 折算后并入此表: 空气与水完全不遮光 (atten 0, cover 0), 草土石沙等
 * 实体方块按数据表衰减遮光, 树叶按厚度衰减 (atten 10, cover 2).
 * 光照传播依赖这里的取值, 数据改动必须同步生成表.
 */
constexpr BlockInfo kLegacyBlockTable[block::LegacyCount] = {
    /* name          solid  opaque  index  top    side   bottom   hardness  tool   material  source atten cover */
    {"air", false, false, 0, kAtlasFallback, kAtlasFallback, kAtlasFallback, -1.0f,
     HarvestTool::kHand, HarvestMaterial::kNone, 0, 0, 0},
    {"grass", true, true, 1, 0, 1, 2, 3.5f, HarvestTool::kShovel, HarvestMaterial::kGrass, 0, 15, 1},
    {"dirt", true, true, 2, kAtlasFallback, kAtlasFallback, kAtlasFallback, 3.5f,
     HarvestTool::kShovel, HarvestMaterial::kGrass, 0, 15, 1},
    {"stone", true, true, 3, kAtlasFallback, kAtlasFallback, kAtlasFallback, 8.0f,
     HarvestTool::kPickaxe, HarvestMaterial::kStone, 0, 15, 1},
    {"sand", true, true, 4, kAtlasFallback, kAtlasFallback, kAtlasFallback, 3.0f,
     HarvestTool::kShovel, HarvestMaterial::kSand, 0, 15, 1},
    {"water", false, false, 5, kAtlasFallback, kAtlasFallback, kAtlasFallback, -1.0f,
     HarvestTool::kHand, HarvestMaterial::kNone, 0, 1, 0},
    {"wood", true, true, 6, 7, 6, 7, 6.0f, HarvestTool::kAxe, HarvestMaterial::kWood, 0, 15, 2},
    {"leaves", true, true, 8, kAtlasFallback, kAtlasFallback, kAtlasFallback, 0.4f,
     HarvestTool::kHand, HarvestMaterial::kGrass, 0, 10, 0},
    {"bedrock", true, true, 9, kAtlasFallback, kAtlasFallback, kAtlasFallback, -1.0f,
     HarvestTool::kHand, HarvestMaterial::kNone, 0, 15, 1},
    {"brick", true, true, 10, kAtlasFallback, kAtlasFallback, kAtlasFallback, 8.0f,
     HarvestTool::kPickaxe, HarvestMaterial::kStone, 0, 15, 2},
    {"cobblestone", true, true, 11, kAtlasFallback, kAtlasFallback, kAtlasFallback, 8.0f,
     HarvestTool::kPickaxe, HarvestMaterial::kStone, 0, 15, 2},
    {"snow", true, true, 12, kAtlasFallback, kAtlasFallback, kAtlasFallback, 1.5f,
     HarvestTool::kShovel, HarvestMaterial::kSnow, 0, 2, 2},
    {"ice", true, true, 13, kAtlasFallback, kAtlasFallback, kAtlasFallback, 5.0f,
     HarvestTool::kPickaxe, HarvestMaterial::kStone, 0, 3, 2},
    {"gravel", true, true, 14, kAtlasFallback, kAtlasFallback, kAtlasFallback, 3.0f,
     HarvestTool::kShovel, HarvestMaterial::kGravel, 0, 15, 1},
    {"planks", true, true, 15, kAtlasFallback, kAtlasFallback, kAtlasFallback, 6.0f,
     HarvestTool::kAxe, HarvestMaterial::kWood, 0, 15, 2},
};

constexpr size_t kLegacyBlockCount = sizeof(kLegacyBlockTable) / sizeof(kLegacyBlockTable[0]);

/*
 * hardness 的合法范围是 -1 到 200, 绝大多数条目落在 0.5 的网格上,
 * 树叶实测为 0.4, 因此这里只校验范围, 不对步长做强制约束.
 * 兼容段在编译期校验, 填入越界硬度会直接编译失败.
 */
constexpr float kHardnessMin = -1.0f;
constexpr float kHardnessMax = 200.0f;

constexpr bool LegacyHardnessValid() {
    for (size_t i = 0; i < kLegacyBlockCount; ++i) {
        const float value = kLegacyBlockTable[i].hardness;
        if (value < kHardnessMin || value > kHardnessMax) {
            return false;
        }
    }
    return true;
}

static_assert(LegacyHardnessValid(), "兼容段方块硬度超出的 -1 到 200 范围");

/*
 * 兼容段英文键逐项校验: 与 block::kLegacyNames 一致, 键序即 id 序,
 * 改名或换序都会破坏存档引用.
 */
constexpr bool LegacyNamesValid() {
    for (size_t i = 0; i < kLegacyBlockCount; ++i) {
        const char* expected = block::kLegacyNames[i];
        const char* actual = kLegacyBlockTable[i].name;
        while (*expected != '\0' || *actual != '\0') {
            if (*expected != *actual) {
                return false;
            }
            ++expected;
            ++actual;
        }
    }
    return true;
}

static_assert(LegacyNamesValid(), "兼容段方块英文键与 block::kLegacyNames 不一致");

/*
 * 扩展段编译期对齐校验: 生成表扩展段条目数, 起始 id 与全量方块数必须一致,
 * 不一致说明方块数据表过期, 重新运行 scripts/gen_block_data.py 即可修复.
 */
static_assert(gen::kRegistryEntryCount ==
                  static_cast<int32_t>(block::Count) - static_cast<int32_t>(block::LegacyCount),
              "扩展段条目数与全量方块数不一致, 请重新运行 scripts/gen_block_data.py");
static_assert(gen::kBlockEntryCount == static_cast<int32_t>(block::Count),
              "生成表条目数与全量方块数不一致, 请重新运行 scripts/gen_block_data.py");

/*
 * 兼容段掉落视图沿用旧版生成表, 与旧 BlockData 的前 15 条逐字段一致:
 * 掉落取值是物品 id, 能折回兼容段的按映射取, 折不回则原地回收自身.
 */
struct LegacyDropView {
    uint32_t sourceId;
    uint32_t dropSourceId;
    uint32_t dropBlock;
};

/*
 * 兼容段方块的掉落映射, 保留数据供物品系统后续与 item_table 的 DropOfBlock 对齐;
 * 当前掉落走物品表, 本表暂未接入, 因此标注 maybe_unused 而不是删除数据.
 */
[[maybe_unused]] constexpr LegacyDropView kLegacyDrops[block::LegacyCount] = {
    {0u, 0u, 0u},      {100u, 101u, 2u},   {101u, 101u, 2u},   {104u, 505u, 10u},
    {106u, 106u, 4u},  {4u, 0u, 0u},       {200u, 200u, 6u},   {218u, 12500u, 7u},
    {1u, 1u, 8u},      {547u, 547u, 9u},   {505u, 505u, 10u},  {115u, 12054u, 11u},
    {123u, 123u, 12u}, {107u, 107u, 13u},  {206u, 206u, 14u},
};

/*
 * 扩展段材质族的编译期值域校验: 生成表 material 取值与 HarvestMaterial 一致,
 * 超出族数说明生成器与 voxel 侧的枚举约定漂移.
 */
constexpr int32_t kHarvestMaterialCount = 11; /* none 与十个音族 */

constexpr bool RegistryMaterialsValid() {
    for (int32_t i = 0; i < gen::kRegistryEntryCount; ++i) {
        if (gen::kRegistryEntries[i].material >= kHarvestMaterialCount) {
            return false;
        }
        if (gen::kRegistryEntries[i].tool > 5) {
            return false;
        }
    }
    return true;
}

static_assert(RegistryMaterialsValid(),
              "扩展段材质族或工具值越界, 请重新运行 scripts/gen_block_data.py");

/* 越界 id 的回落条目, 与空气保持完全一致, 也就是不可破坏. */
constexpr BlockInfo kFallbackBlock = {"air", false, false, 0, kAtlasFallback, kAtlasFallback,
                                     kAtlasFallback, -1.0f, HarvestTool::kHand,
                                     HarvestMaterial::kNone};

/*
 * 采集工具折算: 生成表的工具值与 HarvestTool 的前四档一致, 5 剪刀当前没有
 * 对应档位, 统一按徒手处理, 数据在 toolLevel 里保留原值供后续接入工具.
 */
constexpr HarvestTool ToolOf(uint8_t tool) {
    switch (tool) {
        case 1:
            return HarvestTool::kAxe;
        case 2:
            return HarvestTool::kPickaxe;
        case 3:
            return HarvestTool::kShovel;
        case 0:
        case 5:
        default:
            return HarvestTool::kHand;
    }
}

/*
 * 材质族折算: 生成表的 material 值即 HarvestMaterial 的取值顺序,
 * 1 到 10 逐族直接映射 (grass/stone/sand/wood/snow/gravel/cloth/glass/jar/ladder),
 * 音效侧已按同族接入 dig_cloth/glass/jar 与 walk_cloth/ladder 音源.
 */
constexpr HarvestMaterial MaterialOf(uint8_t material) {
    switch (material) {
        case 1:
            return HarvestMaterial::kGrass;
        case 2:
            return HarvestMaterial::kStone;
        case 3:
            return HarvestMaterial::kSand;
        case 4:
            return HarvestMaterial::kWood;
        case 5:
            return HarvestMaterial::kSnow;
        case 6:
            return HarvestMaterial::kGravel;
        case 7:
            return HarvestMaterial::kCloth;
        case 8:
            return HarvestMaterial::kGlass;
        case 9:
            return HarvestMaterial::kJar;
        case 10:
            return HarvestMaterial::kLadder;
        default:
            return HarvestMaterial::kNone;
    }
}

}  /* namespace */

BlockRegistry::BlockRegistry() = default;

/*
 * 按贴图名查图集格序号, 线性扫描 atlas_index_gen.h 的查找表.
 * 生成表按格序号排序, 名字查找是 O(n), 只在 Info() 首次构造时调用,
 * 构造结果缓存在静态数组里, 运行时零开销.
 */
int32_t AtlasSlotOf(const char* textureName) {
    if (textureName == nullptr || textureName[0] == '\0') {
        return kAtlasFallback;
    }
    for (int32_t i = 0; i < ::immirror::voxel::atlas::kAtlasSlotCount; ++i) {
        if (std::strcmp(::immirror::voxel::atlas::kAtlasSlots[i].name, textureName) == 0) {
            return ::immirror::voxel::atlas::kAtlasSlots[i].slot;
        }
    }
    return kAtlasFallback;
}

const BlockRegistry& BlockRegistry::Instance() {
    static const BlockRegistry instance;
    return instance;
}

const BlockInfo& BlockRegistry::Info(BlockId id) const {
    if (id < block::LegacyCount) {
        /*
         * 兼容段表的字段属存档兼容面, 表本身保持不动; 重力一列由生成表叠加,
         * 避免把数据表口径手抄进兼容段. 叠加结果按 id 缓存, 只算一次.
         */
        static BlockInfo legacyCache[block::LegacyCount];
        static bool legacyFilled[block::LegacyCount] = {};
        if (!legacyFilled[id]) {
            legacyCache[id] = kLegacyBlockTable[id];
            legacyCache[id].gravity = gen::BlockEntryOf(id).gravity;
            legacyCache[id].dropChance = gen::BlockEntryOf(id).dropChance;
            legacyCache[id].slipperiness = gen::BlockEntryOf(id).slipperiness;
            legacyCache[id].replaceable = gen::BlockEntryOf(id).replaceable != 0;
            legacyCache[id].dropItem2 = static_cast<uint16_t>(gen::BlockEntryOf(id).dropSourceId2);
            legacyCache[id].dropChance2 = gen::BlockEntryOf(id).dropChance2;
            legacyCache[id].blastResist = gen::BlockEntryOf(id).blastResist;
            legacyCache[id].flowThrough = gen::BlockEntryOf(id).flowThrough;
            legacyFilled[id] = true;
        }
        return legacyCache[id];
    }
    const int32_t index = static_cast<int32_t>(id) - static_cast<int32_t>(block::LegacyCount);
    if (index >= gen::kRegistryEntryCount) {
        return kFallbackBlock;
    }
    const gen::RegistryEntry& entry = gen::kRegistryEntries[index];
    /*
     * 扩展段条目折算成 BlockInfo 返回: 折算是一次性的整数比较与查表,
     * 调用方拿到的仍是引用语义的只读视图; 折算规则集中在这一处,
     * 音效与图集接入后只改这里的映射, 不动数据.
     */
    /*
     * 条目填充标记: BlockInfo 的 name 默认值是 "air" 而非空指针,
     * 不能用 name 判空作哨兵, 单独用布尔标记记录是否已折算.
     */
    static BlockInfo cache[gen::kRegistryEntryCount == 0 ? 1 : gen::kRegistryEntryCount];
    static bool filled[gen::kRegistryEntryCount == 0 ? 1 : gen::kRegistryEntryCount] = {};
    BlockInfo& info = cache[index];
    if (!filled[index]) {
        info.name = entry.name;
        info.solid = entry.solid;
        info.opaque = entry.opaque;
        info.atlasIndex = AtlasSlotOf(entry.texture);
        const auto& lightEntry = ::immirror::voxel::light::LightEntryOf(
            static_cast<uint16_t>(block::LegacyCount + index));
        info.lightSource = lightEntry.lightSource;
        info.lightAtten = lightEntry.lightAtten;
        info.coverMode = lightEntry.coverMode;
        info.atlasTop = kAtlasFallback;
        info.atlasSide = kAtlasFallback;
        info.atlasBottom = kAtlasFallback;
        /* 形状下标与 voxel::BlockShape 的枚举顺序一致, 生成器侧同步维护. */
        info.shape = static_cast<BlockShape>(entry.shape <= 5 ? entry.shape : 0);
        info.hardness = entry.hardness;
        info.tool = ToolOf(entry.tool);
        info.material = MaterialOf(entry.material);
        info.toolLevel = entry.toolLevel;
        info.gravity = entry.gravity;
        info.dropChance = entry.dropChance;
        info.slipperiness = entry.slipperiness;
        info.replaceable = entry.replaceable != 0;
        info.dropItem2 = static_cast<uint16_t>(entry.dropSourceId2);
        info.dropChance2 = entry.dropChance2;
        info.blastResist = entry.blastResist;
        info.flowThrough = entry.flowThrough;
        /*
         * 顶面贴图: 布局里给了独立的顶面名时单独查图集格,
         * 查不到就保持回落值, 渲染侧会自动沿用侧面, 不会出现空洞.
         */
        if (entry.textureTop != nullptr && entry.textureTop[0] != '\0') {
            info.atlasTop = AtlasSlotOf(entry.textureTop);
        }
        filled[index] = true;
    }
    return info;
}

BlockId BlockRegistry::FindByName(const char* name) const {
    if (name == nullptr) {
        return block::Air;
    }
    for (size_t i = 0; i < kLegacyBlockCount; ++i) {
        if (std::strcmp(kLegacyBlockTable[i].name, name) == 0) {
            return static_cast<BlockId>(i);
        }
    }
    for (int32_t i = 0; i < gen::kRegistryEntryCount; ++i) {
        if (std::strcmp(gen::kRegistryEntries[i].name, name) == 0) {
            return static_cast<BlockId>(block::LegacyCount + i);
        }
    }
    return block::Air;
}

size_t BlockRegistry::Count() const {
    return static_cast<size_t>(block::Count);
}

}  /* namespace immirror::voxel */