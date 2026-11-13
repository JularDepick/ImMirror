/**
 * 运行配置
 * 所有配置项来自环境变量, 缺省值与 .env.example 一致;
 * 配置只在应用组装时读取一次, 便于测试注入不同取值
 */

export type MailTransportKind = "smtp" | "memory";

export interface SmtpConfig {
  host: string;
  port: number;
  secure: boolean;
  user: string;
  password: string;
  from: string;
}

export interface AppConfig {
  host: string;
  port: number;
  dbFile: string;
  sessionTtlDays: number;
  codeTtlSeconds: number;
  sendCooldownSeconds: number;
  sendDailyLimit: number;
  verifyMaxAttempts: number;
  passwordMinLength: number;
  bodyLimitBytes: number;
  loginMaxFailures: number;
  resetHourlyLimit: number;
  sendHourlyIpLimit: number;
  requestMinuteIpLimit: number;
  mailTransport: MailTransportKind;
  mailDebugLog: boolean;
  smtp: SmtpConfig;
  timeZone: string;
  apiVersion: string;
  minClientVersion: string;
}

const DEFAULT_TIME_ZONE = "Asia/Shanghai";

/**
 * 本项目使用的全部配置键.
 * 拼错的键会被静默忽略并回落到缺省值, 因此启动时要把"看起来是配置但没人读"的键列出来.
 */
export const CONFIG_KEYS = [
  "API_VERSION",
  "APP_TIME_ZONE",
  "BODY_LIMIT_BYTES",
  "CODE_TTL_SECONDS",
  "DB_FILE",
  "LOGIN_MAX_FAILURES",
  "MAIL_DEBUG_LOG",
  "MAIL_TRANSPORT",
  "MIN_CLIENT_VERSION",
  "NITRO_HOST",
  "NITRO_PORT",
  "PASSWORD_MIN_LENGTH",
  "REQUEST_MINUTE_IP_LIMIT",
  "RESET_HOURLY_LIMIT",
  "SEND_COOLDOWN_SECONDS",
  "SEND_DAILY_LIMIT",
  "SEND_HOURLY_IP_LIMIT",
  "SESSION_TTL_DAYS",
  "SMTP_FROM",
  "SMTP_HOST",
  "SMTP_PASSWORD",
  "SMTP_PORT",
  "SMTP_SECURE",
  "SMTP_USER",
  "VERIFY_MAX_ATTEMPTS",
] as const;

/** 配置键的命名前缀: 只有这些前缀的未知键才值得提醒, 避免把系统变量当成拼错的配置. */
const CONFIG_PREFIXES = ["NITRO_", "DB_", "SMTP_", "MAIL_", "SESSION_", "SEND_", "RESET_", "API_"];

/**
 * 找出"看起来是本项目配置但没人读"的环境变量.
 * 典型来源是拼错的键名: 服务会照常启动并回落到缺省值, 但行为与预期不同.
 */
export function findUnknownConfigKeys(env: NodeJS.ProcessEnv): string[] {
  const known = new Set<string>(CONFIG_KEYS);
  const unknown: string[] = [];
  for (const key of Object.keys(env)) {
    if (known.has(key)) {
      continue;
    }
    if (CONFIG_PREFIXES.some((prefix) => key.startsWith(prefix))) {
      unknown.push(key);
    }
  }
  unknown.sort();
  return unknown;
}

function readString(env: NodeJS.ProcessEnv, key: string, fallback: string): string {
  const raw = env[key];
  if (raw === undefined) {
    return fallback;
  }
  const trimmed = raw.trim();
  return trimmed === "" ? fallback : trimmed;
}

function readInt(env: NodeJS.ProcessEnv, key: string, fallback: number): number {
  const raw = env[key];
  if (raw === undefined || raw.trim() === "") {
    return fallback;
  }
  const parsed = Number.parseInt(raw.trim(), 10);
  return Number.isFinite(parsed) ? parsed : fallback;
}

function readBool(env: NodeJS.ProcessEnv, key: string, fallback: boolean): boolean {
  const raw = env[key];
  if (raw === undefined || raw.trim() === "") {
    return fallback;
  }
  const normalized = raw.trim().toLowerCase();
  if (["1", "true", "yes", "on"].includes(normalized)) {
    return true;
  }
  if (["0", "false", "no", "off"].includes(normalized)) {
    return false;
  }
  return fallback;
}

function readMailTransport(env: NodeJS.ProcessEnv): MailTransportKind {
  const raw = readString(env, "MAIL_TRANSPORT", "smtp").toLowerCase();
  return raw === "memory" ? "memory" : "smtp";
}

/**
 * 读取完整配置
 * @param env 进程环境变量, 测试可传入自定义对象
 */
export function loadConfig(env: NodeJS.ProcessEnv = process.env): AppConfig {
  return {
    host: readString(env, "NITRO_HOST", "0.0.0.0"),
    port: readInt(env, "NITRO_PORT", 61110),
    dbFile: readString(env, "DB_FILE", "./data/immirror.db"),
    sessionTtlDays: readInt(env, "SESSION_TTL_DAYS", 30),
    codeTtlSeconds: readInt(env, "CODE_TTL_SECONDS", 600),
    sendCooldownSeconds: readInt(env, "SEND_COOLDOWN_SECONDS", 60),
    sendDailyLimit: readInt(env, "SEND_DAILY_LIMIT", 10),
    verifyMaxAttempts: readInt(env, "VERIFY_MAX_ATTEMPTS", 5),
    passwordMinLength: readInt(env, "PASSWORD_MIN_LENGTH", 8),
    bodyLimitBytes: readInt(env, "BODY_LIMIT_BYTES", 8192),
    loginMaxFailures: readInt(env, "LOGIN_MAX_FAILURES", 5),
    resetHourlyLimit: readInt(env, "RESET_HOURLY_LIMIT", 5),
    sendHourlyIpLimit: readInt(env, "SEND_HOURLY_IP_LIMIT", 30),
    requestMinuteIpLimit: readInt(env, "REQUEST_MINUTE_IP_LIMIT", 120),
    mailTransport: readMailTransport(env),
    mailDebugLog: readBool(env, "MAIL_DEBUG_LOG", false),
    smtp: {
      host: readString(env, "SMTP_HOST", ""),
      port: readInt(env, "SMTP_PORT", 465),
      secure: readBool(env, "SMTP_SECURE", true),
      user: readString(env, "SMTP_USER", ""),
      password: readString(env, "SMTP_PASSWORD", ""),
      from: readString(env, "SMTP_FROM", ""),
    },
    timeZone: readString(env, "APP_TIME_ZONE", DEFAULT_TIME_ZONE),
    apiVersion: readString(env, "API_VERSION", "v1"),
    minClientVersion: readString(env, "MIN_CLIENT_VERSION", "0.1.0"),
  };
}

export { DEFAULT_TIME_ZONE };
