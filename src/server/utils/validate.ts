/**
 * 请求字段校验
 * 所有校验失败统一抛 BAD_REQUEST, 由 HTTP 层翻译为 400
 */

import { AppError } from "./errors";

const EMAIL_PATTERN = /^[^\s@]+@[^\s@]+\.[^\s@]{2,}$/;
const CODE_PATTERN = /^\d{6}$/;
const UID_PATTERN = /^\d+$/;

export interface JsonObject {
  [key: string]: unknown;
}

/**
 * 归一化邮箱: 去空白并转小写; 格式非法时抛 BAD_REQUEST
 */
export function requireEmail(value: unknown): string {
  if (typeof value !== "string") {
    throw new AppError("BAD_REQUEST", "邮箱缺失");
  }
  const normalized = value.trim().toLowerCase();
  if (!EMAIL_PATTERN.test(normalized) || normalized.length > 254) {
    throw new AppError("BAD_REQUEST", "邮箱格式不合法");
  }
  return normalized;
}

export function requirePassword(value: unknown, minLength: number): string {
  if (typeof value !== "string") {
    throw new AppError("BAD_REQUEST", "密码缺失");
  }
  if (value.length < minLength) {
    throw new AppError("BAD_REQUEST", `密码长度不得少于 ${minLength} 位`);
  }
  if (value.length > 128) {
    throw new AppError("BAD_REQUEST", "密码长度不得超过 128 位");
  }
  return value;
}

export function requireCode(value: unknown): string {
  if (typeof value !== "string" || !CODE_PATTERN.test(value.trim())) {
    throw new AppError("BAD_REQUEST", "验证码格式不合法");
  }
  return value.trim();
}

export function requireIdentifier(value: unknown): string {
  if (typeof value !== "string" || value.trim() === "") {
    throw new AppError("BAD_REQUEST", "登录标识符缺失");
  }
  return value.trim();
}

export function isUidLike(value: string): boolean {
  return UID_PATTERN.test(value);
}

/**
 * 取 JSON 请求体对象, 非对象一律视为参数非法
 */
export function requireObjectBody(body: unknown): JsonObject {
  if (body === null || typeof body !== "object" || Array.isArray(body)) {
    return {};
  }
  return body as JsonObject;
}
