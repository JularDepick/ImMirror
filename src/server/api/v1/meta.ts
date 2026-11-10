/**
 * 元信息路由
 * 提供协议版本与服务状态, 客户端用它判断是否需要升级
 */

import type { FastifyInstance } from "fastify";
import { ok } from "../../utils/response";
import type { AppContext } from "../../fastify/context";

export function registerMetaRoutes(app: FastifyInstance, ctx: AppContext): void {
  app.get("/api/v1/meta/version", async () => {
    return ok({
      apiVersion: ctx.config.apiVersion,
      minClientVersion: ctx.config.minClientVersion,
      serverTime: ctx.now(),
    });
  });
}
