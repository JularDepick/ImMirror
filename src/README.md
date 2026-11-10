<div align="center">

# ImMirror 我之镜界 服务端

[![Version](https://img.shields.io/badge/Version-v0.1.0-red)](https://github.com/JularDepick/ImMirror/)
[![Copyright](https://img.shields.io/badge/Copyright-JularDepick-0066AA)](../../COPYRIGHT)
[![License](https://img.shields.io/badge/License-Apache--2.0-yellow)](../../LICENSE)

</div>

纯 JSON 接口服务, 不提供任何网页; 账号域 (M1) 与货币签到域 (M2) 已实现。

- 运行时: Node 24 + Nuxt 4 (工程骨架) + Fastify 5 (自写 HTTP 层)
- 存储: better-sqlite3, WAL 模式
- 语言: TypeScript 严格模式
- 包管理: pnpm; 测试: vitest

## 一、技术方案选择

规格第一节给出方案 A (Nitro 的 fastify preset) 与方案 B (Nitro 内构造独立 Fastify 实例) 两种落地方案, 本项目**采用方案 B**, 依据如下实测结论:

1. 本项目所用 Nitro 版本为 2.13.6, 其发布产物中没有 fastify preset, 方案 A 不可用
2. 方案 B 的落地方式: Nitro 只负责进程宿主与构建产物, 由启动插件构造唯一的 Fastify 实例并注册全部业务路由, 再由一个 Nitro 中间件把请求原样转交给 Fastify (`fastify.routing(req, res)`), Nitro 自身不注册业务路由
3. 因此服务对任意路径都只返回 JSON: 未匹配路径由 Fastify 的 404 处理返回统一错误包络, 不会返回 HTML

同时关闭了页面渲染 (ssr=false), 并让 Nitro 不扫描 server/api 目录, 避免生成与 Fastify 重复的 h3 路由。

## 二、目录结构

```
src/
├── .env.example               # 配置样例
├── .gitignore                 # 源码目录级忽略规则
├── .npmrc                     # pnpm 基础配置
├── nuxt.config.ts             # Nuxt/Nitro 配置
├── package.json               # 依赖与脚本
├── pnpm-workspace.yaml        # pnpm 设置 (链接方式, 允许的构建脚本)
├── tsconfig.json              # TypeScript 严格模式配置
├── vitest.config.ts           # 测试配置
├── README.md                  # 本文件
├── app/
│   └── app.vue                # 空壳应用, 服务端不渲染页面
├── docs/                      # 交付文档 (错误码 / 会话 / 部署)
├── server/
│   ├── api/v1/                # 路由层: 把 Fastify 路由注册到应用上
│   │   ├── auth/              # 注册 / 登录 / 重置 / 换绑 / 登出
│   │   ├── me.ts              # 当前用户资料
│   │   ├── wallet.ts          # 钱包与签到
│   │   ├── meta.ts            # 协议版本
│   │   ├── helpers.ts         # 路由层公共工具
│   │   └── index.ts           # 路由汇总
│   ├── db/                    # 数据层
│   │   ├── client.ts          # 连接与建表
│   │   ├── schema.ts          # 表结构 (与规格第四节一致)
│   │   └── repositories/      # 各表 SQL 封装
│   ├── services/              # 服务层: 业务规则
│   │   ├── auth.ts            # 账号域
│   │   ├── session.ts         # 会话
│   │   ├── wallet.ts          # 钱包与签到
│   │   ├── mail.ts            # 邮件发送抽象
│   │   └── rate-limit.ts      # 限流与失败退避
│   ├── utils/                 # 配置, 错误, 响应, 校验, 时间, 随机, 密码
│   ├── fastify/               # Fastify 应用组装, 上下文, 鉴权守卫
│   ├── middleware/            # Nitro 中间件: 请求转交 Fastify
│   └── plugins/               # Nitro 启动插件: 构造 Fastify 实例
└── tests/                     # vitest 接口测试
    ├── helpers.ts             # 独立临时库 + 内存邮件 + 可控时钟
    ├── auth.register.test.ts
    ├── auth.login.test.ts
    ├── auth.reset.test.ts
    ├── auth.rebind.test.ts
    ├── session.test.ts
    ├── wallet.test.ts
    └── common.test.ts
```

分层约定: 路由层只做参数提取与响应封装, 业务规则全部在服务层, SQL 全部在数据层。

## 三、配置项

配置来自环境变量, 缺省值见 `.env.example`, 未设置也能启动:

| 键 | 默认 | 说明 |
|:---:|:---:|:---|
| `NITRO_HOST` | 0.0.0.0 | 监听地址 |
| `NITRO_PORT` | 61110 | 监听端口 |
| `DB_FILE` | ./data/immirror.db | SQLite 文件路径 |
| `SESSION_TTL_DAYS` | 30 | 会话有效期 |
| `CODE_TTL_SECONDS` | 600 | 验证码有效期 |
| `SEND_COOLDOWN_SECONDS` | 60 | 同一邮箱同用途发送冷却 |
| `SEND_DAILY_LIMIT` | 10 | 同一邮箱每日发送上限 |
| `VERIFY_MAX_ATTEMPTS` | 5 | 单条验证码最大校验次数 |
| `PASSWORD_MIN_LENGTH` | 8 | 密码最短长度 |
| `BODY_LIMIT_BYTES` | 8192 | 请求体上限 |
| `LOGIN_MAX_FAILURES` | 5 | 登录失败退避阈值 |
| `RESET_HOURLY_LIMIT` | 5 | 同一邮箱每小时重置密码次数 |
| `SEND_HOURLY_IP_LIMIT` | 30 | 同一 IP 每小时验证码发送次数 |
| `REQUEST_MINUTE_IP_LIMIT` | 120 | 同一 IP 每分钟请求次数 |
| `MAIL_TRANSPORT` | smtp | `smtp` 或 `memory` |
| `MAIL_DEBUG_LOG` | false | memory 模式是否把验证码打到日志 |
| `SMTP_HOST` `SMTP_PORT` `SMTP_SECURE` `SMTP_USER` `SMTP_PASSWORD` `SMTP_FROM` | - | SMTP 参数 |
| `API_VERSION` | v1 | 接口版本 |
| `MIN_CLIENT_VERSION` | 0.1.0 | 最低客户端版本 |

## 四、运行命令

```bash
pnpm install   # 安装依赖
pnpm dev       # 开发模式启动 (默认 61110 端口)
pnpm build     # 生产构建
pnpm start     # 运行生产产物
pnpm test      # 运行 vitest 接口测试
```

开发模式下 Nuxt 会自动读取工程根目录的 `.env` (该文件已被 git 忽略, 用于存放本地 SMTP 凭据等敏感配置);
生产运行不读取 `.env`, 所有配置必须由进程环境注入, 例如:

```bash
NITRO_PORT=61110 DB_FILE=./data/immirror.db MAIL_TRANSPORT=smtp pnpm start
```

## 五、接口清单

统一前缀 `/api/v1`, 统一包络 `{ ok: true, data }` 或 `{ ok: false, error: { code, message } }`, 鉴权头 `Authorization: Bearer <token>`。

| 方法 | 路径 | 认证 | 说明 |
|:---:|:---|:---:|:---|
| POST | `/auth/register/code` | 否 | 发送注册验证码 |
| POST | `/auth/register` | 否 | 邮箱 + 验证码 + 密码注册, 返回 uid 与令牌 |
| POST | `/auth/login` | 否 | 邮箱或 UID + 密码登录 |
| POST | `/auth/login/code` | 否 | 发送登录验证码 |
| POST | `/auth/login/code/verify` | 否 | 邮箱 + 验证码登录 |
| POST | `/auth/password/reset/code` | 否 | 发送重置密码验证码 |
| POST | `/auth/password/reset` | 否 | 邮箱 + 验证码 + 新密码重置 |
| POST | `/auth/email/rebind/code` | 是 | 向新邮箱发送换绑验证码 |
| POST | `/auth/email/rebind` | 是 | 新邮箱 + 验证码 + 密码完成换绑 |
| POST | `/auth/logout` | 是 | 注销当前会话 |
| GET | `/me` | 是 | 账号资料, 余额与本自然月签到进度 |
| GET | `/wallet` | 是 | 余额与更新时间 |
| GET | `/wallet/checkin` | 是 | 签到状态, 下次奖励与里程碑进度 |
| POST | `/wallet/checkin` | 是 | 执行当日签到 |
| GET | `/meta/version` | 否 | 协议版本与服务时间 |

错误码: `BAD_REQUEST` `AUTH_INVALID_IDENTIFIER` `AUTH_INVALID_CREDENTIALS` `AUTH_CODE_INVALID` `AUTH_CODE_EXPIRED` `AUTH_CODE_ATTEMPTS_EXCEEDED` `AUTH_EMAIL_TAKEN` `AUTH_UNAUTHORIZED` `AUTH_SESSION_EXPIRED` `RATE_LIMITED` `WALLET_ALREADY_CHECKED_IN` `NOT_FOUND` `INTERNAL`。

## 六、业务规则要点

| 项 | 规则 |
|:---:|:---|
| 货币 | 中文名"镜币", 内部代号 `mirror_shard`, 只存服务端, 累积不清零 |
| UID | 9 位十进制, 逐位随机且首位非 0, 注册时随机分配并做唯一性重试, 不可修改 |
| 密码 | scrypt (N=16384, r=8, p=1) 加 16 字节随机盐, 存储格式带参数前缀 |
| 会话 | 32 字节随机令牌, 只存 SHA-256, 30 天滑动续期, 登出置撤销标记 |
| 验证码 | 6 位数字, 只存 SHA-256, 默认 600 秒有效, 同邮箱同用途 60 秒冷却, 校验上限 5 次 |
| 枚举防护 | 注册与重置的验证码接口对已存在与不存在邮箱返回完全一致的响应 |
| 签到 | 自然月周期, 时区 Asia/Shanghai, 每自然日一次 |
| 签到奖励 | 单次 `min(100 + (当月已签次数 - 1) * 10, 200)`, 次数包含本次 |
| 里程碑 | 当月第 15 次额外 500, 第 25 次额外 1000, 单独记流水 |
| 并发 | 签到插入与余额更新在同一事务内, 唯一约束 (user_id, date) 兜底 |
| 日志 | 不输出密码, 令牌与验证码明文; 仅 `MAIL_DEBUG_LOG=true` 的 memory 模式打印验证码 |

## 七、测试

```bash
pnpm test
```

- 每个用例创建独立应用与独立临时数据库 (每次用例新建临时目录, 用例结束删除)
- 邮件通道固定为 memory, 验证码直接从内存信箱读取
- 时钟可注入, 覆盖跨日签到, 跨月重置, 会话过期等场景
- 覆盖用例组: 注册, 登录, 重置, 换绑, 会话, 签到, 通用约定 (共 45 条)

## 八、文档索引

| 文档 | 内容 |
|:---:|:---|
| [错误码对照表](./docs/error-codes.md) | 全部错误码的 HTTP 状态, 触发场景, 客户端处理建议与面向用户的中文提示文案 |
| [会话与令牌策略](./docs/session.md) | 令牌签发与保存, 滑动续期的触发点, 失效与重新登录判定, 多端登录行为 |
| [部署指南](./docs/deploy.md) | 部署步骤, 必需环境变量, 反向代理与 HTTPS 建议, SQLite 备份建议 |

## 版权信息

Copyright &copy; 2026 JularDepick

详见 [COPYRIGHT](../../COPYRIGHT) 。

## 许可证

本仓库采用 [Apache-2.0 许可证](../../LICENSE) 。