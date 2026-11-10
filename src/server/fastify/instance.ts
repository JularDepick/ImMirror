/**
 * Fastify 实例注册表
 * Nitro 插件在启动时创建应用并登记, 中间件在处理请求时取用;
 * 使用 globalThis 键保存, 开发模式热重启也不会重复打开数据库
 */

import { createApp } from "./app";
import type { AppContext } from "./context";

const GLOBAL_KEY = Symbol.for("immirror.appContext");

interface GlobalStore {
  [GLOBAL_KEY]?: AppContext;
}

export function getAppContext(): AppContext | null {
  const store = globalThis as unknown as GlobalStore;
  return store[GLOBAL_KEY] ?? null;
}

export function getOrCreateAppContext(): AppContext {
  const existing = getAppContext();
  if (existing) {
    return existing;
  }
  const created = createApp();
  const store = globalThis as unknown as GlobalStore;
  store[GLOBAL_KEY] = created;
  return created;
}

export async function disposeAppContext(): Promise<void> {
  const store = globalThis as unknown as GlobalStore;
  const existing = store[GLOBAL_KEY];
  if (!existing) {
    return;
  }
  store[GLOBAL_KEY] = undefined;
  await existing.close();
}
