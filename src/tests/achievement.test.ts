/**
 * 成就用例组
 * 覆盖鉴权, 初始进度, 增量上报, 首次完成发币, 重复上报幂等, 进度封顶,
 * 参数校验, 钱包流水与多账号隔离
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

interface AchievementItem {
  id: number;
  name: string;
  description: string;
  category: number;
  categoryName: string;
  kind: string;
  kindTitle: string;
  unit: string;
  target: number;
  value: number;
  completed: boolean;
  completedAt: number | null;
  coinReward: number;
}

interface AchievementList {
  balance: number;
  summary: { completed: number; total: number; coins: number };
  achievements: AchievementItem[];
}

interface GrantRecord {
  source: string;
  id: number;
  name: string;
  coins: number;
}

interface ReportResult extends AchievementList {
  granted: GrantRecord[];
  daily: { date: string; tasks: unknown[]; completed: number; coins: number };
}

/* 目标为 1 的某类成就, 用于断言"上报一次即完成"的集合. */
function targetOneIds(kind: string): number[] {
  return ACHIEVEMENT_CATALOG.filter((item) => item.kind === kind && item.target === 1).map((item) => item.id);
}

function coinsOf(ids: number[]): number {
  return ACHIEVEMENT_CATALOG.filter((item) => ids.includes(item.id)).reduce(
    (total, item) => total + item.coinReward,
    0,
  );
}

describe("成就查询", () => {
  it("未登录访问成就接口返回 AUTH_UNAUTHORIZED", async () => {
    const list = await app.get("/api/v1/achievements");
    expect(list.status).toBe(401);
    expectFail(list, "AUTH_UNAUTHORIZED");

    const report = await app.post("/api/v1/achievements/report", { stats: { break_block: 1 } });
    expect(report.status).toBe(401);
    expectFail(report, "AUTH_UNAUTHORIZED");
  });

  it("新账号拿到全部定义且进度为零", async () => {
    const user = await registerUser(app);
    const response = await app.get("/api/v1/achievements", user.token);
    expect(response.status).toBe(200);
    const data = expectOk<AchievementList>(response);
    expect(data.achievements.length).toBe(ACHIEVEMENT_CATALOG.length);
    expect(ACHIEVEMENT_CATALOG.length).toBeGreaterThanOrEqual(50);
    expect(data.summary).toEqual({ completed: 0, total: ACHIEVEMENT_CATALOG.length, coins: 0 });
    expect(data.balance).toBe(0);
    for (const item of data.achievements) {
      expect(item.value).toBe(0);
      expect(item.completed).toBe(false);
      expect(item.completedAt).toBeNull();
      expect(item.target).toBeGreaterThan(0);
      expect(item.name.length).toBeGreaterThan(0);
      expect(item.unit.length).toBeGreaterThan(0);
    }
  });

  it("定义按分类与统计类型完整下发", async () => {
    const user = await registerUser(app);
    const data = expectOk<AchievementList>(await app.get("/api/v1/achievements", user.token));
    const categories = new Set(data.achievements.map((item) => item.categoryName));
    expect(categories.size).toBeGreaterThan(1);
    for (const item of data.achievements) {
      expect([
        "break_block",
        "place_block",
        "pick_item",
        "walk_distance",
        "online_days",
        "craft_item",
        "smelt_item",
        "kill_creature",
      ]).toContain(item.kind);
      expect(item.category).toBeGreaterThanOrEqual(1);
      expect(item.category).toBeLessThanOrEqual(6);
      expect(item.coinReward).toBeGreaterThan(0);
    }
  });
});

describe("成就上报", () => {
  it("首次上报即完成的成就立即发放镜币", async () => {
    const user = await registerUser(app);
    const ids = targetOneIds("break_block");
    expect(ids.length).toBeGreaterThan(0);

    const response = await app.post("/api/v1/achievements/report", { stats: { break_block: 1 } }, user.token);
    expect(response.status).toBe(200);
    const data = expectOk<ReportResult>(response);

    const grantedIds = data.granted.filter((item) => item.source === "achievement").map((item) => item.id);
    expect(grantedIds.sort()).toEqual([...ids].sort());
    expect(data.summary.completed).toBe(ids.length);
    expect(data.balance).toBe(coinsOf(ids));
    expect(data.granted.every((item) => item.coins > 0)).toBe(true);

    const wallet = await app.get("/api/v1/wallet", user.token);
    expect(expectOk<{ balance: number }>(wallet).balance).toBe(coinsOf(ids));
  });

  it("重复上报同一事件不再重复发币", async () => {
    const user = await registerUser(app);
    const first = expectOk<ReportResult>(
      await app.post("/api/v1/achievements/report", { stats: { break_block: 1 } }, user.token),
    );
    const second = expectOk<ReportResult>(
      await app.post("/api/v1/achievements/report", { stats: { break_block: 1 } }, user.token),
    );
    expect(second.granted.length).toBe(0);
    expect(second.balance).toBe(first.balance);
    expect(second.summary.completed).toBe(first.summary.completed);
  });

  it("进度累积到目标后才完成, 完成前不发币", async () => {
    const user = await registerUser(app);
    const definition = ACHIEVEMENT_CATALOG.find((item) => item.kind === "break_block" && item.target === 10);
    expect(definition).toBeDefined();
    const target = definition?.target ?? 0;
    const id = definition?.id ?? 0;

    const partial = expectOk<ReportResult>(
      await app.post("/api/v1/achievements/report", { stats: { break_block: target - 1 } }, user.token),
    );
    const partialItem = partial.achievements.find((item) => item.id === id);
    expect(partialItem?.value).toBe(target - 1);
    expect(partialItem?.completed).toBe(false);
    expect(partial.granted.some((item) => item.id === id)).toBe(false);

    const rest = expectOk<ReportResult>(
      await app.post("/api/v1/achievements/report", { stats: { break_block: 1 } }, user.token),
    );
    const doneItem = rest.achievements.find((item) => item.id === id);
    expect(doneItem?.completed).toBe(true);
    expect(doneItem?.completedAt).toBe(app.clock.now());
    expect(rest.granted.some((item) => item.id === id)).toBe(true);
  });

  it("进度封顶到目标值, 超量上报不再增长", async () => {
    const user = await registerUser(app);
    const data = expectOk<ReportResult>(
      await app.post("/api/v1/achievements/report", { stats: { break_block: 5000 } }, user.token),
    );
    for (const item of data.achievements) {
      if (item.completed) {
        expect(item.value).toBe(item.target);
      }
    }
    const capped = data.achievements.find((item) => item.kind === "break_block" && item.target === 5000);
    expect(capped?.completed).toBe(true);
    expect(capped?.value).toBe(5000);
  });

  it("在线天数按天推进存活类成就", async () => {
    const user = await registerUser(app);
    const definition = ACHIEVEMENT_CATALOG.find((item) => item.kind === "online_days" && item.target === 7);
    expect(definition).toBeDefined();
    const id = definition?.id ?? 0;

    const before = expectOk<ReportResult>(
      await app.post("/api/v1/achievements/report", { stats: { online_days: 6 } }, user.token),
    );
    expect(before.achievements.find((item) => item.id === id)?.completed).toBe(false);

    const after = expectOk<ReportResult>(
      await app.post("/api/v1/achievements/report", { stats: { online_days: 1 } }, user.token),
    );
    const item = after.achievements.find((entry) => entry.id === id);
    expect(item?.completed).toBe(true);
    expect(item?.value).toBe(7);
  });

  it("完成成就写入钱包流水, 记账原因为 achievement", async () => {
    const user = await registerUser(app);
    const ids = targetOneIds("pick_item");
    const data = expectOk<ReportResult>(
      await app.post("/api/v1/achievements/report", { stats: { pick_item: 1 } }, user.token),
    );
    const row = app.ctx.db.prepare("SELECT id FROM users WHERE email = ?").get(user.email) as { id: number };
    const ledger = app.ctx.wallet.listLedger(row.id, 50);
    expect(ledger.length).toBe(ids.length);
    expect(ledger.every((item) => item.reason === "achievement")).toBe(true);
    expect(ledger.reduce((total, item) => total + item.delta, 0)).toBe(data.balance);
  });

  it("多账号进度互相隔离", async () => {
    const first = await registerUser(app, { email: "alpha@immirror.test" });
    const second = await registerUser(app, { email: "beta@immirror.test" });
    expectOk<ReportResult>(
      await app.post("/api/v1/achievements/report", { stats: { break_block: 1 } }, first.token),
    );
    const other = expectOk<AchievementList>(await app.get("/api/v1/achievements", second.token));
    expect(other.summary.completed).toBe(0);
    expect(other.balance).toBe(0);
  });

  it("上报删除了旧日期任务后仍保留当日任务", async () => {
    const user = await registerUser(app);
    const today = expectOk<{ date: string; tasks: { slot: number }[] }>(
      await app.get("/api/v1/achievements/daily", user.token),
    );
    app.clock.advance(3 * 24 * 60 * 60 * 1000);
    expectOk<ReportResult>(
      await app.post("/api/v1/achievements/report", { stats: { break_block: 1 } }, user.token),
    );
    const rows = app.ctx.db
      .prepare("SELECT date FROM user_daily_tasks WHERE user_id = (SELECT id FROM users WHERE email = ?)")
      .all(user.email) as { date: string }[];
    expect(rows.every((item) => item.date !== today.date)).toBe(true);
  });
});

describe("上报参数校验", () => {
  it("未知统计键返回 BAD_REQUEST", async () => {
    const user = await registerUser(app);
    const response = await app.post("/api/v1/achievements/report", { stats: { fly_block: 1 } }, user.token);
    expect(response.status).toBe(400);
    expectFail(response, "BAD_REQUEST");
  });

  it("负数与小数值返回 BAD_REQUEST", async () => {
    const user = await registerUser(app);
    const negative = await app.post("/api/v1/achievements/report", { stats: { break_block: -1 } }, user.token);
    expectFail(negative, "BAD_REQUEST");
    const fraction = await app.post("/api/v1/achievements/report", { stats: { break_block: 1.5 } }, user.token);
    expectFail(fraction, "BAD_REQUEST");
    const text = await app.post("/api/v1/achievements/report", { stats: { break_block: "3" } }, user.token);
    expectFail(text, "BAD_REQUEST");
  });

  it("超大增量与空统计返回 BAD_REQUEST", async () => {
    const user = await registerUser(app);
    const huge = await app.post("/api/v1/achievements/report", { stats: { break_block: 200000 } }, user.token);
    expectFail(huge, "BAD_REQUEST");
    const empty = await app.post("/api/v1/achievements/report", { stats: {} }, user.token);
    expectFail(empty, "BAD_REQUEST");
    const missing = await app.post("/api/v1/achievements/report", {}, user.token);
    expectFail(missing, "BAD_REQUEST");
    const array = await app.post("/api/v1/achievements/report", { stats: [] }, user.token);
    expectFail(array, "BAD_REQUEST");
  });
});


describe("合成熔炼击败事件", () => {
  it("craft_item 上报推进合成类成就并发放镜币", async () => {
    const user = await registerUser(app);
    const ids = targetOneIds("craft_item");
    expect(ids.length).toBeGreaterThan(0);

    const data = expectOk<ReportResult>(
      await app.post("/api/v1/achievements/report", { stats: { craft_item: 1 } }, user.token),
    );
    const grantedIds = data.granted.filter((item) => item.source === "achievement").map((item) => item.id);
    expect(grantedIds.sort()).toEqual([...ids].sort());
    expect(data.balance).toBe(coinsOf(ids));
  });

  it("smelt_item 键被接受且不影响其他统计", async () => {
    const user = await registerUser(app);
    /* 原版成就表没有熔炼目标的条目, 熔炼只出现在每日任务模板, 这里验证键合法可上报. */
    const data = expectOk<ReportResult>(
      await app.post("/api/v1/achievements/report", { stats: { smelt_item: 1, break_block: 1 } }, user.token),
    );
    expect(data.achievements.length).toBe(ACHIEVEMENT_CATALOG.length);
  });

  it("kill_creature 上报推进击败类成就", async () => {
    const user = await registerUser(app);
    const ids = targetOneIds("kill_creature");
    expect(ids.length).toBeGreaterThan(0);

    const data = expectOk<ReportResult>(
      await app.post("/api/v1/achievements/report", { stats: { kill_creature: 1 } }, user.token),
    );
    const grantedIds = data.granted.filter((item) => item.source === "achievement").map((item) => item.id);
    expect(grantedIds.sort()).toEqual([...ids].sort());
  });

  it("三类新键的进度按增量累积", async () => {
    const user = await registerUser(app);
    const definition = ACHIEVEMENT_CATALOG.find((item) => item.kind === "kill_creature" && item.target > 1);
    expect(definition).toBeDefined();
    const target = definition?.target ?? 0;
    const id = definition?.id ?? 0;

    const partial = expectOk<ReportResult>(
      await app.post("/api/v1/achievements/report", { stats: { kill_creature: target - 1 } }, user.token),
    );
    const partialItem = partial.achievements.find((item) => item.id === id);
    expect(partialItem?.completed).toBe(false);

    const done = expectOk<ReportResult>(
      await app.post("/api/v1/achievements/report", { stats: { kill_creature: 1 } }, user.token),
    );
    const doneItem = done.achievements.find((item) => item.id === id);
    expect(doneItem?.completed).toBe(true);
    expect(doneItem?.value).toBe(target);
  });
});
