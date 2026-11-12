/**
 * ImMirror 我之镜界 服务端 作者: JularDepick
 * 方块改动同步: 联机骨架的第二条通道.
 * 内存环形表即弃, 不落库; 客户端按序号增量拉取, 落后太多时整批重拉.
 */

import type { FastifyInstance } from "fastify";
import { createAuthGuard } from "../../../fastify/auth-guard";
import type { AppContext } from "../../../fastify/context";
import { AppError } from "../../../utils/errors";
import { ok } from "../../../utils/response";
import { requireObjectBody } from "../../../utils/validate";

/** 一条方块改动: 世界坐标, 新方块 id 与来源玩家. */
interface BlockEdit {
  seq: number;
  x: number;
  y: number;
  z: number;
  block: number;
  uid: string;
}

/** 返回给客户端的改动条目, 不含来源 uid. */
export interface BlockEditSnapshot {
  seq: number;
  x: number;
  y: number;
  z: number;
  block: number;
}

/** 环形表容量: 约等于一次视距内的改动量, 超出后最老的被覆盖. */
const MAX_EDITS = 4096;
/** 单次上报的改动条数上限, 防御异常请求. */
const MAX_BATCH = 128;
/** 单次拉取的条数上限. */
const MAX_FETCH = 512;
/** 同一坐标在环形表里只保留最新一条, 避免同一格反复改动占满容量. */
const edits: BlockEdit[] = [];
const latestByCell = new Map<string, BlockEdit>();
let nextSeq = 1;

/** 世界坐标打包成键, 与客户端无关, 只用于去重. */
function cellKey(x: number, y: number, z: number): string {
  return `${x},${y},${z}`;
}

/** 追加一条改动, 覆盖同一坐标的旧条目. */
function appendEdit(x: number, y: number, z: number, block: number, uid: string): BlockEdit {
  const key = cellKey(x, y, z);
  const previous = latestByCell.get(key);
  if (previous) {
    const index = edits.indexOf(previous);
    if (index >= 0) {
      edits.splice(index, 1);
    }
  }
  const edit: BlockEdit = { seq: nextSeq, x, y, z, block, uid };
  nextSeq += 1;
  edits.push(edit);
  latestByCell.set(key, edit);
  while (edits.length > MAX_EDITS) {
    const dropped = edits.shift();
    if (dropped) {
      const droppedKey = cellKey(dropped.x, dropped.y, dropped.z);
      if (latestByCell.get(droppedKey) === dropped) {
        latestByCell.delete(droppedKey);
      }
    }
  }
  return edit;
}

/** 取序号大于 since 的改动, 按序号升序. */
function editsSince(since: number): BlockEditSnapshot[] {
  const result: BlockEditSnapshot[] = [];
  for (const edit of edits) {
    if (edit.seq <= since) {
      continue;
    }
    result.push({ seq: edit.seq, x: edit.x, y: edit.y, z: edit.z, block: edit.block });
    if (result.length >= MAX_FETCH) {
      break;
    }
  }
  result.sort((left, right) => left.seq - right.seq);
  return result;
}

/** 当前最大序号, 客户端首次同步时用来对齐基线. */
export function currentBlockSeq(): number {
  return nextSeq - 1;
}

export function registerBlockSyncRoutes(app: FastifyInstance, ctx: AppContext): void {
  const guard = createAuthGuard(ctx);
  void ctx;

  app.post("/api/v1/blocks", { preHandler: guard }, async (request) => {
    const auth = request.auth;
    if (!auth) {
      throw new AppError("AUTH_UNAUTHORIZED");
    }
    const body = requireObjectBody(request.body);
    const list = body.edits;
    if (!Array.isArray(list)) {
      throw new AppError("BAD_REQUEST", "改动列表缺失");
    }
    /*
     * 空列表是正常情况: 客户端每轮都会调用一次, 没有本地改动时只拉取不推.
     * 这里直接返回当前序号, 不当作错误.
     */
    if (list.length === 0) {
      return ok({ accepted: 0, seq: currentBlockSeq() });
    }
    if (list.length > MAX_BATCH) {
      throw new AppError("BAD_REQUEST", "单次改动过多");
    }
    let accepted = 0;
    for (const raw of list) {
      if (typeof raw !== "object" || raw === null) {
        continue;
      }
      const item = raw as Record<string, unknown>;
      const x = item.x;
      const y = item.y;
      const z = item.z;
      const block = item.block;
      if (
        typeof x !== "number" || !Number.isFinite(x) ||
        typeof y !== "number" || !Number.isFinite(y) ||
        typeof z !== "number" || !Number.isFinite(z) ||
        typeof block !== "number" || !Number.isFinite(block)
      ) {
        continue;
      }
      appendEdit(Math.trunc(x), Math.trunc(y), Math.trunc(z), Math.trunc(block), auth.user.uid);
      accepted += 1;
    }
    return ok({ accepted, seq: currentBlockSeq() });
  });

  app.get("/api/v1/blocks", { preHandler: guard }, async (request) => {
    const query = request.query as Record<string, unknown>;
    const rawSince = query.since;
    const since = typeof rawSince === "string" ? Number.parseInt(rawSince, 10) : 0;
    const base = Number.isFinite(since) && since > 0 ? since : 0;
    const items = editsSince(base);
    return ok({ seq: currentBlockSeq(), edits: items });
  });
}
