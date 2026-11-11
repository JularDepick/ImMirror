/**
 * 位置上报与查询接口测试
 */
import { beforeEach, describe, expect, it } from "vitest";
import { createTestApp, expectFail, expectOk, registerUser, type TestApp } from "./helpers";

let app: TestApp;

beforeEach(async () => {
  app = await createTestApp();
});

describe("位置通道", () => {
  it("未登录访问位置接口返回 AUTH_UNAUTHORIZED", async () => {
    const response = await app.get("/api/v1/presence");
    expect(response.status).toBe(401);
  });

  it("首个玩家上报后看不到自己", async () => {
    const user = await registerUser(app, { email: "p1@immirror.test" });
    const response = await app.post(
      "/api/v1/presence",
      { x: 1, y: 64, z: 2, yaw: 0, pitch: 0 },
      user.token,
    );
    expect(response.status).toBe(200);
    expect(response.body).toMatchObject({ ok: true });
  });

  it("双玩家互见位置", async () => {
    const first = await registerUser(app, { email: "p2@immirror.test" });
    const second = await registerUser(app, { email: "p3@immirror.test" });
    await app.post("/api/v1/presence", { x: 1, y: 64, z: 2 }, first.token);
    const seen = await app.post("/api/v1/presence", { x: 3, y: 64, z: 4 }, second.token);
    expect(seen.status).toBe(200);
    const body = seen.body as { ok: boolean; data: { others: Array<{ uid: number; x: number }> } };
    expect(body.data.others).toHaveLength(1);
    expect(body.data.others[0].x).toBe(1);
  });

  it("缺位置字段返回 BAD_REQUEST", async () => {
    const user = await registerUser(app, { email: "p4@immirror.test" });
    const response = await app.post("/api/v1/presence", { x: 1 }, user.token);
    expect(response.status).toBe(400);
  });

  it("只读查询不更新自身位置", async () => {
    const first = await registerUser(app, { email: "p5@immirror.test" });
    const second = await registerUser(app, { email: "p6@immirror.test" });
    await app.post("/api/v1/presence", { x: 5, y: 64, z: 6 }, first.token);
    const others = await app.get("/api/v1/presence", second.token);
    const body = others.body as { data: { others: Array<{ x: number }> } };
    expect(body.data.others).toHaveLength(1);
  });
});
