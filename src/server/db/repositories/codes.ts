/**
 * 验证码表数据访问
 * 表里只存验证码摘要, 明文由邮件通道送达
 */

import type { SqliteDatabase } from "../client";

export type CodePurpose = "register" | "login" | "reset" | "rebind";

export interface CodeRow {
  id: number;
  email: string;
  purpose: string;
  code_hash: string;
  expires_at: number;
  attempts: number;
  consumed_at: number | null;
  created_at: number;
}

export interface NewCodeInput {
  email: string;
  purpose: CodePurpose;
  codeHash: string;
  expiresAt: number;
  now: number;
}

export function insertCode(db: SqliteDatabase, input: NewCodeInput): void {
  db.prepare(
    "INSERT INTO verification_codes (email, purpose, code_hash, expires_at, attempts, created_at) VALUES (?, ?, ?, ?, 0, ?)",
  ).run(input.email, input.purpose, input.codeHash, input.expiresAt, input.now);
}

/**
 * 取某邮箱某用途最新一条未消费验证码
 */
export function findLatestActiveCode(
  db: SqliteDatabase,
  email: string,
  purpose: CodePurpose,
): CodeRow | undefined {
  return db
    .prepare(
      `SELECT * FROM verification_codes
       WHERE email = ? AND purpose = ? AND consumed_at IS NULL
       ORDER BY id DESC LIMIT 1`,
    )
    .get(email, purpose) as CodeRow | undefined;
}

export function increaseCodeAttempts(db: SqliteDatabase, codeId: number): void {
  db.prepare("UPDATE verification_codes SET attempts = attempts + 1 WHERE id = ?").run(codeId);
}

export function consumeCode(db: SqliteDatabase, codeId: number, now: number): void {
  db.prepare("UPDATE verification_codes SET consumed_at = ? WHERE id = ?").run(now, codeId);
}

export function countCodesSince(db: SqliteDatabase, email: string, since: number): number {
  const row = db
    .prepare("SELECT COUNT(*) AS total FROM verification_codes WHERE email = ? AND created_at >= ?")
    .get(email, since) as { total: number };
  return row.total;
}

export function findLatestCodeCreatedAt(db: SqliteDatabase, email: string, purpose: CodePurpose): number | null {
  const row = db
    .prepare("SELECT created_at FROM verification_codes WHERE email = ? AND purpose = ? ORDER BY id DESC LIMIT 1")
    .get(email, purpose) as { created_at: number } | undefined;
  return row ? row.created_at : null;
}

/**
 * 消费某邮箱某用途的全部未消费验证码 (改密后使其失效)
 */
export function consumeAllCodes(db: SqliteDatabase, email: string, purpose: CodePurpose, now: number): void {
  db.prepare(
    "UPDATE verification_codes SET consumed_at = ? WHERE email = ? AND purpose = ? AND consumed_at IS NULL",
  ).run(now, email, purpose);
}
