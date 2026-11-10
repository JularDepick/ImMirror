/**
 * 进程内限流
 * 单实例部署足够使用; 多实例部署时应替换为共享存储实现, 接口保持不变
 */

export interface LimitResult {
  allowed: boolean;
  retryAfterSeconds: number;
}

interface CounterEntry {
  timestamps: number[];
}

/**
 * 滑动窗口计数器: 同一键在窗口内最多 limit 次
 */
export class WindowLimiter {
  private readonly entries = new Map<string, CounterEntry>();

  constructor(private readonly now: () => number) {}

  consume(key: string, limit: number, windowMs: number): LimitResult {
    const current = this.now();
    const entry = this.entries.get(key) ?? { timestamps: [] };
    const lowerBound = current - windowMs;
    entry.timestamps = entry.timestamps.filter((timestamp) => timestamp > lowerBound);
    if (entry.timestamps.length >= limit) {
      const oldest = entry.timestamps[0] ?? current;
      const retryAfterMs = Math.max(0, oldest + windowMs - current);
      this.entries.set(key, entry);
      return { allowed: false, retryAfterSeconds: Math.ceil(retryAfterMs / 1000) };
    }
    entry.timestamps.push(current);
    this.entries.set(key, entry);
    return { allowed: true, retryAfterSeconds: 0 };
  }

  reset(key: string): void {
    this.entries.delete(key);
  }
}

interface BackoffEntry {
  failures: number;
  blockedUntil: number;
}

/**
 * 失败退避: 达到阈值后按 1, 2, 4, 8 分钟指数退避
 */
export class FailureBackoff {
  private readonly entries = new Map<string, BackoffEntry>();

  constructor(
    private readonly now: () => number,
    private readonly threshold: number,
    private readonly baseDelayMs = 60_000,
  ) {}

  check(key: string): LimitResult {
    const entry = this.entries.get(key);
    const current = this.now();
    if (!entry || entry.blockedUntil <= current) {
      return { allowed: true, retryAfterSeconds: 0 };
    }
    return { allowed: false, retryAfterSeconds: Math.ceil((entry.blockedUntil - current) / 1000) };
  }

  recordFailure(key: string): LimitResult {
    const current = this.now();
    const entry = this.entries.get(key) ?? { failures: 0, blockedUntil: 0 };
    entry.failures += 1;
    if (entry.failures >= this.threshold) {
      const overflow = entry.failures - this.threshold;
      const delay = this.baseDelayMs * 2 ** Math.min(overflow, 3);
      entry.blockedUntil = current + delay;
    }
    this.entries.set(key, entry);
    return this.check(key);
  }

  reset(key: string): void {
    this.entries.delete(key);
  }
}
