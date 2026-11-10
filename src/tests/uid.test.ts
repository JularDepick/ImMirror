/**
 * UID 生成用例组
 * 覆盖长度与首位非 0 的格式约束, 以及 UID 冲突时的重新生成与重试上限
 */

import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
import { createTestApp, expectFail, expectOk, registerUser } from "./helpers";
import type { TestApp } from "./helpers";

// 可控 UID 队列: 只替换 randomUid, 其余随机函数保持真实实现
const uidControl = vi.hoisted(() => ({ queue: [] as string[] }));

vi.mock("../server/utils/random", async (importOriginal) => {
  const actual = await importOriginal<typeof import("../server/utils/random")>();
  return {
    ...actual,
    randomUid: (): string => {
      const next = uidControl.queue.shift();
      return next ?? actual.randomUid();
    },
  };
});

let app: TestApp;

beforeEach(async () => {
  uidControl.queue.length = 0;
  app = await createTestApp();
});

afterEach(async () => {
  await app.close();
});

describe("UID 生成", () => {
  it("连续注册返回的 UID 均为 9 位十进制且首位非 0", async () => {
    const seen = new Set<string>();
    for (let index = 0; index < 12; index += 1) {
      const user = await registerUser(app, { email: `uid-${index}@immirror.test` });
      expect(user.uid).toMatch(/^[1-9]\d{8}$/);
      expect(user.uid).toHaveLength(9);
      expect(user.uid.startsWith("0")).toBe(false);
      expect(seen.has(user.uid)).toBe(false);
      seen.add(user.uid);
    }
  });

  it("UID 与既有账号冲突时重新生成后注册成功", async () => {
    const first = await registerUser(app, { email: "collide-first@immirror.test" });
    // 第一次生成必然撞库, 服务应换一个 UID 再试
    uidControl.queue.push(first.uid);

    const second = await registerUser(app, { email: "collide-second@immirror.test" });
    expect(second.uid).not.toBe(first.uid);
    expect(second.uid).toMatch(/^[1-9]\d{8}$/);
    // 队列已被消耗一次, 说明确实发生了重新生成
    expect(uidControl.queue).toHaveLength(0);

    const rows = app.ctx.db.prepare("SELECT uid FROM users ORDER BY id").all() as { uid: string }[];
    expect(rows).toHaveLength(2);
    expect(new Set(rows.map((row) => row.uid)).size).toBe(2);
  });

  it("UID 连续冲突达到重试上限时返回 INTERNAL", async () => {
    const first = await registerUser(app, { email: "exhaust-first@immirror.test" });
    // 服务端最多重试 10 次, 这里让 10 次全部撞库
    for (let index = 0; index < 10; index += 1) {
      uidControl.queue.push(first.uid);
    }
    const email = "exhaust-second@immirror.test";
    await app.post("/api/v1/auth/register/code", { email });
    const code = app.readCode(email, "register");
    const response = await app.post("/api/v1/auth/register", { email, code, password: "P@ssw0rd-2026" });
    expect(response.status).toBe(500);
    expectFail(response, "INTERNAL");
  });

  it("UID 登录使用 9 位账号标识", async () => {
    const user = await registerUser(app, { email: "uid-login@immirror.test" });
    expect(user.uid).toHaveLength(9);
    const response = await app.post("/api/v1/auth/login", { identifier: user.uid, password: user.password });
    expect(response.status).toBe(200);
    expect(expectOk<{ uid: string }>(response).uid).toBe(user.uid);
  });
});
