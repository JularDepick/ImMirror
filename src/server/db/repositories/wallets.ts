/**
 * 钱包与流水表数据访问
 * 余额只允许非负, 变动同时在 wallet_ledger 留痕
 */

import type { SqliteDatabase } from "../client";

export interface WalletRow {
  user_id: number;
  balance: number;
  updated_at: number;
}

export interface LedgerRow {
  id: number;
  user_id: number;
  delta: number;
  balance_after: number;
  reason: string;
  ref: string | null;
  created_at: number;
}

export function ensureWallet(db: SqliteDatabase, userId: number, now: number): WalletRow {
  db.prepare("INSERT OR IGNORE INTO wallets (user_id, balance, updated_at) VALUES (?, 0, ?)").run(userId, now);
  const wallet = findWallet(db, userId);
  if (!wallet) {
    throw new Error("钱包创建后未能读回");
  }
  return wallet;
}

export function findWallet(db: SqliteDatabase, userId: number): WalletRow | undefined {
  return db.prepare("SELECT * FROM wallets WHERE user_id = ?").get(userId) as WalletRow | undefined;
}

export function updateWalletBalance(db: SqliteDatabase, userId: number, balance: number, now: number): void {
  db.prepare("UPDATE wallets SET balance = ?, updated_at = ? WHERE user_id = ?").run(balance, now, userId);
}

export function insertLedger(
  db: SqliteDatabase,
  input: { userId: number; delta: number; balanceAfter: number; reason: string; ref: string | null; now: number },
): void {
  db.prepare(
    "INSERT INTO wallet_ledger (user_id, delta, balance_after, reason, ref, created_at) VALUES (?, ?, ?, ?, ?, ?)",
  ).run(input.userId, input.delta, input.balanceAfter, input.reason, input.ref, input.now);
}

export function findLedgerByUser(db: SqliteDatabase, userId: number, limit = 20): LedgerRow[] {
  return db
    .prepare("SELECT * FROM wallet_ledger WHERE user_id = ? ORDER BY id DESC LIMIT ?")
    .all(userId, limit) as LedgerRow[];
}
