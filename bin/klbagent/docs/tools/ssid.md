# ssid（身份 / 临时 token 唯一入口）

**对外身份与临时 token 的唯一入口**。`auth.*` 工具、`alua auth` CLI、`runtime.session_*` 均已删除；`core.auth` 仅供 ssid 内调（内部语义 → `tools/auth.md`）。

| 工具 | profile | 参数 | 说明 |
|------|---------|------|------|
| `ssid.whoami` | readonly | `via?` / `alua_ssid?` | 回显 `alua_ssid` / `host_tool` / `session_id` / `via` / `caller_kind` / `protocol_version`（原 `runtime.session_whoami` 并入）；禁回显 token / `token_ref` |
| `ssid.ensure` | net | `alua_ssid?` / `profile?` / `force?` | 为当前或指定 ssid 签发/续用临时 token（**仅进程内存**；重启须重签；6h 滑动；一客户端一票）；业务回显仅 `has_token` / `expire_ms` / `profile`（**禁** `token` / `token_ref`） |
| `ssid.begin_ctx` | net | `via?` / `profile?` / `skip_token?` | register + ensure + bind_request 一步到位；返回体仅 `has_token` 等元数据（**禁**明文 / `token_ref`） |
| `ssid.status` | readonly | `detail?` | 脱敏计数（count / lpc_ready / xpc_ready）；默认**不含**全 clients（XPC 单帧约 8KB）；`detail=true` 才附带 |
| `ssid.gc` | net | `max_age_ms?`(默认 7d) / `dry_run?`(默认 true) / `purge_temp_files?` | 清 `ssid.clients.json` 陈旧登记；可选删历史残留 `temp.*.env`；不动 machine key |

CLI（同一套语义）：

```
alua ssid whoami [--json]
alua ssid ensure [--json]          # 签发档固定 net
alua ssid status [--json]
alua ssid begin  [--json]          # begin_ctx; 不回显 token / token_ref
alua ssid gc [--json] [--apply]    # 默认 dry-run; --apply 才真删
```

```
alua call ssid.whoami {}
alua call ssid.ensure {"profile":"net"}
alua call ssid.begin_ctx {"via":"call"}
alua call ssid.status {}
alua call ssid.gc {"dry_run":true}
```

## 面与传输

| 面 | 入口 | 用途 |
|----|------|------|
| 工具面 | `catalog` category `runtime` | 全部调用方（CLI / MCP / serve / remote） |
| 客户端 | `core.ssid.new_client({transport="lpc"\|"xpc"})` | LPC=同进程线程；XPC=跨进程 → owner `alua.ssid` |
| 服务端 | `core.ssid.new_serve()` | 仅 ssid owner 线程；业务代码禁 `new_serve` |
| 服务名 | `alua.ssid`（LPC + XPC） | serve 入口与 MCP 均经此验票 |

**选型（强制）**：

| 调用方 | transport | 前提 |
|--------|-----------|------|
| `alua ssid` / `alua call` / `alua stdio` / `tools.ssid` / MCP / remote 客户端面 | **xpc** | 本机已起 `alua serve`（ssid XPC owner） |
| serve 进程内消费线程（`serve.security.auth` 等） | **lpc** | 同进程 `worker.ssid` owner |

## XPC unix 路径

| 项 | 约定 |
|----|------|
| 格式 | `/tmp/alua.ssid.xpc.unix.pxxxx`（`xxxx` = serve 口，如 `12817`） |
| 谁拼 | **Lua**（`core/ssid/base/xpcpath.lua`）；C `create_unix`/`open_unix` 只透传 `path` |
| serve | owner **先 LPC**；**等** listen 写 `serve.port` 后再按**实口**拼 path 绑 XPC |
| client | 空 path 时读 pref `serve.port`，无则默认 `12817` |
| 禁止 | 依赖 C 默认 `/tmp/klb.xpc.alua.ssid`；用候选首口预绑 XPC |

## 身份标签（`host_tool` / `caller_kind`）

`ssid.whoami` 回的 `host_tool` / `caller_kind` 只作**标签**（不参与授权）。派生在 `core/ssid/base/fingerprint.lua`：

| 步骤 | 规则 |
|------|------|
| 1 显式覆盖 | `ALUA_CALLER_NAME`：**仅当**其值属已知表 `HOST_TOOL_KNOWN`（`mcp-cursor` / `mcp-codebuddy` / `mcp-trae` / `mcp-vscode` / `mcp-copilot` / `mcp-claude` / `mcp-claude-desktop` / `mcp-claude-code` / `mcp-windsurf` / `mcp-unknown`）或以 `mcp-` 开头才采纳；形如 `other-caller` 的任意名会被**忽略**（不是失败） |
| 2 环境位 | 宿主环境标志（cursor / codebuddy / trae / vscode / claude）→ 对应 `mcp-<host>` |
| 3 名称粗匹配 | MCP `clientInfo.name` / 父进程名含 cursor / codebuddy / trae / vscode / claude → 对应 `mcp-<host>` |
| 4 回落 | 以上都不中 → **`mcp-unknown`**（未适配宿主或自研 client）；`client_name` 仍原样回显 `clientInfo.name`，不受影响 |
| `caller_kind` | 识别到宿主（或 `mcp-unknown` 但带 `client_name`）→ `ai_host`；`mcp-unknown` 且无 `client_name` → `mcp_client`；其余按环境（`ci` / `ssh` / `cli` 等） |

## 规则

| MUST | MUST NOT |
|------|----------|
| 业务鉴权只用 `ssid`（临时票 `t.` / 证明票 `p.`；**内存**持有） | 传 `token_ref=file:…`（**已废除**）；读写 `temp.*.env`；业务结果回显 `token` / `token_ref` |
| 原始 token（`ALUA_SERVE_TOKEN` / `serve.env`）只用于签发与 `deploy.*` 自举，且须 TLS | 用原始 token 跑日常业务；把临时票落盘 |
| `ssid.whoami` / `ssid.status` / `ensure` / `begin_ctx` 只回显身份或 `has_token` | 把 token / `token_ref` 真值写入 JSON / 日志 / 技能 |
| `ssid.gc` 先 `dry_run`（默认）后显式删除 | 依赖 `ssid.gc` 清理 machine key |

**排障**：`ssid` 不可用 → `AUTH_ERR_SSID_UNAVAIL`（17）/ HTTP 503 `serve.UNAVAILABLE`；缺证/过期 → 先 `ssid.ensure`，`remote.*` 会自动 re-ensure 一次。
跨进程面（`cli` / MCP / `tools.ssid`）回**数值码**（如 `-3`）即 `errcode.E_IO` = XPC 客户端连不到 owner（unix 路径不可达）；先确认本机只有一个 `alua serve` owner、路径为 `/tmp/alua.ssid.xpc.unix.p<port>`，且进程内 lpc 面（同进程）仍正常。

相关：`tools/auth.md`（内部 auth 语义）· `rules/auth.md`（L1 口径）· `tools/remote.md` · `tools/serve_mux.md`。

# end
