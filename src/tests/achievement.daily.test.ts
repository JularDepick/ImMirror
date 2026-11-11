/**
 * 每日任务用例组
 * 覆盖当日生成, 同日幂等, 模板抽取的确定性, 上报完成发币, 跨日重置与旧行清理
 */

import { afterEach, beforeEach, describe, expect, it } from "vitest";
import { DAILY_TASK_COUNT, DAILY_TASK_TEMPLATES } from "../server/api/v1/achievements/definitions";
import { pickDailyTemplates } from "../server/services/achievement";
import { createTestApp, DAY_MS, expectFail, expectOk, registerUser } from "./helpers";
import type { TestApp } from "./helpers";

let app: TestApp;

beforeEach(async () => {
  app = await createTestApp();
});

afterEach(async () => {
  await app.close();
});

interface DailyTaskItem {
  slot: number;
  templateId: number;
  name: string;
  kind: string;
  kindTitle: string;
  unit: string;
  target: number;
  value: number;
  completed: boolean;
  completedAt: number | null;
  coinReward: number;
}

interface DailyBoardResponse {
  date: string;
  tasks: DailyTaskItem[];
  completed: number;
  coins: number;
}

describe("每日任务", () => {
  it("未登录访问每日任务返回 AUTH_UNAUTHORIZED", async () => {
    const response = await app.get("/api/v1/achievements/daily");
    expect(response.status).toBe(401);
    expectFail(response, "AUTH_UNAUTHORIZED");
  });

  it("首次访问生成三条当日任务", async () => {
    const user = await registerUser(app);
    const response = await app.get("/api/v1/achievements/daily", user.token);
    expect(response.status).toBe(200);
    const board = expectOk<DailyBoardResponse>(response);
    expect(board.date).toBe("2026-11-10");
    expect(board.tasks.length).toBe(DAILY_TASK_COUNT);
    expect(board.completed).toBe(0);
    expect(board.coins).toBe(0);
    for (const task of board.tasks) {
      expect(task.value).toBe(0);
      expect(task.completed).toBe(false);
      expect(task.target).toBeGreaterThan(0);
      expect(task.coinReward).toBeGreaterThan(0);
      expect(task.name.length).toBeGreaterThan(0);
    }
    const slots = board.tasks.map((task) => task.slot);
    expect(slots).toEqual([0, 1, 2]);
    const templates = new Set(board.tasks.map((task) => task.templateId));
    expect(templates.size).toBe(DAILY_TASK_COUNT);
  });

  it("同日重复访问不重建任务", async () => {
    const user = await registerUser(app);
    const first = expectOk<DailyBoardResponse>(await app.get("/api/v1/achievements/daily", user.token));
    app.clock.advance(3600 * 1000);
    const second = expectOk<DailyBoardResponse>(await app.get("/api/v1/achievements/daily", user.token));
    expect(second.date).toBe(first.date);
    expect(second.tasks.map((task) => task.templateId)).toEqual(first.tasks.map((task) => task.templateId));
    const rows = app.ctx.db
      .prepare("SELECT COUNT(*) AS total FROM user_daily_tasks WHERE user_id = (SELECT id FROM users WHERE email = ?)")
      .get(user.email) as { total: number };
    expect(rows.total).toBe(DAILY_TASK_COUNT);
  });

  it("模板抽取只依赖日期键且可重放", () => {
    const first = pickDailyTemplates("2026-11-10").map((item) => item.id);
    const again = pickDailyTemplates("2026-11-10").map((item) => item.id);
    expect(first).toEqual(again);
    expect(first.length).toBe(DAILY_TASK_COUNT);
    expect(new Set(first).size).toBe(DAILY_TASK_COUNT);

    const other = pickDailyTemplates("2026-11-11").map((item) => item.id);
    expect(other.length).toBe(DAILY_TASK_COUNT);
    expect(other.join(",")).not.toBe(first.join(","));

    const ids = DAILY_TASK_TEMPLATES.map((item) => item.id);
    for (const id of first) {
      expect(ids).toContain(id);
    }
  });

  it("上报推进当日进度, 达标发放镜币并记入流水", async () => {
    const user = await registerUser(app);
    const board = expectOk<DailyBoardResponse>(await app.get("/api/v1/achievements/daily", user.token));
    const task = board.tasks[0];
    expect(task).toBeDefined();
    if (!task) {
      return;
    }

    const partial = expectOk<{ daily: DailyBoardResponse }>(
      await app.post(
        "/api/v1/achievements/report",
        { stats: { [task.kind]: task.target - 1 } },
        user.token,
      ),
    );
    const partialTask = partial.daily.tasks.find((item) => item.slot === task.slot);
    expect(partialTask?.value).toBe(task.target - 1);
    expect(partialTask?.completed).toBe(false);

    const done = expectOk<{ daily: DailyBoardResponse; granted: { source: string; coins: number }[]; balance: number }>(
      await app.post("/api/v1/achievements/report", { stats: { [task.kind]: 1 } }, user.token),
    );
    const doneTask = done.daily.tasks.find((item) => item.slot === task.slot);
    expect(doneTask?.completed).toBe(true);
    expect(doneTask?.value).toBe(task.target);
    const dailyGrant = done.granted.find((item) => item.source === "daily_task");
    expect(dailyGrant?.coins).toBe(task.coinReward);
    /* 注册奖励与部分进度里的成就奖励都计入, 余额应等于全部流水之和. */
    /* 余额以钱包流水为准: 流水的 delta 全为正, 累计即当前余额. */
    const row0 = app.ctx.db.prepare("SELECT id FROM users WHERE email = ?").get(user.email) as { id: number };
    const ledgerTotal = app.ctx.wallet.listLedger(row0.id, 50).reduce((total, item) => total + item.delta, 0);
    expect(done.balance).toBe(ledgerTotal);

    const row = app.ctx.db.prepare("SELECT id FROM users WHERE email = ?").get(user.email) as { id: number };
    const ledger = app.ctx.wallet.listLedger(row.id, 50);
    expect(ledger.some((item) => item.reason === "daily_task")).toBe(true);
    const taskLedger = ledger.find((item) => item.reason === "daily_task");
    expect(taskLedger?.ref).toBe(`${board.date}#${task.slot}`);
  });

  it("跨日重置进度并清理旧日期任务", async () => {
    const user = await registerUser(app);
    const board = expectOk<DailyBoardResponse>(await app.get("/api/v1/achievements/daily", user.token));
    const task = board.tasks[0];
    expect(task).toBeDefined();
    if (!task) {
      return;
    }
    expectOk(
      await app.post("/api/v1/achievements/report", { stats: { [task.kind]: task.target } }, user.token),
    );

    app.clock.advance(DAY_MS);
    const tomorrow = expectOk<DailyBoardResponse>(await app.get("/api/v1/achievements/daily", user.token));
    expect(tomorrow.date).toBe("2026-11-11");
    expect(tomorrow.tasks.length).toBe(DAILY_TASK_COUNT);
    expect(tomorrow.completed).toBe(0);
    expect(tomorrow.tasks.every((item) => item.value === 0 && !item.completed)).toBe(true);

    const dates = app.ctx.db
      .prepare("SELECT DISTINCT date FROM user_daily_tasks WHERE user_id = (SELECT id FROM users WHERE email = ?)")
      .all(user.email) as { date: string }[];
    expect(dates.map((item) => item.date)).toEqual(["2026-11-11"]);
  });

  it("每日任务与成就共用一次上报, 各自独立结算", async () => {
    const user = await registerUser(app);
    const board = expectOk<DailyBoardResponse>(await app.get("/api/v1/achievements/daily", user.token));
    const task = board.tasks[0];
    expect(task).toBeDefined();
    if (!task) {
      return;
    }
    const result = expectOk<{
      granted: { source: string; id: number }[];
      summary: { completed: number };
      daily: DailyBoardResponse;
    }>(await app.post("/api/v1/achievements/report", { stats: { [task.kind]: task.target } }, user.token));
    const sources = new Set(result.granted.map((item) => item.source));
    expect(sources.has("daily_task")).toBe(true);
    expect(result.daily.tasks.some((item) => item.slot === task.slot && item.completed)).toBe(true);
  });
});