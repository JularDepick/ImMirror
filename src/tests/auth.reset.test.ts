/**
 * 忘记密码用例组
 * 覆盖正常重置, 旧密码失效, 新密码可登录, 旧会话失效与验证码校验
 */

import { afterEach, beforeEach, describe, expect, it } from "vitest";
import { createTestApp, expectFail, expectOk, registerUser } from "./helpers";
import type { TestApp } from "./helpers";

const NEW_PASSWORD = "N3w-P@ssw0rd-2026";

let app: TestApp;

beforeEach(async () => {
  app = await createTestApp();
});

afterEach(async () => {
  await app.close();
});

describe("忘记密码", () => {
  it("通过邮箱验证码重置密码后旧密码失效, 新密码可登录", async () => {
    const user = await registerUser(app);
    const send = await app.post("/api/v1/auth/password/reset/code", { email: user.email });
    expect(send.status).toBe(200);
    const code = app.readCode(user.email, "reset");

    const reset = await app.post("/api/v1/auth/password/reset", {
      email: user.email,
      code,
      password: NEW_PASSWORD,
    });
    expect(reset.status).toBe(200);
    expect(expectOk<{ ok: boolean }>(reset).ok).toBe(true);

    const oldPassword = await app.post("/api/v1/auth/login", { identifier: user.email, password: user.password });
    expect(oldPassword.status).toBe(401);
    expectFail(oldPassword, "AUTH_INVALID_CREDENTIALS");

    const newPassword = await app.post("/api/v1/auth/login", { identifier: user.email, password: NEW_PASSWORD });
    expect(newPassword.status).toBe(200);
  });

  it("重置密码后原有会话全部失效", async () => {
    const user = await registerUser(app);
    await app.post("/api/v1/auth/password/reset/code", { email: user.email });
    const code = app.readCode(user.email, "reset");
    await app.post("/api/v1/auth/password/reset", { email: user.email, code, password: NEW_PASSWORD });

    const me = await app.get("/api/v1/me", user.token);
    expect(me.status).toBe(401);
    expectFail(me, "AUTH_UNAUTHORIZED");
  });

  it("重置验证码错误返回 AUTH_CODE_INVALID", async () => {
    const user = await registerUser(app);
    await app.post("/api/v1/auth/password/reset/code", { email: user.email });
    const realCode = app.readCode(user.email, "reset");
    const wrongCode = realCode === "000000" ? "111111" : "000000";
    const response = await app.post("/api/v1/auth/password/reset", {
      email: user.email,
      code: wrongCode,
      password: NEW_PASSWORD,
    });
    expect(response.status).toBe(400);
    expectFail(response, "AUTH_CODE_INVALID");
  });

  it("未注册邮箱重置返回与已注册一致的响应形态", async () => {
    const user = await registerUser(app);
    const known = await app.post("/api/v1/auth/password/reset/code", { email: user.email });
    const unknown = await app.post("/api/v1/auth/password/reset/code", { email: "ghost@immirror.test" });
    expect(known.status).toBe(unknown.status);
    expect(known.body).toEqual(unknown.body);
  });

  it("新密码过短返回 BAD_REQUEST", async () => {
    const user = await registerUser(app);
    await app.post("/api/v1/auth/password/reset/code", { email: user.email });
    const code = app.readCode(user.email, "reset");
    const response = await app.post("/api/v1/auth/password/reset", { email: user.email, code, password: "short" });
    expect(response.status).toBe(400);
    expectFail(response, "BAD_REQUEST");
  });
});
