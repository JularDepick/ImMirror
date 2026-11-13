/**
 * 成就与每日任务领取契约测试
 * 覆盖鉴权, 未完成拒绝, 未知条目, 重复领取, 字符串与数字两种 id 形式
 */

import { afterEach, beforeEach, describe, expect, it } from "vitest";
import { ACHIEVEMENT_CATALOG } from "../server/api/v1/achievements/definitions";
import { createTestApp, expectFail, expectOk, registerUser } from "./helpers";
import type { TestApp } from "./helpers";

let app: TestApp;

beforeEach(async () => {
  app = await createTestApp();
});

afterEach(async () => {
  await app.close();
});

interface Summary {
  completed: number;
  total: number;
  claimed: number;
  coins: number;
}

interface ClaimResult {
  balance: number;
  granted: { source: string; id: number; name: string; coins: number }[];
  summary: Summary;
}

/* 目标为 1 的成就, 上报一次即完成, 便于构造可领取状态. */
const COMPLETED_ONCE = ACHIEVEMENT_CATALOG.filter(
  (item) => item.kind === "break_block" && item.target === 1,
)[0];

describe("成就领取", () => {
  it("未登录领取返回 AUTH_UNAUTHORIZED", async () => {
    const response = await app.post("/api/v1/achievements/claim", { id: 1 });
    expect(response.status).toBe(401);
    expectFail(response, "AUTH_UNAUTHORIZED");
    const daily = await app.post("/api/v1/achievements/daily/claim", { slot: 0 });
    expect(daily.status).toBe(401);
  });

  it("未完成的成就不可领取", async () => {
    const user = await registerUser(app);
    const id = COMPLETED_ONCE?.id ?? 0;
    expect(id).toBeGreaterThan(0);
    const response = await app.post("/api/v1/achievements/claim", { id }, user.token);
    expect(response.status).toBe(409);
    expectFail(response, "ACHIEVEMENT_NOT_COMPLETED");
  });

  it("未知成就在领取时返回 ACHIEVEMENT_NOT_FOUND", async () => {
    const user = await registerUser(app);
    const response = await app.post("/api/v1/achievements/claim", { id: 999999 }, user.token);
    expect(response.status).toBe(404);
    expectFail(response, "ACHIEVEMENT_NOT_FOUND");
  });

  it("同一成就重复领取返回 ACHIEVEMENT_ALREADY_CLAIMED, 余额只加一次", async () => {
    const user = await registerUser(app);
    const id = COMPLETED_ONCE?.id ?? 0;
    const reward = COMPLETED_ONCE?.coinReward ?? 0;
    await app.post("/api/v1/achievements/report", { stats: { break_block: 1 } }, user.token);

    const first = expectOk<ClaimResult>(
      await app.post("/api/v1/achievements/claim", { id }, user.token),
    );
    expect(first.granted[0]?.coins).toBe(reward);
    expect(first.balance).toBe(reward);
    expect(first.summary.claimed).toBe(1);

    const second = await app.post("/api/v1/achievements/claim", { id }, user.token);
    expect(second.status).toBe(409);
    expectFail(second, "ACHIEVEMENT_ALREADY_CLAIMED");

    const listed = expectOk<{ balance: number }>(await app.get("/api/v1/achievements", user.token));
    expect(listed.balance).toBe(reward);
  });

  it("字符串形式的成就 id 与数字等价", async () => {
    const user = await registerUser(app);
    const id = COMPLETED_ONCE?.id ?? 0;
    await app.post("/api/v1/achievements/report", { stats: { break_block: 1 } }, user.token);
    const claimed = expectOk<ClaimResult>(
      await app.post("/api/v1/achievements/claim", { id: String(id) }, user.token),
    );
    expect(claimed.summary.claimed).toBe(1);
  });

  it("缺少 id 或 id 非整数返回 BAD_REQUEST", async () => {
    const user = await registerUser(app);
    const missing = await app.post("/api/v1/achievements/claim", {}, user.token);
    expect(missing.status).toBe(400);
    expectFail(missing, "BAD_REQUEST");
    const bad = await app.post("/api/v1/achievements/claim", { id: "abc" }, user.token);
    expect(bad.status).toBe(400);
  });

  it("批量领取一次领完已完成未领取的条目, 再调为空", async () => {
    const user = await registerUser(app);
    const board = expectOk<{ tasks: { slot: number; kind: string; target: number; coinReward: number }[] }>(
      await app.get("/api/v1/achievements/daily", user.token),
    );
    const task = board.tasks[0];
    expect(task).toBeDefined();
    if (!task) {
      return;
    }
    /* 一次上报同时推进成就与当日任务. */
    await app.post("/api/v1/achievements/report", { stats: { break_block: 1, [task.kind]: task.target } }, user.token);

    const first = expectOk<{ granted: { source: string }[]; balance: number; summary: { claimed: number } }>(
      await app.post("/api/v1/achievements/claim-all", {}, user.token),
    );
    expect(first.granted.length).toBeGreaterThan(0);
    expect(first.granted.some((item) => item.source === "daily_task")).toBe(true);
    expect(first.summary.claimed).toBeGreaterThan(0);
    expect(first.balance).toBeGreaterThan(0);

    /* 再调一次: 没有可领的了, 余额不变. */
    const second = expectOk<{ granted: unknown[]; balance: number }>(
      await app.post("/api/v1/achievements/claim-all", {}, user.token),
    );
    expect(second.granted).toEqual([]);
    expect(second.balance).toBe(first.balance);
  });

  it("每日任务领取覆盖未完成, 未知槽位与重复领取", async () => {
    const user = await registerUser(app);
    const board = expectOk<{ date: string; tasks: { slot: number; kind: string; target: number; coinReward: number }[] }>(
      await app.get("/api/v1/achievements/daily", user.token),
    );
    const task = board.tasks[0];
    expect(task).toBeDefined();
    if (!task) {
      return;
    }

    const early = await app.post("/api/v1/achievements/daily/claim", { slot: task.slot }, user.token);
    expect(early.status).toBe(409);
    expectFail(early, "ACHIEVEMENT_NOT_COMPLETED");

    const unknown = await app.post("/api/v1/achievements/daily/claim", { slot: 99 }, user.token);
    expect(unknown.status).toBe(404);
    expectFail(unknown, "DAILY_TASK_NOT_FOUND");

    await app.post("/api/v1/achievements/report", { stats: { [task.kind]: task.target } }, user.token);
    const claimed = expectOk<ClaimResult>(
      await app.post("/api/v1/achievements/daily/claim", { slot: task.slot }, user.token),
    );
    expect(claimed.granted[0]?.source).toBe("daily_task");
    expect(claimed.granted[0]?.coins).toBe(task.coinReward);
    expect(claimed.balance).toBeGreaterThanOrEqual(task.coinReward);

    const again = await app.post("/api/v1/achievements/daily/claim", { slot: task.slot }, user.token);
    expect(again.status).toBe(409);
    expectFail(again, "ACHIEVEMENT_ALREADY_CLAIMED");
  });
});
