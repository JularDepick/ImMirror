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
  return db;
}

export function closeDatabase(db: SqliteDatabase): void {
  if (db.open) {
    db.close();
  }
}
