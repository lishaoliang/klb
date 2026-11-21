# auth / 授权模块（内部）

**对外工具/CLI `auth.*` / `alua auth` 已删除。** 使用面唯一入口 = **`ssid`**（`ssid.*` / `alua ssid` / remote 内沉）。

本页描述 **`klbagent.core.auth`** 内部语义（仅供 `ssid.serve` / owner 内调）；业务方勿直接 `require` 或 `alua call auth.*`。

**原始 token** = `ALUA_SERVE_TOKEN`（根凭证）。  
**临时 token**（日常简称 **token**）= `ssid.ensure` / 内部 `auth.issue` 所得；业务面入口 = `ssid.*`；线上传输走 `X-Alua-Token` / `Authorization: Bearer`（可附 `x-alua-ssid`）。`alua.auth` 仅为 SMP/MNP 首帧 TEXT 头名，**不是**工具。

| 项 | 约定 |
|----|------|
| 原始用途 | **仅** ① 签发授权临时 token；② **自举升级**；禁日常业务 |
| 原始 + TLS | 线上使用原始 token **必须 TLS**（`mnps`/`smps`/`https`/`wss`）；禁明文带 raw |
| 嵌入式例外 | `host_kind=embedded`：内部签发直接返回原始 token（`ttl_ms=0`），业务面按临时 token 放行（免 TLS/免白名单裁剪） |
| 嵌入式自动生成 | `host_kind=embedded` 且无 token 时，serve 启动自动生成 **16 位**原始 token 并追加 `pref/serve.env`（chmod 600）；非嵌入式仍须显式 token |
| TTL | 临时 **6 小时**；每次校验成功 **滑动续期** |
| 隔离 | **一客户端一活跃临时凭证**（同 `client_id` 再签发则替换） |
| 表 | 仅 **`alua serve` 进程内**；**每 10 分钟** 扫过期 |
| 对外 | **仅 `ssid.*`**；入口验票经 `serve.security.auth` → ssid LPC（同进程）；CLI/MCP 经 XPC；禁旁路直读表 |
| 命令行 | **勿**在 JSON/Shell 写原始 token（见 D48）；业务鉴权**唯一入口=`ssid`**（废除对外 `token_ref=file`） |

## 已删除的直接接口

| 旧名 | 替代 |
|------|------|
| `auth.issue` / `alua auth issue\|ensure` | `ssid.ensure` / `alua ssid ensure` |
| `auth.revoke` / `alua auth revoke` | `ssid` destroy / gc（经 ssid 门面） |
| `auth.status` / `alua auth status` | `ssid.status` / `alua ssid status` |
| `auth.verify` | 入口验票 / `ssid` 内 `verify_token`（非公开工具） |
| `auth.whoami` / `alua auth whoami` | `ssid.whoami` / `alua ssid whoami` |

## AI 自维护 / ssid 唯一入口（强制）

线协议（HTTP/MUX）**必须** token + `verify`。  
**使用面唯一入口 = `ssid`（`alua_ssid` / `ssid.*` / remote 内沉）**。  
**已废除**对外 `token_ref=file:…`（含 `file:temp.*`）作为调用参数；临时票仅 **ssid 进程内存**（`mem:<id>`；重启须重签），**不再**读写 `temp.*.env`。

| 项 | 约定 |
|----|------|
| **谁维护** | `ssid` → `remote.resolve_call_token`：当前 `alua_ssid` → 对目标 host **TLS+raw** 内部签发 → 注入临时证 |
| **调用** | `remote.call {"host":…,"name":…}` 或 `ssid.ensure` / `ssid.begin_ctx`；**禁止**再传 `token_ref` |
| **何时 ensure** | 缺证自动；`DENIED`/expired/401 → 自动 re-ensure 一次 |
| **loopback** | 同径签发+校验，不豁免 |
| **仍用 raw** | 仅签发面 / deploy 自举 / `allow_raw_token=true`（须 TLS） |

```bash
# 例：公网业务 (仅 host/name; ssid 内沉)
alua call remote.call '{"host":"192.0.2.10","port":12817,"scheme":"mnps","name":"fs.read","args":{"path":"/usr/local/share/klua/VERSION"}}'
alua call ssid.ensure '{"profile":"net"}'
alua ssid whoami
```

## serve 入口

HTTP `/v1/call`、`/v1/ws`、MUX `alua.auth`：**统一** 经 `serve.security.auth` → ssid 验票（须传 TLS 面；**原始 token 非 TLS 拒**；临时可滑动续期）。

| 项 | 约定 |
|----|------|
| 票形态 | `t.`（临时会话票）/ `p.`（证明票）走 ssid LPC `co_verify_token`；其余按原始 token 本机比对 |
| ssid 传输位 | HTTP 头 `x-alua-ssid`；mux TEXT body JSON `alua_ssid`/`ssid`；RPC 首参 `{alua_ssid=…}`/`{ssid=…}` |
| ssid 不可用 | `AUTH_ERR_SSID_UNAVAIL`（17）→ HTTP 503 `serve.UNAVAILABLE`；mux/rpc 回同码 |
| 入口别名 | `alua.auth` 仅为 SMP/MNP/RPC 首帧 head / 首 REQUEST 方法名，**无**对应 catalog 工具 |
**嵌入式例外**：`host_kind=embedded` 时原始 token 免 TLS，入口按临时 token 放行。

**原始 token 工具面**：仅 `ssid.*`、`deploy.*` 及门禁/探活最小集；日常业务须先 `ssid.ensure` 换临时 token。
**嵌入式例外**：原始 token 即临时 token，业务面免白名单裁剪（`dispatch` 跳过 `allows_raw_tool`）。

## 临时 token 工具面裁剪（D52）

| 项 | 约定 |
|----|------|
| **生效档** | `min(进程 profiles.active, 签发 profile)`；serve 为 unsafe、签发 `net` 时按 **net** |
| **caps** | 可选；`{ categories={...}, tools={...} }` 为**允许列表**；二者皆空=仅按档 |
| **tools 匹配** | 精确名，或 `"fs."` 这类前缀（末尾 `.`） |
| **raw** | 仍走白名单；不按签发档收紧进程档 |

## 正交

≠ peer mesh；≠ proxy hop CRYPTO；≠ 仅改进程全局档（请求级裁剪不写 pref）。
