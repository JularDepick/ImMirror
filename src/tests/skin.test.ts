/**
 * 角色皮肤接口用例组
 * 覆盖缺省值, 取值范围与持久化
 */

import { beforeEach, describe, expect, it } from "vitest";
import { createTestApp, expectFail, expectOk, registerUser } from "./helpers";
import type { TestApp } from "./helpers";

let app: TestApp;

beforeEach(async () => {
  app = await createTestApp();
});

let caseSeq = 0;

function register(tag: string): Promise<{ token: string }> {
  caseSeq += 1;
  return registerUser(app, { email: `s${caseSeq}-${tag}@immirror.test` });
}

describe("角色皮肤", () => {
  it("缺省为默认外观", async () => {
    const user = await register("skin-default");
    const me = expectOk<{ skinId: number }>(await app.get("/api/v1/me", user.token));
    expect(me.skinId).toBe(0);
  });

  it("设置后能读回", async () => {
    const user = await register("skin-set");
    expectOk(await app.post("/api/v1/me/skin", { skinId: 12 }, user.token));
    const me = expectOk<{ skinId: number }>(await app.get("/api/v1/me", user.token));
    expect(me.skinId).toBe(12);
  });

  it("超出范围被拒", async () => {
    const user = await register("skin-range");
    expectFail(await app.post("/api/v1/me/skin", { skinId: 10000 }, user.token), "BAD_REQUEST");
    expectFail(await app.post("/api/v1/me/skin", { skinId: -1 }, user.token), "BAD_REQUEST");
  });

  it("非数值被拒", async () => {
    const user = await register("skin-type");
    expectFail(await app.post("/api/v1/me/skin", { skinId: "12" }, user.token), "BAD_REQUEST");
  });
});
