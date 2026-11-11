/**
 * ImMirror 我之镜界 服务端 作者: JularDepick
 * 成就与每日任务的数据库结构
 *
 * 本文件是 db/schema.ts 的增量片段: 两表由成就模块独占,
 * 主线把 ACHIEVEMENT_SCHEMA_SQL 并入 SCHEMA_SQL 后,
 * 这里只保留建表语句与幂等入口, 不再单独调用 ensureAchievementSchema.
 * 时间戳沿用全库约定, 一律为 Unix 毫秒整数; 外键级联删除用户数据.
 */

import type { SqliteDatabase } from "./client";

export const ACHIEVEMENT_SCHEMA_SQL = `
CREATE TABLE IF NOT EXISTS user_achievements (
  user_id        INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
  achievement_id INTEGER NOT NULL,
  value          INTEGER NOT NULL DEFAULT 0,
  target         INTEGER NOT NULL,
  completed_at   INTEGER,
  updated_at     INTEGER NOT NULL,
  PRIMARY KEY (user_id, achievement_id)
);

CREATE INDEX IF NOT EXISTS idx_user_achievements_completed
  ON user_achievements(user_id, completed_at);

CREATE TABLE IF NOT EXISTS user_daily_tasks (
  id           INTEGER PRIMARY KEY AUTOINCREMENT,
  user_id      INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
  date         TEXT NOT NULL,
  slot         INTEGER NOT NULL,
  template_id  INTEGER NOT NULL,
  kind         TEXT NOT NULL,
  target       INTEGER NOT NULL,
  value        INTEGER NOT NULL DEFAULT 0,
  reward       INTEGER NOT NULL,
  completed_at INTEGER,
  created_at   INTEGER NOT NULL,
  UNIQUE(user_id, date, slot)
);

CREATE INDEX IF NOT EXISTS idx_user_daily_tasks_date
  ON user_daily_tasks(user_id, date);
`;

/**
 * 幂等建表. 建表语句只含 IF NOT EXISTS, 重复调用不会改动已有数据,
 * 因此在路由注册阶段兜底执行一次即可, 不依赖主线是否已把片段并入 schema.ts.
 */
export function ensureAchievementSchema(db: SqliteDatabase): void {
  db.exec(ACHIEVEMENT_SCHEMA_SQL);
}
