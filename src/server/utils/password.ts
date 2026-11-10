/**
 * 密码哈希
 * scrypt (N=16384, r=8, p=1, keylen=32) 加 16 字节随机盐,
 * 存储格式 scrypt$N$r$p$salt_b64$hash_b64
 */

import { randomBytes, scrypt as scryptCallback, timingSafeEqual } from "node:crypto";
import type { ScryptOptions } from "node:crypto";

const SCRYPT_N = 16384;
const SCRYPT_R = 8;
const SCRYPT_P = 1;
const KEY_LENGTH = 32;
const SALT_BYTES = 16;
// scrypt 内存上限需大于 128 * N * r
const SCRYPT_MAX_MEM = 64 * 1024 * 1024;

/**
 * 异步 scrypt 封装
 * Node 的 promisify 重载不包含选项参数, 这里手动包装以传入 N/r/p
 */
function deriveKey(plain: string, salt: Buffer, keyLength: number, options: ScryptOptions): Promise<Buffer> {
  return new Promise((resolve, reject) => {
    scryptCallback(plain, salt, keyLength, options, (error, derivedKey) => {
      if (error) {
        reject(error);
        return;
      }
      resolve(derivedKey);
    });
  });
}

/**
 * 用户不存在时也执行一次同等开销的哈希, 抹平时间差异
 */
// 盐为 16 字节, 摘要为 32 字节, 保证与真实记录同样的计算量
export const DUMMY_PASSWORD_HASH =
  "scrypt$16384$8$1$MDEyMzQ1Njc4OWFiY2RlZg==$" +
  "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";

export async function hashPassword(plain: string): Promise<string> {
  const salt = randomBytes(SALT_BYTES);
  const derived = await deriveKey(plain, salt, KEY_LENGTH, {
    N: SCRYPT_N,
    r: SCRYPT_R,
    p: SCRYPT_P,
    maxmem: SCRYPT_MAX_MEM,
  });
  return [
    "scrypt",
    String(SCRYPT_N),
    String(SCRYPT_R),
    String(SCRYPT_P),
    salt.toString("base64"),
    derived.toString("base64"),
  ].join("$");
}

/**
 * 校验密码, 任何解析失败都按不匹配处理
 */
export async function verifyPassword(plain: string, stored: string): Promise<boolean> {
  const segments = stored.split("$");
  if (segments.length !== 6 || segments[0] !== "scrypt") {
    return false;
  }
  const n = Number.parseInt(segments[1] ?? "", 10);
  const r = Number.parseInt(segments[2] ?? "", 10);
  const p = Number.parseInt(segments[3] ?? "", 10);
  if (!Number.isFinite(n) || !Number.isFinite(r) || !Number.isFinite(p)) {
    return false;
  }
  const salt = Buffer.from(segments[4] ?? "", "base64");
  const expected = Buffer.from(segments[5] ?? "", "base64");
  if (salt.length === 0 || expected.length === 0) {
    return false;
  }
  const derived = await deriveKey(plain, salt, expected.length, {
    N: n,
    r,
    p,
    maxmem: SCRYPT_MAX_MEM,
  });
  if (derived.length !== expected.length) {
    return false;
  }
  return timingSafeEqual(derived, expected);
}

/**
 * 用户不存在时的等耗时校验入口
 */
export async function burnPasswordCheck(plain: string): Promise<void> {
  await verifyPassword(plain, DUMMY_PASSWORD_HASH);
}
