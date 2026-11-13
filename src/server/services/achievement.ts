/**
 * ImMirror 我之镜界 服务端 作者: JularDepick
 * 成就与每日任务服务
 *
 * 客户端上报增量统计 (破坏方块, 放置方块, 采集物品, 行走距离, 在线时长, 签到, 在线天数),
 * 服务端按同键累加到每个用户自己的成就进度与当日任务进度.
 * 成就是一次性进度, 完成时刻写入后不再改; 每日任务按自然日重置, 只保留当日行.
 *
 * 完成与领取分离: 上报只推进进度并记录完成时刻, 镜币由领取接口按条入账,
 * 这样客户端可以逐条点领取, 也避免完成瞬间一次性发一堆币.
 *
 * 镜币发放复用钱包仓储 (wallets 余额 + wallet_ledger 流水), 与签到服务同一条记账路径,
 * 记账原因分别为 achievement 与 daily_task; 领取在一个事务内完成,
 * 标记领取与奖励入账不会出现只落一半的情况.
 */

import {
  ACHIEVEMENT_BY_ID,
  ACHIEVEMENT_CATALOG,
  CATEGORY_NAMES,
  DAILY_TASK_COUNT,
  DAILY_TASK_TEMPLATES,
  STAT_KEYS,
  STAT_TITLES,
  STAT_UNITS,
} from "../api/v1/achievements/definitions";
import type { DailyTaskTemplate, StatKey } from "../api/v1/achievements/definitions";
import type { SqliteDatabase } from "../db/client";
import {
  claimDailyTask,
  deleteDailyTasksBefore,
  insertDailyTask,
  listDailyTasks,
  updateDailyTaskProgress,
} from "../db/repositories/user-daily-tasks";
import type { UserDailyTaskRow } from "../db/repositories/user-daily-tasks";
import {
  claimUserAchievement,
  findUserAchievement,
  listUserAchievements,
  upsertUserAchievement,
} from "../db/repositories/user-achievements";
import { ensureWallet, insertLedger, updateWalletBalance } from "../db/repositories/wallets";
import { ensureAchievementSchema } from "../db/schema-achievements";
import type { AppConfig } from "../utils/config";
import { AppError } from "../utils/errors";
import type { JsonObject } from "../utils/validate";
import { localDateKey } from "../utils/time";

/* 单条统计键单次上报的上限, 以及一次上报所有键增量的总和上限. */
const MAX_STAT_DELTA = 100_000;
const MAX_REPORT_TOTAL = 500_000;

/* 记账原因, 与钱包流水表 reason 列取值一致. */
const REASON_ACHIEVEMENT = "achievement";
const REASON_DAILY_TASK = "daily_task";

const STAT_KEY_SET: ReadonlySet<string> = new Set<string>(STAT_KEYS);

const DAILY_TEMPLATE_BY_ID: ReadonlyMap<number, DailyTaskTemplate> = new Map(
  DAILY_TASK_TEMPLATES.map((item) => [item.id, item]),
);

export interface AchievementProgressView {
  id: number;
  name: string;
  description: string;
  category: number;
  categoryName: string;
  kind: StatKey;
  kindTitle: string;
  unit: string;
  target: number;
  value: number;
  completed: boolean;
  completedAt: number | null;
  /* 奖励是否已领取: 完成与领取分离, 领取后才入账. */
  claimed: boolean;
  claimedAt: number | null;
  coinReward: number;
}

export interface AchievementSummary {
  completed: number;
  total: number;
  /* 已领取条数与已入账镜币, 与完成数分开统计. */
  claimed: number;
  coins: number;
}

export interface DailyTaskView {
  slot: number;
  templateId: number;
  name: string;
  kind: StatKey;
  kindTitle: string;
  unit: string;
  target: number;
  value: number;
  completed: boolean;
  completedAt: number | null;
  claimed: boolean;
  claimedAt: number | null;
  coinReward: number;
}

export interface DailyBoard {
  date: string;
  tasks: DailyTaskView[];
  completed: number;
  claimed: number;
  coins: number;
}

/* 本次上报新发放的一条奖励, 供客户端做完成动画与提示. */
export interface GrantRecord {
  source: "achievement" | "daily_task";
  id: number;
  name: string;
  coins: number;
}

export interface AchievementStatus {
  balance: number;
  achievements: AchievementProgressView[];
  summary: AchievementSummary;
  daily: DailyBoard;
}

export interface ReportResult extends AchievementStatus {
  /* 本次上报新完成的条目, 完成不等于入账, 入账由领取接口完成. */
  granted: GrantRecord[];
}

/* 领取结果: 领取后的余额与最新视图, granted 为本次入账的那一条. */
export interface ClaimResult extends AchievementStatus {
  granted: GrantRecord[];
}

/**
 * 32 位 FNV-1a 散列
 * 每日任务的模板抽取以日期键为种子, 客户端 native/progress/progress_types.cpp 的 Fnv1a32
 * 是同一算法的另一份实现, 两边必须保持一致, 否则同一天会选出不同的任务.
 */
export function fnv1a32(text: string): number {
  let hash = 2166136261;
  for (let index = 0; index < text.length; index += 1) {
    hash ^= text.charCodeAt(index) & 0xff;
    hash = Math.imul(hash, 16777619) >>> 0;
  }
  return hash >>> 0;
}

/* 线性同余随机数: 与客户端每日任务抽取使用的参数一致. */
function nextSeed(seed: number): number {
  return (Math.imul(seed, 1664525) + 1013904223) >>> 0;
}

/**
 * 按日期键抽取当日任务模板
 * 先用 FNV-1a 把日期键折成种子, 再用同一随机序列对模板下标做 Fisher-Yates 洗牌,
 * 取前 count 条. 结果只依赖日期键, 因此同一天所有账号的任务相同, 且可重放.
 */
export function pickDailyTemplates(dateKey: string, count: number = DAILY_TASK_COUNT): readonly DailyTaskTemplate[] {
  const order: number[] = DAILY_TASK_TEMPLATES.map((_item, index) => index);
  let seed = fnv1a32(dateKey);
  for (let index = order.length - 1; index > 0; index -= 1) {
    seed = nextSeed(seed);
    const swap = seed % (index + 1);
    const left = order[index] ?? 0;
    const right = order[swap] ?? 0;
    order[index] = right;
    order[swap] = left;
  }
  const picked: DailyTaskTemplate[] = [];
  for (const position of order) {
    if (picked.length >= count) {
      break;
    }
    const template = DAILY_TASK_TEMPLATES[position];
    if (template) {
      picked.push(template);
    }
  }
  return picked;
}

/**
 * 校验上报的增量统计
 * 只接受 STAT_KEYS 内的键与非负整数增量; 未知键, 负数, 小数与超限值一律按参数非法处理,
 * 避免客户端异常值把进度一次性灌满.
 */
export function parseReportStats(body: JsonObject): Map<StatKey, number> {
  const raw = body.stats;
  if (raw === null || typeof raw !== "object" || Array.isArray(raw)) {
    throw new AppError("BAD_REQUEST", "stats 必须是对象");
  }
  const result = new Map<StatKey, number>();
  let total = 0;
  for (const [key, value] of Object.entries(raw as Record<string, unknown>)) {
    if (!STAT_KEY_SET.has(key)) {
      throw new AppError("BAD_REQUEST", `未知的统计键: ${key}`);
    }
    if (typeof value !== "number" || !Number.isInteger(value)) {
      throw new AppError("BAD_REQUEST", "统计增量必须是整数");
    }
    if (value < 0) {
      throw new AppError("BAD_REQUEST", "统计增量不得为负数");
    }
    if (value > MAX_STAT_DELTA) {
      throw new AppError("BAD_REQUEST", "单条统计增量超出上限");
    }
    total += value;
    result.set(key as StatKey, value);
  }
  if (result.size === 0) {
    throw new AppError("BAD_REQUEST", "stats 不能为空");
  }
  if (total > MAX_REPORT_TOTAL) {
    throw new AppError("BAD_REQUEST", "本次上报总量超出上限");
  }
  return result;
}

export class AchievementService {
  constructor(
    private readonly db: SqliteDatabase,
    private readonly config: AppConfig,
    private readonly now: () => number,
  ) {
    // 建表语句幂等, 主线把片段并入 db/schema.ts 后这里退化为空操作
    ensureAchievementSchema(this.db);
  }

  /* 当前自然日键, 按配置时区计算, 与签到使用同一套口径. */
  todayKey(): string {
    return localDateKey(this.now(), this.config.timeZone);
  }

  /**
   * 生成当日任务并返回当日全部任务行
   * 模板由日期键确定性选出, 插入带唯一约束兜底, 重复调用不会产生重复行;
   * 同时清理更早日期的行, 避免任务表随天数无限增长.
   */
  ensureDailyTasks(userId: number, date: string = this.todayKey()): UserDailyTaskRow[] {
    const now = this.now();
    const templates = pickDailyTemplates(date, DAILY_TASK_COUNT);
    const run = this.db.transaction((): void => {
      deleteDailyTasksBefore(this.db, userId, date);
      templates.forEach((template, slot) => {
        insertDailyTask(this.db, {
          userId,
          date,
          slot,
          templateId: template.id,
          kind: template.kind,
          target: template.target,
          reward: template.coinReward,
          now,
        });
      });
    });
    run();
    return listDailyTasks(this.db, userId, date);
  }

  /* 全部成就定义与本人进度的合并视图, 未上报过的成就按 0 进度展示. */
  listAchievements(userId: number): AchievementProgressView[] {
    const rows = listUserAchievements(this.db, userId);
    const stored = new Map(rows.map((row) => [row.achievement_id, row]));
    return ACHIEVEMENT_CATALOG.map((definition) => {
      const row = stored.get(definition.id);
      const value = row ? Math.min(row.value, definition.target) : 0;
      const completedAt = row ? row.completed_at : null;
      return {
        id: definition.id,
        name: definition.name,
        description: definition.description,
        category: definition.category,
        categoryName: CATEGORY_NAMES[definition.category] ?? "其他",
        kind: definition.kind,
        kindTitle: STAT_TITLES[definition.kind],
        unit: STAT_UNITS[definition.kind],
        target: definition.target,
        value,
        completed: completedAt !== null,
        completedAt,
        claimed: row ? row.claimed_at !== null : false,
        claimedAt: row ? row.claimed_at : null,
        coinReward: definition.coinReward,
      };
    });
  }

  static summarize(achievements: readonly AchievementProgressView[]): AchievementSummary {
    let completed = 0;
    let claimed = 0;
    let coins = 0;
    for (const item of achievements) {
      if (item.completed) {
        completed += 1;
      }
      /* 镜币只统计已领取的条目, 完成但未领取的奖励还没有入账. */
      if (item.claimed) {
        claimed += 1;
        coins += item.coinReward;
      }
    }
    return { completed, total: achievements.length, claimed, coins };
  }

  dailyBoard(userId: number, date: string = this.todayKey()): DailyBoard {
    const rows = this.ensureDailyTasks(userId, date);
    let completed = 0;
    let claimed = 0;
    let coins = 0;
    const tasks = rows.map((row) => {
      const template = DAILY_TEMPLATE_BY_ID.get(row.template_id);
      if (row.completed_at !== null) {
        completed += 1;
      }
      if (row.claimed_at !== null) {
        claimed += 1;
        coins += row.reward;
      }
      return {
        slot: row.slot,
        templateId: row.template_id,
        name: template ? template.name : `任务 ${row.template_id}`,
        kind: row.kind as StatKey,
        kindTitle: STAT_TITLES[row.kind as StatKey] ?? row.kind,
        unit: STAT_UNITS[row.kind as StatKey] ?? "",
        target: row.target,
        value: Math.min(row.value, row.target),
        completed: row.completed_at !== null,
        completedAt: row.completed_at,
        claimed: row.claimed_at !== null,
        claimedAt: row.claimed_at,
        coinReward: row.reward,
      };
    });
    return { date, tasks, completed, claimed, coins };
  }

  /* 只读查询: 成就, 汇总与当日任务, 余额取自钱包. */
  getStatus(userId: number): AchievementStatus {
    const achievements = this.listAchievements(userId);
    const wallet = ensureWallet(this.db, userId, this.now());
    return {
      balance: wallet.balance,
      achievements,
      summary: AchievementService.summarize(achievements),
      daily: this.dailyBoard(userId),
    };
  }

  /**
   * 上报增量统计并推进成就与当日任务
   * 先写进度再发奖励, 全部在一个事务内; 已完成的成就与任务不再重复发放.
   */
  report(userId: number, increments: ReadonlyMap<StatKey, number>): ReportResult {
    const now = this.now();
    const date = localDateKey(now, this.config.timeZone);
    const run = this.db.transaction((): { granted: GrantRecord[]; balance: number } => {
      this.ensureDailyTasks(userId, date);
      const granted: GrantRecord[] = [];
      const balance = ensureWallet(this.db, userId, now).balance;
      const dailyRows = listDailyTasks(this.db, userId, date);

      for (const [kind, delta] of increments) {
        if (delta <= 0) {
          continue;
        }
        for (const definition of ACHIEVEMENT_CATALOG) {
          if (definition.kind !== kind) {
            continue;
          }
          const row = findUserAchievement(this.db, userId, definition.id);
          if (row && row.completed_at !== null) {
            continue;
          }
          const current = row ? Math.min(row.value, definition.target) : 0;
          const next = Math.min(definition.target, current + delta);
          const completedAt = next >= definition.target ? now : null;
          upsertUserAchievement(this.db, {
            userId,
            achievementId: definition.id,
            value: next,
            target: definition.target,
            completedAt,
            now,
          });
          /* 完成只记账不发放, 镜币由领取接口按条入账. */
          if (completedAt !== null) {
            granted.push({
              source: "achievement",
              id: definition.id,
              name: definition.name,
              coins: definition.coinReward,
            });
          }
        }

        for (const task of dailyRows) {
          if (task.kind !== kind || task.completed_at !== null) {
            continue;
          }
          const next = Math.min(task.target, task.value + delta);
          const completedAt = next >= task.target ? now : null;
          updateDailyTaskProgress(this.db, { userId, date, slot: task.slot, value: next, completedAt });
          if (completedAt !== null) {
            const template = DAILY_TEMPLATE_BY_ID.get(task.template_id);
            granted.push({
              source: "daily_task",
              id: task.template_id,
              name: template ? template.name : `任务 ${task.template_id}`,
              coins: task.reward,
            });
          }
        }
      }
      return { granted, balance };
    });

    const { granted, balance } = run();
    const achievements = this.listAchievements(userId);
    return {
      balance,
      granted,
      achievements,
      summary: AchievementService.summarize(achievements),
      daily: this.dailyBoard(userId, date),
    };
  }

  /**
   * 领取一条成就奖励.
   * 只对已完成且未领取的条目生效; 入账与标记领取在同一事务内,
   * 未完成返回 ACHIEVEMENT_NOT_COMPLETED, 重复领取返回 ACHIEVEMENT_ALREADY_CLAIMED.
   */
  claimAchievement(userId: number, achievementId: number): ClaimResult {
    const definition = ACHIEVEMENT_BY_ID.get(achievementId);
    if (!definition) {
      throw new AppError("ACHIEVEMENT_NOT_FOUND");
    }
    const now = this.now();
    const run = this.db.transaction((): number => {
      const row = findUserAchievement(this.db, userId, achievementId);
      if (!row || row.completed_at === null) {
        throw new AppError("ACHIEVEMENT_NOT_COMPLETED");
      }
      if (row.claimed_at !== null) {
        throw new AppError("ACHIEVEMENT_ALREADY_CLAIMED");
      }
      /* 标记与入账同事务: 标记失败说明并发下已被别的请求领走, 直接按已领取报错. */
      if (claimUserAchievement(this.db, userId, achievementId, now) === 0) {
        throw new AppError("ACHIEVEMENT_ALREADY_CLAIMED");
      }
      const balance = ensureWallet(this.db, userId, now).balance;
      return this.grantCoins(userId, definition.coinReward, REASON_ACHIEVEMENT,
        String(achievementId), now, balance);
    });
    const balance = run();
    const achievements = this.listAchievements(userId);
    return {
      balance,
      granted: [{
        source: "achievement",
        id: achievementId,
        name: definition.name,
        coins: definition.coinReward,
      }],
      achievements,
      summary: AchievementService.summarize(achievements),
      daily: this.dailyBoard(userId),
    };
  }

  /**
   * 批量领取: 把所有已完成未领取的成就与当日任务一次领完.
   * 逐条复用单条领取的入账路径, 因此与逐条点击的结果完全一致;
   * 整批在一个事务里完成, 中途失败不会只落一半.
   */
  claimAll(userId: number): ClaimResult {
    const now = this.now();
    const date = this.todayKey();
    const granted: GrantRecord[] = [];
    const run = this.db.transaction((): number => {
      this.ensureDailyTasks(userId, date);
      let balance = ensureWallet(this.db, userId, now).balance;
      for (const definition of ACHIEVEMENT_CATALOG) {
        const row = findUserAchievement(this.db, userId, definition.id);
        if (!row || row.completed_at === null || row.claimed_at !== null) {
          continue;
        }
        if (claimUserAchievement(this.db, userId, definition.id, now) === 0) {
          continue;
        }
        balance = this.grantCoins(userId, definition.coinReward, REASON_ACHIEVEMENT,
          String(definition.id), now, balance);
        granted.push({
          source: "achievement",
          id: definition.id,
          name: definition.name,
          coins: definition.coinReward,
        });
      }
      for (const task of listDailyTasks(this.db, userId, date)) {
        if (task.completed_at === null || task.claimed_at !== null) {
          continue;
        }
        if (claimDailyTask(this.db, userId, date, task.slot, now) === 0) {
          continue;
        }
        const template = DAILY_TEMPLATE_BY_ID.get(task.template_id);
        balance = this.grantCoins(userId, task.reward, REASON_DAILY_TASK,
          `${date}#${task.slot}`, now, balance);
        granted.push({
          source: "daily_task",
          id: task.template_id,
          name: template ? template.name : `任务 ${task.template_id}`,
          coins: task.reward,
        });
      }
      return balance;
    });
    const balance = run();
    const achievements = this.listAchievements(userId);
    return {
      balance,
      granted,
      achievements,
      summary: AchievementService.summarize(achievements),
      daily: this.dailyBoard(userId, date),
    };
  }

  /**
   * 领取一条每日任务奖励.
   * 日期缺省为今天, 与每日任务板的生成口径一致.
   */
  claimDaily(userId: number, slot: number, date: string = this.todayKey()): ClaimResult {
    const now = this.now();
    const run = this.db.transaction((): { balance: number; task: UserDailyTaskRow } => {
      this.ensureDailyTasks(userId, date);
      const task = listDailyTasks(this.db, userId, date).find((row) => row.slot === slot);
      if (!task) {
        throw new AppError("DAILY_TASK_NOT_FOUND");
      }
      if (task.completed_at === null) {
        throw new AppError("ACHIEVEMENT_NOT_COMPLETED", "任务尚未完成");
      }
      if (task.claimed_at !== null) {
        throw new AppError("ACHIEVEMENT_ALREADY_CLAIMED", "任务奖励已领取");
      }
      if (claimDailyTask(this.db, userId, date, slot, now) === 0) {
        throw new AppError("ACHIEVEMENT_ALREADY_CLAIMED", "任务奖励已领取");
      }
      const balance = ensureWallet(this.db, userId, now).balance;
      const next = this.grantCoins(userId, task.reward, REASON_DAILY_TASK,
        `${date}#${slot}`, now, balance);
      return { balance: next, task };
    });
    const { balance, task } = run();
    const template = DAILY_TEMPLATE_BY_ID.get(task.template_id);
    const achievements = this.listAchievements(userId);
    return {
      balance,
      granted: [{
        source: "daily_task",
        id: task.template_id,
        name: template ? template.name : `任务 ${task.template_id}`,
        coins: task.reward,
      }],
      achievements,
      summary: AchievementService.summarize(achievements),
      daily: this.dailyBoard(userId, date),
    };
  }

  /**
   * 发放镜币
   * 与签到服务同一条记账路径: 先算新余额, 再写流水, 最后更新余额.
   * 调用方必须已在事务内, 这里不额外开事务.
   */
  private grantCoins(
    userId: number,
    amount: number,
    reason: string,
    ref: string,
    now: number,
    balanceBefore: number,
  ): number {
    if (amount <= 0) {
      return balanceBefore;
    }
    const balance = balanceBefore + amount;
    insertLedger(this.db, { userId, delta: amount, balanceAfter: balance, reason, ref, now });
    updateWalletBalance(this.db, userId, balance, now);
    return balance;
  }
}
