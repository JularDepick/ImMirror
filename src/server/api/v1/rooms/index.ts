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
  /*
   * 生物快照: 房主每轮上报的活体列表, 服务端只保存最近一轮并原样转发.
   * 不落库也不做插值, 客户端按上报节拍自行平滑.
   */
  creatures: RoomCreature[];
  /* 生物快照版本号, 每次房主上报自增; 客人据此判断是否需要重绘. */
  creatureSeq: number;
  /* 地面掉落物: 房主每轮上报当前地上的掉落物, 客人只做展示. */
  drops: RoomDrop[];
  /* 储物箱内容: 房主上行, 客人只读展示; 客人开箱看到的是房主的箱子. */
  chests: RoomChest[];
  /* 熔炉内容: 与箱子同口径, 只同步三个格, 燃烧计时仍由房主自己推进. */
  furnaces: RoomFurnace[];
  /* 内容版本: 生物, 掉落物, 储物箱与熔炉任一变化时递增, 客人据此判断是否需要整份内容. */
  contentSeq: number;
  /* 世界时间总秒数: 房主上行, 客人据此对齐昼夜, 0 表示尚未上报. */
  timeOfDay: number;
  /* 天气编号与强度: 房主上行, 客人据此对齐降水与天色. */
  weatherKind: number;
  weatherIntensity: number;
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

/** 房间内一只生物的状态快照, 由房主上报, 服务端只做转发. */
interface RoomCreature {
  id: number;
  kind: number;
  x: number;
  y: number;
  z: number;
  yaw: number;
  hp: number;
}

/** 房间内一个地面掉落物的快照, 由房主上报, 服务端只做转发. */
interface RoomDrop {
  id: number;
  block: number;
  x: number;
  y: number;
  z: number;
}

/** 房间内一个储物箱的内容快照: 位置键 + 非空格列表. */
interface RoomChest {
  key: string;
  slots: { i: number; c: number }[];
}

/** 房间内一个熔炉的内容快照: 位置键 + 三个格 (0 输入 1 燃料 2 产出). */
interface RoomFurnace {
  key: string;
  slots: { i: number; item: number; c: number }[];
}

/** 对外生物快照. */
export interface RoomCreatureSnapshot {
  id: number;
  kind: number;
  x: number;
  y: number;
  z: number;
  yaw: number;
  hp: number;
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
/** 单轮上报的生物快照条数上限. */
const MAX_CREATURES = 64;
/** 单轮上报的地面掉落物条数上限. */
const MAX_DROPS = 64;
/** 单轮上报的储物箱数量上限与单箱格数上限. */
const MAX_CHESTS = 8;
const MAX_CHEST_SLOTS = 64;
/** 单轮上报的熔炉数量上限. */
const MAX_FURNACES = 8;
/** 房间人数下限与上限. */
const MIN_PLAYERS = 2;
const MAX_PLAYERS = 16;
/** 房间名长度上限. */
const NAME_MAX = 24;
/** 每人同时拥有的房间数上限. */
const OWNED_ROOM_LIMIT = 3;
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
    /* 每人同时拥有的房间数上限: 防止刷房占满内存, 也避免把自己挤出房间列表. */
    let owned = 0;
    for (const candidate of rooms.values()) {
      if (candidate.ownerUserId === auth.user.id) {
        owned += 1;
      }
    }
    if (owned >= OWNED_ROOM_LIMIT) {
      throw new AppError("ROOM_FORBIDDEN", `每人最多同时拥有 ${OWNED_ROOM_LIMIT} 个房间`);
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
      creatures: [],
      creatureSeq: 0,
      drops: [],
      chests: [],
      furnaces: [],
      contentSeq: 1,
      timeOfDay: 0,
      weatherKind: 0,
      weatherIntensity: 0,
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

  /** 改房间信息: 只有房主可以改房间名与人数上限, 人数不得少于当前在线人数. */
  app.post("/api/v1/rooms/:id/update", { preHandler: guard }, async (request) => {
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
    requireMember(room, auth.user.id);
    if (room.ownerUserId !== auth.user.id) {
      throw new AppError("ROOM_FORBIDDEN", "只有房主可以修改房间信息");
    }
    const rawName = body.name;
    if (rawName !== undefined) {
      if (typeof rawName !== "string" || rawName.trim() === "") {
        throw new AppError("BAD_REQUEST", "房间名不合法");
      }
      const name = rawName.trim();
      if (name.length > NAME_MAX) {
        throw new AppError("BAD_REQUEST", `房间名不得超过 ${NAME_MAX} 个字符`);
      }
      room.name = name;
    }
    const rawPlayers = body.maxPlayers;
    if (rawPlayers !== undefined) {
      if (typeof rawPlayers !== "number" || !Number.isFinite(rawPlayers)) {
        throw new AppError("BAD_REQUEST", "人数上限必须为数值");
      }
      const maxPlayers = Math.trunc(rawPlayers);
      if (maxPlayers < MIN_PLAYERS || maxPlayers > MAX_PLAYERS) {
        throw new AppError("BAD_REQUEST", `房间人数必须在 ${MIN_PLAYERS} 到 ${MAX_PLAYERS} 之间`);
      }
      if (maxPlayers < room.members.size) {
        throw new AppError("BAD_REQUEST", "人数上限不得少于当前在线人数");
      }
      room.maxPlayers = maxPlayers;
    }
    room.updatedAt = now;
    return ok({ room: summarize(room) });
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

    /*
     * 生物快照: 只有房主可以上报, 服务端整轮替换并自增版本号;
     * 客人的上报按拒绝计数, 与方块改动同一口径.
     */
    let rejectedCreatures = 0;
    const rawCreatures = body.creatures;
    if (rawCreatures !== undefined) {
      if (!Array.isArray(rawCreatures)) {
        throw new AppError("BAD_REQUEST", "生物列表必须为数组");
      }
      if (rawCreatures.length > MAX_CREATURES) {
        throw new AppError("BAD_REQUEST", "单次生物快照过多");
      }
      if (room.ownerUserId !== auth.user.id) {
        rejectedCreatures = rawCreatures.length;
      } else {
        const next: RoomCreature[] = [];
        for (const raw of rawCreatures) {
          if (typeof raw !== "object" || raw === null) {
            continue;
          }
          const item = raw as Record<string, unknown>;
          const values = ["id", "kind", "x", "y", "z", "yaw", "hp"].map((key) => item[key]);
          if (values.some((value) => typeof value !== "number" || !Number.isFinite(value))) {
            continue;
          }
          next.push({
            id: Math.trunc(values[0] as number),
            kind: Math.trunc(values[1] as number),
            x: values[2] as number,
            y: values[3] as number,
            z: values[4] as number,
            yaw: values[5] as number,
            hp: values[6] as number,
          });
        }
        room.creatures = next;
        room.creatureSeq += 1;
        room.contentSeq += 1;
      }
    }

    /* 地面掉落物: 与生物快照同一套房主权威口径, 客人上报按拒绝计数. */
    let rejectedDrops = 0;
    const rawDrops = body.drops;
    if (rawDrops !== undefined) {
      if (!Array.isArray(rawDrops)) {
        throw new AppError("BAD_REQUEST", "掉落物列表必须为数组");
      }
      if (rawDrops.length > MAX_DROPS) {
        throw new AppError("BAD_REQUEST", "单次掉落物快照过多");
      }
      if (room.ownerUserId !== auth.user.id) {
        rejectedDrops = rawDrops.length;
      } else {
        const next: RoomDrop[] = [];
        for (const raw of rawDrops) {
          if (typeof raw !== "object" || raw === null) {
            continue;
          }
          const item = raw as Record<string, unknown>;
          const values = ["id", "block", "x", "y", "z"].map((key) => item[key]);
          if (values.some((value) => typeof value !== "number" || !Number.isFinite(value))) {
            continue;
          }
          next.push({
            id: Math.trunc(values[0] as number),
            block: Math.trunc(values[1] as number),
            x: values[2] as number,
            y: values[3] as number,
            z: values[4] as number,
          });
        }
        room.drops = next;
        room.contentSeq += 1;
      }
    }

    /* 储物箱内容: 房主上行整轮替换, 客人上报按拒绝计数. */
    let rejectedChests = 0;
    const rawChests = body.chests;
    if (rawChests !== undefined) {
      if (!Array.isArray(rawChests)) {
        throw new AppError("BAD_REQUEST", "储物箱列表必须为数组");
      }
      if (rawChests.length > MAX_CHESTS) {
        throw new AppError("BAD_REQUEST", "单次储物箱快照过多");
      }
      if (room.ownerUserId !== auth.user.id) {
        rejectedChests = rawChests.length;
      } else {
        const next: RoomChest[] = [];
        for (const raw of rawChests) {
          if (typeof raw !== "object" || raw === null) {
            continue;
          }
          const item = raw as Record<string, unknown>;
          const key = typeof item.key === "string" ? item.key : "";
          if (key === "" || !Array.isArray(item.slots)) {
            continue;
          }
          const slots: { i: number; c: number }[] = [];
          for (const rawSlot of item.slots.slice(0, MAX_CHEST_SLOTS)) {
            if (typeof rawSlot !== "object" || rawSlot === null) {
              continue;
            }
            const slot = rawSlot as Record<string, unknown>;
            const index = slot.i;
            const count = slot.c;
            if (typeof index !== "number" || !Number.isFinite(index) ||
                typeof count !== "number" || !Number.isFinite(count) || count <= 0) {
              continue;
            }
            slots.push({ i: Math.trunc(index), c: Math.trunc(count) });
          }
          next.push({ key, slots });
        }
        room.chests = next;
        room.contentSeq += 1;
      }
    }

    /* 熔炉内容: 与箱子同一套房主权威口径, 只同步三个格. */
    let rejectedFurnaces = 0;
    const rawFurnaces = body.furnaces;
    if (rawFurnaces !== undefined) {
      if (!Array.isArray(rawFurnaces)) {
        throw new AppError("BAD_REQUEST", "熔炉列表必须为数组");
      }
      if (rawFurnaces.length > MAX_FURNACES) {
        throw new AppError("BAD_REQUEST", "单次熔炉快照过多");
      }
      if (room.ownerUserId !== auth.user.id) {
        rejectedFurnaces = rawFurnaces.length;
      } else {
        const next: RoomFurnace[] = [];
        for (const raw of rawFurnaces) {
          if (typeof raw !== "object" || raw === null) {
            continue;
          }
          const item = raw as Record<string, unknown>;
          const key = typeof item.key === "string" ? item.key : "";
          if (key === "" || !Array.isArray(item.slots)) {
            continue;
          }
          const slots: { i: number; item: number; c: number }[] = [];
          for (const rawSlot of item.slots.slice(0, 3)) {
            if (typeof rawSlot !== "object" || rawSlot === null) {
              continue;
            }
            const slot = rawSlot as Record<string, unknown>;
            const index = slot.i;
            const itemId = slot.item;
            const count = slot.c;
            if (typeof index !== "number" || !Number.isFinite(index) ||
                typeof itemId !== "number" || !Number.isFinite(itemId) ||
                typeof count !== "number" || !Number.isFinite(count) || count <= 0) {
              continue;
            }
            slots.push({ i: Math.trunc(index), item: Math.trunc(itemId), c: Math.trunc(count) });
          }
          next.push({ key, slots });
        }
        room.furnaces = next;
        room.contentSeq += 1;
      }
    }

    /* 世界时间: 只有房主可以设定, 客人上报忽略. */
    const rawTime = body.timeOfDay;
    if (typeof rawTime === "number" && Number.isFinite(rawTime) && rawTime >= 0 &&
        room.ownerUserId === auth.user.id) {
      room.timeOfDay = rawTime;
    }

    /* 天气: 与时间同一条路, 只有房主的设定生效. */
    const rawWeatherKind = body.weatherKind;
    const rawWeatherIntensity = body.weatherIntensity;
    if (room.ownerUserId === auth.user.id &&
        typeof rawWeatherKind === "number" && Number.isFinite(rawWeatherKind) &&
        rawWeatherKind >= 0 && rawWeatherKind <= 3 &&
        typeof rawWeatherIntensity === "number" && Number.isFinite(rawWeatherIntensity)) {
      room.weatherKind = Math.trunc(rawWeatherKind);
      room.weatherIntensity = Math.max(0, Math.min(1, rawWeatherIntensity));
    }

    const rawSince = body.since;
    const since = typeof rawSince === "number" && Number.isFinite(rawSince) ? Math.trunc(rawSince) : 0;
    /* 内容增量: 与读取接口同一口径, 版本没变就不重复下发四份数组. */
    const rawContent = body.content;
    const contentBase =
      typeof rawContent === "number" && Number.isFinite(rawContent) ? Math.trunc(rawContent) : 0;
    const contentChanged = contentBase !== room.contentSeq;
    return ok({
      seq: room.nextSeq - 1,
      players: othersOf(room, auth.user.id),
      edits: editsSince(room, since > 0 ? since : 0),
      contentSeq: room.contentSeq,
      contentChanged,
      creatures: contentChanged ? room.creatures : undefined,
      creatureSeq: room.creatureSeq,
      drops: contentChanged ? room.drops : undefined,
      chests: contentChanged ? room.chests : undefined,
      furnaces: contentChanged ? room.furnaces : undefined,
      timeOfDay: room.timeOfDay,
      weatherKind: room.weatherKind,
      weatherIntensity: room.weatherIntensity,
      rejectedEdits,
      rejectedCreatures,
      rejectedDrops,
      rejectedChests,
      rejectedFurnaces,
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
    /*
     * 内容按版本号增量下发: 客人带上上次看到的内容版本, 版本没变就不重复传
     * 生物, 掉落物, 储物箱与熔炉四份数组, 只回一个内容版本号.
     */
    const rawContent = query.content;
    const contentSince =
      typeof rawContent === "string" ? Number.parseInt(rawContent, 10) : 0;
    const contentBase = Number.isFinite(contentSince) && contentSince > 0 ? contentSince : 0;
    const contentChanged = contentBase !== room.contentSeq;
    return ok({
      seq: room.nextSeq - 1,
      players: othersOf(room, auth.user.id),
      edits: editsSince(room, base),
      contentSeq: room.contentSeq,
      contentChanged,
      creatures: contentChanged ? room.creatures : undefined,
      creatureSeq: room.creatureSeq,
      drops: contentChanged ? room.drops : undefined,
      chests: contentChanged ? room.chests : undefined,
      furnaces: contentChanged ? room.furnaces : undefined,
      timeOfDay: room.timeOfDay,
      weatherKind: room.weatherKind,
      weatherIntensity: room.weatherIntensity,
      serverTime: now,
    });
  });
}
