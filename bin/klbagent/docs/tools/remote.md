# remote.call / pull / push / deploy

Call a remote `alua serve` over LAN HTTP(S). Reuses the same JSON envelope as local `alua call` / `stdio`.

## Setup (remote)

```
# token required for ALL profiles
export ALUA_SERVE_TOKEN=...
# optional TLS (same port HTTP+HTTPS via klbhttp tls+plain)
# default cert/key: kenv.pref_path("klua","alua") → ~/.local/share/klua/alua/serve.crt|key
export ALUA_SERVE_CERT=~/.local/share/klua/alua/serve.crt
export ALUA_SERVE_KEY=~/.local/share/klua/alua/serve.key
# ALUA_SERVE_TLS=0 forces plain-only even if cert/key set
# ALUA_SERVE_TLS_RESIGN=1 forces re-selfsign default cert on next serve start
# TLS face (互斥): unset → mnps (default); ALUA_SERVE_TLS_AS_HTTP=1|http → https; =smp → smps
alua --serve 12817 unsafe
```

Token: set `ALUA_SERVE_TOKEN` on both sides (header `X-Alua-Token` or Bearer). **Required** to start serve; `/v1/call` and `/v1/health` both check it.

Endpoints: `POST /v1/call`, `GET /v1/health`, WebSocket `GET /v1/ws` (text JSON envelope, same token headers). Preferred port `12817`; bare `alua --serve` may bind pool/random (→ `rules/ports.md`; pref `serve.port` / health `port`). Main listen uses `klblisten` probe mux. **Default TLS face = mnps**（与客户端默认 `scheme=mnps` 对齐；`ALUA_SERVE_TLS_AS_HTTP=http` 可改回 https 面）。明文 `http`/`smp`/`mnp` 仍可同口；https/ws 在 mnps TLS 面下不可用。Proxy entry stays on `:12818` (not merged).

## Client: remote.rpc_call (R2)

Native SMP/MNP RPC on the same serve port (`smprpc` / `mnprpc`). Session: `alua.auth` then tool. Prefer this for `smprpc`/`mnprpc`; do not pass those as `remote.call.scheme` yet.

```
alua call remote.rpc_call {"host":"127.0.0.1","port":12817,"name":"runtime.ping","args":{},"transport":"smprpc"}
```

Needs `profile≥net` and `ALUA_SERVE_TOKEN`. See `docs/tools/serve_rpc_lpc.md`.

`tls=true`: C mux 的 TLS 单路只投 MNP/SMP TEXT（线上无 `mnprpcs`/`smprpcs` RPC 面），故本工具改走同协议 TEXT（`mnprpc`→`mnps`，`smprpc`→`smps`），返回 `protocol` 字段标明实际面。`tls=false` 才走原生 RPC 会话。

## Client: remote.call

Needs local `runtime.profile=net` (or higher).

**Auth (ssid only):** wire auth still requires a token on serve. **Usage entry is only `ssid`** (`alua_ssid` / auto ensure). **`token_ref=file:…` is removed** from the public call surface (legacy args ignored). Temp tickets live in **ssid memory only** (no `temp.*.env` read/write). **`auth.*` tools/CLI removed.** `resolve_call_token` binds current `alua_ssid`, issues a **temp** token on the target via TLS+raw (ssid 内沉), and injects it. Loopback uses the same path. On `DENIED`/expired/401, one automatic re-ensure + retry. Raw `serve.env` only with `allow_raw_token=true` (TLS).

**Named peer (mesh):** pass `peer` instead of `host` when the id is **trusted** in pref `peers.json` (see `tools/peers.md`). Fills host/port/scheme/`cert_sha256`. Auth still via ssid (peer row `token_ref` not used for business). `pending`/`revoked` → error (use `peer.approve` first).

```
alua call remote.call {"peer":"windows","name":"runtime.ping","args":{}}
alua peer list
alua peer approve windows
```

| `scheme` | 行为 |
|----------|------|
| **`mnps`** (default) | TLS + MNP TEXT + 默认 CRYPTO（PC=CHACHA20/44；embedded=AES_CTR/24）；**no** fallback |
| **`auto`** | try **https** first; fall back to **http** only on transport failure (HTTP 401 does not fall back) |
| **`https`** / **`http`** | `POST /v1/call` only; no fallback |
| **`wss`** / **`ws`** | WebSocket `GET /v1/ws` text JSON envelope (`klbws`); handshake header `X-Alua-Token`; **no** fallback |
| **`smps`** / **`smp`** / **`mnp`** | SMP/MNP TEXT（`smps`=TLS）；默认 CRYPTO 同 `mnps` 路径 |

**Trust self-signed (prefer pin):**

| 方式 | 参数 / 环境变量 | 说明 |
|------|-----------------|------|
| 指纹钉扎 | `cert_sha256` / `ALUA_SERVE_CERT_SHA256` | openssl SHA256; **强制 TLS** (`https`/`wss`), 不匹配则失败 |
| CA 文件 | `ca_path` / `ALUA_SERVE_CA` | curl `--cacert` on HTTP path; klbhttp/WS 路径仍多为 VERIFY_NONE |

```
# 取指纹 (本机或隧道口)
openssl x509 -in ~/.local/share/klua/alua/serve.crt -noout -fingerprint -sha256
# 或: echo | openssl s_client -connect 127.0.0.1:22817 2>/dev/null | openssl x509 -fingerprint -sha256 -noout

# Local raw in serve.env is only for issue/deploy (and curl). remote.call auth = ssid only.
export ALUA_SERVE_CERT_SHA256=...   # 去冒号小写亦可
alua call config.set {"module":"runtime","cfg":{"profile":"net","io":"stdio"}}
# ssid auto ensure (do not pass token_ref):
alua call remote.call {"host":"127.0.0.1","port":22817,"name":"runtime.ping","args":{}}
alua call remote.call {"host":"127.0.0.1","port":12817,"scheme":"https","name":"runtime.ping","args":{}}
alua call remote.call {"host":"127.0.0.1","port":12817,"scheme":"wss","name":"runtime.ping","args":{}}
alua call remote.call {"host":"127.0.0.1","port":12817,"scheme":"ws","name":"runtime.ping","args":{}}
```

(WSL2: prefer direct `127.0.0.1:12817` to Linux `alua serve`. Else a local tunnel such as port `22817`; prefer tunnel over Windows portproxy.)

### Relay: device.uart

Peer must run `alua --serve <port> device` (or higher). Same `device.uart.*` names; handle stays in the **peer** process across calls. See `docs/tools/device.md`.

On peer tool failure, `remote.call` result includes `hop=remote`, `peer_tool`, `peer_code`/`peer_name`, and when applicable `uart_err` (stable `uart:<tag>` message, no Lua path).

```
alua call remote.call {"host":"127.0.0.1","port":12827,"scheme":"http","name":"device.uart.open","args":{"path":"/dev/ttyUSB0","name":"console","baud":115200}}
alua call remote.call {"host":"127.0.0.1","port":12827,"scheme":"http","name":"device.uart.transact","args":{"handle":"uart-console","data":"\r\n","until":">","timeout_ms":2000}}
```

## Client: remote.pull / remote.push (chunked files)

Large files: loop `fs.read` / `fs.write` in chunks (default 512KiB, max 2MiB). Same `scheme` rules as `remote.call` (`auto`|`https`|`http`|`ws`|`wss`; via shared `transport.peer_tool`).

```
alua call remote.pull {"host":"127.0.0.1","port":12817,"remote_path":"/tmp/a.bin","local_path":"/tmp/a.bin"}
alua call remote.push {"host":"127.0.0.1","port":12817,"local_path":"/tmp/a.bin","remote_path":"/tmp/a.bin"}
alua call remote.push {"host":"127.0.0.1","port":12817,"scheme":"wss","local_path":"/tmp/a.bin","remote_path":"/tmp/a.bin","ca_path":"/path/serve.crt"}
alua call remote.pull {"host":"127.0.0.1","port":12817,"scheme":"ws","remote_path":"/tmp/a.bin","local_path":"/tmp/b.bin"}
```

`pull` writes local file (`write_fs`). `push` needs peer profile ≥ net.

## Peer: deploy.self_install / deploy.pull_install

Profile **unsafe**; peer needs `caps.deploy_pull` (linux/wsl + rsync + curl|wget).  
**windows / macos / embedded**: `deploy_pull` false — do not use linux `self_install`/`pull_install`/`remote.deploy` for Win.  
**windows**: use `deploy.win_install` + `*-setup.exe` (see below).  
Install on linux/wsl **merges** into `/usr/local` only (no target `--delete`). Stale trees: ops or local `install.mk`.

| tool | source |
|------|--------|
| `deploy.self_install` | absolute `path` to `.tgz` / `.tar.gz` or extracted stage (`bin/alua` or `usr/local/bin/alua`) |
| `deploy.pull_install` | `url` http(s) to `.tgz` (external pack host / CDN) |
| `deploy.source_install` | **Layer D**: local source tree → `make` stage → install (needs `caps.deploy_source`) |
| `deploy.scripts_install` | **Layer E scripts**: rsync `share/klua/klbagent` only + `runtime.reload` (no `.so` hot-swap) |
| `deploy.setup_priv` | **Scheme 1 once**: write limited NOPASSWD `/etc/sudoers.d/alua-deploy` (`rsync`/`ldconfig`/`mkdir`); then `sudo -n` for deploy |
| `deploy.win_install` | **windows only**: absolute `path` or `url` to `klua-*-setup.exe`; silent **full** (`/S`); default async schedule |

**P2 verify / rollback:** before rsync, pin-check stage `share/klua/klbagent/VERSION`; backup overlapping `/usr/local` tops; after rsync run `alua -v` + pin again; on fail restore backup and **never** schedule restart.

| arg | meaning |
|-----|---------|
| `expect_version` | optional; must equal VERSION or appear in `alua -v` |
| `min_version` | optional; numeric floor on VERSION (`0.1.27.202609` style) |
| `restart` | default true; only after verify ok |

**P1 restart:** `serve.schedule_restart` — linux/wsl: prefer `systemctl --user` / `sudo systemctl`; else async **exec** new `alua serve` + kill old pid. **windows**: `schtasks` `/End`+`/Run` `klua\alua-serve` (never fake systemctl). Result: `restart_mode`=`systemctl`|`schtasks`|`exec`|`none`.

```
# same host: stage already on disk
alua call deploy.self_install {"path":"/tmp/usr_local_stage.tgz","restart":true}
alua call deploy.self_install {"path":"/abs/path/to/stage/usr/local","restart":false,"expect_version":"0.1.27.202609"}
# URL pull (optional external)
alua call deploy.pull_install {"url":"http://192.0.2.10:18080/usr_local_stage.tgz","restart":true,"min_version":"0.1.27"}
# Layer D: only serve + local source tree (env for source root / stage dir)
alua call deploy.source_install {"source_tree":"/abs/path/to/source","rebuild":true,"restart":true}
# Layer E scripts: Lua only, hot-reload in process
alua call deploy.scripts_install {"path":"/abs/path/to/source","reload":true}
# Scheme 1 once on target — prefer human CLI (prompts password; not NOPASSWD:ALL)
alua setup sudo
alua setup sudo check
# alias: alua sudo / alua sudo check
# AI/JSON equivalent:
alua call deploy.setup_priv {"check_only":true}
alua call deploy.setup_priv {"confirm":true,"sudo_pass":"..."}
```

After `setup_priv` / `alua setup sudo` succeeds, later `deploy.*` can rely on `sudo -n`. Linux/wsl only.

## Peer: deploy.win_install (win alua)

Profile **unsafe**; **`host_kind=windows` only**. Upgrade pack = `klua-<ver>-setup.exe` (same stage as human release).  
**GUI** installer defaults unchanged. **Bootstrap silent** = NSIS `/S` → full install (all sections; fixed `%LocalAppData%\Programs\klua`).

| arg | meaning |
|-----|---------|
| `path` / `url` | absolute Win path or http(s) to setup.exe (not `.tgz`) |
| `sync` | default **false**: async delayed `/S` (serve can return before installer kills process) |
| `sync=true` | wait for setup; pin-check `klbagent\VERSION`; optional `schedule_restart` |
| `silent_args` | default `/S`; Inno: `/VERYSILENT /SUPPRESSMSGBOXES /TYPE=full /TASKS=aluaServe` |
| `expect_version` / `min_version` | pin (sync verifies; async: verify after serve recovers) |

From-machine trigger (peer win alua unsafe): `remote.push` setup.exe → `remote.call` `deploy.win_install`. **Do not** use `remote.deploy` (linux usrlocal path).

```
alua call deploy.win_install {"path":"C:\\Users\\me\\Downloads\\klua-setup.exe","expect_version":"0.1.111.202610"}
alua call deploy.win_install {"url":"http://192.0.2.10:18080/klua-setup.exe","sync":true,"restart":true}
# from-machine → win peer:
alua call remote.push {"host":"<win>","port":12817,"local_path":"/path/klua-setup.exe","remote_path":"C:\\Users\\me\\AppData\\Local\\Temp\\klua-setup.exe"}
alua call remote.call {"host":"<win>","port":12817,"name":"deploy.win_install","args":{"path":"C:\\Users\\me\\AppData\\Local\\Temp\\klua-setup.exe","expect_version":"<pin>"},"timeout_sec":30}
```

## Client: remote.deploy (alua deploy to peer)

When peer `alua serve` is reachable: local build stage → tar → **same_fs** (`127.0.0.1`/`localhost`) or `remote.push` → peer `deploy.self_install` → `/usr/local` (+ optional async restart via P1). **No** python `http.server`. Forwards `expect_version` / `min_version` to peer. `scheme` on push path matches `remote.call` (`ws`/`wss` ok when `transport=push`).

```
alua call remote.deploy {"host":"127.0.0.1","port":22817,"rebuild":true,"restart":true,"sudo_pass":"..."}
alua call remote.deploy {"host":"127.0.0.1","port":22817,"transport":"push","scheme":"wss","ca_path":"/path/serve.crt","rebuild":false,"restart":false}
```

Result fields: `transport`=`same_fs`|`push`, `remote_path`, `stage`.