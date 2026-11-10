/**
 * 注册路由
 * 验证码发送与注册提交分开, 注册成功即返回可用会话
 */

import type { FastifyInstance } from "fastify";
import type { AppContext } from "../../../fastify/context";
import { ok } from "../../../utils/response";
import { requestBody, requestIp, requestMeta } from "../helpers";

export function registerRegisterRoutes(app: FastifyInstance, ctx: AppContext): void {
  app.post("/api/v1/auth/register/code", async (request) => {
    const body = requestBody(request);
    return ok(await ctx.auth.requestRegisterCode(body.email, requestIp(request)));
  });

  app.post("/api/v1/auth/register", async (request) => {
    const body = requestBody(request);
    return ok(await ctx.auth.register({ email: body.email, code: body.code, password: body.password }, requestMeta(request)));
  });
}
