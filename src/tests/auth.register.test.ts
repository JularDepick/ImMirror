/**
 * 注册用例组
 * 覆盖正常注册, 重复邮箱, 错误验证码, 密码过短与枚举防护
 */

import { afterEach, beforeEach, describe, expect, it } from "vitest";
import { createTestApp, expectFail, expectOk } from "./helpers";
import type { TestApp } from "./helpers";

const EMAIL = "register@immirror.test";
const PASSWORD = "P@ssw0rd-2026";

let app: TestApp;

beforeEach(async () => {
  app = await createTestApp();
});

afterEach(async () => {
  await app.close();
});

describe("注册", () => {
  it("发送注册验证码返回有效期与冷却时间", async () => {
    const response = await app.post("/api/v1/auth/register/code", { email: EMAIL });
    expect(response.status).toBe(200);
    const data = expectOk<{ ttl: number; cooldown: number }>(response);
    expect(data.ttl).toBe(600);
    expect(data.cooldown).toBe(0);
  });

  it("邮箱加验证码加密码注册成功并返回 9 位 UID 与令牌", async () => {
    await app.post("/api/v1/auth/register/code", { email: EMAIL });
    const code = app.readCode(EMAIL, "register");
    const response = await app.post("/api/v1/auth/register", { email: EMAIL, code, password: PASSWORD });
    expect(response.status).toBe(200);
    const data = expectOk<{ uid: string; email: string; token: string; expiresAt: number }>(response);
    expect(data.uid).toMatch(/^[1-9]\d{8}$/);
    expect(data.uid).toHaveLength(9);
    expect(data.uid.startsWith("0")).toBe(false);
    expect(data.email).toBe(EMAIL);
    expect(data.token).toMatch(/^[0-9a-f]{64}$/);
    expect(data.expiresAt).toBeGreaterThan(app.clock.now());

    const wallet = await app.get("/api/v1/wallet", data.token);
    expect(expectOk<{ balance: number }>(wallet).balance).toBe(0);
  });

  it("重复邮箱注册返回 AUTH_EMAIL_TAKEN", async () => {
    await app.post("/api/v1/auth/register/code", { email: EMAIL });
    await app.post("/api/v1/auth/register", {
      email: EMAIL,
      code: app.readCode(EMAIL, "register"),
      password: PASSWORD,
    });

    await app.post("/api/v1/auth/register/code", { email: EMAIL });
    const secondCode = app.readCode(EMAIL, "register");
    const response = await app.post("/api/v1/auth/register", {
      email: EMAIL,
      code: secondCode,
      password: PASSWORD,
    });
    expect(response.status).toBe(409);
    expectFail(response, "AUTH_EMAIL_TAKEN");
  });

  it("验证码错误返回 AUTH_CODE_INVALID", async () => {
    await app.post("/api/v1/auth/register/code", { email: EMAIL });
    const realCode = app.readCode(EMAIL, "register");
    const wrongCode = realCode === "000000" ? "111111" : "000000";
    const response = await app.post("/api/v1/auth/register", {
      email: EMAIL,
      code: wrongCode,
      password: PASSWORD,
    });
    expect(response.status).toBe(400);
    expectFail(response, "AUTH_CODE_INVALID");
  });

  it("密码过短返回 BAD_REQUEST", async () => {
    const response = await app.post("/api/v1/auth/register", {
      email: EMAIL,
      code: "123456",
      password: "short",
    });
    expect(response.status).toBe(400);
    expectFail(response, "BAD_REQUEST");
  });

  it("邮箱格式非法返回 BAD_REQUEST", async () => {
    const response = await app.post("/api/v1/auth/register/code", { email: "not-an-email" });
    expect(response.status).toBe(400);
    expectFail(response, "BAD_REQUEST");
  });

  it("已注册与未注册邮箱的验证码响应形态一致", async () => {
    await app.post("/api/v1/auth/register/code", { email: EMAIL });
    await app.post("/api/v1/auth/register", {
      email: EMAIL,
      code: app.readCode(EMAIL, "register"),
      password: PASSWORD,
    });

    const existing = await app.post("/api/v1/auth/register/code", { email: EMAIL });
    const fresh = await app.post("/api/v1/auth/register/code", { email: "nobody@immirror.test" });
    expect(existing.status).toBe(fresh.status);
    expect(existing.body).toEqual(fresh.body);
  });
});
