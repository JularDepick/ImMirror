/**
 * 当前用户资料路由
 * 返回账号基本信息, 钱包余额与本自然月签到进度
 */

import type { FastifyInstance } from "fastify";
import { createAuthGuard } from "../../fastify/auth-guard";
import type { AppContext } from "../../fastify/context";
import { ok } from "../../utils/response";

export function registerMeRoutes(app: FastifyInstance, ctx: AppContext): void {
  const guard = createAuthGuard(ctx);

  app.get("/api/v1/me", { preHandler: guard }, async (request) => {
    const auth = request.auth;
    if (!auth) {
      return ok(null);
    }
    const user = auth.user;
    const wallet = ctx.wallet.getSummary(user.id);
    return ok({
      uid: user.uid,
      email: user.email,
      createdAt: user.created_at,
      emailVerified: user.email_verified_at !== null,
      wallet: { balance: wallet.balance },
      checkin: ctx.wallet.getCheckinStatus(user.id),
    });
  });
}
