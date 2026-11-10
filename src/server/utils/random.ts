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

/**
 * 生成 8 位十进制 UID, 首位非 0
 */
export function randomUid(): string {
  let uid = String(randomInt(1, 10));
  while (uid.length < 8) {
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
