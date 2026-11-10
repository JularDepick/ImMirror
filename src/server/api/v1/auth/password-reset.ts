/**
 * 忘记密码路由
 * 通过邮箱验证码重置密码, 重置成功后旧会话全部失效
 */

import type { FastifyInstance } from "fastify";
import type { AppContext } from "../../../fastify/context";
import { ok } from "../../../utils/response";
import { requestBody, requestIp } from "../helpers";


export function registerPasswordResetRoutes(app: FastifyInstance, ctx: AppContext): void {
  app.post("/api/v1/auth/password/reset/code", async (request) => {
    const body = requestBody(request);
    return ok(await ctx.auth.requestPasswordResetCode(body.email, requestIp(request)));
  });

  app.post("/api/v1/auth/password/reset", async (request) => {
    const body = requestBody(request);
    await ctx.auth.resetPassword({ email: body.email, code: body.code, password: body.password });
    return ok({ ok: true });
  });
}
