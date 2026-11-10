/**
 * Fastify 类型扩展
 * 鉴权通过后把用户与会话挂到请求对象上, 路由处理函数直接读取
 */

import type { AuthContext } from "../services/session";

declare module "fastify" {
  interface FastifyRequest {
    auth: AuthContext | null;
    authToken: string | null;
  }
}
