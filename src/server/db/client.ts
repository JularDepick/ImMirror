/**
 * SQLite 连接与建表
 * 使用 better-sqlite3 同步驱动, 开启 WAL 与外键约束
 */

import { mkdirSync } from "node:fs";
import { dirname, resolve } from "node:path";
import Database from "better-sqlite3";
import { SCHEMA_SQL } from "./schema";

export type SqliteDatabase = Database.Database;

/**
 * 打开数据库并完成初始化
 * @param dbFile 数据库文件路径, 传 :memory: 时使用内存库 (测试用)
 */
export function openDatabase(dbFile: string): SqliteDatabase {
  if (dbFile !== ":memory:") {
    mkdirSync(dirname(resolve(dbFile)), { recursive: true });
  }
  const db = new Database(dbFile);
  db.pragma("journal_mode = WAL");
  db.pragma("synchronous = NORMAL");
  db.pragma("foreign_keys = ON");
  db.pragma("busy_timeout = 5000");
  db.exec(SCHEMA_SQL);
  ensureUserColumns(db);
  return db;
}

/**
 * 早期库缺少的列在这里补上.
 * 建表语句只含 IF NOT EXISTS, 已存在的表不会被改动, 因此新增列必须单独补.
 */
function ensureUserColumns(db: SqliteDatabase): void {
  const columns = db.prepare("PRAGMA table_info(users)").all() as { name: string }[];
  const has = (name: string) => columns.some((column) => column.name === name);
  if (!has("skin_id")) {
    db.exec("ALTER TABLE users ADD COLUMN skin_id INTEGER NOT NULL DEFAULT 0");
  }
}

export function closeDatabase(db: SqliteDatabase): void {
  if (db.open) {
    db.close();
  }
}
