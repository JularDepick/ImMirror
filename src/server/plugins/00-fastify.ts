/**
 * Nitro 启动插件
 * 构造唯一的 Fastify 实例并等待其就绪, 关闭时释放数据库连接
 */

import { defineNitroPlugin } from "nitropack/runtime";
import { disposeAppContext, getOrCreateAppContext } from "../fastify/instance";
import { startDiscoveryListener, stopDiscoveryListener } from "../discovery";

export default defineNitroPlugin(async (nitroApp) => {
  const ctx = getOrCreateAppContext();
  await ctx.fastify.ready();
  /* 局域网自动发现: UDP 监听独立于 HTTP 端口, 失败只记日志不影响主服务. */
  startDiscoveryListener(Number(process.env.NITRO_PORT ?? 61110));
  nitroApp.hooks.hook("close", async () => {
    stopDiscoveryListener();
    await disposeAppContext();
  });
});