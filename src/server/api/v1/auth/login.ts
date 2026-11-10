/**
 * 登录路由
 * 支持邮箱或 UID 加密码, 以及邮箱验证码登录
 */

import type { FastifyInstance } from "fastify";
import type { AppContext } from "../../../fastify/context";
import { ok } from "../../../utils/response";
import { requestBody, requestIp, requestMeta } from "../helpers";

export function registerLoginRoutes(app: FastifyInstance, ctx: AppContext): void {
  app.post("/api/v1/auth/login", async (request) => {
    const body = requestBody(request);
    return ok(
      await ctx.auth.login({ identifier: body.identifier, password: body.password }, requestMeta(request)),
    );
  });

  app.post("/api/v1/auth/login/code", async (request) => {
    const body = requestBody(request);
    return ok(await ctx.auth.requestLoginCode(body.email, requestIp(request)));
  });

  app.post("/api/v1/auth/login/code/verify", async (request) => {
    const body = requestBody(request);
    return ok(await ctx.auth.loginWithCode({ email: body.email, code: body.code }, requestMeta(request)));
  });
}
