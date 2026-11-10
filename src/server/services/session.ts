/**
 * 会话服务
 * 令牌 32 字节随机, 数据库只存 SHA-256; 每次鉴权按需滑动续期
 */

import type { SqliteDatabase } from "../db/client";
import type { SessionRow } from "../db/repositories/sessions";
import {
  findSessionByTokenHash,
  insertSession,
  revokeAllSessions,
  revokeSession,
  touchSessionExpiry,
} from "../db/repositories/sessions";
import { findUserById } from "../db/repositories/users";
import type { UserRow } from "../db/repositories/users";
import type { AppConfig } from "../utils/config";
import { AppError } from "../utils/errors";
import { randomToken, sha256Hex } from "../utils/random";

export interface SessionMeta {
  ip: string | null;
  ua: string | null;
}

export interface CreatedSession {
  token: string;
  expiresAt: number;
}

export interface AuthContext {
  user: UserRow;
  session: SessionRow;
}

export class SessionService {
  constructor(
    private readonly db: SqliteDatabase,
    private readonly config: AppConfig,
    private readonly now: () => number,
  ) {}

  private get ttlMs(): number {
    return this.config.sessionTtlDays * 24 * 60 * 60 * 1000;
  }

  create(userId: number, meta: SessionMeta): CreatedSession {
    const current = this.now();
    const token = randomToken(32);
    const expiresAt = current + this.ttlMs;
    insertSession(this.db, {
      userId,
      tokenHash: sha256Hex(token),
      ip: meta.ip,
      ua: meta.ua,
      now: current,
      expiresAt,
    });
    return { token, expiresAt };
  }

  /**
   * 校验令牌; 剩余时长不足完整有效期时滑动续期
   */
  authenticate(token: string): AuthContext {
    const row = findSessionByTokenHash(this.db, sha256Hex(token));
    if (!row || row.revoked_at !== null) {
      throw new AppError("AUTH_UNAUTHORIZED");
    }
    const current = this.now();
    if (row.expires_at <= current) {
      throw new AppError("AUTH_SESSION_EXPIRED");
    }
    const user = findUserById(this.db, row.user_id);
    if (!user) {
      throw new AppError("AUTH_UNAUTHORIZED");
    }
    const renewedExpiry = current + this.ttlMs;
    if (row.expires_at < renewedExpiry) {
      touchSessionExpiry(this.db, row.id, renewedExpiry);
      row.expires_at = renewedExpiry;
    }
    return { user, session: row };
  }

  revokeByToken(token: string): void {
    const row = findSessionByTokenHash(this.db, sha256Hex(token));
    if (row) {
      revokeSession(this.db, row.id, this.now());
    }
  }

  revokeAllForUser(userId: number): void {
    revokeAllSessions(this.db, userId, this.now());
  }
}
