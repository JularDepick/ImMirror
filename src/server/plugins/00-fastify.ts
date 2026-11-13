/**
 * Nitro 启动插件
 * 构造唯一的 Fastify 实例并等待其就绪, 关闭时释放数据库连接
 */

import { defineNitroPlugin } from "nitropack/runtime";
import { disposeAppContext, getOrCreateAppContext } from "../fastify/instance";
import { startDiscoveryListener, stopDiscoveryListener } from "../discovery";
import { findUnknownConfigKeys } from "../utils/config";

export default defineNitroPlugin(async (nitroApp) => {
  /*
   * 拼错的配置键会被静默忽略并回落到缺省值, 服务照常启动但行为与预期不同.
   * 启动时把"看起来是本项目配置但没人读"的键列出来.
   */
  const unknownKeys = findUnknownConfigKeys(process.env);
  if (unknownKeys.length > 0) {
    console.warn("[config] 未识别的配置键, 已按缺省值运行: " + unknownKeys.join(", "));
  }
  const ctx = getOrCreateAppContext();
  await ctx.fastify.ready();
  /* 局域网自动发现: UDP 监听独立于 HTTP 端口, 失败只记日志不影响主服务. */
  startDiscoveryListener(Number(process.env.NITRO_PORT ?? 61110));
  nitroApp.hooks.hook("close", async () => {
    stopDiscoveryListener();
    await disposeAppContext();
  });
});