/**
 * 应用上下文
 * 组装后由路由层与 Nitro 插件共同持有
 */

import type { FastifyInstance } from "fastify";
import type { SqliteDatabase } from "../db/client";
import type { AuthService } from "../services/auth";
import type { MailService } from "../services/mail";
import type { FailureBackoff, WindowLimiter } from "../services/rate-limit";
import type { SessionService } from "../services/session";
import type { WalletService } from "../services/wallet";
import type { AppConfig } from "../utils/config";

export interface AppLimiters {
  sendByIp: WindowLimiter;
  resetByEmail: WindowLimiter;
  loginByAccount: FailureBackoff;
  loginByIp: FailureBackoff;
  requestByIp: WindowLimiter;
}

export interface AppContext {
  config: AppConfig;
  db: SqliteDatabase;
  mail: MailService;
  session: SessionService;
  auth: AuthService;
  wallet: WalletService;
  limiters: AppLimiters;
  fastify: FastifyInstance;
  now: () => number;
  close(): Promise<void>;
}
