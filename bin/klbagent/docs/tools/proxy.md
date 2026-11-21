# proxy tools

Optional package `packages/proxy` + tools `proxy.status` / `proxy.check`.

Design: `packages/proxy/` (根 `init.lua` 取面 `service` / `entry` / `config` / `chain` / `pac` / `stats`; `entry/` + `service.lua`; `chain/` / `config/` / `transport/` / `relay/` / `pac/` / `acl/` / `bench/` / `stats/` / `hop_token.lua`).

| tool / cmd | profile | notes |
|------------|---------|-------|
| `proxy.status` | net | capabilities + default `listen` + `entry_running` |
| `proxy.check` | net | capability; soft-probe default listen; hard-probe `hops`/`proxy`/`ALUA_PROXY` |
| `proxy.bench` | net | full-path multi-round bench; `phases[]` (resolve/connect/auth/pool/hello_connect/first_byte/total); optional `compare_direct` |
| `alua pkg run proxy check [proxy_url]` | — | selfcheck + probe（无参=listen 软探活） |
| `alua pkg run proxy pac [prefer] [port]` | — | 打印 PAC（B-3）；样例 `packages/proxy/pac/alua_proxy.pac` |
| `alua pkg run proxy serve [config path\|@path\|file.lua/.json] [host:]port [user:pass]` | — | SOCKS5 + HTTP CONNECT；默认 `127.0.0.1:12818`；可选入口鉴权 / LAN host / 配置读盘；有 `kco`/`kenv` 时每连接 `fork` + `pump.co_relay_duplex`（浏览器并发） |
| `ALUA_PROXY_CONFIG` | — | 默认配置路径（CLI `config`/`@path`/文件优先） |
| `config.set` module=`proxy` | readonly | `scope=once\|permanent`（默认 permanent）；live→`core.config.proxy` + 运行中 `entry.apply_live`；listen 变口 `restart_needed` |
| `policy` (cfg) | — | 目的地黑/白名单：默认 `mode=blacklist` 且 `deny=[]`=**全部代理**；可切 `whitelist`；同站 **deny>allow**；`deny_action`/`miss_action`=`direct`\|`reject` |

```
alua pkg list
alua pkg run proxy check
alua pkg run proxy check socks5h://127.0.0.1:12818
alua pkg run proxy pac > /tmp/alua_proxy.pac
alua call proxy.status {}
alua call proxy.check {}
alua call proxy.check '{"proxy":"socks5h://127.0.0.1:12818"}'
# full-path latency (phases + summary; uses proxy.json default chain if no hops):
alua call proxy.bench '{"host":"1.1.1.1","port":443,"rounds":3,"compare_direct":true}'
alua call proxy.bench '{"host":"example.com","port":80,"hops":[{"type":"mnps","host":"x.x.x.x","port":12817}],"rounds":3}'
# foreground entry (optional port for multi-hop lab):
alua pkg run proxy serve
alua pkg run proxy serve 12819
# config file (N0): Lua return table / JSON object; sample packages/proxy/config/sample.lua
# (alua C shell rejects --flag for pkg args; use config / @path / *.lua|*.json)
alua pkg run proxy serve config packages/proxy/config/sample.lua
alua pkg run proxy serve @./my_proxy.json 12819
alua pkg run proxy serve ./my_proxy.lua
ALUA_PROXY_CONFIG=./my_proxy.lua alua pkg run proxy serve
# entry auth (N1-1): CLI user:pass 或环境变量 ALUA_ENTRY_AUTH
alua pkg run proxy serve 12818 alice:secret
ALUA_ENTRY_AUTH=alice:secret alua pkg run proxy serve
# LAN listen (N1-2): 非回环必须带鉴权
alua pkg run proxy serve 0.0.0.0:12818 alice:secret
ALUA_ENTRY_HOST=0.0.0.0 ALUA_ENTRY_AUTH=alice:secret alua pkg run proxy serve
```

## PAC (B-3)

1. `alua pkg run proxy serve`
2. `alua pkg run proxy pac > /tmp/alua_proxy.pac`（或用包内样例 `packages/proxy/pac/alua_proxy.pac`）
3. 浏览器：自动代理配置 URL → `file:///tmp/alua_proxy.pac`（或 http 托管该文件）

默认 PAC：局域网/`localhost` → `DIRECT`；其它 → `SOCKS5 127.0.0.1:12818`（回落 `PROXY`）。  
`prefer`：`socks5`(默认) / `http` / `socks5-only` / `http-only`。

## Acceptance (P2 check)

| 步 | 动作 | 合格 |
|----|------|------|
| 1 | `alua call proxy.status {}` | `available=true`；`features.hop_probe` |
| 2 | `alua pkg run proxy serve` 后 `proxy.check {}` | `listen_probe.ok` 或 message 含 listen ok |
| 3 | `proxy.check` + `proxy=socks5h://127.0.0.1:12818` | `probe.ok`；死 hop 有 `failed_at` |
| 4 | `open_tunnel` 多跳失败 | 错误串含 `hop[N]:` |

Outbound chain: `socks5.client` / `http_connect.client` / `ws.client` / `chain.open_tunnel` / `chain.hops_from_proxy`.  
Multi-hop: `open_tunnel(target, { hopA, hopB, ... })` — each hop tunnels to the next hop host:port; last hop to target.  
Hop probe: `chain.probe_hop` / `probe_hops`（含 `type=ws`/`wss` 升级探活）.  
`net.http_*` resolves `ALUA_PROXY` via single-hop shorthand (see `docs/tools/net.md`).

## hop ws / wss (N5 P1)

| 项 | 约定 |
|----|------|
| hop `type` | `ws`（明文）/ `wss`（TCP 后 `upgrade_tls`） |
| 路径 | 默认 **`/alua-tunnel`**（可选 `hop.path`；亦接受 `/v1/tunnel`） |
| 载荷 | WebSocket **binary** 帧承载 **alua-tunnel** HELLO/CONNECT；OK 后裸字节中继 |
| 入口 | 与 HTTP 同口：`GET` + `Upgrade: websocket` → 101 → 隧道（勿与 `alua serve` `/v1/ws` tool RPC 混淆） |
| 探活 | `probe_hop`：完成 101 即可 |
| 单跳 | `open_tunnel(target, {{ type="wss", host, port }})` |

```lua
chain.open_tunnel({ host = "127.0.0.1", port = 9 }, {
  { type = "ws", host = "127.0.0.1", port = 12818 },
})
chain.probe_hop({ type = "wss", host = "127.0.0.1", port = 12820 })
```

## hop mnps / smps / mnp+CRYPTO (N5 P4 · alua↔alua)

| 项 | 约定 |
|----|------|
| 拓扑 | `SOCKS5 → 点A --mnp*|smp*--> 点B → 目标` |
| TLS | `type=mnps`/`smps`（或 `tls=true`） |
| CRYPTO | 与 TLS **正交**：`hop.crypto` |
| `hop.crypto` | `true`=默认（PC=`CHACHA20`/44；embedded=`AES_CTR`/24）；`false`=强制无 CRYPTO；`string`/`{format,key_size,key?}`=显式 format |
| format | `XOR` / `AES_CTR` / `AES_OFB` / `AES_ECB` / `CHACHA20` / `AES_CBC`（XOR/AES_ECB 弱，须显式） |
| 默认 | `mnps`/`smps`：TLS+默认 CRYPTO；`mnp`/`smp`：无 CRYPTO（除非 `crypto=…`） |
| token | **使用入口=`ssid` 证明票**：`hop.alua_ssid` / `hop.client_id` 须为 **16 位小写 hex**；非 hex 逻辑名（如 `cursor-ai-public`）由本机 `machine.make_ssid("hop\|"+名)` 映射为稳定 hex（**不再**静默回落裸 machine_sid）；缺省 → 请求上下文 → 本机 machine ssid。本机 raw 派生 C1 证明票 `p.`（进程内缓存）；SMP/MNP TEXT body 与 WS 头**同时**送解析后的 `alua_ssid`。远端 serve 经 `serve.security.auth` → `ssid.co_verify_token` 验票。**废除** `hop.token_ref=file`。显式 `hop.token` 仅排障。**命令/JSON 勿写原始 token** |
| 暖池死连接 | `hop_pool.take` 后隧道失败 → `drop_key` 剔同 key；`auth early`/`auth timeout` 等瞬时错冷建再试；`auth deny`/`proof mismatch` 等再 `ensure_hop(force)` 重试 |
| 握手 | TEXT `alua.auth` → BINARY `alua-tunnel` HELLO/CONNECT → 泵送（加密在 MNP 层） |
| 入口收 mnps | `listen.protocols` 含 `mnps`：`klblisten` + `tls_as_mnp`；证同 serve |

```lua
-- 点 A 链示例 (鉴权经 ssid; 勿写 token_ref=file)
chains = {
  secure = {
    { type = "mnps", host = "x", port = 12817, alua_ssid = "<ssid>" },
  },
  mnp_xor = {
    { type = "mnp", host = "x", port = 12817, alua_ssid = "<ssid>",
      crypto = { format = "XOR", key_size = 16 } },
  },
  mnps_plain = {
    { type = "mnps", host = "x", port = 12817, crypto = false },
  },
}
```

## C binding (`kproxy`)

| Lua | notes |
|-----|-------|
| `kproxy.tcp_connect(host,port[,timeout_ms])` | blocking TCP; `:send`/`:recv(max[,timeout_ms])`/`:close` |
| `kproxy.listen(host,port[,backlog])` | listener; `:accept([timeout_ms])`/`:close` |
| `kproxy.listen_raw(...)` | alias of `listen` |
| `pump.relay(a,b[,opts])` | Lua 门面 → `kproxy.relay`；非 userdata 时 `relay_poll`（N3-2） |
| `kproxy.relay(a,b[,opts])` | C select 双向泵送（勿在业务直接调用） |
| `kproxy.tunnel_connect(...)` | 预留 C 加密; 现行 `use_lua_tunnel_frame`（握手在 Lua） |

## Same-port demux (N3-3)

`packages/proxy/relay/demux.lua` classifies the first byte on the shared listen (`entry.start` / `kproxy.listen` via `tunnel.server.listen`):

| First byte | Protocol |
|------------|----------|
| `0xA7` (`frame.MAGIC0`) | `alua-tunnel` |
| `0x05` | SOCKS5 |
| ASCII letter | HTTP CONNECT / forward |
| other | close (`unknown protocol`) |

`tunnel.server.listen(host, port)` = real `kproxy.listen` wrapper. `tunnel.server.start(cfg)` forces `protocols={alua-tunnel}` then reuses `entry.start` (tunnel-only port). Same-port keeps all three in `listen.protocols`.

## alua-tunnel frame (N3-1 / N3-2)

魔数首字节 **`0xA7`**（与 SOCKS5/`HTTP` 分流；分类见上节 demux）。

| 字段 | 说明 |
|------|------|
| 头 | `magic0 magic1 ver type len_be16`（6 字节）+ payload |
| HELLO / HELLO_OK/NAK | type `0x01/02/03`；明文 payload=token；加密见下 |
| CONNECT / OK/NAK | type `0x10/11/12`；CONNECT payload=`host_len(1)+host+port_be16` |
| DATA | type `0x20`；仅 PSK 会话；payload=`seq_be32 + ciphertext` |
| 之后（明文） | 裸 TCP 双向中继 |
| 之后（PSK） | DATA 帧 SHA256-CTR 密封中继 |

### PSK 会话（N3-2）

| 项 | 约定 |
|----|------|
| PSK 来源 | hop/`opts.key`，或 `auth.key`，或 `SHA256(alua-tunnel-psk\|user\|u\|p)` / token |
| HELLO crypto | payload=`0x01 + token_len_be16 + token + nonce_c(16)` |
| HELLO_OK crypto | payload=`0x01 + nonce_s(16)` |
| 会话键 | `SHA256(alua-tunnel-sess\|psk\|nc\|ns)` |
| 密封 | 非 TLS；EtM AEAD：`seq(4)+CTR_ct+tag(16)`；CSPRNG 优先 `/dev/urandom`（与旧纯 CTR **不互通**） |

```lua
-- 明文 hop
chain.open_tunnel({ host = "t", port = 80 }, {
  { type = "alua-tunnel", host = "127.0.0.1", port = 12818 },
})
-- PSK hop（两端同 key / 同 user:pass）
chain.open_tunnel({ host = "t", port = 80 }, {
  { type = "alua-tunnel", host = "127.0.0.1", port = 12818, key = "shared-psk" },
})
```

入口侧：`cfg.key` 或 `cfg.auth.key` / `user`+`pass` 与客户端一致时自动建会话。

Default entry port **12818** (≠ `alua serve` **12817**).

## Timeouts / BND / Auth / ACL / Stats

| 项 | 约定 |
|----|------|
| `cfg.timeouts` | `connect_ms`(15000) / `probe_ms`(5000) / `idle_ms`(100000) / `poll_ms`(40) / `accept_ms`(200) / `hop_idle_ms`(3600000) |
| `cfg.hop_pool` | `max`(2)：proxy↔alua（mnp/smp…）鉴权暖会话槽；见下节 Hop long conn |
| SOCKS5 BND | 成功 REP 回读上游 `tcp:local_addr()`；失败回落 `0.0.0.0:0` |
| HTTP 认证 | `auth.mode=userpass`（`user`/`pass` 或 `users`）或 `bearer`（`token`）；客户端 keep-alive 可复用入口连接做多笔 forward |
| listen | `backlog`；`port_fallback`；`port=0` 临时口 + `listener:local_addr`；可选 `listens[]` 多监听器 |
| check | `check.probe_targets[]`；`check.latency.ok_ms/slow_ms` → hop `grade` |
| security | `allow_from`（IP/CIDR）；鉴权失败限速封禁；未知协议静默关；`entry.stats`/`proxy.status.stats` |
| `policy` | 见下节 Destination policy |

## Destination policy (M1)

选链前：`no_proxy` → **`policy`** → `routes` → default chain。

| 项 | 默认 | 说明 |
|----|------|------|
| `mode` | `blacklist` | `whitelist` 可切 |
| `deny[]` / `allow[]` | `{}` | 匹配同 `routes.match`（host / `.suffix` / `*.suffix` / `host:port`） |
| 空 `deny` + blacklist | — | **全部代理** |
| 同站双列 | — | **deny 优先** |
| `deny_action` | `direct` | 或 `reject`（入口报错关连） |
| `miss_action` | `direct` | 仅白名单未命中 `allow` |

```
# 黑名单：局域网后缀直连，其余走链
alua call config.set '{"module":"proxy","cfg":{"policy":{"mode":"blacklist","deny":[".local","intranet.example"]}}}'
# 白名单：仅列出域名走代理
alua call config.set '{"module":"proxy","cfg":{"policy":{"mode":"whitelist","allow":[".google.com"],"miss_action":"direct"}}}'
```

## Limits (N0-3)

下列路径 **默认不经 / 未默认覆盖** 本机点 A（`127.0.0.1:12818`）；配置代理前先看清边界。

| 场景 | 限制 | 现行约定 |
|------|------|----------|
| Cursor Agent / 云侧流量 (C-5) | 云端或 Agent 出站通常 **不经过** 本机 IDE/进程代理 | 仅本机 Cursor 进程若配了 `Http: Proxy` / `ALL_PROXY` 才走点 A |
| 扩展 / MCP 出站 (C-6) | 扩展或 MCP 若 **自带 HTTP 客户端** 且不读系统代理 / `ALL_PROXY` / IDE 代理 | 不经点 A；能配代理时显式指 `127.0.0.1:12818`（SOCKS5 或 HTTP） |
| 默认 listen (A-3) | 入口默认只绑 **`127.0.0.1`** | 局域网其它主机连不上；LAN 见下节 Listen host |

其它已知缺口：UDP（N3-4）。`alua-tunnel` AEAD+CSPRNG、全阶段 `timeouts`、SOCKS5 真实 BND **done**。同口分流（N3-3）**done**。

## Entry auth (N1-1)

入口鉴权关闭默认（`auth.mode=none`）。开启后：

| 协议 | 机制 | 配置 |
|------|------|------|
| SOCKS5 | RFC1929 username/password | `auth.mode=userpass` + `user`/`pass` |
| HTTP CONNECT | `Proxy-Authorization: Basic`；缺凭证回 **407** + `Proxy-Authenticate` | 同上（`basic` 为别名） |

接线方式（任选其一）：

1. CLI：`alua pkg run proxy serve [port] user:pass`
2. 环境：`ALUA_ENTRY_AUTH=user:pass`（`pass` 可含 `:`；按首个 `:` 分割）
3. 配置表：`cfg.auth = { mode = "userpass", user = "…", pass = "…" }`（`entry.start(cfg)`）

客户端示例：

```
# HTTP CONNECT
curl -sS -m 8 -x http://alice:secret@127.0.0.1:12818 --proxytunnel http://127.0.0.1:<local-http>/
# SOCKS5
curl -sS -m 8 --socks5-hostname alice:secret@127.0.0.1:12818 http://127.0.0.1:<local-http>/
```

无凭证或错凭证时应失败（HTTP 407 / SOCKS5 鉴权拒绝）。

## Listen host (N1-2 / A-3)

| 项 | 约定 |
|----|------|
| 默认 | `listen.host=127.0.0.1`（仅本机） |
| LAN | `0.0.0.0` / `*` / 网卡 IP；**必须**同时开入口鉴权，否则 `validate` 失败 |
| CLI | `serve 0.0.0.0:12818 user:pass` 或 `serve 0.0.0.0 12818 user:pass` |
| 环境 | `ALUA_ENTRY_HOST` / `ALUA_ENTRY_PORT`（可与 `ALUA_ENTRY_AUTH` 组合） |

局域网客户端指向 **本机局域网 IP:端口**（不是对端的 `127.0.0.1`），并带上与入口相同的 user/pass。

```
# 本机冒烟 (绑 0.0.0.0, 经 127.0.0.1 访问)
alua pkg run proxy serve 0.0.0.0:12818 alice:secret
curl -sS -m 8 --socks5-hostname alice:secret@127.0.0.1:12818 http://127.0.0.1:<local-http>/
```

## status --probe vs proxy.check (M3 / Todo.2026-40)

| 入口 | 受众 | 行为 | 勿混用 |
|------|------|------|--------|
| `alua status proxy [--probe]` | **运维人**只读摘要 | 模块详情/嵌套; `--probe`=可选 hop **软探活**（摘要级, 不替代能力验收） | 不等于 `proxy.check` 的硬探活/grade |
| `alua call proxy.check …` / `alua pkg run proxy check …` | **能力+验收** | 能力面 + listen 软探活; 显式 `hops`/`proxy`/`ALUA_PROXY` 时 **硬探活** + `grade` | 勿用 `status --probe` 当发布验收 |

`alua status` 总览下的 `proxy:` 仅二级简略（可否服务/口/有无下级）; hop 细节见 `alua status proxy`.

## Outbound TLS (M1 / Todo.2026-36)

出站/隧道就地 TLS（`kproxy.upgrade_tls` / `net.http_*` https over chain）可配:

| 字段 | 含义 | 默认 |
|------|------|------|
| `tls.verify` | `false`/`true` 或 `0`/`1`/`2`(none/optional/required) | **false**（兼容历史 VERIFY_NONE） |
| `tls.sni` / `hostname` | SNI; `false` 关闭 | `true` → 用目标 host |
| `tls.ca_pem` | CA PEM（verify 时宜配） | 无 |
| `tls.alpn` | 字符串 csv 或 `{ "h2","http/1.1" }` | 无 |
| `tls.pin_sha256` | 对端证书 DER 的 SHA256 hex(64) | 无 |

配置层级: `cfg.tls` ← `hop.tls` ← `net.http_*` `args.tls`（后者覆盖）。坏证/pin 不匹配 → 握手失败。  
**范围**: `kproxy` 就地 TLS 主路径; `mnps`/`wss` 走协议栈自有 TLS（同名字段预留, 未全量透传）。

## Upstream pool (原 M2 / Todo.2026-39 → D54)

HTTP **明文绝对 URI 转发**且选链为**直连**时: 客户端 keep-alive 循环结束后将上游 tcp 放入 `pool`（默认 max=16 / idle=30s）, 下次同 `host:port` 优先复用。有 hops 的链不入池。统计见 `entry.stats().pool`。

## Hop long conn (M2 / Todo.2026-43)

proxy↔alua（hop `type=mnp|smp|mnps|smps`）长连接约定:

| 项 | 约定 |
|----|------|
| 暖会话 | 鉴权完成后、CONNECT 前放入 `hop_pool`；下次请求优先 `co_tunnel`；隧道消耗后异步补槽 |
| 保活 | 对端 serve **C Ping**；客户端自动 Pong；泵 `idle` 对 hop 会话对齐 `hop_idle_ms`（禁因 ~100s 泵空闲掐 hop） |
| 空闲自断 | 默认 `timeouts.hop_idle_ms=3600000`（1h）无入口请求 → **客户端**断开暖会话；`0` 关闭暖池 |
| 配置 | `hop_pool.max`（默认 2）；统计 `entry.stats().hop_pool`；`features.hop_long_conn=true` |
| 非范围 | 直连 HTTP `pool`（上节）；多跳中间跳复用；协议多路复用 CONNECT |

## Multi listen (M4 / Todo.2026-41)

| 项 | 约定 |
|----|------|
| 主口 | `listen`（兼容；CLI/`ALUA_ENTRY_*` 只改此项） |
| 附加口 | `listens[]`：与 `listen` 同 schema（`host`/`port`/`protocols`/`backlog`/`mnps_port`/`id`） |
| 仅 `listens` | 允许：首项升为主 `listen` |
| 协议子集 | 各口可分协议，如主口 SOCKS5+HTTP，附加口仅 `http-connect` |
| 口唯一 | 组内 kproxy `port` 与 `mnps_port` 不得冲突 |
| 热更 | `config.set`/`apply_live`：链/路由/鉴权/`protocols` 可热换；**绑定面**（host/port/mnps_port/增减口）→ `restart_needed` |
| 状态 | `proxy.status` / selfcheck 含 `listens`（`listen_group`）；`features.multi_listen=true` |

```lua
return {
  listen = {
    host = "127.0.0.1", port = 12818,
    protocols = { "socks5", "http-connect", "alua-tunnel" },
  },
  listens = {
    { id = "http-only", host = "127.0.0.1", port = 12828,
      protocols = { "http-connect" } },
  },
}
```

冒烟：双口同时起后，SOCKS5→`:12818` 与 HTTP CONNECT→`:12828` 均可到目标。

## Routes (N1-3)

按**目的地**选链（入口 `open_upstream` → `config.select_hops`）：

1. `no_proxy` 命中 → **直连**（优先于 routes；注意 `.local` 会盖住 `*.corp.local`）
2. 按序匹配 `routes[]`（先匹配先生效）
3. 未命中 → `proxy.use` 临时覆盖 → `default` 角色；无则直连

### 链 id 与角色（2026-10）

| 项 | 约定 |
|----|------|
| `chains` key | **随机 6 位** `[a-z0-9]{6}`（勿再用字面 `default`/`debug` 作 key） |
| `default` | 角色字段 → 正式链 id；**hops 与指针不可被普通写覆盖**；**启动默认** |
| `debug` | 角色字段 → **预制**调试链 id（定义可落盘） |
| 临时切链 | `proxy.use`（写 `proxy_use.json`，**不**写 `proxy.json`） |
| 入口重启 | `entry.start` 清除 `proxy_use.json` → 回 `default` |
| 旧配置 | `chains.default` 自动迁随机 id；遗留 `active_chain` 字段剥除 |

`routes[].chain` 仍可写角色名 `default`/`debug`（解析到角色字段），或直接写链 id。

```
alua call proxy.use {"chain":"debug"}
alua call proxy.use {"chain":"default"}
```

## NO_PROXY (N2-3)

入口选路与 `net.http_*` **同一套**匹配（`util.no_proxy_match`）：

| 来源 | 说明 |
|------|------|
| `cfg.no_proxy` | 默认含 `127.0.0.1` / `localhost` / `.local` |
| 环境 | `ALUA_NO_PROXY` > `NO_PROXY` > `no_proxy`（逗号列表；serve/load 时合并进 cfg） |

模式：`*` / `.dom` / `*.dom` / `host` / `host:port`。

| `match` | 含义 |
|---------|------|
| `default` / `*` | 兜底 |
| `host` | 精确主机（大小写不敏感） |
| `.suffix` / `*.suffix` | 后缀（如 `.internal`） |
| `host:port` | 主机+端口精确 |

| `chain` | 含义 |
|---------|------|
| 链名 | 取 `chains[name]` hops |
| `direct` / 空 | 直连（`open_tunnel` 空 hops / `type=direct`） |

## Direct hop (N1-5)

| 写法 | 行为 |
|------|------|
| `open_tunnel(target, nil)` / `{}` | `kproxy.tcp_connect` 直连目标 |
| `{ type = "direct" }` | 同上（可作链中显式直连跳；不可接在已建立隧道之后） |
| `routes[].chain = "direct"` | `select_hops` 返回空 hops → 入口直连 |

```lua
chain.open_tunnel({ host = "127.0.0.1", port = 80 }, nil)
chain.open_tunnel({ host = "127.0.0.1", port = 80 }, { { type = "direct" } })
```

配置示例（Lua/`cfg`）：

```lua
cfg.default = "a1b2c3"
cfg.debug = "d4e5f6"
-- 临时切链: alua call proxy.use {"chain":"debug"}
cfg.chains = {
  a1b2c3 = { { type = "socks5", host = "127.0.0.1", port = 1080 } },
  e7f8g9 = { { type = "http-connect", host = "proxy.corp", port = 8080 } },
  d4e5f6 = { { type = "mnps", host = "127.0.0.1", port = 12819 } },
}
cfg.routes = {
  { match = ".corp.example", chain = "e7f8g9" },
  { match = "api.example.com", chain = "e7f8g9" },
  { match = "default", chain = "default" }, -- 角色名 → cfg.default
}
cfg.no_proxy = { "127.0.0.1", "localhost", ".local" }
```

`pkg run proxy serve` 默认空 chains/routes（直连目标）。多链：`config path` / `@path` / `file.lua|.json` / `ALUA_PROXY_CONFIG` 读 `.lua`（`return {…}`）或 `.json`，再叠加 CLI listen/auth 与环境变量。亦可代码内 `entry.start(cfg)`。

## HTTP forward (N1-4)

入口除 CONNECT 外支持**明文绝对 URI**（`GET http://host/path HTTP/1.1`）：

1. 解析 `http://host[:port]/path`（`https://` 回 501，请用 CONNECT）
2. 去掉 `Proxy-Authorization` / `Proxy-Connection`，改写为 origin-form 发往源站
3. 经 `select_hops` 直连或链，再 `pump.relay`

```
# 明文 GET（不要 --proxytunnel）
curl -sS -m 8 -x http://127.0.0.1:12818 http://127.0.0.1:<local-http>/
# 带入口鉴权
curl -sS -m 8 -x http://alice:secret@127.0.0.1:12818 http://127.0.0.1:<local-http>/
```

## Client smoke (B-2 / A-2)

Terminal 1:

```
alua pkg run proxy serve
```

Terminal 2 (SOCKS5 手动代理等价路径):

```
curl -sS -m 8 --socks5-hostname 127.0.0.1:12818 http://127.0.0.1:<local-http>/
# 或
ALL_PROXY=socks5h://127.0.0.1:12818 curl -sS -m 8 http://127.0.0.1:<local-http>/
```

浏览器 B-2: 手动代理 → SOCKS5 主机 `127.0.0.1` 端口 `12818`（先起 `proxy serve`）。

Cursor C-3（推荐，当前可用）:

```
export ALL_PROXY=socks5h://127.0.0.1:12818
```

Cursor C-1 / C-2（HTTP 代理）:

```
# 明文绝对 URI
curl -sS -m 8 -x http://127.0.0.1:12818 http://127.0.0.1:<local-http>/
# HTTPS / CONNECT
curl -sS -m 8 --proxytunnel -x http://127.0.0.1:12818 http://127.0.0.1:<local-http>/
# IDE: Http: Proxy = http://127.0.0.1:12818
```

## Client acceptance (P3-2)

| ID | 状态 | 要点 |
|----|------|------|
| C-1 / C-2 | done | `http://127.0.0.1:12818`（CONNECT + 明文绝对 URI GET，N1-4） |
| C-3 | done | `ALL_PROXY=socks5h://127.0.0.1:12818` |
| C-4 | done | `ALUA_NO_PROXY` / `NO_PROXY` 豁免直连（`net.http_*` 已冒烟） |
| C-5 | done* | 见上节 Limits：云侧/Agent 通常不经点 A |
| C-6 | done* | 见上节 Limits：自带出站且不读代理则不经点 A |
| B-1 | done | OS 系统代理指 `:12818`（协议同 B-2） |
| B-2 | done | 浏览器手动 SOCKS5/HTTP → `:12818` |
| B-3 | done | PAC：`alua pkg run proxy pac` |
| B-4 | done | 规则扩展样例：SOCKS5 或 HTTP 代理主机 `127.0.0.1` 端口 `12818` |
| B-5 | done | HTTPS 走 HTTP CONNECT 入口 |
| A-1 / A-2 | done | `ALUA_PROXY` / curl socks5h |
| A-3 | done* | 见上节 Limits：默认仅 `127.0.0.1`；LAN 须改 listen + 鉴权 |

# end
