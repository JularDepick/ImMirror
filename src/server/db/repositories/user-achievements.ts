/**
 * 用户成就进度表数据访问
 * 进度只增不减, 完成时刻一旦写入不再清空; 主键 (user_id, achievement_id) 保证一人一成就一行
 */

import type { SqliteDatabase } from "../client";

export interface UserAchievementRow {
  user_id: number;
  achievement_id: number;
  value: number;
  target: number;
  completed_at: number | null;
  updated_at: number;
}

export interface UpsertAchievementInput {
  userId: number;
  achievementId: number;
  value: number;
  target: number;
  completedAt: number | null;
  now: number;
}

export function listUserAchievements(db: SqliteDatabase, userId: number): UserAchievementRow[] {
  return db
    .prepare("SELECT * FROM user_achievements WHERE user_id = ? ORDER BY achievement_id")
    .all(userId) as UserAchievementRow[];
}

export function findUserAchievement(
  db: SqliteDatabase,
  userId: number,
  achievementId: number,
): UserAchievementRow | undefined {
  return db
    .prepare("SELECT * FROM user_achievements WHERE user_id = ? AND achievement_id = ?")
    .get(userId, achievementId) as UserAchievementRow | undefined;
}

/**
 * 写入或覆盖一条进度. completed_at 只在首次完成时由调用方给出,
 * 之后即便再次上报也不会改动, 因此这里按整行覆盖而不单独判断.
 */
export function upsertUserAchievement(db: SqliteDatabase, input: UpsertAchievementInput): void {
  db.prepare(
    `INSERT INTO user_achievements (user_id, achievement_id, value, target, completed_at, updated_at)
     VALUES (?, ?, ?, ?, ?, ?)
     ON CONFLICT(user_id, achievement_id) DO UPDATE SET
       value = excluded.value,
       target = excluded.target,
       completed_at = COALESCE(user_achievements.completed_at, excluded.completed_at),
       updated_at = excluded.updated_at`,
  ).run(input.userId, input.achievementId, input.value, input.target, input.completedAt, input.now);
}

export function countCompleted(db: SqliteDatabase, userId: number): number {
  const row = db
    .prepare("SELECT COUNT(*) AS total FROM user_achievements WHERE user_id = ? AND completed_at IS NOT NULL")
    .get(userId) as { total: number };
  return row.total;
}
