/**
 * 随机与摘要工具
 * 令牌与验证码明文只在生成瞬间存在, 数据库只保存摘要
 */

import { createHash, randomBytes, randomInt, timingSafeEqual } from "node:crypto";

export function sha256Hex(input: string | Buffer): string {
  return createHash("sha256").update(input).digest("hex");
}

/**
 * 生成十六进制随机令牌, 默认 32 字节
 */
export function randomToken(byteLength = 32): string {
  return randomBytes(byteLength).toString("hex");
}

/**
 * 生成固定位数的数字验证码, 允许前导 0
 */
export function randomNumericCode(digits = 6): string {
  let code = "";
  while (code.length < digits) {
    code += String(randomInt(0, 10));
  }
  return code;
}

// UID 长度: 9 位十进制
export const UID_LENGTH = 9;

/**
 * 逐位随机生成 UID
 * 每一位取 0 到 9 的数字, 首位取 1 到 9, 因此结果固定为 9 位且没有前导 0
 */
export function randomUid(): string {
  let uid = String(randomInt(1, 10));
  while (uid.length < UID_LENGTH) {
    uid += String(randomInt(0, 10));
  }
  return uid;
}

/**
 * 定长摘要常量时间比较
 */
export function safeEqualHex(left: string, right: string): boolean {
  const leftBuffer = Buffer.from(left, "hex");
  const rightBuffer = Buffer.from(right, "hex");
  if (leftBuffer.length !== rightBuffer.length || leftBuffer.length === 0) {
    return false;
  }
  return timingSafeEqual(leftBuffer, rightBuffer);
}
