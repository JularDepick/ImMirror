/**
 * 账号服务
 * 覆盖注册, 邮箱或 UID 加密码登录, 邮箱验证码登录, 忘记密码重置, 换绑邮箱与登出;
 * 验证码发送与校验、登录失败退避都收敛在本服务内, 路由层只做参数提取
 */

import type { SqliteDatabase } from "../db/client";
import {
  consumeAllCodes,
  consumeCode,
  countCodesSince,
  findLatestActiveCode,
  findLatestCodeCreatedAt,
  increaseCodeAttempts,
  insertCode,
} from "../db/repositories/codes";
import type { CodePurpose } from "../db/repositories/codes";
import {
  findUserByEmail,
  findUserById,
  findUserByUid,
  insertUser,
  updateUserEmail,
  updateUserPassword,
} from "../db/repositories/users";
import type { UserRow } from "../db/repositories/users";
import type { AppConfig } from "../utils/config";
import { AppError } from "../utils/errors";
import {
  burnPasswordCheck,
  hashPassword,
  verifyPassword,
} from "../utils/password";
import { randomNumericCode, randomUid, safeEqualHex, sha256Hex } from "../utils/random";
import { isUidLike, requireCode, requireEmail, requireIdentifier, requirePassword } from "../utils/validate";
import type { MailService } from "./mail";
import type { FailureBackoff, WindowLimiter } from "./rate-limit";
import type { SessionMeta, SessionService } from "./session";
import type { WalletService } from "./wallet";

const DAY_MS = 24 * 60 * 60 * 1000;
const HOUR_MS = 60 * 60 * 1000;
const UID_RETRY_LIMIT = 10;

export interface AuthLimiters {
  sendByIp: WindowLimiter;
  resetByEmail: WindowLimiter;
  loginByAccount: FailureBackoff;
  loginByIp: FailureBackoff;
}

export interface AuthDependencies {
  db: SqliteDatabase;
  config: AppConfig;
  mail: MailService;
  session: SessionService;
  wallet: WalletService;
  limiters: AuthLimiters;
  now: () => number;
}

export interface CodeDelivery {
  ttl: number;
  cooldown: number;
}

export interface LoginResult {
  uid: string;
  email: string;
  token: string;
  expiresAt: number;
}

export interface RegisterInput {
  email: unknown;
  code: unknown;
  password: unknown;
}

export interface LoginInput {
  identifier: unknown;
  password: unknown;
}

export interface CodeLoginInput {
  email: unknown;
  code: unknown;
}

export interface ResetPasswordInput {
  email: unknown;
  code: unknown;
  password: unknown;
}

export interface RebindInput {
  newEmail: unknown;
  code: unknown;
  password: unknown;
}

export class AuthService {
  constructor(private readonly deps: AuthDependencies) {}

  private get db(): SqliteDatabase {
    return this.deps.db;
  }

  private get config(): AppConfig {
    return this.deps.config;
  }

  private get now(): number {
    return this.deps.now();
  }

  /**
   * 生成并投递验证码
   * 同一邮箱同用途 60 秒冷却, 同一邮箱每日总量受限, 同 IP 每小时总量受限
   */
  private async issueCode(email: string, purpose: CodePurpose, ip: string | null): Promise<CodeDelivery> {
    const current = this.now;
    if (ip !== null) {
      const ipLimit = this.deps.limiters.sendByIp.consume(ip, this.config.sendHourlyIpLimit, HOUR_MS);
      if (!ipLimit.allowed) {
        throw new AppError("RATE_LIMITED", "验证码发送过于频繁, 请稍后再试");
      }
    }
    const latestCreatedAt = findLatestCodeCreatedAt(this.db, email, purpose);
    if (latestCreatedAt !== null && current - latestCreatedAt < this.config.sendCooldownSeconds * 1000) {
      throw new AppError("RATE_LIMITED", "验证码发送过于频繁, 请稍后再试");
    }
    if (countCodesSince(this.db, email, current - DAY_MS) >= this.config.sendDailyLimit) {
      throw new AppError("RATE_LIMITED", "该邮箱今日验证码发送次数已达上限");
    }
    const code = randomNumericCode(6);
    insertCode(this.db, {
      email,
      purpose,
      codeHash: sha256Hex(code),
      expiresAt: current + this.config.codeTtlSeconds * 1000,
      now: current,
    });
    await this.deps.mail.sendCode(email, purpose, code, this.config.codeTtlSeconds);
    return { ttl: this.config.codeTtlSeconds, cooldown: this.config.sendCooldownSeconds };
  }

  /**
   * 校验并消费验证码
   * 过期与次数超限分别返回独立错误码, 校验成功立即置 consumed_at
   */
  private consumeVerificationCode(email: string, purpose: CodePurpose, code: string): void {
    const row = findLatestActiveCode(this.db, email, purpose);
    if (!row) {
      throw new AppError("AUTH_CODE_INVALID");
    }
    if (row.attempts >= this.config.verifyMaxAttempts) {
      throw new AppError("AUTH_CODE_ATTEMPTS_EXCEEDED");
    }
    if (row.expires_at <= this.now) {
      throw new AppError("AUTH_CODE_EXPIRED");
    }
    increaseCodeAttempts(this.db, row.id);
    if (!safeEqualHex(sha256Hex(code), row.code_hash)) {
      if (row.attempts + 1 >= this.config.verifyMaxAttempts) {
        throw new AppError("AUTH_CODE_ATTEMPTS_EXCEEDED");
      }
      throw new AppError("AUTH_CODE_INVALID");
    }
    consumeCode(this.db, row.id, this.now);
  }

  /**
   * 生成唯一 UID 并写入用户, 邮箱唯一冲突按已注册处理
   */
  private createUser(email: string, passwordHash: string): UserRow {
    for (let attempt = 0; attempt < UID_RETRY_LIMIT; attempt += 1) {
      const uid = randomUid();
      try {
        return insertUser(this.db, { uid, email, passwordHash, now: this.now });
      } catch (error) {
        const message = error instanceof Error ? error.message : String(error);
        if (message.includes("users.email")) {
          throw new AppError("AUTH_EMAIL_TAKEN");
        }
        if (!message.includes("users.uid")) {
          throw error;
        }
      }
    }
    throw new AppError("INTERNAL", "UID 分配失败");
  }

  private createSessionResult(user: UserRow, meta: SessionMeta): LoginResult {
    const session = this.deps.session.create(user.id, meta);
    return { uid: user.uid, email: user.email, token: session.token, expiresAt: session.expiresAt };
  }

  async requestRegisterCode(emailInput: unknown, ip: string | null): Promise<CodeDelivery> {
    const email = requireEmail(emailInput);
    return this.issueCode(email, "register", ip);
  }

  async requestLoginCode(emailInput: unknown, ip: string | null): Promise<CodeDelivery> {
    const email = requireEmail(emailInput);
    return this.issueCode(email, "login", ip);
  }

  async requestPasswordResetCode(emailInput: unknown, ip: string | null): Promise<CodeDelivery> {
    const email = requireEmail(emailInput);
    const limit = this.deps.limiters.resetByEmail.consume(email, this.config.resetHourlyLimit, HOUR_MS);
    if (!limit.allowed) {
      throw new AppError("RATE_LIMITED", "该邮箱重置密码请求过于频繁");
    }
    return this.issueCode(email, "reset", ip);
  }

  async register(input: RegisterInput, meta: SessionMeta): Promise<LoginResult> {
    const email = requireEmail(input.email);
    const password = requirePassword(input.password, this.config.passwordMinLength);
    const code = requireCode(input.code);
    this.consumeVerificationCode(email, "register", code);
    if (findUserByEmail(this.db, email)) {
      throw new AppError("AUTH_EMAIL_TAKEN");
    }
    const passwordHash = await hashPassword(password);
    const user = this.createUser(email, passwordHash);
    this.deps.wallet.ensure(user.id);
    return this.createSessionResult(user, meta);
  }

  /**
   * 解析登录标识符: 含 @ 视为邮箱, 纯数字视为 UID
   */
  private resolveIdentifier(identifierInput: unknown): { kind: "email" | "uid"; value: string } {
    const identifier = requireIdentifier(identifierInput);
    if (identifier.includes("@")) {
      return { kind: "email", value: requireEmail(identifier) };
    }
    if (isUidLike(identifier)) {
      return { kind: "uid", value: identifier };
    }
    throw new AppError("AUTH_INVALID_IDENTIFIER");
  }

  private assertLoginAllowed(accountKey: string, ip: string | null): void {
    const accountLimit = this.deps.limiters.loginByAccount.check(accountKey);
    if (!accountLimit.allowed) {
      throw new AppError("RATE_LIMITED", "登录失败次数过多, 请稍后再试");
    }
    if (ip !== null) {
      const ipLimit = this.deps.limiters.loginByIp.check(ip);
      if (!ipLimit.allowed) {
        throw new AppError("RATE_LIMITED", "登录失败次数过多, 请稍后再试");
      }
    }
  }

  private recordLoginFailure(accountKey: string, ip: string | null): void {
    this.deps.limiters.loginByAccount.recordFailure(accountKey);
    if (ip !== null) {
      this.deps.limiters.loginByIp.recordFailure(ip);
    }
  }

  private resetLoginFailures(accountKey: string, ip: string | null): void {
    this.deps.limiters.loginByAccount.reset(accountKey);
    if (ip !== null) {
      this.deps.limiters.loginByIp.reset(ip);
    }
  }

  async login(input: LoginInput, meta: SessionMeta): Promise<LoginResult> {
    const target = this.resolveIdentifier(input.identifier);
    const password = typeof input.password === "string" ? input.password : "";
    const accountKey = target.value;
    this.assertLoginAllowed(accountKey, meta.ip);
    const user = target.kind === "email" ? findUserByEmail(this.db, target.value) : findUserByUid(this.db, target.value);
    if (!user) {
      // 账号不存在时也执行一次哈希, 抹平时间差异
      await burnPasswordCheck(password);
      this.recordLoginFailure(accountKey, meta.ip);
      throw new AppError("AUTH_INVALID_CREDENTIALS");
    }
    const matched = await verifyPassword(password, user.password_hash);
    if (!matched) {
      this.recordLoginFailure(accountKey, meta.ip);
      throw new AppError("AUTH_INVALID_CREDENTIALS");
    }
    this.resetLoginFailures(accountKey, meta.ip);
    return this.createSessionResult(user, meta);
  }

  async loginWithCode(input: CodeLoginInput, meta: SessionMeta): Promise<LoginResult> {
    const email = requireEmail(input.email);
    const code = requireCode(input.code);
    this.consumeVerificationCode(email, "login", code);
    const user = findUserByEmail(this.db, email);
    if (!user) {
      throw new AppError("AUTH_INVALID_CREDENTIALS");
    }
    return this.createSessionResult(user, meta);
  }

  async resetPassword(input: ResetPasswordInput): Promise<void> {
    const email = requireEmail(input.email);
    const password = requirePassword(input.password, this.config.passwordMinLength);
    const code = requireCode(input.code);
    const limit = this.deps.limiters.resetByEmail.consume(email, this.config.resetHourlyLimit, HOUR_MS);
    if (!limit.allowed) {
      throw new AppError("RATE_LIMITED", "该邮箱重置密码请求过于频繁");
    }
    this.consumeVerificationCode(email, "reset", code);
    const user = findUserByEmail(this.db, email);
    if (!user) {
      throw new AppError("AUTH_CODE_INVALID");
    }
    const passwordHash = await hashPassword(password);
    updateUserPassword(this.db, user.id, passwordHash, this.now);
    // 改密后旧会话全部失效, 并清理该邮箱剩余的重置验证码
    this.deps.session.revokeAllForUser(user.id);
    consumeAllCodes(this.db, email, "reset", this.now);
  }

  async requestEmailRebind(user: UserRow, newEmailInput: unknown, ip: string | null): Promise<CodeDelivery> {
    const newEmail = requireEmail(newEmailInput);
    if (newEmail === user.email) {
      throw new AppError("BAD_REQUEST", "新邮箱与当前邮箱相同");
    }
    return this.issueCode(newEmail, "rebind", ip);
  }

  async rebindEmail(user: UserRow, input: RebindInput): Promise<{ uid: string; email: string }> {
    const newEmail = requireEmail(input.newEmail);
    const password = requirePassword(input.password, this.config.passwordMinLength);
    const code = requireCode(input.code);
    const fresh = findUserById(this.db, user.id);
    if (!fresh) {
      throw new AppError("AUTH_UNAUTHORIZED");
    }
    const matched = await verifyPassword(password, fresh.password_hash);
    if (!matched) {
      throw new AppError("AUTH_INVALID_CREDENTIALS");
    }
    if (newEmail === fresh.email) {
      throw new AppError("BAD_REQUEST", "新邮箱与当前邮箱相同");
    }
    if (findUserByEmail(this.db, newEmail)) {
      throw new AppError("AUTH_EMAIL_TAKEN");
    }
    this.consumeVerificationCode(newEmail, "rebind", code);
    updateUserEmail(this.db, fresh.id, newEmail, this.now);
    return { uid: fresh.uid, email: newEmail };
  }

  logout(token: string): void {
    this.deps.session.revokeByToken(token);
  }
}
