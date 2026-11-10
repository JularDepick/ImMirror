/**
 * 会话用例组
 * 覆盖无令牌, 无效令牌, 过期令牌, 登出失效与资料接口结构
 */

import { afterEach, beforeEach, describe, expect, it } from "vitest";
import { sha256Hex } from "../server/utils/random";
import { createTestApp, DAY_MS, expectFail, expectOk, registerUser } from "./helpers";
import type { TestApp } from "./helpers";

let app: TestApp;

beforeEach(async () => {
  app = await createTestApp();
});

afterEach(async () => {
  await app.close();
});

describe("会话", () => {
  it("缺少令牌返回 AUTH_UNAUTHORIZED", async () => {
    const response = await app.get("/api/v1/me");
    expect(response.status).toBe(401);
    expectFail(response, "AUTH_UNAUTHORIZED");
  });

  it("令牌格式非法返回 AUTH_UNAUTHORIZED", async () => {
    const response = await app.get("/api/v1/me", "not-a-valid-token");
    expect(response.status).toBe(401);
    expectFail(response, "AUTH_UNAUTHORIZED");
  });

  it("手写非 Bearer 头返回 AUTH_UNAUTHORIZED", async () => {
    const response = await app.request({
      method: "GET",
      url: "/api/v1/me",
      headers: { authorization: "Basic abc" },
    });
    expect(response.status).toBe(401);
    expectFail(response, "AUTH_UNAUTHORIZED");
  });

  it("会话过期返回 AUTH_SESSION_EXPIRED", async () => {
    const user = await registerUser(app);
    app.ctx.db
      .prepare("UPDATE sessions SET expires_at = ? WHERE token_hash = ?")
      .run(app.clock.now() - 1000, sha256Hex(user.token));
    const response = await app.get("/api/v1/me", user.token);
    expect(response.status).toBe(401);
    expectFail(response, "AUTH_SESSION_EXPIRED");
  });

  it("登出后令牌失效", async () => {
    const user = await registerUser(app);
    const logout = await app.post("/api/v1/auth/logout", undefined, user.token);
    expect(logout.status).toBe(200);
    expect(expectOk<{ ok: boolean }>(logout).ok).toBe(true);

    const response = await app.get("/api/v1/me", user.token);
    expect(response.status).toBe(401);
    expectFail(response, "AUTH_UNAUTHORIZED");
  });

  it("会话滑动续期: 鉴权后有效期不早于当前时间加完整周期", async () => {
    const user = await registerUser(app);
    app.clock.advance(10 * DAY_MS);
    await app.get("/api/v1/me", user.token);
    const row = app.ctx.db
      .prepare("SELECT expires_at FROM sessions WHERE token_hash = ?")
      .get(sha256Hex(user.token)) as { expires_at: number };
    expect(row.expires_at).toBeGreaterThanOrEqual(app.clock.now() + 30 * DAY_MS - 1000);
  });

  it("资料接口返回账号, 钱包与本自然月签到进度", async () => {
    const user = await registerUser(app);
    const response = await app.get("/api/v1/me", user.token);
    expect(response.status).toBe(200);
    const data = expectOk<{
      uid: string;
      email: string;
      createdAt: number;
      emailVerified: boolean;
      wallet: { balance: number };
      checkin: { signedToday: boolean; monthCount: number; month: string; milestones: unknown[] };
    }>(response);
    expect(data.uid).toBe(user.uid);
    expect(data.email).toBe(user.email);
    expect(data.emailVerified).toBe(true);
    expect(data.wallet.balance).toBe(0);
    expect(data.checkin.signedToday).toBe(false);
    expect(data.checkin.monthCount).toBe(0);
    expect(data.checkin.month).toBe("2026-11");
    expect(data.checkin.milestones.length).toBe(2);
  });
});
