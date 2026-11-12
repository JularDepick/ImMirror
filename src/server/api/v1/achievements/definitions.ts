/**
 * ImMirror 我之镜界 服务端 作者: JularDepick
 * 成就与每日任务定义表
 *
 * 数据源: single-branch/assets/project/csvdef/autogen/achievement.csv (原版成就表, 双表头)
 * 提取规则: 原版 37 种目标类型中保留能落到客户端本地事件的九种,
 * 目标 1 拾取物品 -> pick_item, 2 合成物品 -> craft_item, 4 使用物品 -> place_block
 * (客户端没有物品使用系统, 按消耗物品的最接近行为折算), 5 熔炼物品 -> smelt_item,
 * 6 开采物品 -> break_block, 7 击败生物 -> kill_creature, 10 与 11 行走与游泳 ->
 * walk_distance (单位米, 与 10 合并计程), 37 存活天数 -> online_days (单位天).
 * 共 142 条, 按原版 ID 升序排列. target 是达成所需的事件累计量, 单位随 kind 变化.
 * 新增三类与 scripts/gen_achievement_data.py 的生成结果 (catalog.ts) 对齐,
 * 条目内容以生成器的折算为准, 本表不再人工挑条目.
 *
 * 统计键与客户端 native/progress/progress_types.cpp 的 StatKey 完全同名,
 * 客户端上报的 stats 对象直接以这些键为字段名, 服务端不做名字映射.
 *
 * 镜币奖励由成就点数与达成量档位换算 (原版奖励域为道具, 星星与迷你币, 与镜币不同域):
 * 奖励 = 30 + 成就点数 * 20 + 档位加成 (达成量 <=10 加 0, <=100 加 30, <=1000 加 80,
 * 更高加 150), 结果裁剪到 30 与 500 之间.
 *
 * 本表由人工按上述规则从原版表提取并校验, 不由脚本生成, 因此不会被素材管线的
 * 重新生成覆盖; 原版表变化时按同一规则重排本文件即可.
 */

/* 统计键: 与客户端 StatKind 一一对应, 顺序即客户端枚举顺序. */
export const STAT_KEYS = [
  "break_block",
  "place_block",
  "pick_item",
  "walk_distance",
  "online",
  "checkin",
  "online_days",
  "craft_item",
  "smelt_item",
  "kill_creature",
] as const;

export type StatKey = (typeof STAT_KEYS)[number];

/* 统计键到中文单位的映射, 用于接口返回里的展示文案. */
export const STAT_UNITS: Record<StatKey, string> = {
  break_block: "块",
  place_block: "块",
  pick_item: "件",
  walk_distance: "米",
  online: "分钟",
  checkin: "次",
  online_days: "天",
  craft_item: "件",
  smelt_item: "件",
  kill_creature: "只",
};

/* 统计键到中文名的映射, 与客户端 StatTitle 同名. */
export const STAT_TITLES: Record<StatKey, string> = {
  break_block: "破坏方块",
  place_block: "放置方块",
  pick_item: "采集物品",
  walk_distance: "行走距离",
  online: "在线时长",
  checkin: "签到",
  online_days: "在线天数",
  craft_item: "合成物品",
  smelt_item: "熔炼物品",
  kill_creature: "击败生物",
};

/* 分组: 1 新手, 2 主线, 3 生产, 4 战斗, 5 其他, 6 极限. */
export const CATEGORY_NAMES: Record<number, string> = {
  1: "新手",
  2: "主线",
  3: "生产",
  4: "战斗",
  5: "其他",
  6: "极限",
};

export interface AchievementDefinition {
  readonly id: number;
  readonly name: string;
  readonly description: string;
  readonly category: number;
  readonly kind: StatKey;
  readonly target: number;
  readonly coinReward: number;
}

export interface DailyTaskTemplate {
  readonly id: number;
  readonly kind: StatKey;
  readonly target: number;
  readonly coinReward: number;
  readonly name: string;
}

/* 成就定义, 按原版 ID 升序排列. */
export const ACHIEVEMENT_CATALOG: readonly AchievementDefinition[] = [
  { id: 1000, name: "我需要些木头", description: "采集树木，获得一个木头方块 （长按树干采集）", category: 2, kind: "break_block", target: 1, coinReward: 50 },
  { id: 1001, name: "手艺活！", description: "用任意木头合成木板，用4个木板制作1个工具箱 （先打开背包，左边标签选择合成）", category: 2, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1002, name: "蛮人必须败！", description: "用木板和木棍制作木棒 （需要工具箱，初级的武器）", category: 2, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1003, name: "初级矿工", description: "用木板和木棍制作木矿镐 （需要工具箱，夜晚前挖个藏身洞）", category: 2, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1004, name: "农耕生活！", description: "用木板和木棍制作木锄头 （需要工具箱，使土块变成耕地）", category: 2, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1005, name: "第一次战斗", description: "击败1只野人，它似乎带着兽头 （野人天黑会出现，是危险的生物）", category: 2, kind: "kill_creature", target: 1, coinReward: 50 },
  { id: 1006, name: "烧烤时间！", description: "用8个石块制作1个熔炼炉 （需要工具箱，可熔炼各种矿物）", category: 2, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1007, name: "上帝的恩赐", description: "除小草获得麦子的种子 （长按小草可能获得）", category: 2, kind: "pick_item", target: 1, coinReward: 50 },
  { id: 1010, name: "高级矿工", description: "用精铁锭和木棍制作铁矿镐 （铁矿镐可以采集蓝钻矿）", category: 2, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1011, name: "我发财了！", description: "用铁矿镐或钻头采集闪金矿石 （在地底深处）", category: 2, kind: "break_block", target: 1, coinReward: 50 },
  { id: 1012, name: "哦耶，蓝钻！", description: "用铁矿镐或钻头采集蓝钻矿石 （在地底深处）", category: 2, kind: "break_block", target: 1, coinReward: 50 },
  { id: 1013, name: "第一次触电", description: "用铁矿镐或钻头采集电石矿石 （在地底深处）", category: 2, kind: "break_block", target: 1, coinReward: 50 },
  { id: 1014, name: "硬家伙", description: "用钻头采集黑晶石 (可能会出现在岩浆附近）", category: 2, kind: "break_block", target: 1, coinReward: 50 },
  { id: 1015, name: "魔法师", description: "制作融合附魔箱 （需要工具箱，可使工具附上魔法）", category: 2, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1016, name: "这东西在看我", description: "击败地心人获得地心之眼 （地心之眼好像会飞向某个地方）", category: 2, kind: "pick_item", target: 1, coinReward: 50 },
  { id: 1017, name: "远古宝石", description: "从地牢中获得远古宝石，丢掉再捡起来，因为它散发着远古的魔能…", category: 2, kind: "pick_item", target: 1, coinReward: 50 },
  { id: 1018, name: "地心门砖", description: "远古宝石和黑晶石等合成地心门砖", category: 2, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1022, name: "地心传送门", description: "制作1个地心传送门块。把地心门框门内填满开启地心传送门 （中心2*3空间）", category: 2, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1023, name: "另一个世界", description: "进入地心击败熔岩巨人 （这个世界充满火焰）", category: 2, kind: "kill_creature", target: 1, coinReward: 50 },
  { id: 1024, name: "地上地下最强！", description: "击败远古黑龙 (没那么容易哦）", category: 2, kind: "kill_creature", target: 1, coinReward: 50 },
  { id: 1025, name: "纪念品", description: "拾取黑龙雕像 （好像野人很害怕黑龙雕像）", category: 2, kind: "pick_item", target: 1, coinReward: 50 },
  { id: 1026, name: "未知的世界", description: "结束了？这才刚开始！ （某某某：我还会回来的。。。）", category: 2, kind: "pick_item", target: 1, coinReward: 50 },
  { id: 1027, name: "铁桶", description: "制作1个铁桶打水 （1格水灌溉周围4格耕地）", category: 3, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1028, name: "耕种季节", description: "对耕地使用麦子种子播种 （只能种植在耕地上）", category: 3, kind: "place_block", target: 1, coinReward: 50 },
  { id: 1029, name: "收获季节", description: "获得麦子 （生长成熟后才能获得麦子）", category: 3, kind: "pick_item", target: 1, coinReward: 50 },
  { id: 1030, name: "烘焙面包", description: "制作1个小麦面包 （随时吃一个，不会饿肚子）", category: 3, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1031, name: "面包师", description: "制作1个长面包", category: 3, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1032, name: "蛋糕师", description: "制作蛋糕", category: 3, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1033, name: "斗牛士", description: "击败牛获得生牛肉 （牛吃草，哞哞哞）", category: 3, kind: "pick_item", target: 1, coinReward: 50 },
  { id: 1034, name: "皮革", description: "击败牛获得软皮革", category: 3, kind: "pick_item", target: 1, coinReward: 50 },
  { id: 1036, name: "小鸡快跑", description: "击败鸡获得生鸡肉 （鸡吃麦子种子，叽叽叽）", category: 3, kind: "pick_item", target: 1, coinReward: 50 },
  { id: 1037, name: "金蛋银蛋", description: "拾取小鸡蛋", category: 3, kind: "pick_item", target: 1, coinReward: 50 },
  { id: 1039, name: "咩", description: "击败羊获得生羊腿 （羊吃草，咩咩咩）", category: 3, kind: "pick_item", target: 1, coinReward: 50 },
  { id: 1040, name: "掠夺毛料", description: "击败羊或使用铁剪获得毛料", category: 3, kind: "pick_item", target: 1, coinReward: 50 },
  { id: 1041, name: "毛料地毡", description: "制作地毡", category: 3, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1042, name: "爱美之心", description: "拾取花朵 （美丽的花朵，比较稀有）", category: 3, kind: "pick_item", target: 1, coinReward: 50 },
  { id: 1043, name: "色彩世界", description: "制作一种颜料", category: 3, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1044, name: "嘿，这真好看", description: "制作红色毛料", category: 3, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1045, name: "轨道旅行", description: "制作铁轨道", category: 3, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1046, name: "出发！", description: "制作轨道车", category: 3, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1049, name: "致富靠养猪", description: "获得超过1000块生猪排", category: 3, kind: "pick_item", target: 1000, coinReward: 130 },
  { id: 1050, name: "牛魔王", description: "获得超过1000块生牛肉", category: 3, kind: "pick_item", target: 1000, coinReward: 130 },
  { id: 1051, name: "量产羊腿", description: "获得超过1000块生羊腿", category: 3, kind: "pick_item", target: 1000, coinReward: 130 },
  { id: 1052, name: "愤怒的小鸡", description: "获得超过1000块生鸡肉", category: 3, kind: "pick_item", target: 1000, coinReward: 130 },
  { id: 1053, name: "食物链顶端", description: "制作100个水果拼盘", category: 3, kind: "craft_item", target: 100, coinReward: 80 },
  { id: 1054, name: "石矛", description: "制作1把石矛 (危险总是出现在黑暗中)", category: 4, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1055, name: "更锋利", description: "制作1把短剑 （搭个房子能有效保护自己）", category: 4, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1056, name: "无用废材", description: "制作1把突刺长枪 （装备齐全，就不担心被偷袭）", category: 4, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1057, name: "削怪如泥", description: "制作1把能量剑 （现在要惩罚那些欺负人的野人）", category: 4, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1058, name: "远程打击", description: "制作1把木制短弓 (木制短弓拉满可以射得更远，需要石箭）", category: 4, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1059, name: "我需要石箭", description: "制作1支石箭", category: 4, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1060, name: "皮甲头", description: "制作1顶皮头盔 （可以穿在身上，增加防御）", category: 4, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1061, name: "铁甲头", description: "制作1顶精铁头盔 （实用的装备，增加防御）", category: 4, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1062, name: "钻甲头", description: "制作1顶蓝钻头盔 （高级装备，增加很多防御）", category: 4, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1063, name: "皮甲胸", description: "制作1件皮胸甲 （可以穿在身上，增加防御）", category: 4, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1064, name: "铁甲胸", description: "制作1件精铁胸甲 （实用的装备，增加防御）", category: 4, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1065, name: "钻甲胸", description: "制作1件蓝钻胸甲 （高级装备，增加很多防御）", category: 4, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1066, name: "皮甲腿", description: "制作1条皮护腿 （可以穿在身上，增加防御）", category: 4, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1067, name: "铁甲腿", description: "制作1条精铁护腿 （实用的装备，增加防御）", category: 4, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1068, name: "钻甲腿", description: "制作1条蓝钻护腿 （高级装备，增加很多防御）", category: 4, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1069, name: "皮甲足", description: "制作1双皮靴子 （可以穿在身上，增加防御）", category: 4, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1070, name: "铁甲足", description: "制作1双精铁靴子 （实用的装备，增加防御）", category: 4, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1071, name: "钻甲足", description: "制作1双蓝钻靴子 （高级装备，增加很多防御）", category: 4, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1074, name: "我需要纸", description: "制作1张纸张 （采集黑皮甘蔗制作）", category: 4, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1075, name: "灌魔手册", description: "制作1本空白手册", category: 4, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1078, name: "猎手公敌", description: "击败200只野人猎手", category: 4, kind: "kill_creature", target: 200, coinReward: 130 },
  { id: 1079, name: "地心公敌", description: "击败100只地心人", category: 4, kind: "kill_creature", target: 100, coinReward: 80 },
  { id: 1080, name: "野人公敌", description: "击败500只野人", category: 4, kind: "kill_creature", target: 500, coinReward: 130 },
  { id: 1081, name: "爆爆蛋！", description: "击败200只爆爆蛋", category: 4, kind: "kill_creature", target: 200, coinReward: 130 },
  { id: 1083, name: "勤劳矿工一", description: "制作100支石矿镐 （石矿镐可以采集铁矿）", category: 5, kind: "craft_item", target: 100, coinReward: 80 },
  { id: 1084, name: "勤劳矿工二", description: "制作50支铁矿镐 （铁矿镐可以采集高级矿）", category: 5, kind: "craft_item", target: 50, coinReward: 80 },
  { id: 1085, name: "勤劳矿工三", description: "制作20支钻头 （钻头可以采集黑晶石）", category: 5, kind: "craft_item", target: 20, coinReward: 80 },
  { id: 1092, name: "蓝钻矿一", description: "开采10块蓝钻矿石", category: 5, kind: "break_block", target: 10, coinReward: 50 },
  { id: 1093, name: "蓝钻矿二", description: "开采50块蓝钻矿石", category: 5, kind: "break_block", target: 50, coinReward: 80 },
  { id: 1094, name: "蓝钻矿三", description: "开采200块蓝钻矿石", category: 5, kind: "break_block", target: 200, coinReward: 130 },
  { id: 1101, name: "电路高手一", description: "开采10块电石矿石", category: 5, kind: "break_block", target: 10, coinReward: 50 },
  { id: 1102, name: "电路高手二", description: "开采200块电石矿石", category: 5, kind: "break_block", target: 200, coinReward: 130 },
  { id: 1103, name: "电路高手三", description: "开采500块电石矿石", category: 5, kind: "break_block", target: 500, coinReward: 130 },
  { id: 1104, name: "我需要石箭一", description: "制作10支石箭", category: 5, kind: "craft_item", target: 10, coinReward: 50 },
  { id: 1105, name: "我需要石箭二", description: "制作100支石箭", category: 5, kind: "craft_item", target: 100, coinReward: 80 },
  { id: 1106, name: "我需要石箭三", description: "制作1000支石箭", category: 5, kind: "craft_item", target: 1000, coinReward: 130 },
  { id: 1107, name: "森林卫士一", description: "种植超过10棵树", category: 5, kind: "place_block", target: 10, coinReward: 50 },
  { id: 1108, name: "森林卫士二", description: "种植超过200棵树", category: 5, kind: "place_block", target: 200, coinReward: 130 },
  { id: 1109, name: "森林卫士三", description: "种植超过2000棵树", category: 5, kind: "place_block", target: 2000, coinReward: 200 },
  { id: 1122, name: "咩咩咩一", description: "获得50份毛料", category: 5, kind: "pick_item", target: 50, coinReward: 80 },
  { id: 1123, name: "咩咩咩二", description: "获得200份毛料", category: 5, kind: "pick_item", target: 200, coinReward: 130 },
  { id: 1124, name: "咩咩咩三", description: "获得1000份毛料", category: 5, kind: "pick_item", target: 1000, coinReward: 130 },
  { id: 1125, name: "岩石粉碎机一", description: "敲掉100块岩石", category: 5, kind: "break_block", target: 100, coinReward: 80 },
  { id: 1126, name: "岩石粉碎机二", description: "敲掉1000块岩石", category: 5, kind: "break_block", target: 1000, coinReward: 130 },
  { id: 1127, name: "岩石粉碎机三", description: "敲掉5000块岩石", category: 5, kind: "break_block", target: 5000, coinReward: 200 },
  { id: 1128, name: "我怕黑一", description: "制作50支火炬", category: 5, kind: "craft_item", target: 50, coinReward: 80 },
  { id: 1129, name: "我怕黑二", description: "制作200支火炬", category: 5, kind: "craft_item", target: 200, coinReward: 130 },
  { id: 1130, name: "我怕黑三", description: "制作500支火炬", category: 5, kind: "craft_item", target: 500, coinReward: 130 },
  { id: 1137, name: "混乱终结", description: "击败混乱黑龙 （混乱黑龙：\\(-^-)/）", category: 2, kind: "kill_creature", target: 1, coinReward: 230 },
  { id: 1138, name: "制作火箭", description: "搜集材料制作火箭", category: 2, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1139, name: "登陆萌眼星！", description: "乘坐火箭到萌眼星采集萌眼星石块", category: 2, kind: "break_block", target: 1, coinReward: 50 },
  { id: 1140, name: "激活图腾", description: "使用星启石激活图腾", category: 2, kind: "place_block", target: 1, coinReward: 50 },
  { id: 1141, name: "星铜钻头", description: "合成星铜钻头", category: 2, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1142, name: "喷射钩爪", description: "合成喷射钩爪", category: 2, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1143, name: "待续…", description: "更多冒险即将到来", category: 2, kind: "pick_item", target: 1, coinReward: 50 },
  { id: 1144, name: "气体提炼器", description: "合成气体提炼器", category: 3, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1145, name: "压缩气罐", description: "合成压缩气罐", category: 3, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1146, name: "氧气背包", description: "合成氧气背包", category: 3, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1147, name: "培养基", description: "合成培养基", category: 3, kind: "craft_item", target: 1, coinReward: 50 },
  { id: 1149, name: "击败萌眼叮叮", description: "击败200个萌眼叮叮", category: 4, kind: "kill_creature", target: 200, coinReward: 130 },
  { id: 1150, name: "击败萌眼咚咚", description: "击败200个萌眼咚咚", category: 4, kind: "kill_creature", target: 200, coinReward: 130 },
  { id: 1151, name: "击败萌眼当当", description: "击败200个萌眼当当", category: 4, kind: "kill_creature", target: 200, coinReward: 130 },
  { id: 1152, name: "击败重力巫师", description: "击败100个重力巫师", category: 4, kind: "kill_creature", target: 100, coinReward: 80 },
  { id: 1154, name: "萌眼星生存一", description: "在萌眼星生存7天", category: 5, kind: "online_days", target: 7, coinReward: 50 },
  { id: 1155, name: "萌眼星生存二", description: "在萌眼星生存60天", category: 5, kind: "online_days", target: 60, coinReward: 80 },
  { id: 1156, name: "萌眼星生存三", description: "在萌眼星生存365天", category: 5, kind: "online_days", target: 365, coinReward: 130 },
  { id: 1157, name: "氧气果一", description: "吃掉10个氧气果", category: 5, kind: "place_block", target: 10, coinReward: 50 },
  { id: 1158, name: "氧气果二", description: "吃掉100个氧气果", category: 5, kind: "place_block", target: 100, coinReward: 80 },
  { id: 1159, name: "氧气果三", description: "吃掉500个氧气果", category: 5, kind: "place_block", target: 500, coinReward: 130 },
  { id: 1160, name: "激活图腾一", description: "激活5个图腾", category: 5, kind: "place_block", target: 5, coinReward: 50 },
  { id: 1161, name: "激活图腾二", description: "激活50个图腾", category: 5, kind: "place_block", target: 50, coinReward: 80 },
  { id: 1162, name: "激活图腾三", description: "激活300个图腾", category: 5, kind: "place_block", target: 300, coinReward: 130 },
  { id: 1163, name: "点亮图腾一", description: "点亮5个图腾", category: 5, kind: "place_block", target: 5, coinReward: 50 },
  { id: 1164, name: "点亮图腾二", description: "点亮50个图腾", category: 5, kind: "place_block", target: 50, coinReward: 80 },
  { id: 1165, name: "点亮图腾三", description: "点亮300个图腾", category: 5, kind: "place_block", target: 300, coinReward: 130 },
  { id: 1166, name: "星铜矿石一", description: "采集10个星铜矿石", category: 5, kind: "break_block", target: 10, coinReward: 50 },
  { id: 1167, name: "星铜矿石二", description: "采集100个星铜矿石", category: 5, kind: "break_block", target: 100, coinReward: 80 },
  { id: 1168, name: "星铜矿石三", description: "采集500个星铜矿石", category: 5, kind: "break_block", target: 500, coinReward: 130 },
  { id: 1169, name: "重力矿石一", description: "采集10个重力矿石", category: 5, kind: "break_block", target: 10, coinReward: 50 },
  { id: 1170, name: "重力矿石二", description: "采集100个重力矿石", category: 5, kind: "break_block", target: 100, coinReward: 80 },
  { id: 1171, name: "重力矿石三", description: "采集500个重力矿石", category: 5, kind: "break_block", target: 500, coinReward: 130 },
  { id: 1172, name: "远古巨像", description: "钻石/孔雀石/星铜石/紫荧石块在图腾核心旁4格放下召唤远古巨像击败它", category: 2, kind: "kill_creature", target: 1, coinReward: 50 },
  { id: 1173, name: "远古巨人", description: "击败远古巨人", category: 2, kind: "kill_creature", target: 1, coinReward: 50 },
  { id: 1201, name: "站稳脚跟", description: "在极限模式中生存7天", category: 6, kind: "online_days", target: 7, coinReward: 50 },
  { id: 1202, name: "红红火火", description: "在极限模式中生存60天", category: 6, kind: "online_days", target: 60, coinReward: 120 },
  { id: 1203, name: "生存大师", description: "在极限模式中生存365天", category: 6, kind: "online_days", target: 365, coinReward: 310 },
  { id: 1204, name: "野人！", description: "击败50只野人", category: 6, kind: "kill_creature", target: 50, coinReward: 80 },
  { id: 1205, name: "野萌宝！", description: "击败20只野萌宝", category: 6, kind: "kill_creature", target: 20, coinReward: 120 },
  { id: 1206, name: "地心人！", description: "击败50只地心人", category: 6, kind: "kill_creature", target: 50, coinReward: 260 },
  { id: 1210, name: "爆爆蛋！", description: "击败30只爆爆蛋", category: 6, kind: "kill_creature", target: 30, coinReward: 80 },
  { id: 1211, name: "硫磺弓手！", description: "击败100只硫磺弓手", category: 6, kind: "kill_creature", target: 100, coinReward: 120 },
  { id: 1212, name: "屠龙勇士！", description: "击败远古黑龙", category: 6, kind: "kill_creature", target: 1, coinReward: 230 },
  { id: 1213, name: "电路材料", description: "挖掘100块电石矿石", category: 6, kind: "break_block", target: 100, coinReward: 80 },
  { id: 1215, name: "一颗恒久远", description: "挖掘20块蓝钻矿石", category: 6, kind: "break_block", target: 20, coinReward: 260 },
  { id: 1216, name: "香喷烤鸡", description: "制作100只烤全鸡", category: 6, kind: "craft_item", target: 100, coinReward: 80 },
  { id: 1217, name: "汉堡大王", description: "制作50个牛肉汉堡", category: 6, kind: "craft_item", target: 50, coinReward: 120 },
  { id: 1218, name: "金的也能吃？", description: "制作50个超级金野果", category: 6, kind: "craft_item", target: 50, coinReward: 260 },
];

export const ACHIEVEMENT_BY_ID: ReadonlyMap<number, AchievementDefinition> = new Map(
  ACHIEVEMENT_CATALOG.map((item) => [item.id, item]),
);

/* 每日任务条数, 与客户端 DailyTaskBoard 的条数一致. */
export const DAILY_TASK_COUNT = 3;

/**
 * 每日任务模板: 每条是一种成就条件 (事件类型 + 达成量 + 镜币档位).
 * 达成量档位取自成就条件的目标档位 (10 / 50 / 200 量级), 在线时长按分钟折算,
 * 签到一天一次, 因此只给一档.
 */
export const DAILY_TASK_TEMPLATES: readonly DailyTaskTemplate[] = [
  { id: 1, kind: "break_block", target: 20, coinReward: 40, name: "开山取石" },
  { id: 2, kind: "break_block", target: 60, coinReward: 80, name: "挖掘达人" },
  { id: 3, kind: "place_block", target: 10, coinReward: 40, name: "开工建造" },
  { id: 4, kind: "place_block", target: 30, coinReward: 80, name: "施工队长" },
  { id: 5, kind: "pick_item", target: 10, coinReward: 40, name: "采集新手" },
  { id: 6, kind: "pick_item", target: 30, coinReward: 80, name: "采集行家" },
  { id: 7, kind: "walk_distance", target: 100, coinReward: 30, name: "四处走走" },
  { id: 8, kind: "walk_distance", target: 300, coinReward: 60, name: "长途跋涉" },
  { id: 9, kind: "online", target: 15, coinReward: 40, name: "上线报到" },
  { id: 10, kind: "online", target: 30, coinReward: 80, name: "冒险常客" },
  { id: 11, kind: "checkin", target: 1, coinReward: 30, name: "每日签到" },
  { id: 12, kind: "craft_item", target: 10, coinReward: 40, name: "巧手匠心" },
  { id: 13, kind: "smelt_item", target: 5, coinReward: 40, name: "炉火纯青" },
  { id: 14, kind: "kill_creature", target: 3, coinReward: 40, name: "初试锋芒" },
];
