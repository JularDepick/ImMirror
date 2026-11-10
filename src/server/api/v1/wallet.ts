/**
 * 钱包与签到路由
 * 查询余额, 查询签到状态, 执行当日签到
 */

import type { FastifyInstance } from "fastify";
import { createAuthGuard } from "../../fastify/auth-guard";
import type { AppContext } from "../../fastify/context";
import { AppError } from "../../utils/errors";
import { ok } from "../../utils/response";

export function registerWalletRoutes(app: FastifyInstance, ctx: AppContext): void {
  const guard = createAuthGuard(ctx);

  const requireAuth = (request: { auth: { user: { id: number } } | null }) => {
    if (!request.auth) {
      throw new AppError("AUTH_UNAUTHORIZED");
    }
    return request.auth.user;
  };

  app.get("/api/v1/wallet", { preHandler: guard }, async (request) => {
    const user = requireAuth(request);
    const wallet = ctx.wallet.getSummary(user.id);
    return ok({ balance: wallet.balance, updatedAt: wallet.updatedAt });
  });

  app.get("/api/v1/wallet/checkin", { preHandler: guard }, async (request) => {
    const user = requireAuth(request);
    return ok(ctx.wallet.getCheckinStatus(user.id));
  });

  app.post("/api/v1/wallet/checkin", { preHandler: guard }, async (request) => {
    const user = requireAuth(request);
    return ok(ctx.wallet.checkin(user.id));
  });
}
