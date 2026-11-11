/**
 * v1 路由汇总
 * 所有业务路由都注册在同一个 Fastify 实例上, 路径统一以 /api/v1 开头
 */

import type { FastifyInstance } from "fastify";
import type { AppContext } from "../../fastify/context";
import { registerEmailRebindRoutes } from "./auth/email-rebind";
import { registerLoginRoutes } from "./auth/login";
import { registerLogoutRoutes } from "./auth/logout";
import { registerPasswordResetRoutes } from "./auth/password-reset";
import { registerRegisterRoutes } from "./auth/register";
import { registerMeRoutes } from "./me";
import { registerMetaRoutes } from "./meta";
import { registerAchievementRoutes } from "./achievements";
import { registerWalletRoutes } from "./wallet";

export function registerApiRoutes(app: FastifyInstance, ctx: AppContext): void {
  registerMetaRoutes(app, ctx);
  registerRegisterRoutes(app, ctx);
  registerLoginRoutes(app, ctx);
  registerPasswordResetRoutes(app, ctx);
  registerEmailRebindRoutes(app, ctx);
  registerLogoutRoutes(app, ctx);
  registerMeRoutes(app, ctx);
  registerWalletRoutes(app, ctx);
  registerAchievementRoutes(app, ctx);
}