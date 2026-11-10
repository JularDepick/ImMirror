/**
 * 签到用例组
 * 覆盖首次签到, 同日重复, 余额累加, 里程碑奖励, 跨月重置与流水记录
 */

import { afterEach, beforeEach, describe, expect, it } from "vitest";
import { createTestApp, DAY_MS, expectFail, expectOk, registerUser } from "./helpers";
import type { TestApp } from "./helpers";

let app: TestApp;

beforeEach(async () => {
  app = await createTestApp();
});

afterEach(async () => {
  await app.close();
});

interface CheckinResult {
  reward: number;
  balance: number;
  monthCount: number;
  milestoneReward: number;
}

describe("签到", () => {
  it("首次签到获得 100 镜币", async () => {
    const user = await registerUser(app);
    const response = await app.post("/api/v1/wallet/checkin", undefined, user.token);
    expect(response.status).toBe(200);
    const data = expectOk<CheckinResult>(response);
    expect(data.reward).toBe(100);
    expect(data.balance).toBe(100);
    expect(data.monthCount).toBe(1);
    expect(data.milestoneReward).toBe(0);

    const wallet = await app.get("/api/v1/wallet", user.token);
    const summary = expectOk<{ balance: number; updatedAt: number }>(wallet);
    expect(summary.balance).toBe(100);
    expect(summary.updatedAt).toBe(app.clock.now());
  });

  it("同日重复签到返回 WALLET_ALREADY_CHECKED_IN", async () => {
    const user = await registerUser(app);
    await app.post("/api/v1/wallet/checkin", undefined, user.token);
    const again = await app.post("/api/v1/wallet/checkin", undefined, user.token);
    expect(again.status).toBe(409);
    expectFail(again, "WALLET_ALREADY_CHECKED_IN");
  });

  it("余额逐日累加, 单次奖励按次数递增并封顶 200", async () => {
    const user = await registerUser(app);
    const rewards: number[] = [];
    for (let day = 1; day <= 11; day += 1) {
      const response = await app.post("/api/v1/wallet/checkin", undefined, user.token);
      const data = expectOk<CheckinResult>(response);
      expect(data.monthCount).toBe(day);
      rewards.push(data.reward);
      app.clock.advance(DAY_MS);
    }
    expect(rewards[0]).toBe(100);
    expect(rewards[1]).toBe(110);
    expect(rewards[9]).toBe(190);
    expect(rewards[10]).toBe(200);

    const wallet = await app.get("/api/v1/wallet", user.token);
    const expected = rewards.reduce((sum, item) => sum + item, 0);
    expect(expectOk<{ balance: number }>(wallet).balance).toBe(expected);
  });

  it("当月第 15 次签到触发 500 里程碑奖励", async () => {
    const user = await registerUser(app);
    let last: CheckinResult | null = null;
    for (let day = 1; day <= 15; day += 1) {
      const response = await app.post("/api/v1/wallet/checkin", undefined, user.token);
      last = expectOk<CheckinResult>(response);
      app.clock.advance(DAY_MS);
    }
    expect(last).not.toBeNull();
    expect(last?.monthCount).toBe(15);
    expect(last?.reward).toBe(200);
    expect(last?.milestoneReward).toBe(500);
    // 前 10 天累计 1450, 第 11 到 15 天每日 200 共 1000, 再加里程碑 500
    expect(last?.balance).toBe(1450 + 1000 + 500);
  });

  it("跨自然月后签到次数重置", async () => {
    const user = await registerUser(app);
    for (let day = 1; day <= 3; day += 1) {
      await app.post("/api/v1/wallet/checkin", undefined, user.token);
      app.clock.advance(DAY_MS);
    }
    // 2026-11-13 之后跳到 2026-12-01
    app.clock.advance(20 * DAY_MS);
    const status = await app.get("/api/v1/wallet/checkin", user.token);
    const before = expectOk<{ monthCount: number; month: string; signedToday: boolean }>(status);
    expect(before.month).toBe("2026-12");
    expect(before.monthCount).toBe(0);
    expect(before.signedToday).toBe(false);

    const response = await app.post("/api/v1/wallet/checkin", undefined, user.token);
    const data = expectOk<CheckinResult>(response);
    expect(data.monthCount).toBe(1);
    expect(data.reward).toBe(100);
  });

  it("签到状态接口返回当日状态, 下次奖励与里程碑进度", async () => {
    const user = await registerUser(app);
    const before = await app.get("/api/v1/wallet/checkin", user.token);
    const pending = expectOk<{
      signedToday: boolean;
      monthCount: number;
      nextReward: number | null;
      todayReward: number | null;
      milestones: { count: number; reward: number; achieved: boolean }[];
    }>(before);
    expect(pending.signedToday).toBe(false);
    expect(pending.nextReward).toBe(100);
    expect(pending.todayReward).toBeNull();
    expect(pending.milestones).toEqual([
      { count: 15, reward: 500, achieved: false },
      { count: 25, reward: 1000, achieved: false },
    ]);

    await app.post("/api/v1/wallet/checkin", undefined, user.token);
    const after = await app.get("/api/v1/wallet/checkin", user.token);
    const signed = expectOk<{
      signedToday: boolean;
      monthCount: number;
      nextReward: number | null;
      todayReward: number | null;
    }>(after);
    expect(signed.signedToday).toBe(true);
    expect(signed.monthCount).toBe(1);
    expect(signed.nextReward).toBeNull();
    expect(signed.todayReward).toBe(100);
  });

  it("签到写入流水, 里程碑单独记账", async () => {
    const user = await registerUser(app);
    for (let day = 1; day <= 15; day += 1) {
      await app.post("/api/v1/wallet/checkin", undefined, user.token);
      app.clock.advance(DAY_MS);
    }
    const row = app.ctx.db.prepare("SELECT id FROM users WHERE email = ?").get(user.email) as { id: number };
    const ledger = app.ctx.wallet.listLedger(row.id, 50);
    expect(ledger.length).toBe(16);
    expect(ledger[0]?.reason).toBe("checkin_milestone");
    expect(ledger[0]?.delta).toBe(500);
    expect(ledger.filter((item) => item.reason === "checkin").length).toBe(15);
  });

  it("未登录签到返回 AUTH_UNAUTHORIZED", async () => {
    const response = await app.post("/api/v1/wallet/checkin");
    expect(response.status).toBe(401);
    expectFail(response, "AUTH_UNAUTHORIZED");
  });
});
