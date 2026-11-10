/**
 * 换绑邮箱用例组
 * 覆盖正常换绑, 新邮箱被占用, 密码确认与鉴权要求
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

describe("换绑邮箱", () => {
  it("验证新邮箱后完成换绑, 新邮箱可登录", async () => {
    const user = await registerUser(app, { email: "old@immirror.test" });
    const newEmail = "new@immirror.test";
    const send = await app.post("/api/v1/auth/email/rebind/code", { newEmail }, user.token);
    expect(send.status).toBe(200);
    const code = app.readCode(newEmail, "rebind");

    const rebind = await app.post(
      "/api/v1/auth/email/rebind",
      { newEmail, code, password: user.password },
      user.token,
    );
    expect(rebind.status).toBe(200);
    const data = expectOk<{ uid: string; email: string }>(rebind);
    expect(data.uid).toBe(user.uid);
    expect(data.email).toBe(newEmail);

    const loginNew = await app.post("/api/v1/auth/login", { identifier: newEmail, password: user.password });
    expect(loginNew.status).toBe(200);
    const loginOld = await app.post("/api/v1/auth/login", { identifier: user.email, password: user.password });
    expect(loginOld.status).toBe(401);
  });

  it("新邮箱已被占用返回 AUTH_EMAIL_TAKEN", async () => {
    const first = await registerUser(app, { email: "first@immirror.test" });
    const second = await registerUser(app, { email: "second@immirror.test" });
    await app.post("/api/v1/auth/email/rebind/code", { newEmail: second.email }, first.token);
    const code = app.readCode(second.email, "rebind");
    const response = await app.post(
      "/api/v1/auth/email/rebind",
      { newEmail: second.email, code, password: first.password },
      first.token,
    );
    expect(response.status).toBe(409);
    expectFail(response, "AUTH_EMAIL_TAKEN");
  });

  it("密码不匹配返回 AUTH_INVALID_CREDENTIALS", async () => {
    const user = await registerUser(app, { email: "confirm@immirror.test" });
    const newEmail = "confirm-new@immirror.test";
    await app.post("/api/v1/auth/email/rebind/code", { newEmail }, user.token);
    const code = app.readCode(newEmail, "rebind");
    const response = await app.post(
      "/api/v1/auth/email/rebind",
      { newEmail, code, password: "wrong-password" },
      user.token,
    );
    expect(response.status).toBe(401);
    expectFail(response, "AUTH_INVALID_CREDENTIALS");
  });

  it("未登录时换绑返回 AUTH_UNAUTHORIZED", async () => {
    const response = await app.post("/api/v1/auth/email/rebind/code", { newEmail: "anon@immirror.test" });
    expect(response.status).toBe(401);
    expectFail(response, "AUTH_UNAUTHORIZED");
  });
});
