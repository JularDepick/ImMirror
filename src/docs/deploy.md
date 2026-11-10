# 部署指南

> 本文档描述服务端的生产部署方式: 依赖安装, 构建, 启动, 环境变量, 反向代理与 HTTPS, SQLite 备份。
> 服务只提供 JSON 接口, 不需要额外的网页容器或静态资源服务。

## 一、前置条件

| 项 | 要求 |
|:---:|:---|
| Node.js | 24 LTS。构建与运行都需要, 版本过低会导致语法与原生模块不兼容 |
| pnpm | 10 及以上, 用于安装依赖与执行脚本 |
| 磁盘 | 需要可写目录存放 SQLite 数据库文件, 以及构建产物的落盘空间 |
| 网络 | 出站可达 npm 源 (安装依赖) 与 SMTP 服务器 (发送验证码) |
| 端口 | 默认监听 61110, 需与反向代理或客户端保持一致 |

SQLite 与 HTTP 服务都由进程内模块提供, 不需要单独安装数据库或运行时中间件。

## 二、部署步骤

```bash
# 1. 取出代码后进入工程根目录

# 2. 安装依赖, 有锁文件时建议锁定版本
pnpm install --frozen-lockfile

# 3. 注入环境变量 (见第三节), 然后构建生产产物
pnpm build

# 4. 启动服务, 前台运行, 由进程守护工具接管
pnpm start
```

健康检查: 启动后请求 `GET /api/v1/meta/version`, 返回 `{ "ok": true, "data": { "apiVersion": "v1", ... } }` 即表示服务可用。

注意事项:

1. 生产运行不会自动读取工程目录下的本地环境文件, 配置必须由进程环境注入 (systemd 的 `Environment=`, 容器的 `env_file` 或 `docker run -e`)
2. 构建与运行必须使用同一份环境变量中的端口与数据库路径, 否则健康检查会连到错误端口
3. 首次启动会自动创建数据库文件与全部表, 但不会创建账号数据
4. 服务为单进程模型, 不要在同一数据库上并行启动多个实例 (见第六节)

## 三、环境变量

### 3.1 生产必需

| 键 | 示例 | 说明 |
|:---:|:---:|:---|
| `NITRO_HOST` | 127.0.0.1 | 监听地址。前置反向代理时绑 `127.0.0.1`; 直接对外时用 `0.0.0.0` |
| `NITRO_PORT` | 61110 | 监听端口, 需与代理和客户端约定一致 |
| `DB_FILE` | ./data/immirror.db | 数据库文件路径, 其父目录必须存在或可被自动创建且可写 |
| `MAIL_TRANSPORT` | smtp | 生产必须取 `smtp`; `memory` 只用于测试, 邮件不会真正投递 |
| `SMTP_HOST` | smtp.example.com | SMTP 主机 |
| `SMTP_USER` | no-reply@example.com | SMTP 账号 |
| `SMTP_PASSWORD` | 授权码 | SMTP 密码或授权码 |
| `SMTP_FROM` | 我之镜界 <no-reply@example.com> | 发件人显示名与地址 |

### 3.2 生产建议

| 键 | 建议值 | 说明 |
|:---:|:---:|:---|
| `MAIL_DEBUG_LOG` | false | 只能为 false; 打开后会把验证码明文写进日志 |
| `SMTP_PORT` | 465 | 465 走隐式 TLS; 587 需配合 `SMTP_SECURE=false` 与 STARTTLS |
| `SMTP_SECURE` | true | 与端口匹配 |
| `BODY_LIMIT_BYTES` | 8192 | 请求体上限, 反向代理侧的请求体上限不要低于该值 |
| `REQUEST_MINUTE_IP_LIMIT` | 120 | 单 IP 每分钟请求上限, 反向代理下按代理转发的客户端地址计数 |

### 3.3 可选项

未设置的键取内置默认值, 一般无需改动: `SESSION_TTL_DAYS` `CODE_TTL_SECONDS` `SEND_COOLDOWN_SECONDS` `SEND_DAILY_LIMIT` `VERIFY_MAX_ATTEMPTS` `PASSWORD_MIN_LENGTH` `LOGIN_MAX_FAILURES` `RESET_HOURLY_LIMIT` `SEND_HOURLY_IP_LIMIT` `API_VERSION` `MIN_CLIENT_VERSION`。

完整清单与默认值见工程根目录的 `.env.example`。

## 四、反向代理与 HTTPS

### 4.1 HTTPS 终止位置

推荐由反向代理终止 TLS, 服务只监听回环地址。若确实需要让 Node 进程直接持有证书, 该运行时可读取 `NITRO_SSL_CERT` 与 `NITRO_SSL_KEY` 两个环境变量并直接提供 HTTPS, 但证书续期与协议版本控制会麻烦很多, 不推荐。

### 4.2 Nginx 示例

```nginx
server {
    listen 443 ssl;
    server_name api.example.com;

    ssl_certificate     /etc/ssl/immirror/fullchain.pem;
    ssl_certificate_key /etc/ssl/immirror/privkey.pem;

    # 与服务端 BODY_LIMIT_BYTES 对齐, 略留余量
    client_max_body_size 64k;

    location /api/v1/ {
        proxy_pass         http://127.0.0.1:61110;
        proxy_http_version 1.1;

        # 必须覆盖下列头, 否则服务端 IP 限流会按代理地址计数
        proxy_set_header Host              $host;
        proxy_set_header X-Real-IP         $remote_addr;
        proxy_set_header X-Forwarded-For   $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;

        proxy_connect_timeout 5s;
        proxy_read_timeout    30s;
    }
}

server {
    listen 80;
    server_name api.example.com;
    return 301 https://$host$request_uri;
}
```

要点:

1. 服务启动时已开启代理信任, 会读取 `X-Forwarded-For` 的首个地址作为客户端地址; 因此代理必须覆盖该头, 不能把客户端自带的同名头原样透传
2. 只把 `/api/v1/` 前缀转发到服务, 其余路径直接拒绝, 减少暴露面
3. 服务不返回 HTML, 也不需要静态资源, 代理侧不要配置目录浏览或默认站点
4. 客户端与代理之间统一使用 HTTPS; 明文端口建议只用于本机健康检查

## 五、SQLite 备份

数据库使用 WAL 模式, 数据分布在主库文件与同目录的 `-wal` `-shm` 附属文件中, 因此备份不能只复制主库文件。

| 方式 | 命令 | 适用场景 |
|:---:|:---|:---|
| 在线备份 | `sqlite3 ./data/immirror.db ".backup './backup/immirror-$(date +%F).db'"` | 服务运行中, 由 SQLite 保证一致性 |
| 导出快照 | `sqlite3 ./data/immirror.db "VACUUM INTO './backup/immirror-$(date +%F).db'"` | 服务运行中, 生成紧凑副本 |
| 停服复制 | 先停止进程, 再复制主库与 `-wal` `-shm` 三个文件 | 计划内维护窗口 |

建议:

1. 每天至少备份一次, 保留 7 到 14 份, 备份文件放到与数据库不同的磁盘或对象存储
2. 备份前可执行 `PRAGMA wal_checkpoint(TRUNCATE);` 把 WAL 内容并回主库, 减少附属文件体积
3. 只读校验: 用 `PRAGMA integrity_check;` 检查备份可打开且结构完整
4. 账号与货币数据只有这一份存储, 备份策略应与数据丢失容忍度匹配; 恢复时停止服务, 用备份文件替换数据库后重启

## 六、运行与扩展约束

1. 限流与失败退避计数保存在进程内存, 多实例部署会各算各的, 因此当前只支持"单实例 + 单数据库"
2. SQLite 是单写模型, 写并发高时应先评估改为外部数据库, 而不是简单加实例
3. 进程需要优雅退出以释放数据库句柄, 建议由 systemd 或容器运行时发送 `SIGTERM` 而不是 `SIGKILL`
4. 服务无状态文件, 只有数据库与日志, 容器化时把数据库目录挂成持久卷即可

## 七、升级与回滚

1. 升级前先做一次数据库备份
2. 拉取新代码后执行 `pnpm install --frozen-lockfile` 与 `pnpm build`
3. 重启进程, 用 `GET /api/v1/meta/version` 与一次注册登录流程验证
4. 回滚时切回上一份代码与依赖, 并在必要时用升级前的备份恢复数据库; 新旧版本对表结构保持向后兼容时可以不回滚数据库

## 八、常见排查

| 现象 | 可能原因 | 处理 |
|:---:|:---|:---|
| 启动即退出, 提示端口占用 | 61110 已被其他进程占用 | 改动 `NITRO_PORT` 或释放端口 |
| 启动报数据库无法打开 | `DB_FILE` 的父目录不可写, 或路径拼写错误 | 检查目录权限与绝对路径 |
| 注册一直收不到验证码 | `MAIL_TRANSPORT=memory`; SMTP 参数错误; 发件域被拦截 | 检查传输方式与 SMTP 凭据, 查看服务日志中的投递失败信息 |
| 客户端全部接口 401 | 代理未透传 `Authorization` 头 | 在代理配置中保留请求头, 不要做鉴权剥离 |
| 大量 429 | 代理未覆盖 `X-Forwarded-For`, 所有请求被算成同一 IP | 修正代理头设置 |
| 健康检查返回 HTML 或 404 页面 | 请求打到了别的站点 | 核对 `server_name` 与 `proxy_pass` 指向 |
