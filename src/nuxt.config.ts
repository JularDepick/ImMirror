/**
 * ImMirror 服务端 Nuxt/Nitro 骨架配置
 * 本工程只提供 JSON 接口: 不产出页面, 业务路由全部由 Fastify 实例承担,
 * Nitro 只负责进程宿主与构建产物
 */

import { defineNuxtConfig } from "nuxt/config";

export default defineNuxtConfig({
  compatibilityDate: "2026-01-01",
  ssr: false,
  telemetry: false,
  devtools: { enabled: false },
  nitro: {
    /* 业务路由由 Fastify 注册, Nitro 不扫描 server/api 目录, 避免生成重复的 h3 路由 */
    ignore: ["api/**"],
    sourceMap: false,
  },
});
