/**
 * ImMirror 我之镜界 服务端 作者: JularDepick
 * 成就与每日任务定义表
 *
 * 数据源: single-branch/assets/project/csvdef/autogen/achievement.csv (原版成就表, 双表头)
 * 提取规则: 原版 37 种目标类型中只保留能落到客户端本地事件的几种,
 * 目标 1 拾取物品 -> pick_item, 4 使用物品 -> place_block (客户端没有物品使用系统,
 * 按消耗物品的最接近行为折算), 6 开采物品 -> break_block, 10 与 11 行走与游泳 ->
 * walk_distance (单位米, 与 10 合并计程), 37 存活天数 -> online_days (单位天).
 * 共 64 条, 按原版 ID 升序排列. target 是达成所需的事件累计量, 单位随 kind 变化.
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
  { id: 1007, name: "上帝的恩赐", description: "除小草获得麦子的种子 （长按小草可能获得）", category: 2, kind: "pick_item", target: 1, coinReward: 50 },
  { id: 1011, name: "我发财了！", description: "用铁矿镐或钻头采集闪金矿石 （在地底深处）", category: 2, kind: "break_block", target: 1, coinReward: 50 },
  { id: 1012, name: "哦耶，蓝钻！", description: "用铁矿镐或钻头采集蓝钻矿石 （在地底深处）", category: 2, kind: "break_block", target: 1, coinReward: 50 },
  { id: 1013, name: "第一次触电", description: "用铁矿镐或钻头采集电石矿石 （在地底深处）", category: 2, kind: "break_block", target: 1, coinReward: 50 },
  { id: 1014, name: "硬家伙", description: "用钻头采集黑晶石 (可能会出现在岩浆附近）", category: 2, kind: "break_block", target: 1, coinReward: 50 },
  { id: 1016, name: "这东西在看我", description: "击败地心人获得地心之眼 （地心之眼好像会飞向某个地方）", category: 2, kind: "pick_item", target: 1, coinReward: 50 },
  { id: 1017, name: "远古宝石", description: "从地牢中获得远古宝石，丢掉再捡起来，因为它散发着远古的魔能…", category: 2, kind: "pick_item", target: 1, coinReward: 50 },
  { id: 1025, name: "纪念品", description: "拾取黑龙雕像 （好像野人很害怕黑龙雕像）", category: 2, kind: "pick_item", target: 1, coinReward: 50 },
  { id: 1026, name: "未知的世界", description: "结束了？这才刚开始！ （某某某：我还会回来的。。。）", category: 2, kind: "pick_item", target: 1, coinReward: 50 },
  { id: 1028, name: "耕种季节", description: "对耕地使用麦子种子播种 （只能种植在耕地上）", category: 3, kind: "place_block", target: 1, coinReward: 50 },
  { id: 1029, name: "收获季节", description: "获得麦子 （生长成熟后才能获得麦子）", category: 3, kind: "pick_item", target: 1, coinReward: 50 },
  { id: 1033, name: "斗牛士", description: "击败牛获得生牛肉 （牛吃草，哞哞哞）", category: 3, kind: "pick_item", target: 1, coinReward: 50 },
  { id: 1034, name: "皮革", description: "击败牛获得软皮革", category: 3, kind: "pick_item", target: 1, coinReward: 50 },
  { id: 1036, name: "小鸡快跑", description: "击败鸡获得生鸡肉 （鸡吃麦子种子，叽叽叽）", category: 3, kind: "pick_item", target: 1, coinReward: 50 },
  { id: 1037, name: "金蛋银蛋", description: "拾取小鸡蛋", category: 3, kind: "pick_item", target: 1, coinReward: 50 },
  { id: 1039, name: "咩", description: "击败羊获得生羊腿 （羊吃草，咩咩咩）", category: 3, kind: "pick_item", target: 1, coinReward: 50 },
  { id: 1040, name: "掠夺毛料", description: "击败羊或使用铁剪获得毛料", category: 3, kind: "pick_item", target: 1, coinReward: 50 },
  { id: 1042, name: "爱美之心", description: "拾取花朵 （美丽的花朵，比较稀有）", category: 3, kind: "pick_item", target: 1, coinReward: 50 },
  { id: 1049, name: "致富靠养猪", description: "获得超过1000块生猪排", category: 3, kind: "pick_item", target: 1000, coinReward: 130 },
  { id: 1050, name: "牛魔王", description: "获得超过1000块生牛肉", category: 3, kind: "pick_item", target: 1000, coinReward: 130 },
  { id: 1051, name: "量产羊腿", description: "获得超过1000块生羊腿", category: 3, kind: "pick_item", target: 1000, coinReward: 130 },
  { id: 1052, name: "愤怒的小鸡", description: "获得超过1000块生鸡肉", category: 3, kind: "pick_item", target: 1000, coinReward: 130 },
  { id: 1092, name: "蓝钻矿一", description: "开采10块蓝钻矿石", category: 5, kind: "break_block", target: 10, coinReward: 50 },
  { id: 1093, name: "蓝钻矿二", description: "开采50块蓝钻矿石", category: 5, kind: "break_block", target: 50, coinReward: 80 },
  { id: 1094, name: "蓝钻矿三", description: "开采200块蓝钻矿石", category: 5, kind: "break_block", target: 200, coinReward: 130 },
  { id: 1101, name: "电路高手一", description: "开采10块电石矿石", category: 5, kind: "break_block", target: 10, coinReward: 50 },
  { id: 1102, name: "电路高手二", description: "开采200块电石矿石", category: 5, kind: "break_block", target: 200, coinReward: 130 },
  { id: 1103, name: "电路高手三", description: "开采500块电石矿石", category: 5, kind: "break_block", target: 500, coinReward: 130 },
  { id: 1107, name: "森林卫士一", description: "种植超过10棵树", category: 5, kind: "place_block", target: 10, coinReward: 50 },
  { id: 1108, name: "森林卫士二", description: "种植超过200棵树", category: 5, kind: "place_block", target: 200, coinReward: 130 },
  { id: 1109, name: "森林卫士三", description: "种植超过2000棵树", category: 5, kind: "place_block", target: 2000, coinReward: 200 },
  { id: 1122, name: "咩咩咩一", description: "获得50份毛料", category: 5, kind: "pick_item", target: 50, coinReward: 80 },
  { id: 1123, name: "咩咩咩二", description: "获得200份毛料", category: 5, kind: "pick_item", target: 200, coinReward: 130 },
  { id: 1124, name: "咩咩咩三", description: "获得1000份毛料", category: 5, kind: "pick_item", target: 1000, coinReward: 130 },
  { id: 1125, name: "岩石粉碎机一", description: "敲掉100块岩石", category: 5, kind: "break_block", target: 100, coinReward: 80 },
  { id: 1126, name: "岩石粉碎机二", description: "敲掉1000块岩石", category: 5, kind: "break_block", target: 1000, coinReward: 130 },
  { id: 1127, name: "岩石粉碎机三", description: "敲掉5000块岩石", category: 5, kind: "break_block", target: 5000, coinReward: 200 },
  { id: 1139, name: "登陆萌眼星！", description: "乘坐火箭到萌眼星采集萌眼星石块", category: 2, kind: "break_block", target: 1, coinReward: 50 },
  { id: 1140, name: "激活图腾", description: "使用星启石激活图腾", category: 2, kind: "place_block", target: 1, coinReward: 50 },
  { id: 1143, name: "待续…", description: "更多冒险即将到来", category: 2, kind: "pick_item", target: 1, coinReward: 50 },
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
  { id: 1201, name: "站稳脚跟", description: "在极限模式中生存7天", category: 6, kind: "online_days", target: 7, coinReward: 50 },
  { id: 1202, name: "红红火火", description: "在极限模式中生存60天", category: 6, kind: "online_days", target: 60, coinReward: 120 },
  { id: 1203, name: "生存大师", description: "在极限模式中生存365天", category: 6, kind: "online_days", target: 365, coinReward: 310 },
  { id: 1213, name: "电路材料", description: "挖掘100块电石矿石", category: 6, kind: "break_block", target: 100, coinReward: 80 },
  { id: 1215, name: "一颗恒久远", description: "挖掘20块蓝钻矿石", category: 6, kind: "break_block", target: 20, coinReward: 260 },
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
];
