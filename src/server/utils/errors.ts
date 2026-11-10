/**
 * 统一错误模型
 * 业务代码只抛 AppError, 由 HTTP 层翻译为响应包络;
 * 错误码与 HTTP 状态的映射见规格 5.3
 */

export interface ErrorDefinition {
  code: string;
  status: number;
  message: string;
}

export const ERROR_DEFINITIONS = {
  BAD_REQUEST: { code: "BAD_REQUEST", status: 400, message: "请求参数不合法" },
  AUTH_INVALID_IDENTIFIER: { code: "AUTH_INVALID_IDENTIFIER", status: 400, message: "登录标识符无法解析" },
  AUTH_INVALID_CREDENTIALS: { code: "AUTH_INVALID_CREDENTIALS", status: 401, message: "账号或密码错误" },
  AUTH_CODE_INVALID: { code: "AUTH_CODE_INVALID", status: 400, message: "验证码错误或已失效" },
  AUTH_CODE_EXPIRED: { code: "AUTH_CODE_EXPIRED", status: 400, message: "验证码已过期" },
  AUTH_CODE_ATTEMPTS_EXCEEDED: { code: "AUTH_CODE_ATTEMPTS_EXCEEDED", status: 429, message: "验证码校验次数超限" },
  AUTH_EMAIL_TAKEN: { code: "AUTH_EMAIL_TAKEN", status: 409, message: "邮箱已被注册" },
  AUTH_UNAUTHORIZED: { code: "AUTH_UNAUTHORIZED", status: 401, message: "缺少有效的会话凭证" },
  AUTH_SESSION_EXPIRED: { code: "AUTH_SESSION_EXPIRED", status: 401, message: "会话已过期" },
  RATE_LIMITED: { code: "RATE_LIMITED", status: 429, message: "请求过于频繁" },
  WALLET_ALREADY_CHECKED_IN: { code: "WALLET_ALREADY_CHECKED_IN", status: 409, message: "今日已签到" },
  NOT_FOUND: { code: "NOT_FOUND", status: 404, message: "接口不存在" },
  INTERNAL: { code: "INTERNAL", status: 500, message: "服务内部错误" },
} as const satisfies Record<string, ErrorDefinition>;

export type ErrorCode = keyof typeof ERROR_DEFINITIONS;

export class AppError extends Error {
  readonly code: ErrorCode;
  readonly status: number;

  constructor(code: ErrorCode, message?: string, status?: number) {
    const definition = ERROR_DEFINITIONS[code];
    super(message ?? definition.message);
    this.name = "AppError";
    this.code = code;
    this.status = status ?? definition.status;
  }
}

export function isAppError(error: unknown): error is AppError {
  return error instanceof AppError;
}
