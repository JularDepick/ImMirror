/**
 * 时间工具
 * 存储一律使用 Unix 毫秒整数; 业务自然日与自然月按配置时区 (默认 Asia/Shanghai) 计算
 */

import { DEFAULT_TIME_ZONE } from "./config";

const formatterCache = new Map<string, Intl.DateTimeFormat>();

function formatterFor(timeZone: string): Intl.DateTimeFormat {
  const cached = formatterCache.get(timeZone);
  if (cached) {
    return cached;
  }
  const formatter = new Intl.DateTimeFormat("en-CA", {
    timeZone,
    year: "numeric",
    month: "2-digit",
    day: "2-digit",
  });
  formatterCache.set(timeZone, formatter);
  return formatter;
}

interface LocalParts {
  year: string;
  month: string;
  day: string;
}

function localParts(ms: number, timeZone: string): LocalParts {
  const parts = formatterFor(timeZone).formatToParts(new Date(ms));
  const pick = (type: string): string => {
    const found = parts.find((part) => part.type === type);
    return found ? found.value : "00";
  };
  return { year: pick("year"), month: pick("month"), day: pick("day") };
}

/**
 * 取自然日键, 形如 2026-11-10
 */
export function localDateKey(ms: number, timeZone: string = DEFAULT_TIME_ZONE): string {
  const parts = localParts(ms, timeZone);
  return `${parts.year}-${parts.month}-${parts.day}`;
}

/**
 * 取自然月键, 形如 2026-11
 */
export function localMonthKey(ms: number, timeZone: string = DEFAULT_TIME_ZONE): string {
  const parts = localParts(ms, timeZone);
  return `${parts.year}-${parts.month}`;
}
