/**
 * 登录用例组
 * 覆盖邮箱登录, UID 登录, 密码错误, 账号不存在, 验证码登录与失败退避
 */

import { afterEach, beforeEach, describe, expect, it } from "vitest";
import { createTestApp, expectFail, expectOk, registerUser } from "./helpers";
import type { TestApp } from "./helpers";

let app: TestApp;

beforeEach(async () => {
  app = await createTestApp();
});

afterEach(async () => {
  await app.close();
});

describe("登录", () => {
  it("邮箱加密码登录成功", async () => {
    const user = await registerUser(app);
    const response = await app.post("/api/v1/auth/login", { identifier: user.email, password: user.password });
    expect(response.status).toBe(200);
    const data = expectOk<{ uid: string; email: string; token: string; expiresAt: number }>(response);
    expect(data.uid).toBe(user.uid);
    expect(data.email).toBe(user.email);
    expect(data.token).toMatch(/^[0-9a-f]{64}$/);
    expect(data.expiresAt).toBeGreaterThan(app.clock.now());
  });

  it("UID 加密码登录成功", async () => {
    const user = await registerUser(app);
    const response = await app.post("/api/v1/auth/login", { identifier: user.uid, password: user.password });
    expect(response.status).toBe(200);
    expect(expectOk<{ uid: string }>(response).uid).toBe(user.uid);
  });

  it("密码错误返回 AUTH_INVALID_CREDENTIALS", async () => {
    const user = await registerUser(app);
    const response = await app.post("/api/v1/auth/login", { identifier: user.email, password: "wrong-password" });
    expect(response.status).toBe(401);
    expectFail(response, "AUTH_INVALID_CREDENTIALS");
  });

  it("账号不存在与密码错误返回同一错误码", async () => {
    const response = await app.post("/api/v1/auth/login", {
      identifier: "ghost@immirror.test",
      password: "whatever-2026",
    });
    expect(response.status).toBe(401);
    expectFail(response, "AUTH_INVALID_CREDENTIALS");
  });

  it("邮箱加验证码登录成功", async () => {
    const user = await registerUser(app);
    await app.post("/api/v1/auth/login/code", { email: user.email });
    const code = app.readCode(user.email, "login");
    const response = await app.post("/api/v1/auth/login/code/verify", { email: user.email, code });
    expect(response.status).toBe(200);
    const data = expectOk<{ token: string }>(response);
    expect(data.token).toMatch(/^[0-9a-f]{64}$/);

    const me = await app.get("/api/v1/me", data.token);
    expect(expectOk<{ uid: string }>(me).uid).toBe(user.uid);
  });

  it("标识符既非邮箱也非 UID 返回 AUTH_INVALID_IDENTIFIER", async () => {
    const response = await app.post("/api/v1/auth/login", { identifier: "not-an-account", password: "whatever-2026" });
    expect(response.status).toBe(400);
    expectFail(response, "AUTH_INVALID_IDENTIFIER");
  });

  it("连续失败达到阈值后触发退避限流", async () => {
    const user = await registerUser(app);
    for (let attempt = 0; attempt < 5; attempt += 1) {
      const failed = await app.post("/api/v1/auth/login", { identifier: user.email, password: "wrong-password" });
      expect(failed.status).toBe(401);
    }
    const blocked = await app.post("/api/v1/auth/login", { identifier: user.email, password: user.password });
    expect(blocked.status).toBe(429);
    expectFail(blocked, "RATE_LIMITED");

    // 退避 1 分钟后恢复
    app.clock.advance(61 * 1000);
    const recovered = await app.post("/api/v1/auth/login", { identifier: user.email, password: user.password });
    expect(recovered.status).toBe(200);
  });
});
