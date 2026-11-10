/**
 * Nitro 中间件
 * 把所有请求原样交给 Fastify 实例处理, Nitro 自身不注册任何业务路由;
 * 本工程的 HTTP 层完全由 Fastify 承担, 因此也不会产出任何 HTML 页面
 */

import { defineEventHandler } from "h3";
import { getAppContext } from "../fastify/instance";

interface RoutingFastify {
  routing(request: unknown, response: unknown): void;
}

export default defineEventHandler(async (event) => {
  const ctx = getAppContext();
  if (!ctx) {
    return;
  }
  const request = event.node.req;
  const response = event.node.res;
  const routing = ctx.fastify as unknown as RoutingFastify;
  await new Promise<void>((resolve) => {
    let settled = false;
    const finish = (): void => {
      if (!settled) {
        settled = true;
        resolve();
      }
    };
    response.on("finish", finish);
    response.on("close", finish);
    try {
      routing.routing(request, response);
    } catch (error) {
      finish();
      if (!response.headersSent) {
        response.statusCode = 500;
        response.setHeader("content-type", "application/json; charset=utf-8");
        response.end(JSON.stringify({ ok: false, error: { code: "INTERNAL", message: "服务内部错误" } }));
      }
      console.error("[immirror] fastify routing failed:", error instanceof Error ? error.message : error);
    }
  });
  (event as unknown as { handled: boolean }).handled = true;
});
