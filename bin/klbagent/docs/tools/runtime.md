# runtime tools

| tool | profile | notes |
|------|---------|-------|
| `runtime.version` | readonly | script version `base.ym` |
| `runtime.ping` | readonly | `{ pong=true }` |
| `runtime.set_profile` | readonly | `scope=once` (default): live override only; `scope=permanent`: write `runtime.json` + live; raise to **unsafe** needs `confirm=true`; serve 无 argv 档时重启跟 pref |
| `runtime.ai_bootstrap` | readonly | L0 `bootstrap.md` text + docs manifest + host_reflection + reconcile (`status`/`user_prompt`) + `ack_required`/`gate` |
| `runtime.ai_load_ack` | readonly | Ack L0 for serve/stdio hard gate; unlock business tools |
| `runtime.selfcheck` | readonly | install/env sanity; `host` + `proxy` + `serve` + **`ai_load`** + **`ai_load_gate`** + `warn` |
| `runtime.hostinfo` | readonly | `host_kind` / `virt` / `arch` / `network` / `caps` |
| `runtime.cwd` | readonly | process cwd |
| `runtime.env_get` | readonly | one env var; sees `env_set` overlay |
| `runtime.env_set` | **net+** | pref overlay (`env_overlay.json`); `value` or `unset=true`; applies to later `env_get` and `unsafe.exec` |
| `runtime.tls_resign` | **net+** | re-selfsign serve cert/key (`ktls`); SAN=`alua-serve`/`localhost`/`127.0.0.1` + **本机 hostname/DNS** + **本机 IPv4** + 可选 `san[]`; CN 默认本机短名；`hot`/`restart` 同前 |
| `runtime.reload` | **net+** | clear `package.loaded` by `scope`/`prefix`/`prefixes`; **sticky** `klbagent.core.*`（除 `registry` / `core.config*`）+ `version`；`catalog.clear`+`dispatch.clear` 再注册；返回 `preserved`/`restart_needed`；**不**热换 C/TLS/daemons |
| `runtime.parallel` | readonly | in-process multi-tool fan-in: `calls[{name,args?,id?}]`; mode=`inproc` (serial on Lua VM); max 32; no nesting |

Process-local session (`core.session`) is **host-internal only** (MCP / cli / stdio / serve inject `session_id`); **no** `runtime.session_*` tools. Identity echo → `ssid.whoami`.  
Identity / temp-token tools are `ssid.whoami` / `ssid.ensure` / `ssid.begin_ctx` / `ssid.status` / `ssid.gc` (also category `runtime`) → `docs/tools/ssid.md`.

TLS auto renew on `alua --serve` start (default path only): expired / remaining under 7 days / `ALUA_SERVE_TLS_RESIGN=1`.  
Custom `ALUA_SERVE_CERT/KEY` are never auto-overwritten.  
TLS hot reload: `listen:reload_tls` + `klbweb:reload_tls`（新 accept 用新证；旧连接仍持旧 ctx）。

Log: library `klbagent.core.log` + tools `log.status` / `log.tail` (see `docs/tools/log.md`). `config.set {module:"log", cfg:{…}, scope:"once|permanent"}` controls level / file / remote_level (`once`=live only; default `permanent`=write `log.json` + live).

`host_kind`: `wsl` | `linux` | `windows` | `macos` | `embedded`. Override with env `ALUA_HOST_KIND`.  
`arch`: e.g. `x86_64` / `aarch64` / `armv7l` (orthogonal; ARM board heuristics may yield `embedded`).  
`caps.deploy_pull` is true only on linux/wsl with `rsync` and `curl`/`wget` (forced false on windows/macos/embedded).  
`caps.deploy_source` needs `deploy_pull` plus `make` (for `deploy.source_install`).  
Full matrix (install / upgrade / CRYPTO / AI checklist): **`docs/rules/host.md`** (L1; stub `docs/host.md`).  
AI load order: **`runtime.ai_bootstrap`** / `docs/bootstrap.md` (L0) → `docs/rules/` → `docs/skills/*/SKILL.md`.  
`ai_load.status`: `ok` | `stale` | `missing`. On `stale`/`missing` AI **MUST** show `user_prompt` to the **user**.  
Optional args on bootstrap/selfcheck: `host_docs_revision`, `host_fingerprint`, `host_capable`.  
`GET /v1/health` also returns host fields plus live `mux` / `transport` (see `docs/tools/serve_mux.md`).
`runtime.selfcheck.serve` reports mux capability lists even when serve is not listening.

```
alua call runtime.ai_bootstrap {}
alua call runtime.ai_bootstrap {"host_docs_revision":"0.1.99.202610","host_capable":true}
alua call runtime.hostinfo {}
alua call runtime.selfcheck {}
alua call runtime.selfcheck {"host_fingerprint":"<from last bootstrap>","host_capable":true}
# selfcheck.ai_load: status / user_prompt / manifest / host_reflection
# selfcheck.proxy: { configured, invalid, proxy?, env, no_proxy }; warn may include ALUA_PROXY_invalid
alua call runtime.cwd {}
alua call runtime.env_get {"name":"HOME"}
alua call runtime.env_get {"name":"ALUA_PROXY"}
alua call config.set {"module":"runtime","cfg":{"profile":"net","io":"stdio"}}
alua call config.set {"module":"log","cfg":{"level":"info"},"scope":"once"}
alua call config.set {"module":"serve","cfg":{"tls_face":"mnp"}}
# once (default): live only; permanent: runtime.json + live (serve bare restart follows pref)
alua call runtime.set_profile {"profile":"net"}
alua call runtime.set_profile {"profile":"unsafe","confirm":true}
alua call runtime.set_profile {"profile":"unsafe","scope":"permanent","confirm":true}
alua call remote.call {"host":"<peer>","port":12817,"name":"runtime.set_profile","args":{"profile":"unsafe","scope":"permanent","confirm":true}}
alua call runtime.env_set {"name":"ALUA_DEMO","value":"1"}
alua call runtime.env_set {"name":"ALUA_DEMO","unset":true}
alua call runtime.tls_resign {}
alua call runtime.tls_resign {"hot":true}
alua call runtime.tls_resign {"restart":true}
alua call runtime.reload {}
alua call runtime.reload {"scope":"all"}
alua call runtime.reload {"scope":"packages"}
alua call runtime.reload {"prefix":"klbagent.core.tools.fs"}
alua call runtime.reload {"prefixes":["klbagent.core.tools.","klbcore."]}
alua call config.set {"module":"log","cfg":{"level":"info","to_file":true}}

# runtime.reload scopes (default tools)
#   tools     → klbagent.core.tools.* + re-register
#   packages  → klbagent.packages.* (running pkg daemons still need restart)
#   klbcore   → klbcore.*
#   agent     → tools + packages
#   klbagent  → klbagent.* but sticky core.* (except registry + core.config*) + version
#   all       → klbagent.* + klbcore.* (same sticky)
# Always restart for: alua --serve C/TLS; alua pkg run daemons; .so / bin/alua (deploy + restart)
# core.serve / core.dispatch still sticky (process restart); core.config* hot-reloadable

# parallel fan-in (same process; Lua single-thread => serial dispatch)
alua call runtime.parallel {"calls":[{"name":"runtime.ping","id":1},{"name":"runtime.cwd","id":2}]}
```
