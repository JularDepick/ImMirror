/**
 * 响应包络
 * 成功为 { ok: true, data }, 失败为 { ok: false, error: { code, message } }
 */

export interface ApiSuccess<T> {
  ok: true;
  data: T;
}

export interface ApiFailure {
  ok: false;
  error: {
    code: string;
    message: string;
  };
}

export function ok<T>(data: T): ApiSuccess<T> {
  return { ok: true, data };
}

export function fail(code: string, message: string): ApiFailure {
  return { ok: false, error: { code, message } };
}
