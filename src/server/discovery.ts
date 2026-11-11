/**
 * ImMirror 我之镜界 服务端 作者: JularDepick
 * 局域网自动发现: 服务端监听 UDP 广播, 收到探测包后回应自己的 HTTP 端口与版本.
 * 客户端在登录页与联机入口广播探测, 收到回应即自动填充服务端地址, 免手动输入.
 *
 * 协议: 探测包为固定魔数 + 版本; 回应为 JSON {apiVersion, httpPort, serverTime}.
 * 端口默认 61111 (HTTP 61110), 可用环境变量 PRESENCE_DISCOVERY_PORT 覆盖.
 */
import { createSocket } from "dgram";

const MAGIC = "IMMIRROR_DISCOVER";
const MAGIC_BYTES = Buffer.from(MAGIC, "utf-8");

let socket: ReturnType<typeof createSocket> | null = null;

/** 启动发现监听; 端口被占用只记日志, 不影响主服务. */
export function startDiscoveryListener(httpPort: number): void {
  const port = Number(process.env.PRESENCE_DISCOVERY_PORT ?? 61111);
  socket = createSocket({ type: "udp4", reuseAddr: true });

  socket.on("message", (buffer, remote) => {
    if (!buffer.subarray(0, MAGIC_BYTES.length).equals(MAGIC_BYTES)) {
      return;
    }
    const reply = JSON.stringify({
      apiVersion: "v1",
      httpPort,
      serverTime: Date.now(),
    });
    socket?.send(reply, remote.port, remote.address, (error) => {
      if (error) {
        console.warn("[discovery] reply failed:", error.message);
      }
    });
  });

  socket.on("error", (error) => {
    console.warn("[discovery] socket error:", error.message);
    socket?.close();
    socket = null;
  });

  socket.bind(port, () => {
    socket?.setBroadcast(true);
    console.log("[discovery] listening on udp/" + port);
  });
}

export function stopDiscoveryListener(): void {
  socket?.close();
  socket = null;
}
