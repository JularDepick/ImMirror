/**
 * 通用用例组
 * 覆盖未知路径, 非 JSON 请求, 响应包络一致性, 版本接口与 HTML 屏蔽
 */

import { afterEach, beforeEach, describe, expect, it } from "vitest";
import { createTestApp, expectFail, expectOk } from "./helpers";
import type { TestApp } from "./helpers";

let app: TestApp;

beforeEach(async () => {
  app = await createTestApp();
});

afterEach(async () => {
  await app.close();
});

describe("通用约定", () => {
  it("未知路径返回 404 与 NOT_FOUND", async () => {
    const response = await app.get("/api/v1/unknown");
    expect(response.status).toBe(404);
    expect(response.contentType).toContain("application/json");
    expectFail(response, "NOT_FOUND");
  });

  it("根路径不返回 HTML", async () => {
    const response = await app.get("/");
    expect(response.status).toBe(404);
    expect(response.contentType).toContain("application/json");
    expect(response.contentType).not.toContain("text/html");
  });

  it("非 JSON 请求体返回 BAD_REQUEST", async () => {
    const response = await app.request({
      method: "POST",
      url: "/api/v1/auth/register",
      payload: "email=a@b.com",
      headers: { "content-type": "text/plain" },
    });
    expect(response.status).toBe(400);
    expectFail(response, "BAD_REQUEST");
  });

  it("畸形 JSON 请求体返回 BAD_REQUEST", async () => {
    const response = await app.request({
      method: "POST",
      url: "/api/v1/auth/register",
      payload: "{not-json",
      headers: { "content-type": "application/json" },
    });
    expect(response.status).toBe(400);
    expectFail(response, "BAD_REQUEST");
  });

  it("版本接口返回协议版本与服务时间", async () => {
    const response = await app.get("/api/v1/meta/version");
    expect(response.status).toBe(200);
    const data = expectOk<{ apiVersion: string; minClientVersion: string; serverTime: number }>(response);
    expect(data.apiVersion).toBe("v1");
    expect(data.minClientVersion).toBe("0.1.0");
    expect(data.serverTime).toBe(app.clock.now());
  });

  it("所有响应都符合统一包络", async () => {
    const success = await app.get("/api/v1/meta/version");
    expect(success.body).toHaveProperty("ok", true);
    expect(success.body).toHaveProperty("data");

    const failure = await app.get("/api/v1/me");
    expect(failure.body).toHaveProperty("ok", false);
    expect(failure.body).toHaveProperty("error.code");
    expect(failure.body).toHaveProperty("error.message");
  });

  it("请求体超过上限返回 4xx 与统一包络", async () => {
    const response = await app.request({
      method: "POST",
      url: "/api/v1/auth/register",
      payload: { email: "a@b.com", code: "123456", password: "x".repeat(20000) },
    });
    expect(response.status).toBeGreaterThanOrEqual(400);
    expect(response.status).toBeLessThan(500);
    const body = response.body as { ok: boolean };
    expect(body.ok).toBe(false);
  });
});
