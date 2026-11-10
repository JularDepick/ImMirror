/**
 * 钱包与签到服务
 * 签到周期为自然月, 每日一次; 单次奖励按当月累计次数递增并有上限, 第 15 与第 25 次有里程碑奖励
 */

import type { SqliteDatabase } from "../db/client";
import { countCheckinsInMonth, findCheckinByDate, insertCheckin } from "../db/repositories/checkins";
import {
  ensureWallet,
  findLedgerByUser,
  findWallet,
  insertLedger,
  updateWalletBalance,
} from "../db/repositories/wallets";
import type { LedgerRow } from "../db/repositories/wallets";
import type { AppConfig } from "../utils/config";
import { AppError } from "../utils/errors";
import { localDateKey, localMonthKey } from "../utils/time";

// 签到奖励常量: 单次基础 100, 每多签一天加 10, 单次上限 200
const DAILY_BASE_REWARD = 100;
const DAILY_STEP_REWARD = 10;
const DAILY_MAX_REWARD = 200;

// 里程碑: 当月累计第 15 次与第 25 次额外奖励
export const CHECKIN_MILESTONES = [
  { count: 15, reward: 500 },
  { count: 25, reward: 1000 },
] as const;

export interface WalletSummary {
  balance: number;
  updatedAt: number;
}

export interface CheckinMilestoneState {
  count: number;
  reward: number;
  achieved: boolean;
}

export interface CheckinStatus {
  signedToday: boolean;
  monthCount: number;
  month: string;
  nextReward: number | null;
  todayReward: number | null;
  milestones: CheckinMilestoneState[];
}

export interface CheckinResult {
  reward: number;
  balance: number;
  monthCount: number;
  milestoneReward: number;
}

export class WalletService {
  constructor(
    private readonly db: SqliteDatabase,
    private readonly config: AppConfig,
    private readonly now: () => number,
  ) {}

  /**
   * 按次数计算单次签到奖励, 次数包含本次
   */
  static rewardForCount(monthCount: number): number {
    const raw = DAILY_BASE_REWARD + (monthCount - 1) * DAILY_STEP_REWARD;
    return Math.min(raw, DAILY_MAX_REWARD);
  }

  static milestoneRewardForCount(monthCount: number): number {
    const milestone = CHECKIN_MILESTONES.find((item) => item.count === monthCount);
    return milestone ? milestone.reward : 0;
  }

  ensure(userId: number): WalletSummary {
    const wallet = ensureWallet(this.db, userId, this.now());
    return { balance: wallet.balance, updatedAt: wallet.updated_at };
  }

  getSummary(userId: number): WalletSummary {
    const wallet = findWallet(this.db, userId);
    if (!wallet) {
      return this.ensure(userId);
    }
    return { balance: wallet.balance, updatedAt: wallet.updated_at };
  }

  listLedger(userId: number, limit = 20): LedgerRow[] {
    return findLedgerByUser(this.db, userId, limit);
  }

  getCheckinStatus(userId: number): CheckinStatus {
    const current = this.now();
    const date = localDateKey(current, this.config.timeZone);
    const month = localMonthKey(current, this.config.timeZone);
    const monthCount = countCheckinsInMonth(this.db, userId, month);
    const today = findCheckinByDate(this.db, userId, date);
    const signedToday = today !== undefined;
    return {
      signedToday,
      monthCount,
      month,
      nextReward: signedToday ? null : WalletService.rewardForCount(monthCount + 1),
      todayReward: signedToday ? today.reward : null,
      milestones: CHECKIN_MILESTONES.map((item) => ({
        count: item.count,
        reward: item.reward,
        achieved: monthCount >= item.count,
      })),
    };
  }

  /**
   * 执行当日签到
   * 插签到与加余额在同一事务内完成, 唯一约束兜底并发重复提交
   */
  checkin(userId: number): CheckinResult {
    const current = this.now();
    const date = localDateKey(current, this.config.timeZone);
    const month = localMonthKey(current, this.config.timeZone);

    const run = this.db.transaction((): CheckinResult => {
      const existing = findCheckinByDate(this.db, userId, date);
      if (existing) {
        throw new AppError("WALLET_ALREADY_CHECKED_IN");
      }
      const monthCount = countCheckinsInMonth(this.db, userId, month) + 1;
      const reward = WalletService.rewardForCount(monthCount);
      const milestoneReward = WalletService.milestoneRewardForCount(monthCount);
      const wallet = ensureWallet(this.db, userId, current);
      insertCheckin(this.db, { userId, date, month, reward, now: current });
      let balance = wallet.balance + reward;
      insertLedger(this.db, {
        userId,
        delta: reward,
        balanceAfter: balance,
        reason: "checkin",
        ref: date,
        now: current,
      });
      if (milestoneReward > 0) {
        balance += milestoneReward;
        insertLedger(this.db, {
          userId,
          delta: milestoneReward,
          balanceAfter: balance,
          reason: "checkin_milestone",
          ref: `${month}#${monthCount}`,
          now: current,
        });
      }
      updateWalletBalance(this.db, userId, balance, current);
      return { reward, balance, monthCount, milestoneReward };
    });

    try {
      return run();
    } catch (error) {
      const message = error instanceof Error ? error.message : String(error);
      if (message.includes("UNIQUE constraint failed: checkins.user_id, checkins.date")) {
        throw new AppError("WALLET_ALREADY_CHECKED_IN");
      }
      throw error;
    }
  }
}
