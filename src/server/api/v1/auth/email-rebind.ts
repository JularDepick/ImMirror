/**
 * 换绑邮箱路由
 * 需要登录, 向新邮箱发送验证码并要求密码确认
 */

import type { FastifyInstance } from "fastify";
import { createAuthGuard } from "../../../fastify/auth-guard";
import type { AppContext } from "../../../fastify/context";
import { AppError } from "../../../utils/errors";
import { ok } from "../../../utils/response";
import { requestBody, requestIp } from "../helpers";

export function registerEmailRebindRoutes(app: FastifyInstance, ctx: AppContext): void {
  const guard = createAuthGuard(ctx);

  app.post("/api/v1/auth/email/rebind/code", { preHandler: guard }, async (request) => {
    if (!request.auth) {
      throw new AppError("AUTH_UNAUTHORIZED");
    }
    const body = requestBody(request);
    return ok(await ctx.auth.requestEmailRebind(request.auth.user, body.newEmail, requestIp(request)));
  });

  app.post("/api/v1/auth/email/rebind", { preHandler: guard }, async (request) => {
    if (!request.auth) {
      throw new AppError("AUTH_UNAUTHORIZED");
    }
    const body = requestBody(request);
    return ok(
      await ctx.auth.rebindEmail(request.auth.user, {
        newEmail: body.newEmail,
        code: body.code,
        password: body.password,
      }),
    );
  });
}
