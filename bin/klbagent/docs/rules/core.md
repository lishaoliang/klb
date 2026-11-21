# L1 core rules (AI only)

## Profiles (permission)

Levels: `readonly` < `net` < `device` < `unsafe`.  
Tools declare `min_profile`; lower → error code `20`.  
Pref JSON: `runtime` / `log` / `proxy` / `serve` / `peers` (+ `env_overlay` via `runtime.env_set`).  
`config.get` / `config.set` module=`runtime|log|proxy|serve|peers`. Secrets → `serve.env` / `peers.<id>.env` (not JSON).  
Mesh: `peer.list|ping|approve|revoke` + `remote.* peer=<id>` when trusted → `rules/peers.md`.  
**≠** `host_kind` (morph). Never put `wsl` / `windows` into profile names.

**runtime.json 字段**

| 键 | 默认 | 说明 |
|----|------|------|
| `profile` | `readonly` | 权限档 |
| `io` | `stdio` | IO 形态 |
| `loop_sleep_ms` | `5` | `kenv` 主循环休眠上限(ms)；CLI 启动与 `config.set` 热生效；`<1` 回落默认；**已跑** `kthread` 不回溯，**新** `kthread.start` 继承当前值 |

**config.set `scope`**（统一契约；默认 **permanent** 兼容旧写盘）

| `scope` | 行为 |
|---------|------|
| `once` | 仅进程 live；不写 pref；重启丢失 |
| `permanent`（默认） | 写对应 JSON + live；`serve.tls_face` / `proxy` listen 变口可能 `restart_needed` |

Partial `cfg` merges onto current. Raise `runtime.profile` → **unsafe** needs `confirm=true`.

```
alua call config.set {"module":"log","cfg":{"level":"info"},"scope":"once"}
alua call config.set {"module":"serve","cfg":{"tls_face":"http"}}
alua call config.set {"module":"runtime","cfg":{"profile":"net","io":"stdio"}}
alua call config.set {"module":"runtime","cfg":{"loop_sleep_ms":1}}
alua call config.set {"module":"runtime","cfg":{"loop_sleep_ms":10},"scope":"once"}
alua call config.get {"module":"runtime"}
```

**Live / permanent profile** (authorized call already has token): `runtime.set_profile`（薄封装；默认 **once**）

| `scope` | 行为 |
|---------|------|
| `once`（默认） | 仅进程 `set_override`；不写 pref；重启丢失 |
| `permanent` | 写 `runtime.json` + 立即生效；`alua --serve` **未**在 argv 点名档位时，重启跟 pref |

Raise to `unsafe` needs `confirm=true`. Explicit `alua --serve … unsafe` still overrides pref for that process.

```
alua call runtime.set_profile {"profile":"net"}
alua call runtime.set_profile {"profile":"unsafe","confirm":true}
alua call runtime.set_profile {"profile":"unsafe","scope":"permanent","confirm":true}
```

## Error codes

| code | meaning |
|------|---------|
| 0 | ok |
| 2 | not implemented |
| 10 | bad request |
| 20 | denied (profile / side_effects / planned ai_load ack) |
| 30 | tool not found |
| 40 | tool failed |

## docs tools

```
alua call docs.list {}
alua call docs.read {"path":"bootstrap.md"}
```

- Paths relative to `docs/` only.
- **Forbidden**: `..`, absolute paths.
- Audience: AI only (not human manuals).

## Must / must-not

| MUST | MUST NOT |
|------|----------|
| Follow L0→L1→L2→L3 | Treat editor-only skills as alua runtime docs |
| Branch on `host_kind`+`caps` for deploy | Assume `/usr/local` on windows/embedded |
| Use catalog for tool contracts | Invent a second OS-specific tool vocabulary |
| Read L3 only when needed | Paste all `docs/tools/*` every turn |
| Self-check via `runtime.ai_bootstrap` / `selfcheck.ai_load` | Ignore `stale`/`missing` without prompting the **user** |
| If host can persist rules: soft-link/pointer to `klbagent/docs` | Full-copy L3 bodies into host rules |
| Explain / ask in text or JSON | Open UI dialogs, modal popups, or block waiting for interactive input (AI and tools alike) |

Reconcile args: `host_docs_revision` / `host_fingerprint` / `host_capable` on `runtime.selfcheck` or `runtime.ai_bootstrap`.

## Commands (short)

Help: `alua` / `alua -h` = command list (`core/cli/init.lua` `_usage()`). `alua <cmd> -h` / `--help` / `help` = that command only (`M.help` in `core/cli/<cmd>.lua`).

Per-command human render: `core/cli/<cmd>.lua` (`M.run`; same shape as `status.lua`); shared `cli/fmt.lua`. `init.lua` dispatches only. Shell getopt uses `+hvp:` so `-h` after the subcommand is not eaten as shell help.

**Output format** (docker-like): default = human-readable text; `--json` / `json` / `-j` = JSON for AI / machine API. Put format flags **after** the subcommand. Shell opts (`-h/-v/-p`) stay **before** the command. Internally: structured data first, then render (or encode).

```
alua status [module [sub...]] [--json] [port]
alua --serve|-s [port] [profile]          # listen (unique service entry; def 12817 readonly)
alua serve yes|no [--port N] [--profile P] # persistent enable/disable (no listen)
alua pkg list [--json]
alua pkg run <name> [args...]
alua peer list|ping|approve|revoke … [--json]
alua ssid whoami|ensure|status|begin|gc … [--json]
alua proxy check [--json] [--force]
alua mcp serve|list|call|probe|listen|close …
alua ui | alua gui
alua setup sudo [check] | alua sudo [check]
alua setup [sudo|serve|proxy|web] [--json]
alua setup proxy yes|no [--port N]
alua setup web yes|no [--listen PORT:PROTO] [--addr IP] [--allow CIDR] [--deny CIDR] [--acl-default allow|deny]
alua docs [path] [--json]
alua list [category] [--json]
alua categories [--json]
alua call <tool> [json_args]    # always JSON
alua run <path.lua> [args...]   # always JSON
alua stdio                      # always JSON line protocol
```

`alua setup` = human maintenance entry (`--json` = `schema=alua.setup.v1`): `sudo` (target limited NOPASSWD, linux/wsl only), `serve` (persistent enable/disable, `--port`/`--profile`), `proxy` (persistent, `--port`), `web` (config web page; port group `PORT:PROTO`, default 8080 http+https same port; page + `/api/v1/*`; ACL allow/deny `IP|CIDR`, deny wins, `--acl-default` = verdict when allowlist empty; acl hot-reloads in ~1s from `web.json`, listen/addr/cert still need restart). Serve backend: linux systemd `system|user` (user unit auto-generated; system scope needs sudo → interactive tty password or `ALUA_DEPLOY_SUDO`), embedded SysV `/etc/init.d/S99_alua` (alua-managed marker; foreign script refused), Windows Task Scheduler `klua\alua-serve`; proxy backend: systemd / Task Scheduler only (embedded → serve only). `alua status` reports the same backend via `unit.kind` (`systemd` / `initd` / `task_scheduler`).

`alua status`: **ops-first** brief overview (serve/unit/proxy/peers/packages/caps). Under `proxy:` indented `serve` (yes/no + host:port) and `downstream` (yes/no + chain count); **no** hop host/token here. `alua status proxy` = detail; nested `proxy chains|listen|routes|stats|hop`; also `status serve|peers|unit|config`. Optional `--probe` = ops soft hop probe (**≠** `proxy.check` hard probe/grade; see `docs/tools/proxy.md`). JSON carries `schema=alua.status.v1`. Does **not** print token value.

Serve: preferred port `12817`; bare `alua --serve` auto-picks pool `12817`→`12810` then random `11000`–`12000` (see `rules/ports.md`); `alua serve yes|no` only toggles the persistent unit and never listens. Explicit `<port>` = that port only. **all profiles** need `ALUA_SERVE_TOKEN` (embedded auto-generates a **16-char** raw token into `pref/serve.env` when absent). Details → L2 `skills/remote` / L3 `tools/serve_mux.md`.
