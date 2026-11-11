/**
 * ImMirror 我之镜界 服务端 作者: JularDepick
 * Fastify 应用组装
 * 依次完成配置读取, 数据库初始化, 领域服务构造, HTTP 层钩子与业务路由注册
 */

import Fastify from "fastify";
import type { FastifyError } from "fastify";
import { closeDatabase, openDatabase } from "../db/client";
import { registerApiRoutes } from "../api/v1";
import { AuthService } from "../services/auth";
import { createMailService } from "../services/mail";
import type { MailService } from "../services/mail";
import { FailureBackoff, WindowLimiter } from "../services/rate-limit";
import { SessionService } from "../services/session";
import { WalletService } from "../services/wallet";
import type { AppConfig } from "../utils/config";
import { loadConfig } from "../utils/config";
import { startDiscoveryListener, stopDiscoveryListener } from "../discovery";
import { AppError, isAppError } from "../utils/errors";
import { fail } from "../utils/response";
import type { AppContext, AppLimiters } from "./context";
import "./types";

const MINUTE_MS = 60 * 1000;

export interface CreateAppOptions {
  config?: AppConfig;
  // 覆盖数据库文件路径, 测试用独立临时库
  dbFile?: string;
  // 覆盖时钟, 测试用固定时间点
  now?: () => number;
  // 复用邮件服务, 测试读取内存通道中的验证码
  mail?: MailService;
}

export function createApp(options: CreateAppOptions = {}): AppContext {
  const config = options.config ?? loadConfig();
  const clock = options.now ?? (() => Date.now());
  const db = openDatabase(options.dbFile ?? config.dbFile);
  const mail = options.mail ?? createMailService(config);

  const session = new SessionService(db, config, clock);
  const wallet = new WalletService(db, config, clock);
  const limiters: AppLimiters = {
    sendByIp: new WindowLimiter(clock),
    resetByEmail: new WindowLimiter(clock),
    loginByAccount: new FailureBackoff(clock, config.loginMaxFailures),
    loginByIp: new FailureBackoff(clock, config.loginMaxFailures),
    requestByIp: new WindowLimiter(clock),
  };
  const auth = new AuthService({ db, config, mail, session, wallet, limiters, now: clock });

  const fastify = Fastify({
    logger: false,
    bodyLimit: config.bodyLimitBytes,
    trustProxy: true,
  });

  const ctx = {
    config,
    db,
    mail,
    session,
    auth,
    wallet,
    limiters,
    fastify,
    now: clock,
    close: async (): Promise<void> => {
      await fastify.close();
      closeDatabase(db);
    },
  } satisfies AppContext;

  // 只接受 application/json, 其余类型统一返回 BAD_REQUEST
  fastify.removeAllContentTypeParsers();
  fastify.addContentTypeParser("application/json", { parseAs: "string" }, (request, body, done) => {
    void request;
    const text = typeof body === "string" ? body.trim() : "";
    if (text === "") {
      done(null, {});
      return;
    }
    try {
      done(null, JSON.parse(text));
    } catch {
      done(new AppError("BAD_REQUEST", "请求体不是合法 JSON"));
    }
  });
  fastify.addContentTypeParser("*", { parseAs: "string" }, (_request, _body, done) => {
    done(new AppError("BAD_REQUEST", "请求体必须为 application/json"));
  });

  fastify.decorateRequest("auth", null);
  fastify.decorateRequest("authToken", null);

  // 单 IP 每分钟请求上限
  fastify.addHook("onRequest", async (request) => {
    const ip = request.ip;
    const limit = ctx.limiters.requestByIp.consume(ip, config.requestMinuteIpLimit, MINUTE_MS);
    if (!limit.allowed) {
      throw new AppError("RATE_LIMITED");
    }
  });

  fastify.setNotFoundHandler((_request, reply) => {
    reply.code(404).type("application/json; charset=utf-8").send(fail("NOT_FOUND", "接口不存在"));
  });

  fastify.setErrorHandler((error: FastifyError, request, reply) => {
    if (isAppError(error)) {
      reply
        .code(error.status)
        .type("application/json; charset=utf-8")
        .send(fail(error.code, error.message));
      return;
    }
    const statusCode = typeof error.statusCode === "number" ? error.statusCode : 500;
    if (statusCode < 500) {
      reply
        .code(statusCode)
        .type("application/json; charset=utf-8")
        .send(fail("BAD_REQUEST", error.message));
      return;
    }
    // 未捕获异常只记录摘要, 不输出请求体与令牌
    console.error(`[immirror] unhandled error on ${request.method} ${request.url}: ${error.message}`);
    reply.code(500).type("application/json; charset=utf-8").send(fail("INTERNAL", "服务内部错误"));
  });

  registerApiRoutes(fastify, ctx);

  return ctx;
}