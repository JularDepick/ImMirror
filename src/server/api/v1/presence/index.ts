/**
 * ImMirror 我之镜界 服务端 作者: JularDepick
 * 位置上报与查询: 联机骨架 M9 的最小位置通道.
 * 内存表即弃, 30 秒超时剔除, 不落库; 轮询间隔由客户端决定.
 * 模块级内存表是进程单例, 重启即清空, 与账号会话的持久化语义刻意不同.
 */

import type { FastifyInstance } from "fastify";
import { createAuthGuard } from "../../../fastify/auth-guard";
import type { AppContext } from "../../../fastify/context";
import { AppError } from "../../../utils/errors";
import { ok } from "../../../utils/response";
import { requireObjectBody } from "../../../utils/validate";

/** 单个玩家的最新位置快照, uid 为对外脱敏昵称. */
interface PresenceEntry {
  uid: string;
  x: number;
  y: number;
  z: number;
  yaw: number;
  pitch: number;
  updatedAt: number;
}

/** 返回给其他玩家的脱敏位置条目. */
export interface PresenceSnapshot {
  uid: string;
  x: number;
  y: number;
  z: number;
  yaw: number;
  pitch: number;
}

/** 位置快照存活期, 超过即视为离线并剔除. */
const ENTRY_TTL_MS = 30 * 1000;
/** 内存表容量上限, 防止异常流量撑爆进程内存. */
const MAX_TRACKED = 256;

/**
 * 进程级内存表: 键为用户自增 id, 值为最新位置快照.
 * 不落库, 服务重启即空, 客户端按轮询周期自动重新上报.
 */
const entries = new Map<number, PresenceEntry>();

/** 清理超过存活期的位置快照; 每次读写前顺带执行, 无独立定时器. */
function sweepExpired(now: number): void {
  for (const [userId, entry] of entries) {
    if (now - entry.updatedAt > ENTRY_TTL_MS) {
      entries.delete(userId);
    }
  }
}

/** 同世界其他玩家的位置列表, 不含查询者本人; 列表随查询即时构建. */
export function othersOf(userId: number): PresenceSnapshot[] {
  const result: PresenceSnapshot[] = [];
  for (const [otherId, entry] of entries) {
    if (otherId === userId) {
      continue;
    }
    result.push({
      uid: entry.uid,
      x: entry.x,
      y: entry.y,
      z: entry.z,
      yaw: entry.yaw,
      pitch: entry.pitch,
    });
  }
  return result;
}

/** 清空内存表, 仅供测试构造隔离场景. */
export function resetPresenceTableForTest(): void {
  entries.clear();
}

export function registerPresenceRoutes(app: FastifyInstance, ctx: AppContext): void {
  const guard = createAuthGuard(ctx);

  /**
   * 上报自身位置并返回其他玩家快照.
   * 轮询客户端可以只用这一个接口完成上报与互见.
   */
  app.post("/api/v1/presence", { preHandler: guard }, async (request) => {
    const auth = request.auth;
    if (!auth) {
      throw new AppError("AUTH_UNAUTHORIZED");
    }
    const body = requireObjectBody(request.body);
    // 数值字段统一校验, 缺失或非数值一律 BAD_REQUEST; yaw 与 pitch 缺省为零
    const readNumber = (key: "x" | "y" | "z"): number => {
      const value = body[key];
      if (typeof value !== "number" || !Number.isFinite(value)) {
        throw new AppError("BAD_REQUEST", "位置字段必须为数值");
      }
      return value;
    };
    const x = readNumber("x");
    const y = readNumber("y");
    const z = readNumber("z");
    const yaw = typeof body.yaw === "number" && Number.isFinite(body.yaw) ? body.yaw : 0;
    const pitch = typeof body.pitch === "number" && Number.isFinite(body.pitch) ? body.pitch : 0;

    // 时钟统一走上下文注入, 便于测试用固定时间点验证超时剔除
    const now = ctx.now();
    sweepExpired(now);
    if (!entries.has(auth.user.id) && entries.size >= MAX_TRACKED) {
      throw new AppError("PRESENCE_FULL");
    }
    // 脱敏: 对外只暴露九位数字 uid, 不带邮箱等任何账号信息
    entries.set(auth.user.id, {
      uid: auth.user.uid,
      x,
      y,
      z,
      yaw,
      pitch,
      updatedAt: now,
    });
    return ok({ others: othersOf(auth.user.id), serverTime: now });
  });

  /** 只读查询其他玩家, 不更新自身位置, 不刷新自身存活期. */
  app.get("/api/v1/presence", { preHandler: guard }, async (request) => {
    const auth = request.auth;
    if (!auth) {
      throw new AppError("AUTH_UNAUTHORIZED");
    }
    const now = ctx.now();
    sweepExpired(now);
    return ok({ others: othersOf(auth.user.id), serverTime: now });
  });
}
