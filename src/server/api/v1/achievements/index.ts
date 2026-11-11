/**
 * 成就与每日任务路由
 * 查询成就总览, 查询当日任务, 上报增量统计; 三个接口都需要 Bearer 鉴权
 */

import type { FastifyInstance } from "fastify";
import { createAuthGuard } from "../../../fastify/auth-guard";
import type { AppContext } from "../../../fastify/context";
import { AchievementService, parseReportStats } from "../../../services/achievement";
import { AppError } from "../../../utils/errors";
import { ok } from "../../../utils/response";
import { requestBody } from "../helpers";

export function registerAchievementRoutes(app: FastifyInstance, ctx: AppContext): void {
  const guard = createAuthGuard(ctx);
  // 服务无状态, 全部进度落在库里, 因此在路由注册时构造一次即可
  const service = new AchievementService(ctx.db, ctx.config, ctx.now);

  const requireAuth = (request: { auth: { user: { id: number } } | null }) => {
    if (!request.auth) {
      throw new AppError("AUTH_UNAUTHORIZED");
    }
    return request.auth.user;
  };

  /* 成就总览: 全部定义与本人进度的合并视图, 附完成数与镜币汇总. */
  app.get("/api/v1/achievements", { preHandler: guard }, async (request) => {
    const user = requireAuth(request);
    const status = service.getStatus(user.id);
    return ok({
      balance: status.balance,
      summary: status.summary,
      achievements: status.achievements,
    });
  });

  /* 当日任务: 首次访问时按日期键生成, 之后只读. */
  app.get("/api/v1/achievements/daily", { preHandler: guard }, async (request) => {
    const user = requireAuth(request);
    return ok(service.dailyBoard(user.id));
  });

  /*
   * 上报增量统计: 客户端上报自上次上报以来的累计增量, 服务端累加并结算奖励.
   * 返回本次新完成的成就与任务, 便于客户端做完成提示.
   */
  app.post("/api/v1/achievements/report", { preHandler: guard }, async (request) => {
    const user = requireAuth(request);
    const increments = parseReportStats(requestBody(request));
    return ok(service.report(user.id, increments));
  });
}
