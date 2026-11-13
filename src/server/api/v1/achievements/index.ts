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

  /* 领取单条成就奖励: 完成与领取分离, 只有已完成的条目可领. */
  app.post("/api/v1/achievements/claim", { preHandler: guard }, async (request) => {
    const user = requireAuth(request);
    const id = readInteger(requestBody(request), "id");
    return ok(service.claimAchievement(user.id, id));
  });

  /* 批量领取: 一次领完所有已完成未领取的成就与当日任务. */
  app.post("/api/v1/achievements/claim-all", { preHandler: guard }, async (request) => {
    const user = requireAuth(request);
    return ok(service.claimAll(user.id));
  });

  /* 领取单条每日任务奖励: 槽位取当日任务板, 日期按服务端时区当天. */
  app.post("/api/v1/achievements/daily/claim", { preHandler: guard }, async (request) => {
    const user = requireAuth(request);
    const slot = readInteger(requestBody(request), "slot");
    return ok(service.claimDaily(user.id, slot));
  });
}

/**
 * 取整数字段: 允许数字或数字字符串, 缺失与非数字一律 BAD_REQUEST.
 * 客户端统一以字符串下发, 因此两种形式都要接受.
 */
function readInteger(body: { [key: string]: unknown }, key: string): number {
  const raw = body[key];
  const value = typeof raw === "number" ? raw : typeof raw === "string" ? Number.parseInt(raw, 10) : Number.NaN;
  if (!Number.isFinite(value)) {
    throw new AppError("BAD_REQUEST", `${key} 必须为整数`);
  }
  return Math.trunc(value);
}
