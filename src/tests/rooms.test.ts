/**
 * 房间通道测试
 * 覆盖鉴权, 建房与列表, 按房间号加入, 人数上限, 房主权威的改动同步,
 * 增量拉取, 房主移交与掉线回收
 */

import { afterEach, beforeEach, describe, expect, it } from "vitest";
import { createTestApp, expectFail, expectOk, registerUser } from "./helpers";
import type { TestApp } from "./helpers";
import { resetRoomsForTest } from "../server/api/v1/rooms";

let app: TestApp;
let caseSeq = 0;

interface RoomSummary {
  id: string;
  code: string;
  name: string;
  ownerUid: string;
  maxPlayers: number;
  memberCount: number;
  seed: number;
  worldName: string;
  createdAt: number;
  hasPassword: boolean;
}

interface RoomMemberSnapshot {
  uid: string;
  x: number;
  y: number;
  z: number;
  yaw: number;
  pitch: number;
  owner: boolean;
}

interface RoomEditSnapshot {
  seq: number;
  x: number;
  y: number;
  z: number;
  block: number;
}

beforeEach(async () => {
  /* 房间表是进程单例, 先清空避免上一用例的房间渗入本用例. */
  resetRoomsForTest();
  app = await createTestApp();
  caseSeq += 1;
});

afterEach(async () => {
  await app.close();
  resetRoomsForTest();
});

/** 每个用例用独立邮箱注册, 避免不同应用实例间的邮箱唯一冲突. */
function registerCasePlayer(tag: string, index: number) {
  return registerUser(app, { email: `r${caseSeq}-${tag}${index}@immirror.test` });
}

/** 建房并返回房间摘要. */
async function createRoom(token: string, payload: Record<string, unknown> = {}): Promise<RoomSummary> {
  const response = await app.post("/api/v1/rooms", payload, token);
  const data = expectOk<{ room: RoomSummary }>(response);
  return data.room;
}

describe("房间通道", () => {
  it("未登录建房返回 AUTH_UNAUTHORIZED", async () => {
    const response = await app.post("/api/v1/rooms", {});
    expect(response.status).toBe(401);
    expectFail(response, "AUTH_UNAUTHORIZED");
  });

  it("建房返回房间号与房主, 房间出现在列表里", async () => {
    const owner = await registerCasePlayer("create", 1);
    const room = await createRoom(owner.token, { name: "测试房间", maxPlayers: 4 });
    expect(room.code).toHaveLength(6);
    expect(room.name).toBe("测试房间");
    expect(room.ownerUid).toBe(owner.uid);
    expect(room.memberCount).toBe(1);
    expect(room.maxPlayers).toBe(4);

    const listed = expectOk<{ rooms: RoomSummary[] }>(await app.get("/api/v1/rooms", owner.token));
    expect(listed.rooms).toHaveLength(1);
    expect(listed.rooms[0]?.id).toBe(room.id);
  });

  it("房间名缺省时用房间号补位, 人数越界返回 BAD_REQUEST", async () => {
    const owner = await registerCasePlayer("default", 1);
    const room = await createRoom(owner.token);
    expect(room.name).toContain(room.code);

    const tooMany = await app.post("/api/v1/rooms", { maxPlayers: 99 }, owner.token);
    expect(tooMany.status).toBe(400);
    expectFail(tooMany, "BAD_REQUEST");
    const tooFew = await app.post("/api/v1/rooms", { maxPlayers: 1 }, owner.token);
    expect(tooFew.status).toBe(400);
  });

  it("按房间号加入后双方互见位置", async () => {
    const owner = await registerCasePlayer("join", 1);
    const guest = await registerCasePlayer("join", 2);
    const room = await createRoom(owner.token);

    const joined = expectOk<{ room: RoomSummary; players: RoomMemberSnapshot[] }>(
      await app.post("/api/v1/rooms/join", { code: room.code.toLowerCase() }, guest.token),
    );
    expect(joined.room.memberCount).toBe(2);
    expect(joined.players).toHaveLength(1);

    await app.post(`/api/v1/rooms/${room.id}/state`, { x: 8, y: 83, z: 4, yaw: 1.2 }, owner.token);
    const seen = expectOk<{ players: RoomMemberSnapshot[] }>(
      await app.post(`/api/v1/rooms/${room.id}/state`, { x: 9, y: 83, z: 5 }, guest.token),
    );
    expect(seen.players).toHaveLength(1);
    const host = seen.players[0];
    expect(host?.uid).toBe(owner.uid);
    expect(host?.x).toBe(8);
    expect(host?.owner).toBe(true);
  });

  it("人数已满时第三人加入返回 ROOM_FULL", async () => {
    const owner = await registerCasePlayer("full", 1);
    const guest = await registerCasePlayer("full", 2);
    const third = await registerCasePlayer("full", 3);
    const room = await createRoom(owner.token, { maxPlayers: 2 });
    await app.post("/api/v1/rooms/join", { code: room.code }, guest.token);

    const rejected = await app.post("/api/v1/rooms/join", { code: room.code }, third.token);
    expect(rejected.status).toBe(409);
    expectFail(rejected, "ROOM_FULL");
  });

  it("房主上报的方块改动生效, 客人上报的被拒绝", async () => {
    const owner = await registerCasePlayer("authority", 1);
    const guest = await registerCasePlayer("authority", 2);
    const room = await createRoom(owner.token);
    await app.post("/api/v1/rooms/join", { code: room.code }, guest.token);

    const guestReport = expectOk<{ rejectedEdits: number; edits: RoomEditSnapshot[] }>(
      await app.post(
        `/api/v1/rooms/${room.id}/state`,
        { edits: [{ x: 1, y: 2, z: 3, block: 5 }] },
        guest.token,
      ),
    );
    expect(guestReport.rejectedEdits).toBe(1);
    expect(guestReport.edits).toEqual([]);

    const ownerReport = expectOk<{ rejectedEdits: number }>(
      await app.post(
        `/api/v1/rooms/${room.id}/state`,
        { edits: [{ x: 8, y: 83, z: 6, block: 3 }] },
        owner.token,
      ),
    );
    expect(ownerReport.rejectedEdits).toBe(0);

    const guestView = expectOk<{ edits: RoomEditSnapshot[] }>(
      await app.get(`/api/v1/rooms/${room.id}/state`, guest.token),
    );
    expect(guestView.edits).toHaveLength(1);
    expect(guestView.edits[0]?.block).toBe(3);
  });

  it("增量拉取按序号过滤, 同一格只保留最新改动", async () => {
    const owner = await registerCasePlayer("since", 1);
    const room = await createRoom(owner.token);
    const first = expectOk<{ seq: number }>(
      await app.post(`/api/v1/rooms/${room.id}/state`, { edits: [{ x: 1, y: 2, z: 3, block: 4 }] }, owner.token),
    );
    const second = expectOk<{ seq: number }>(
      await app.post(`/api/v1/rooms/${room.id}/state`, { edits: [{ x: 5, y: 6, z: 7, block: 8 }] }, owner.token),
    );
    expect(second.seq).toBeGreaterThan(first.seq);

    const delta = expectOk<{ edits: RoomEditSnapshot[] }>(
      await app.get(`/api/v1/rooms/${room.id}/state?since=${first.seq}`, owner.token),
    );
    expect(delta.edits).toHaveLength(1);
    expect(delta.edits[0]?.block).toBe(8);

    /* 同一坐标再次改动只保留一条, 序号前进而条数不变. */
    await app.post(`/api/v1/rooms/${room.id}/state`, { edits: [{ x: 1, y: 2, z: 3, block: 9 }] }, owner.token);
    const all = expectOk<{ edits: RoomEditSnapshot[] }>(
      await app.get(`/api/v1/rooms/${room.id}/state`, owner.token),
    );
    expect(all.edits).toHaveLength(2);
    expect(all.edits.map((edit) => edit.block).sort()).toEqual([8, 9]);
  });

  it("非成员访问房间状态返回 ROOM_FORBIDDEN, 房间号不存在返回 ROOM_NOT_FOUND", async () => {
    const owner = await registerCasePlayer("member", 1);
    const stranger = await registerCasePlayer("member", 2);
    const room = await createRoom(owner.token);

    const forbidden = await app.get(`/api/v1/rooms/${room.id}/state`, stranger.token);
    expect(forbidden.status).toBe(403);
    expectFail(forbidden, "ROOM_FORBIDDEN");

    const missing = await app.post("/api/v1/rooms/join", { code: "ZZZZZZ" }, stranger.token);
    expect(missing.status).toBe(404);
    expectFail(missing, "ROOM_NOT_FOUND");
  });

  it("房主离房后房主身份移交给最早加入的成员", async () => {
    const owner = await registerCasePlayer("handover", 1);
    const guest = await registerCasePlayer("handover", 2);
    const room = await createRoom(owner.token);
    await app.post("/api/v1/rooms/join", { code: room.code }, guest.token);

    const left = expectOk<{ left: boolean; room: RoomSummary | null }>(
      await app.post(`/api/v1/rooms/${room.id}/leave`, {}, owner.token),
    );
    expect(left.left).toBe(true);
    expect(left.room?.ownerUid).toBe(guest.uid);
    expect(left.room?.memberCount).toBe(1);
  });

  it("掉线成员被剔除, 空房间超时后关闭", async () => {
    const owner = await registerCasePlayer("ttl", 1);
    const guest = await registerCasePlayer("ttl", 2);
    const room = await createRoom(owner.token);
    await app.post("/api/v1/rooms/join", { code: room.code }, guest.token);

    /* 成员存活期 30 秒: 推进到存活期之外后查询触发剔除. */
    app.clock.advance(31 * 1000);
    const afterSweep = expectOk<{ rooms: RoomSummary[] }>(await app.get("/api/v1/rooms", guest.token));
    expect(afterSweep.rooms).toHaveLength(1);
    expect(afterSweep.rooms[0]?.memberCount).toBe(0);

    /* 空房间保留 60 秒, 超过后从列表消失. */
    app.clock.advance(61 * 1000);
    const closed = expectOk<{ rooms: RoomSummary[] }>(await app.get("/api/v1/rooms", guest.token));
    expect(closed.rooms).toEqual([]);
  });

  it("房间密码: 无密码与错密码都被拒, 正确密码可加入", async () => {
    const owner = await registerCasePlayer("pass", 1);
    const guest = await registerCasePlayer("pass", 2);
    const room = await createRoom(owner.token, { password: "room-1234" });
    expect(room.hasPassword).toBe(true);

    const missing = await app.post("/api/v1/rooms/join", { code: room.code }, guest.token);
    expect(missing.status).toBe(403);
    expectFail(missing, "ROOM_PASSWORD_REQUIRED");

    const wrong = await app.post(
      "/api/v1/rooms/join",
      { code: room.code, password: "nope" },
      guest.token,
    );
    expect(wrong.status).toBe(403);
    expectFail(wrong, "ROOM_PASSWORD_INVALID");

    const joined = expectOk<{ room: RoomSummary }>(
      await app.post("/api/v1/rooms/join", { code: room.code, password: "room-1234" }, guest.token),
    );
    expect(joined.room.memberCount).toBe(2);
  });

  it("房主上报的生物快照被转发给客人, 客人的上报被拒", async () => {
    const owner = await registerCasePlayer("creature", 1);
    const guest = await registerCasePlayer("creature", 2);
    const room = await createRoom(owner.token);
    await app.post("/api/v1/rooms/join", { code: room.code }, guest.token);

    const creatures = [
      { id: 1, kind: 3101, x: 8.5, y: 74, z: 4.5, yaw: 0.3, hp: 20 },
      { id: 2, kind: 3102, x: 12.5, y: 74, z: 9.5, yaw: -1.2, hp: 8 },
    ];
    const reported = expectOk<{ creatures: unknown[]; creatureSeq: number; rejectedCreatures: number }>(
      await app.post(`/api/v1/rooms/${room.id}/state`, { creatures }, owner.token),
    );
    expect(reported.rejectedCreatures).toBe(0);
    expect(reported.creatureSeq).toBe(1);
    expect(reported.creatures).toHaveLength(2);

    /* 客人拉取到同一批快照. */
    const seen = expectOk<{ creatures: { id: number; kind: number }[]; creatureSeq: number }>(
      await app.get(`/api/v1/rooms/${room.id}/state`, guest.token),
    );
    expect(seen.creatureSeq).toBe(1);
    expect(seen.creatures.map((item) => item.id)).toEqual([1, 2]);

    /* 客人上报被拒, 快照不变. */
    const rejected = expectOk<{ rejectedCreatures: number; creatureSeq: number }>(
      await app.post(
        `/api/v1/rooms/${room.id}/state`,
        { creatures: [{ id: 9, kind: 1, x: 0, y: 0, z: 0, yaw: 0, hp: 1 }] },
        guest.token,
      ),
    );
    expect(rejected.rejectedCreatures).toBe(1);
    expect(rejected.creatureSeq).toBe(1);

    /* 房主整轮替换: 快照条数与版本号都更新. */
    const replaced = expectOk<{ creatures: unknown[]; creatureSeq: number }>(
      await app.post(
        `/api/v1/rooms/${room.id}/state`,
        { creatures: [{ id: 3, kind: 3103, x: 1, y: 2, z: 3, yaw: 0, hp: 5 }] },
        owner.token,
      ),
    );
    expect(replaced.creatureSeq).toBe(2);
    expect(replaced.creatures).toHaveLength(1);
  });

  it("无密码房间的 hasPassword 为假", async () => {
    const owner = await registerCasePlayer("nopass", 1);
    const room = await createRoom(owner.token);
    expect(room.hasPassword).toBe(false);
  });

  it("房主可以踢人, 非房主与被踢对象受限", async () => {
    const owner = await registerCasePlayer("kick", 1);
    const guest = await registerCasePlayer("kick", 2);
    const outsider = await registerCasePlayer("kick", 3);
    const room = await createRoom(owner.token);
    await app.post("/api/v1/rooms/join", { code: room.code }, guest.token);

    /* 客人不能踢人. */
    const byGuest = await app.post(
      `/api/v1/rooms/${room.id}/kick`,
      { uid: owner.uid },
      guest.token,
    );
    expect(byGuest.status).toBe(403);
    expectFail(byGuest, "ROOM_FORBIDDEN");

    /* 房主不能踢自己. */
    const self = await app.post(
      `/api/v1/rooms/${room.id}/kick`,
      { uid: owner.uid },
      owner.token,
    );
    expect(self.status).toBe(400);

    /* 不在房间内的玩家按无权处理. */
    const stranger = await app.post(
      `/api/v1/rooms/${room.id}/kick`,
      { uid: outsider.uid },
      owner.token,
    );
    expect(stranger.status).toBe(403);

    const kicked = expectOk<{ kicked: string; room: RoomSummary }>(
      await app.post(`/api/v1/rooms/${room.id}/kick`, { uid: guest.uid }, owner.token),
    );
    expect(kicked.kicked).toBe(guest.uid);
    expect(kicked.room.memberCount).toBe(1);

    /* 被踢后不能再上报状态. */
    const after = await app.post(
      `/api/v1/rooms/${room.id}/state`,
      { x: 1, y: 2, z: 3 },
      guest.token,
    );
    expect(after.status).toBe(403);
    expectFail(after, "ROOM_FORBIDDEN");
  });
});
