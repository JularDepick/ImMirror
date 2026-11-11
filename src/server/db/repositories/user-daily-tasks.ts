/**
 * 用户每日任务表数据访问
 * 每人每天每条任务一行, 唯一约束 (user_id, date, slot) 保证同日同槽位只有一条;
 * 任务由模板按日期键确定性生成, 生成后不再变动, 只更新进度与完成时刻
 */

import type { SqliteDatabase } from "../client";

export interface UserDailyTaskRow {
  id: number;
  user_id: number;
  date: string;
  slot: number;
  template_id: number;
  kind: string;
  target: number;
  value: number;
  reward: number;
  completed_at: number | null;
  created_at: number;
}

export interface NewDailyTaskInput {
  userId: number;
  date: string;
  slot: number;
  templateId: number;
  kind: string;
  target: number;
  reward: number;
  now: number;
}

export function listDailyTasks(db: SqliteDatabase, userId: number, date: string): UserDailyTaskRow[] {
  return db
    .prepare("SELECT * FROM user_daily_tasks WHERE user_id = ? AND date = ? ORDER BY slot")
    .all(userId, date) as UserDailyTaskRow[];
}

export function insertDailyTask(db: SqliteDatabase, input: NewDailyTaskInput): void {
  db.prepare(
    `INSERT INTO user_daily_tasks (user_id, date, slot, template_id, kind, target, value, reward, created_at)
     VALUES (?, ?, ?, ?, ?, ?, 0, ?, ?)
     ON CONFLICT(user_id, date, slot) DO NOTHING`,
  ).run(
    input.userId,
    input.date,
    input.slot,
    input.templateId,
    input.kind,
    input.target,
    input.reward,
    input.now,
  );
}

/* 更新当日进度; completed_at 同样只写一次, 重复完成不会改写完成时刻. */
export function updateDailyTaskProgress(
  db: SqliteDatabase,
  input: { userId: number; date: string; slot: number; value: number; completedAt: number | null },
): void {
  db.prepare(
    `UPDATE user_daily_tasks
       SET value = ?,
           completed_at = COALESCE(completed_at, ?)
     WHERE user_id = ? AND date = ? AND slot = ?`,
  ).run(input.value, input.completedAt, input.userId, input.date, input.slot);
}

/**
 * 清理早于指定日期的任务行. 日期键为年-月-日定宽文本, 字典序与时间序一致,
 * 因此可以直接用字符串比较, 不需要按时区重新解析.
 */
export function deleteDailyTasksBefore(db: SqliteDatabase, userId: number, date: string): number {
  const result = db
    .prepare("DELETE FROM user_daily_tasks WHERE user_id = ? AND date < ?")
    .run(userId, date);
  return result.changes;
}
