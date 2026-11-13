/**
 * 运行配置用例组
 * 覆盖缺省值, 类型容错与拼错键名的识别
 */

import { describe, expect, it } from "vitest";
import { CONFIG_KEYS, findUnknownConfigKeys, loadConfig } from "../server/utils/config";

describe("运行配置", () => {
  it("空环境下取缺省值", () => {
    const config = loadConfig({});
    expect(config.port).toBe(61110);
    expect(config.dbFile).toBe("./data/immirror.db");
    expect(config.mailTransport).toBe("smtp");
  });

  it("数值键写错时回落缺省值", () => {
    const config = loadConfig({ NITRO_PORT: "not-a-number" });
    expect(config.port).toBe(61110);
  });

  it("布尔键接受多种写法", () => {
    expect(loadConfig({ SMTP_SECURE: "yes" }).smtp.secure).toBe(true);
    expect(loadConfig({ SMTP_SECURE: "off" }).smtp.secure).toBe(false);
  });

  it("拼错的配置键能被识别出来", () => {
    const unknown = findUnknownConfigKeys({ NITRO_POT: "61110", DB_FILE: "x.db", HOME: "/root" });
    /* 只有带本项目前缀的未知键才提醒, 系统变量不算. */
    expect(unknown).toEqual(["NITRO_POT"]);
  });

  it("已登记的配置键不算未知", () => {
    const env: Record<string, string> = {};
    for (const key of CONFIG_KEYS) {
      env[key] = "1";
    }
    expect(findUnknownConfigKeys(env)).toEqual([]);
  });
});
