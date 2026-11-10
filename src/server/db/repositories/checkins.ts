/**
 * 签到表数据访问
 * 唯一约束 (user_id, date) 是并发重复签到的最终兜底
 */

import type { SqliteDatabase } from "../client";

export interface CheckinRow {
  id: number;
  user_id: number;
  date: string;
  month: string;
  reward: number;
  created_at: number;
}

export interface NewCheckinInput {
  userId: number;
  date: string;
  month: string;
  reward: number;
  now: number;
}

export function insertCheckin(db: SqliteDatabase, input: NewCheckinInput): void {
  db.prepare("INSERT INTO checkins (user_id, date, month, reward, created_at) VALUES (?, ?, ?, ?, ?)").run(
    input.userId,
    input.date,
    input.month,
    input.reward,
    input.now,
  );
}

export function findCheckinByDate(db: SqliteDatabase, userId: number, date: string): CheckinRow | undefined {
  return db.prepare("SELECT * FROM checkins WHERE user_id = ? AND date = ?").get(userId, date) as
    | CheckinRow
    | undefined;
}

export function countCheckinsInMonth(db: SqliteDatabase, userId: number, month: string): number {
  const row = db.prepare("SELECT COUNT(*) AS total FROM checkins WHERE user_id = ? AND month = ?").get(userId, month) as {
    total: number;
  };
  return row.total;
}
