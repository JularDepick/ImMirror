/**
 * ImMirror 我之镜界 服务端 作者: JularDepick
 * 房间与房间内实时状态: 联机骨架的第三条通道.
 * 房间与成员都放内存, 重启即清空; 方块改动按房间隔离, 只有房主上报的改动生效.
 * 房主权威: 客人只上报自身位置与操作意图, 世界状态由房主上行, 服务端只做转发.
 */

import { randomInt } from "node:crypto";
import { sha256Hex } from "../../../utils/random";
import type { FastifyInstance } from "fastify";
import { createAuthGuard } from "../../../fastify/auth-guard";
import type { AppContext } from "../../../fastify/context";
import { AppError } from "../../../utils/errors";
import { ok } from "../../../utils/response";
import { requireObjectBody } from "../../../utils/validate";

/** 房间成员的最新位置与朝向. */
interface RoomMember {
  userId: number;
  uid: string;
  x: number;
  y: number;
  z: number;
  yaw: number;
  pitch: number;
  joinedAt: number;
  updatedAt: number;
}

/** 房间内的一条方块改动, 序号在房间内单调递增. */
interface RoomEdit {
  seq: number;
  x: number;
  y: number;
  z: number;
  block: number;
  uid: string;
}

/** 房间本体: 元信息, 成员表与改动环形表. */
interface Room {
  id: string;
  code: string;
  name: string;
  ownerUserId: number;
  ownerUid: string;
  maxPlayers: number;
  seed: number;
  worldName: string;
  createdAt: number;
  updatedAt: number;
  nextSeq: number;
  edits: RoomEdit[];
  latestByCell: Map<string, RoomEdit>;
  members: Map<number, RoomMember>;
  /* 房间密码摘要; 空串表示无密码. 只存摘要, 与账号密码同口径. */
  passwordHash: string;
}

/** 对外房间摘要, 不含成员位置. */
export interface RoomSummary {
  id: string;
  code: string;
  name: string;
  ownerUid: string;
  maxPlayers: number;
  memberCount: number;
  seed: number;
  worldName: string;
  createdAt: number;
  /* 是否有房间密码, 列表里据此显示锁标记; 密码本身不外发. */
  hasPassword: boolean;
}

/** 对外成员快照, owner 标记房主身份. */
export interface RoomMemberSnapshot {
  uid: string;
  x: number;
  y: number;
  z: number;
  yaw: number;
  pitch: number;
  owner: boolean;
}

/** 对外改动快照, 不含来源 uid. */
export interface RoomEditSnapshot {
  seq: number;
  x: number;
  y: number;
  z: number;
  block: number;
}

/** 成员存活期: 超过即视为掉线并从房间移除. */
const MEMBER_TTL_MS = 30 * 1000;
/** 空房间保留期: 最后一人离开后这段时间内可重新加入. */
const EMPTY_ROOM_TTL_MS = 60 * 1000;
/** 房间总数上限, 防御异常流量撑爆进程内存. */
const MAX_ROOMS = 64;
/** 单个房间的改动环形表容量. */
const MAX_EDITS = 4096;
/** 单次上报的改动条数上限. */
const MAX_BATCH = 128;
/** 单次拉取的改动条数上限. */
const MAX_FETCH = 512;
/** 房间人数下限与上限. */
const MIN_PLAYERS = 2;
const MAX_PLAYERS = 16;
/** 房间名长度上限. */
const NAME_MAX = 24;
/** 房间密码长度上限. */
const PASSWORD_MAX = 16;
/** 房间号位数与字符集: 去掉 0/O 与 1/I 等易混字符, 便于口头分享. */
const CODE_LENGTH = 6;
const CODE_ALPHABET = "23456789ABCDEFGHJKLMNPQRSTUVWXYZ";

/** 进程级房间表: 键为房间 id, 值为房间本体. */
const rooms = new Map<string, Room>();
/** 房间号到房间 id 的索引, 用于按房间号加入. */
const roomsByCode = new Map<string, string>();

/** 世界坐标打包成键, 与客户端无关, 只用于去重. */
function cellKey(x: number, y: number, z: number): string {
  return `${x},${y},${z}`;
}

/** 生成一个未被占用的房间号. */
function allocateCode(): string {
  for (let attempt = 0; attempt < 32; attempt += 1) {
    let code = "";
    while (code.length < CODE_LENGTH) {
      code += CODE_ALPHABET.charAt(randomInt(0, CODE_ALPHABET.length));
    }
    if (!roomsByCode.has(code)) {
      return code;
    }
  }
  throw new AppError("INTERNAL", "房间号分配失败");
}

/** 生成房间 id: 十位十六进制, 与房间号解耦, 房间号重排不影响 id. */
function allocateRoomId(): string {
  for (let attempt = 0; attempt < 32; attempt += 1) {
    const id = randomInt(0x10000000, 0x7fffffff).toString(16);
    if (!rooms.has(id)) {
      return id;
    }
  }
  throw new AppError("INTERNAL", "房间 id 分配失败");
}

/** 追加一条改动, 覆盖同一坐标的旧条目. */
function appendEdit(room: Room, x: number, y: number, z: number, block: number, uid: string): void {
  const key = cellKey(x, y, z);
  const previous = room.latestByCell.get(key);
  if (previous) {
    const index = room.edits.indexOf(previous);
    if (index >= 0) {
      room.edits.splice(index, 1);
    }
  }
  const edit: RoomEdit = { seq: room.nextSeq, x, y, z, block, uid };
  room.nextSeq += 1;
  room.edits.push(edit);
  room.latestByCell.set(key, edit);
  while (room.edits.length > MAX_EDITS) {
    const dropped = room.edits.shift();
    if (dropped) {
      const droppedKey = cellKey(dropped.x, dropped.y, dropped.z);
      if (room.latestByCell.get(droppedKey) === dropped) {
        room.latestByCell.delete(droppedKey);
      }
    }
  }
}

/** 取序号大于 since 的改动, 按序号升序. */
function editsSince(room: Room, since: number): RoomEditSnapshot[] {
  const result: RoomEditSnapshot[] = [];
  for (const edit of room.edits) {
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

/** 移除掉线成员, 移交房主, 回收空房间. */
function sweepRooms(now: number): void {
  for (const [id, room] of rooms) {
    for (const [userId, member] of room.members) {
      if (now - member.updatedAt > MEMBER_TTL_MS) {
        room.members.delete(userId);
        room.updatedAt = now;
      }
    }
    if (room.members.size > 0) {
      /* 房主掉线时把房主身份交给最早加入的在线成员, 房间不因房主退出而解散. */
      if (!room.members.has(room.ownerUserId)) {
        let successor: RoomMember | null = null;
        for (const member of room.members.values()) {
          if (successor === null || member.joinedAt < successor.joinedAt) {
            successor = member;
          }
        }
        if (successor) {
          room.ownerUserId = successor.userId;
          room.ownerUid = successor.uid;
          room.updatedAt = now;
        }
      }
      continue;
    }
    if (now - room.updatedAt > EMPTY_ROOM_TTL_MS) {
      rooms.delete(id);
      roomsByCode.delete(room.code);
    }
  }
}

/** 取房间, 不存在时抛 ROOM_NOT_FOUND. */
function requireRoom(id: string): Room {
  const room = rooms.get(id);
  if (!room) {
    throw new AppError("ROOM_NOT_FOUND");
  }
  return room;
}

/** 取成员, 不是成员时抛 ROOM_FORBIDDEN. */
function requireMember(room: Room, userId: number): RoomMember {
  const member = room.members.get(userId);
  if (!member) {
    throw new AppError("ROOM_FORBIDDEN");
  }
  return member;
}

/** 房间摘要. */
function summarize(room: Room): RoomSummary {
  return {
    id: room.id,
    code: room.code,
    name: room.name,
    ownerUid: room.ownerUid,
    maxPlayers: room.maxPlayers,
    memberCount: room.members.size,
    seed: room.seed,
    worldName: room.worldName,
    createdAt: room.createdAt,
    hasPassword: room.passwordHash !== "",
  };
}

/** 其他成员的位置快照, 不含查询者本人. */
function othersOf(room: Room, userId: number): RoomMemberSnapshot[] {
  const result: RoomMemberSnapshot[] = [];
  for (const [otherId, member] of room.members) {
    if (otherId === userId) {
      continue;
    }
    result.push({
      uid: member.uid,
      x: member.x,
      y: member.y,
      z: member.z,
      yaw: member.yaw,
      pitch: member.pitch,
      owner: otherId === room.ownerUserId,
    });
  }
  return result;
}

/** 清空房间表, 仅供测试构造隔离场景. */
export function resetRoomsForTest(): void {
  rooms.clear();
  roomsByCode.clear();
}

export function registerRoomRoutes(app: FastifyInstance, ctx: AppContext): void {
  const guard = createAuthGuard(ctx);

  /** 建房: 房主即创建者, 房间号随机分配. */
  app.post("/api/v1/rooms", { preHandler: guard }, async (request) => {
    const auth = request.auth;
    if (!auth) {
      throw new AppError("AUTH_UNAUTHORIZED");
    }
    const body = requireObjectBody(request.body);
    const now = ctx.now();
    sweepRooms(now);
    if (rooms.size >= MAX_ROOMS) {
      throw new AppError("ROOM_FULL", "房间数量已达上限");
    }
    const rawName = body.name;
    const name = typeof rawName === "string" ? rawName.trim() : "";
    if (name.length > NAME_MAX) {
      throw new AppError("BAD_REQUEST", `房间名不得超过 ${NAME_MAX} 个字符`);
    }
    const rawPlayers = body.maxPlayers;
    const maxPlayers = typeof rawPlayers === "number" && Number.isFinite(rawPlayers)
      ? Math.trunc(rawPlayers)
      : MAX_PLAYERS;
    if (maxPlayers < MIN_PLAYERS || maxPlayers > MAX_PLAYERS) {
      throw new AppError("BAD_REQUEST", `房间人数必须在 ${MIN_PLAYERS} 到 ${MAX_PLAYERS} 之间`);
    }
    const rawSeed = body.seed;
    const seed = typeof rawSeed === "number" && Number.isFinite(rawSeed)
      ? Math.trunc(rawSeed)
      : randomInt(1, 0x7fffffff);
    const rawWorld = body.worldName;
    const worldName = typeof rawWorld === "string" ? rawWorld.trim().slice(0, NAME_MAX) : "";
    const rawPassword = body.password;
    const password = typeof rawPassword === "string" ? rawPassword.trim() : "";
    if (password.length > PASSWORD_MAX) {
      throw new AppError("BAD_REQUEST", `房间密码不得超过 ${PASSWORD_MAX} 个字符`);
    }

    const room: Room = {
      id: allocateRoomId(),
      code: allocateCode(),
      name,
      ownerUserId: auth.user.id,
      ownerUid: auth.user.uid,
      maxPlayers,
      seed,
      worldName,
      createdAt: now,
      updatedAt: now,
      nextSeq: 1,
      edits: [],
      latestByCell: new Map<string, RoomEdit>(),
      members: new Map<number, RoomMember>(),
      passwordHash: password === "" ? "" : sha256Hex(password),
    };
    /* 房间名缺省时用房间号补位, 保证列表里每一项都能被区分. */
    if (name === "") {
      room.name = `房间 ${room.code}`;
    }
    room.members.set(auth.user.id, {
      userId: auth.user.id,
      uid: auth.user.uid,
      x: 0,
      y: 0,
      z: 0,
      yaw: 0,
      pitch: 0,
      joinedAt: now,
      updatedAt: now,
    });
    rooms.set(room.id, room);
    roomsByCode.set(room.code, room.id);
    return ok({ room: summarize(room), players: [] as RoomMemberSnapshot[], seq: room.nextSeq - 1 });
  });

  /** 房间列表: 只暴露摘要, 供联机大厅列表展示. */
  app.get("/api/v1/rooms", { preHandler: guard }, async () => {
    const now = ctx.now();
    sweepRooms(now);
    const list: RoomSummary[] = [];
    for (const room of rooms.values()) {
      list.push(summarize(room));
    }
    list.sort((left, right) => right.createdAt - left.createdAt);
    return ok({ rooms: list, serverTime: now });
  });

  /** 按房间号加入; 已在房间内时幂等, 只刷新存活期. */
  app.post("/api/v1/rooms/join", { preHandler: guard }, async (request) => {
    const auth = request.auth;
    if (!auth) {
      throw new AppError("AUTH_UNAUTHORIZED");
    }
    const body = requireObjectBody(request.body);
    const rawCode = body.code;
    if (typeof rawCode !== "string" || rawCode.trim() === "") {
      throw new AppError("BAD_REQUEST", "房间号缺失");
    }
    const code = rawCode.trim().toUpperCase();
    const now = ctx.now();
    sweepRooms(now);
    const roomId = roomsByCode.get(code);
    if (!roomId) {
      throw new AppError("ROOM_NOT_FOUND");
    }
    const room = requireRoom(roomId);
    /* 房间密码: 只比对摘要, 明文不进内存表也不回包. */
    if (room.passwordHash !== "") {
      const rawPassword = body.password;
      const password = typeof rawPassword === "string" ? rawPassword.trim() : "";
      if (password === "") {
        throw new AppError("ROOM_PASSWORD_REQUIRED");
      }
      if (sha256Hex(password) !== room.passwordHash) {
        throw new AppError("ROOM_PASSWORD_INVALID");
      }
    }
    const existing = room.members.get(auth.user.id);
    if (!existing && room.members.size >= room.maxPlayers) {
      throw new AppError("ROOM_FULL");
    }
    if (existing) {
      existing.updatedAt = now;
    } else {
      room.members.set(auth.user.id, {
        userId: auth.user.id,
        uid: auth.user.uid,
        x: 0,
        y: 0,
        z: 0,
        yaw: 0,
        pitch: 0,
        joinedAt: now,
        updatedAt: now,
      });
    }
    room.updatedAt = now;
    return ok({
      room: summarize(room),
      players: othersOf(room, auth.user.id),
      seq: room.nextSeq - 1,
      serverTime: now,
    });
  });

  /** 离房: 房主离开时房主身份移交给最早加入的在线成员. */
  app.post("/api/v1/rooms/:id/leave", { preHandler: guard }, async (request) => {
    const auth = request.auth;
    if (!auth) {
      throw new AppError("AUTH_UNAUTHORIZED");
    }
    const params = request.params as Record<string, unknown>;
    const id = typeof params.id === "string" ? params.id : "";
    const now = ctx.now();
    sweepRooms(now);
    const room = requireRoom(id);
    requireMember(room, auth.user.id);
    room.members.delete(auth.user.id);
    room.updatedAt = now;
    sweepRooms(now);
    return ok({ left: true, room: rooms.has(id) ? summarize(room) : null });
  });

  /** 踢人: 只有房主可以踢, 被踢成员立即移出房间, 房主自己不能被踢. */
  app.post("/api/v1/rooms/:id/kick", { preHandler: guard }, async (request) => {
    const auth = request.auth;
    if (!auth) {
      throw new AppError("AUTH_UNAUTHORIZED");
    }
    const params = request.params as Record<string, unknown>;
    const id = typeof params.id === "string" ? params.id : "";
    const body = requireObjectBody(request.body);
    const targetUid = typeof body.uid === "string" ? body.uid.trim() : "";
    if (targetUid === "") {
      throw new AppError("BAD_REQUEST", "目标玩家缺失");
    }
    const now = ctx.now();
    sweepRooms(now);
    const room = requireRoom(id);
    requireMember(room, auth.user.id);
    if (room.ownerUserId !== auth.user.id) {
      throw new AppError("ROOM_FORBIDDEN", "只有房主可以踢人");
    }
    if (targetUid === room.ownerUid) {
      throw new AppError("BAD_REQUEST", "不能踢出房主");
    }
    let removed = false;
    for (const [userId, member] of room.members) {
      if (member.uid === targetUid) {
        room.members.delete(userId);
        removed = true;
        break;
      }
    }
    if (!removed) {
      throw new AppError("ROOM_FORBIDDEN", "该玩家不在房间内");
    }
    room.updatedAt = now;
    return ok({ kicked: targetUid, room: summarize(room) });
  });

  /**
   * 上报自身状态并拉取增量: 位置人人可报, 方块改动只有房主上报的生效.
   * 客人的改动被计入 rejectedEdits, 由客户端提示无权修改.
   */
  app.post("/api/v1/rooms/:id/state", { preHandler: guard }, async (request) => {
    const auth = request.auth;
    if (!auth) {
      throw new AppError("AUTH_UNAUTHORIZED");
    }
    const params = request.params as Record<string, unknown>;
    const id = typeof params.id === "string" ? params.id : "";
    const body = requireObjectBody(request.body);
    const now = ctx.now();
    sweepRooms(now);
    const room = requireRoom(id);
    const member = requireMember(room, auth.user.id);

    const readNumber = (key: string, fallback: number): number => {
      const value = body[key];
      if (value === undefined) {
        return fallback;
      }
      if (typeof value !== "number" || !Number.isFinite(value)) {
        throw new AppError("BAD_REQUEST", "位置字段必须为数值");
      }
      return value;
    };
    member.x = readNumber("x", member.x);
    member.y = readNumber("y", member.y);
    member.z = readNumber("z", member.z);
    member.yaw = readNumber("yaw", member.yaw);
    member.pitch = readNumber("pitch", member.pitch);
    member.updatedAt = now;
    room.updatedAt = now;

    let rejectedEdits = 0;
    const rawEdits = body.edits;
    if (rawEdits !== undefined) {
      if (!Array.isArray(rawEdits)) {
        throw new AppError("BAD_REQUEST", "改动列表必须为数组");
      }
      if (rawEdits.length > MAX_BATCH) {
        throw new AppError("BAD_REQUEST", "单次改动过多");
      }
      const isOwner = room.ownerUserId === auth.user.id;
      for (const raw of rawEdits) {
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
        if (!isOwner) {
          rejectedEdits += 1;
          continue;
        }
        appendEdit(room, Math.trunc(x), Math.trunc(y), Math.trunc(z), Math.trunc(block), auth.user.uid);
      }
    }

    const rawSince = body.since;
    const since = typeof rawSince === "number" && Number.isFinite(rawSince) ? Math.trunc(rawSince) : 0;
    return ok({
      seq: room.nextSeq - 1,
      players: othersOf(room, auth.user.id),
      edits: editsSince(room, since > 0 ? since : 0),
      rejectedEdits,
      serverTime: now,
    });
  });

  /** 只读拉取: 不刷新自身存活期, 供大厅预览使用. */
  app.get("/api/v1/rooms/:id/state", { preHandler: guard }, async (request) => {
    const auth = request.auth;
    if (!auth) {
      throw new AppError("AUTH_UNAUTHORIZED");
    }
    const params = request.params as Record<string, unknown>;
    const id = typeof params.id === "string" ? params.id : "";
    const now = ctx.now();
    sweepRooms(now);
    const room = requireRoom(id);
    requireMember(room, auth.user.id);
    const query = request.query as Record<string, unknown>;
    const rawSince = query.since;
    const since = typeof rawSince === "string" ? Number.parseInt(rawSince, 10) : 0;
    const base = Number.isFinite(since) && since > 0 ? since : 0;
    return ok({
      seq: room.nextSeq - 1,
      players: othersOf(room, auth.user.id),
      edits: editsSince(room, base),
      serverTime: now,
    });
  });
}
