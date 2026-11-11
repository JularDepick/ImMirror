/**
 * 位置上报与查询接口测试
 * 覆盖鉴权, 字段校验, 单玩家上报查询, 双玩家互见与超时剔除
 */

import { afterEach, beforeEach, describe, expect, it } from "vitest";
import { createTestApp, expectFail, expectOk, registerUser } from "./helpers";
import type { TestApp } from "./helpers";
import { resetPresenceTableForTest } from "../server/api/v1/presence";

let app: TestApp;
let caseSeq = 0;

interface PresenceOther {
  uid: string;
  x: number;
  y: number;
  z: number;
  yaw: number;
  pitch: number;
}

beforeEach(async () => {
  // 内存表是进程单例, 先清空避免上一用例的快照渗入本用例
  resetPresenceTableForTest();
  app = await createTestApp();
  caseSeq += 1;
});

afterEach(async () => {
  await app.close();
  resetPresenceTableForTest();
});

/** 每个用例用独立邮箱注册, 避免不同应用实例间的邮箱唯一冲突. */
function registerCasePlayer(tag: string, index: number) {
  return registerUser(app, { email: `p${caseSeq}-${tag}${index}@immirror.test` });
}

describe("位置通道", () => {
  it("未登录访问位置接口返回 AUTH_UNAUTHORIZED", async () => {
    const response = await app.post("/api/v1/presence", { x: 0, y: 0, z: 0 });
    expect(response.status).toBe(401);
    expectFail(response, "AUTH_UNAUTHORIZED");
  });

  it("首个玩家上报后看不到自己, 响应携带时间戳", async () => {
    const user = await registerCasePlayer("solo", 1);
    const response = await app.post(
      "/api/v1/presence",
      { x: 1.5, y: 64, z: 2.25, yaw: 0.3, pitch: -0.1 },
      user.token,
    );
    const data = expectOk<{ others: PresenceOther[]; serverTime: number }>(response);
    expect(data.others).toEqual([]);
    expect(data.serverTime).toBe(app.clock.now());
  });

  it("双玩家互见位置与脱敏 uid, 重复上报覆盖旧值", async () => {
    const first = await registerCasePlayer("pair", 1);
    const second = await registerCasePlayer("pair", 2);
    await app.post("/api/v1/presence", { x: 1, y: 64, z: 2 }, first.token);
    const seen = await app.post(
      "/api/v1/presence",
      { x: 3, y: 64, z: 4 },
      second.token,
    );
    const data = expectOk<{ others: PresenceOther[] }>(seen);
    expect(data.others).toHaveLength(1);
    const snapshot = data.others[0];
    expect(snapshot).toBeDefined();
    expect(snapshot?.x).toBe(1);
    expect(snapshot?.uid).toBe(first.uid);

    // first 移动后再次上报, second 只读查询应看到最新位置
    await app.post("/api/v1/presence", { x: 9, y: 65, z: 8 }, first.token);
    const refreshed = await app.get("/api/v1/presence", second.token);
    const latest = expectOk<{ others: PresenceOther[] }>(refreshed);
    expect(latest.others).toHaveLength(1);
    const moved = latest.others[0];
    expect(moved).toBeDefined();
    expect(moved?.x).toBe(9);
    expect(moved?.y).toBe(65);
  });

  it("缺位置字段或字段非数值返回 BAD_REQUEST", async () => {
    const user = await registerCasePlayer("bad", 1);
    const missing = await app.post("/api/v1/presence", { x: 1 }, user.token);
    expect(missing.status).toBe(400);
    expectFail(missing, "BAD_REQUEST");
    const notNumber = await app.post(
      "/api/v1/presence",
      { x: "a", y: 64, z: 2 },
      user.token,
    );
    expect(notNumber.status).toBe(400);
  });

  it("只读查询不更新自身位置, 对其他玩家保持隐身", async () => {
    const first = await registerCasePlayer("ghost", 1);
    const second = await registerCasePlayer("ghost", 2);
    await app.post("/api/v1/presence", { x: 5, y: 64, z: 6 }, first.token);
    const others = await app.get("/api/v1/presence", second.token);
    const data = expectOk<{ others: PresenceOther[] }>(others);
    expect(data.others).toHaveLength(1);
    // second 从未上报, first 视野里不应出现 second
    const fromFirst = await app.get("/api/v1/presence", first.token);
    const hidden = expectOk<{ others: PresenceOther[] }>(fromFirst);
    expect(hidden.others).toEqual([]);
  });

  it("超过 30 秒未上报的位置被剔除", async () => {
    const first = await registerCasePlayer("ttl", 1);
    const second = await registerCasePlayer("ttl", 2);
    await app.post("/api/v1/presence", { x: 1, y: 64, z: 2 }, first.token);
    // 时钟推进到存活期之外, 下一次查询触发剔除
    app.clock.advance(31 * 1000);
    const seen = await app.get("/api/v1/presence", second.token);
    const data = expectOk<{ others: PresenceOther[] }>(seen);
    expect(data.others).toEqual([]);
  });

  it("上报后再推进一整天, 新上报立即生效", async () => {
    const user = await registerCasePlayer("day", 1);
    app.clock.advance(24 * 60 * 60 * 1000);
    const response = await app.post("/api/v1/presence", { x: 0, y: 0, z: 0 }, user.token);
    const data = expectOk<{ serverTime: number }>(response);
    expect(data.serverTime).toBe(app.clock.now());
  });
});
