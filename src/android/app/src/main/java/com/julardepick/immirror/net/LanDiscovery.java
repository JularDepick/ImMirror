package com.julardepick.immirror.net;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;

import org.json.JSONObject;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketTimeoutException;
import java.util.LinkedHashSet;
import java.util.Set;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/*
 * 局域网服务端自动发现.
 * 登录页与联机入口广播 UDP 探测包, 局域网内的服务端回应自己的 HTTP 端口,
 * 客户端收集回应后自动填充地址, 免手动输入 IP.
 * 探测协议: 固定魔数 IMMIRROR_DISCOVER; 回应为 JSON {apiVersion, httpPort, serverTime}.
 * 发现端口 61111 与服务端约定一致.
 */
public final class LanDiscovery {

    public interface Callback {
        void onFound(Set<String> addresses);

        void onFinished(int found);
    }

    private static final String MAGIC = "IMMIRROR_DISCOVER";
    private static final int DISCOVERY_PORT = 61111;
    private static final int TIMEOUT_MS = 3000;
    private static final ExecutorService EXECUTOR = Executors.newSingleThreadExecutor();
    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private final Context context;

    public LanDiscovery(Context context) {
        this.context = context.getApplicationContext();
    }

    /* 广播探测并在超时内收集回应; 回调只在主线程触发一次. */
    public void discover(final Callback callback) {
        EXECUTOR.execute(() -> {
            final Set<String> found = new LinkedHashSet<>();
            DatagramSocket socket = null;
            try {
                socket = new DatagramSocket();
                socket.setBroadcast(true);
                socket.setSoTimeout(TIMEOUT_MS);
                final byte[] magic = MAGIC.getBytes("UTF-8");
                final InetAddress broadcast = InetAddress.getByName("255.255.255.255");
                socket.send(new DatagramPacket(magic, magic.length, broadcast, DISCOVERY_PORT));
                final byte[] buffer = new byte[1024];
                final long deadline = System.currentTimeMillis() + TIMEOUT_MS;
                while (System.currentTimeMillis() < deadline) {
                    final DatagramPacket packet = new DatagramPacket(buffer, buffer.length);
                    try {
                        socket.setSoTimeout((int) Math.max(1, deadline - System.currentTimeMillis()));
                        socket.receive(packet);
                        final String reply = new String(packet.getData(), 0, packet.getLength(), "UTF-8");
                        final JSONObject json = new JSONObject(reply);
                        final int httpPort = json.optInt("httpPort", 61110);
                        final String address = packet.getAddress().getHostAddress() + ":" + httpPort;
                        if (found.add(address)) {
                            postFound(callback, found);
                        }
                    } catch (SocketTimeoutException error) {
                        break;
                    } catch (RuntimeException error) {
                        break;
                    }
                }
            } catch (Exception error) {
                // 网络不可用或权限缺失时静默结束, 界面回落到手动输入
            } finally {
                if (socket != null && !socket.isClosed()) {
                    socket.close();
                }
            }
            final int count = found.size();
            mainHandler.post(() -> callback.onFinished(count));
        });
    }

    private void postFound(final Callback callback, final Set<String> found) {
        mainHandler.post(() -> callback.onFound(new LinkedHashSet<>(found)));
    }
}
