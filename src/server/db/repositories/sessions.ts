/**
 * 会话表数据访问
 * 数据库只保存令牌摘要, 明文令牌只在登录响应中出现
 */

import type { SqliteDatabase } from "../client";

export interface SessionRow {
  id: number;
  user_id: number;
  token_hash: string;
  ip: string | null;
  ua: string | null;
  created_at: number;
  expires_at: number;
  revoked_at: number | null;
}

export interface NewSessionInput {
  userId: number;
  tokenHash: string;
  ip: string | null;
  ua: string | null;
  now: number;
  expiresAt: number;
}

export function insertSession(db: SqliteDatabase, input: NewSessionInput): SessionRow {
  const result = db
    .prepare(
      "INSERT INTO sessions (user_id, token_hash, ip, ua, created_at, expires_at) VALUES (?, ?, ?, ?, ?, ?)",
    )
    .run(input.userId, input.tokenHash, input.ip, input.ua, input.now, input.expiresAt);
  const row = db.prepare("SELECT * FROM sessions WHERE id = ?").get(Number(result.lastInsertRowid)) as
    | SessionRow
    | undefined;
  if (!row) {
    throw new Error("会话写入后未能读回");
  }
  return row;
}

export function findSessionByTokenHash(db: SqliteDatabase, tokenHash: string): SessionRow | undefined {
  return db.prepare("SELECT * FROM sessions WHERE token_hash = ?").get(tokenHash) as SessionRow | undefined;
}

export function touchSessionExpiry(db: SqliteDatabase, sessionId: number, expiresAt: number): void {
  db.prepare("UPDATE sessions SET expires_at = ? WHERE id = ?").run(expiresAt, sessionId);
}

export function revokeSession(db: SqliteDatabase, sessionId: number, now: number): void {
  db.prepare("UPDATE sessions SET revoked_at = ? WHERE id = ? AND revoked_at IS NULL").run(now, sessionId);
}

export function revokeAllSessions(db: SqliteDatabase, userId: number, now: number): void {
  db.prepare("UPDATE sessions SET revoked_at = ? WHERE user_id = ? AND revoked_at IS NULL").run(now, userId);
}
