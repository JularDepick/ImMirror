/**
 * 当前用户资料路由
 * 返回账号基本信息, 钱包余额与本自然月签到进度
 */

import type { FastifyInstance } from "fastify";
import { createAuthGuard } from "../../fastify/auth-guard";
import type { AppContext } from "../../fastify/context";
import { ok } from "../../utils/response";
import { AppError } from "../../utils/errors";

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
      /* 角色皮肤: 0 表示默认外观. */
      skinId: (user as { skin_id?: number }).skin_id ?? 0,
    });
  });

  /**
   * 设置角色皮肤.
   * 只做取值范围校验: 皮肤表在客户端, 服务端不复制一份, 避免两处数据漂移.
   */
  app.post("/api/v1/me/skin", { preHandler: guard }, async (request) => {
    const auth = request.auth;
    if (!auth) {
      return ok(null);
    }
    const body = request.body;
    if (typeof body !== "object" || body === null) {
      throw new AppError("BAD_REQUEST", "请求体必须为对象");
    }
    const raw = (body as Record<string, unknown>).skinId;
    if (typeof raw !== "number" || !Number.isFinite(raw)) {
      throw new AppError("BAD_REQUEST", "皮肤编号必须为数值");
    }
    const skinId = Math.trunc(raw);
    if (skinId < 0 || skinId > 9999) {
      throw new AppError("BAD_REQUEST", "皮肤编号超出范围");
    }
    ctx.db
      .prepare("UPDATE users SET skin_id = ?, updated_at = ? WHERE id = ?")
      .run(skinId, ctx.now(), auth.user.id);
    return ok({ skinId });
  });
}
