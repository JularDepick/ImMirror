/**
 * 用户表数据访问
 * 只做 SQL 与行到对象的映射, 不包含业务判断
 */

import type { SqliteDatabase } from "../client";

export interface UserRow {
  id: number;
  uid: string;
  email: string;
  password_hash: string;
  status: string;
  email_verified_at: number | null;
  created_at: number;
  updated_at: number;
}

export interface NewUserInput {
  uid: string;
  email: string;
  passwordHash: string;
  now: number;
}

export function findUserByEmail(db: SqliteDatabase, email: string): UserRow | undefined {
  return db.prepare("SELECT * FROM users WHERE email = ?").get(email) as UserRow | undefined;
}

export function findUserByUid(db: SqliteDatabase, uid: string): UserRow | undefined {
  return db.prepare("SELECT * FROM users WHERE uid = ?").get(uid) as UserRow | undefined;
}

export function findUserById(db: SqliteDatabase, id: number): UserRow | undefined {
  return db.prepare("SELECT * FROM users WHERE id = ?").get(id) as UserRow | undefined;
}

export function insertUser(db: SqliteDatabase, input: NewUserInput): UserRow {
  const result = db
    .prepare(
      `INSERT INTO users (uid, email, password_hash, status, email_verified_at, created_at, updated_at)
       VALUES (?, ?, ?, 'active', ?, ?, ?)`,
    )
    .run(input.uid, input.email, input.passwordHash, input.now, input.now, input.now);
  const created = findUserById(db, Number(result.lastInsertRowid));
  if (!created) {
    throw new Error("用户写入后未能读回");
  }
  return created;
}

export function updateUserEmail(db: SqliteDatabase, userId: number, email: string, now: number): void {
  db.prepare("UPDATE users SET email = ?, email_verified_at = ?, updated_at = ? WHERE id = ?").run(
    email,
    now,
    now,
    userId,
  );
}

export function updateUserPassword(db: SqliteDatabase, userId: number, passwordHash: string, now: number): void {
  db.prepare("UPDATE users SET password_hash = ?, updated_at = ? WHERE id = ?").run(passwordHash, now, userId);
}
