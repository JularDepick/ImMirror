/**
 * Bearer 鉴权守卫
 * 解析 Authorization 头, 校验会话并把结果挂到请求对象
 */

import type { preHandlerHookHandler } from "fastify";
import { AppError } from "../utils/errors";
import type { AppContext } from "./context";

export function extractBearerToken(header: string | undefined): string | null {
  if (!header) {
    return null;
  }
  const match = /^Bearer\s+(\S+)$/i.exec(header.trim());
  if (!match || !match[1]) {
    return null;
  }
  return match[1];
}

export function createAuthGuard(ctx: AppContext): preHandlerHookHandler {
  return async (request) => {
    const token = extractBearerToken(request.headers.authorization);
    if (!token) {
      throw new AppError("AUTH_UNAUTHORIZED");
    }
    request.auth = ctx.session.authenticate(token);
    request.authToken = token;
  };
}
