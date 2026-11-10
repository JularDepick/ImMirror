/**
 * 路由层公共工具
 * 统一提取客户端信息与请求体, 避免各路由重复判断
 */

import type { FastifyRequest } from "fastify";
import type { SessionMeta } from "../../services/session";
import { requireObjectBody } from "../../utils/validate";
import type { JsonObject } from "../../utils/validate";

export function requestMeta(request: FastifyRequest): SessionMeta {
  const userAgent = request.headers["user-agent"];
  return {
    ip: request.ip,
    ua: typeof userAgent === "string" ? userAgent.slice(0, 255) : null,
  };
}

export function requestIp(request: FastifyRequest): string {
  return request.ip;
}

export function requestBody(request: FastifyRequest): JsonObject {
  return requireObjectBody(request.body);
}
