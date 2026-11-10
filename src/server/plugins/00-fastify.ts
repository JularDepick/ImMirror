/**
 * Nitro 启动插件
 * 构造唯一的 Fastify 实例并等待其就绪, 关闭时释放数据库连接
 */

import { defineNitroPlugin } from "nitropack/runtime";
import { disposeAppContext, getOrCreateAppContext } from "../fastify/instance";

export default defineNitroPlugin(async (nitroApp) => {
  const ctx = getOrCreateAppContext();
  await ctx.fastify.ready();
  nitroApp.hooks.hook("close", async () => {
    await disposeAppContext();
  });
});
