/**
 * 测试辅助
 * 每个用例创建独立应用与独立临时数据库, 通过 Fastify inject 直接调用接口,
 * 邮件走 memory 通道, 验证码从内存信箱读取
 */

import { mkdirSync, mkdtempSync, rmSync } from "node:fs";
import { join } from "node:path";
import type { InjectOptions } from "fastify";
import type { CodePurpose } from "../server/db/repositories/codes";
import { createApp } from "../server/fastify/app";
import type { AppContext } from "../server/fastify/context";
import type { AppConfig } from "../server/utils/config";
import { loadConfig } from "../server/utils/config";
import type { ApiFailure, ApiSuccess } from "../server/utils/response";

// 测试基准时间: 2026-11-10 10:00:00 (Asia/Shanghai)
export const BASE_TIME = Date.UTC(2026, 10, 10, 2, 0, 0);
export const DAY_MS = 24 * 60 * 60 * 1000;

export interface TestClock {
  now(): number;
  set(ms: number): void;
  advance(ms: number): void;
  reset(): void;
}

export interface TestResponse<T = unknown> {
  status: number;
  body: T;
  contentType: string;
}

export interface TestApp {
  ctx: AppContext;
  clock: TestClock;
  request(options: InjectOptions): Promise<TestResponse>;
  post(path: string, payload?: unknown, token?: string): Promise<TestResponse>;
  get(path: string, token?: string): Promise<TestResponse>;
  readCode(email: string, purpose: CodePurpose): string;
  close(): Promise<void>;
}

function buildConfig(overrides: Partial<AppConfig>): AppConfig {
  const base = loadConfig({
    MAIL_TRANSPORT: "memory",
    DB_FILE: ":memory:",
    // 测试放宽发送与请求频次限制, 与业务限流用例分开验证
    SEND_COOLDOWN_SECONDS: "0",
    SEND_DAILY_LIMIT: "100",
    SEND_HOURLY_IP_LIMIT: "1000",
    RESET_HOURLY_LIMIT: "100",
    REQUEST_MINUTE_IP_LIMIT: "100000",
  } as NodeJS.ProcessEnv);
  return { ...base, ...overrides, smtp: { ...base.smtp, ...(overrides.smtp ?? {}) } };
}

export async function createTestApp(overrides: Partial<AppConfig> = {}): Promise<TestApp> {
  const config = buildConfig(overrides);
  const tempRoot = join(process.cwd(), "temp");
  mkdirSync(tempRoot, { recursive: true });
  const tempDir = mkdtempSync(join(tempRoot, "case-"));
  const dbFile = join(tempDir, "test.db");

  let current = BASE_TIME;
  const clock: TestClock = {
    now: () => current,
    set: (ms: number) => {
      current = ms;
    },
    advance: (ms: number) => {
      current += ms;
    },
    reset: () => {
      current = BASE_TIME;
    },
  };

  const ctx = createApp({ config, dbFile, now: () => current });
  await ctx.fastify.ready();

  const request = async (options: InjectOptions): Promise<TestResponse> => {
    const response = await ctx.fastify.inject(options);
    const contentType = String(response.headers["content-type"] ?? "");
    return {
      status: response.statusCode,
      body: response.body === "" ? null : JSON.parse(response.body),
      contentType,
    };
  };

  const app: TestApp = {
    ctx,
    clock,
    request,
    async post(path, payload, token) {
      return request({
        method: "POST",
        url: path,
        ...(payload === undefined ? {} : { payload: payload as object }),
        headers: token ? { authorization: `Bearer ${token}` } : {},
      });
    },
    async get(path, token) {
      return request({
        method: "GET",
        url: path,
        headers: token ? { authorization: `Bearer ${token}` } : {},
      });
    },
    readCode(email, purpose) {
      const message = ctx.mail.memory?.findLatest(email, purpose);
      if (!message || !message.code) {
        throw new Error(`未找到 ${email} 的 ${purpose} 验证码`);
      }
      return message.code;
    },
    async close() {
      await ctx.close();
      rmSync(tempDir, { recursive: true, force: true });
    },
  };

  return app;
}

export function expectOk<T>(response: TestResponse): T {
  const body = response.body as ApiSuccess<T> | ApiFailure | null;
  if (!body || body.ok !== true) {
    throw new Error(`期望成功响应, 实际: ${JSON.stringify(response.body)}`);
  }
  return body.data;
}

export function expectFail(response: TestResponse, code: string): ApiFailure {
  const body = response.body as ApiFailure | null;
  if (!body || body.ok !== false) {
    throw new Error(`期望失败响应, 实际: ${JSON.stringify(response.body)}`);
  }
  if (body.error.code !== code) {
    throw new Error(`期望错误码 ${code}, 实际 ${body.error.code}`);
  }
  return body;
}

/**
 * 注册一个账号并返回登录令牌与 UID
 */
export async function registerUser(
  app: TestApp,
  options: { email?: string; password?: string } = {},
): Promise<{ email: string; password: string; uid: string; token: string }> {
  const email = options.email ?? "player@immirror.test";
  const password = options.password ?? "P@ssw0rd-2026";
  await app.post("/api/v1/auth/register/code", { email });
  const code = app.readCode(email, "register");
  const response = await app.post("/api/v1/auth/register", { email, code, password });
  const data = expectOk<{ uid: string; email: string; token: string }>(response);
  return { email: data.email, password, uid: data.uid, token: data.token };
}
