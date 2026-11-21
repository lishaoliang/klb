# L1 auth rules (AI)

| Term | Meaning |
|------|---------|
| **原始 token** | `ALUA_SERVE_TOKEN` / `serve.env`；**仅**签发授权临时 token、自举升级 |
| **临时 token** / **token** | `ssid.ensure`（内部 `core.auth.issue`）；日常业务鉴权；6h 滑动；一客户端一凭证；**仅内存**（`mem:<id>`；**不**读写 `temp.*.env`；重启须重签） |

**Must**

- 日常命令/JSON **勿**写原始 token；业务鉴权**唯一入口=`ssid`**（**废除**对外 `token_ref=file`；**废除** `auth.*` 工具 / `alua auth` CLI）
- **原始 token 必须走 TLS**（`mnps`/`smps`/`https`/`wss`）；禁明文口带 raw
- **嵌入式例外**（`host_kind=embedded`）：内部签发直接返回原始 token 作临时 token，业务面免 TLS/免白名单裁剪
- **嵌入式原始 token 自动生成**：embedded 且无 token（env / `serve.env`）时，serve 启动自动生成 **16 位**原始 token 并追加 `pref/serve.env`（chmod 600）；非嵌入式仍须显式 token
- 原始 token **勿**作日常业务默认鉴权
- serve 入口鉴权经 `serve.security.auth` → ssid；`core.auth` **仅供 ssid 内调**
- `ssid.status` / 日志 **禁**打印 token 真值
- **ssid 唯一使用入口**：`remote.*` / MCP 缺省经 ssid 对目标 host TLS 签发并注入；**禁止**调用方传 `token_ref=file`；DENIED/过期自动 re-ensure 一次

**Code**：入口 `verify(..., {tls=})` 拒明文 raw；`dispatch` 对 `kind=raw` 仅放行 `ssid.*` / `deploy.*`（及门禁探活最小集）；`kind=temp` 生效档=`min(进程档, 签发 profile)`，可选 `caps.categories` / `caps.tools` 再裁。

**See** `docs/tools/ssid.md`（对外唯一入口）· `docs/tools/auth.md`（内部 `core.auth` 语义）· `docs/tools/serve_mux.md`（入口验票面）；仓内 `Todo.2026-33` / **D49–D52**。
