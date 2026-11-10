/**
 * 登出路由
 * 注销当前会话令牌, 其余设备会话不受影响
 */

import type { FastifyInstance } from "fastify";
import { createAuthGuard } from "../../../fastify/auth-guard";
import type { AppContext } from "../../../fastify/context";
import { AppError } from "../../../utils/errors";
import { ok } from "../../../utils/response";

export function registerLogoutRoutes(app: FastifyInstance, ctx: AppContext): void {
  const guard = createAuthGuard(ctx);

  app.post("/api/v1/auth/logout", { preHandler: guard }, async (request) => {
    if (!request.authToken) {
      throw new AppError("AUTH_UNAUTHORIZED");
    }
    ctx.auth.logout(request.authToken);
    return ok({ ok: true });
  });
}
