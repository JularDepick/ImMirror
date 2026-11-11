/**
 * ImMirror 我之镜界 服务端 作者: JularDepick
 * 位置上报与查询: 联机骨架的位置通道.
 * 内存表即弃, 30 秒超时剔除, 不落库; 轮询间隔由客户端决定.
 */
import type { FastifyInstance } from "fastify";
import { fail, ok } from "../../../utils/response";
import type { AppContext } from "../../../fastify/context";
import { createAuthGuard } from "../../../fastify/auth-guard";

/** 单个玩家的位置快照. */
interface PresenceEntry {
  x: number;
  y: number;
  z: number;
  yaw: number;
  pitch: number;
  updatedAt: number;
}

const ENTRY_TTL_MS = 30 * 1000;
const MAX_TRACKED = 256;
const entries = new Map<number, PresenceEntry>();

/** 剔除超过存活期的位置快照. */
function sweepExpired(now: number): void {
  for (const [userId, entry] of entries) {
    if (now - entry.updatedAt > ENTRY_TTL_MS) {
      entries.delete(userId);
    }
  }
}

/** 同世界其他玩家的位置列表, 不含上报者本人. */
function othersOf(userId: number): Array<{ uid: number; x: number; y: number; z: number; yaw: number; pitch: number }> {
  const result: Array<{ uid: number; x: number; y: number; z: number; yaw: number; pitch: number }> = [];
  for (const [otherId, entry] of entries) {
    if (otherId === userId) {
      continue;
    }
    result.push({ uid: otherId, x: entry.x, y: entry.y, z: entry.z, yaw: entry.yaw, pitch: entry.pitch });
  }
  return result;
}

export interface PresenceHandlers {
  register(app: FastifyInstance, ctx: AppContext): void;
}

export function registerPresenceRoutes(app: FastifyInstance, ctx: AppContext): void {
  const guard = createAuthGuard(ctx);

  /** 上报自身位置并返回其他玩家快照. */
  app.post("/api/v1/presence", { preHandler: guard }, async (request, reply) => {
    if (!request.auth) {
      return reply.code(401).send(fail("AUTH_UNAUTHORIZED", "缺少有效的会话凭证"));
    }
    const user = request.auth.user;
    const body = request.body as { x?: number; y?: number; z?: number; yaw?: number; pitch?: number } | null;
    if (!body || typeof body.x !== "number" || typeof body.y !== "number" || typeof body.z !== "number") {
      return reply.code(400).send(
        fail("BAD_REQUEST", "缺少位置字段"),
      );
    }
    const now = Date.now();
    sweepExpired(now);
    if (!entries.has(user.id) && entries.size >= MAX_TRACKED) {
      return reply.code(400).send(fail("PRESENCE_FULL", "在线人数已达上限"));
    }
    entries.set(user.id, {
      x: body.x,
      y: body.y,
      z: body.z,
      yaw: typeof body.yaw === "number" ? body.yaw : 0,
      pitch: typeof body.pitch === "number" ? body.pitch : 0,
      updatedAt: now,
    });
    return ok({ others: othersOf(user.id) });
  });

  /** 只读查询其他玩家, 不更新自身位置. */
  app.get("/api/v1/presence", { preHandler: guard }, async (request) => {
    if (!request.auth) {
      return reply.code(401).send(fail("AUTH_UNAUTHORIZED", "缺少有效的会话凭证"));
    }
    const user = request.auth.user;
    sweepExpired(Date.now());
    return ok({ others: othersOf(user.id) });
  });
}