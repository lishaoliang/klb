# packages (optional)

One of **three** root majors under `klbagent/` (`core` / `docs` / `packages`); built-in tools live in `core/tools/` (no root `tools/`). **All optional extensions** go here (not beside package root). Non-mandatory: on embedded ARM set `INSTALL_ALUA_EMBEDDED=1` or `INSTALL_ALUA_OPTIONAL_PACKAGES=0` when installing so `tier=optional` packages are omitted.

## Commands

```
alua pkg list [--json]          # default human table; --json for AI/machine
alua pkg run <name> [args...]
```

## Layout

```
packages/
  init.lua
  <name>/
    package.lua            # name, tier, kind, version, summary
    init.lua               # entry() / register_tools()
```

| field | values |
|-------|--------|
| `tier` | `base` \| `optional` |
| `kind` | `daemon` \| `cli` \| `tools` \| `lib` |

| kind | `alua pkg run` |
|------|----------------|
| `daemon` / `cli` | calls `mod.entry(argv, manifest)` (daemon usually blocks) |
| `tools` | calls `mod.register_tools(argv, manifest)` |
| `lib` | rejected (`require` only; listable) |

## Service hosting contract

Each service runs as **one process per service** (`alua pkg run <name>`). The symbols below are internal: `entry()` uses them, and any host that embeds a service must honour the same rules.

| symbol | needed | contract |
|--------|--------|----------|
| `M.start_embed(opts)` | optional | **non-blocking**: validate → bind → fork accept coroutine → return `ok, err`; never runs the main loop |
| `M.stop_embed()` | optional | **idempotent** teardown, reverse order: listener → app → session cache |
| `M.pump_embed()` | no | only for legacy hosts that delegate looping; the host owns the **single pump**, it does not call per-service pumps |
| `M.entry(argv, manifest)` | for `kind=daemon` | blocking form = `start_embed` + own pump + `stop_embed` |

`opts` carries **this instance's** overrides (`listen` / `port` / `svnserve` / `repo_root` …); defaults still come from `ALUA_*` env. Env is **process-wide**, so single-process multi-instance config must go through `opts`.

| rule | reason |
|------|--------|
| exactly one `kenv.loop_once` call site in the process | nested pump corrupts the event loop (proxy hot path SEGV) |
| start failure = **fail-fast** + roll back already-started services | partial startup must not leave half-open ports |
| `stop_embed` callable while accept coroutines are parked | close listener first, then app, then sessions |

```
alua pkg run proxy serve
alua pkg run svn_https
```


## proxy

Optional `tier=optional` / `kind=daemon`: local SOCKS5/HTTP CONNECT/forward entry + multi-hop chain + PAC + alua-tunnel PSK. Default listen `127.0.0.1:12818`.

```
alua pkg run proxy check
alua pkg run proxy serve
alua pkg run proxy serve config packages/proxy/config/sample.lua
alua pkg run proxy pac
```

Tools: `proxy.status` / `proxy.check` (category net). Contract → `docs/tools/proxy.md`. Design → `packages/proxy/` (root face `init.lua` → `service` / `entry` / `config` / `chain` / `pac` / `stats`; `entry/` + `service.lua`, plus `acl/` / `bench/` / `transport/` / `relay/` / `hop_token.lua`).

## svn_https

### Scheme B (target; P0 contract frozen)

Client stays on **ra_serf** `https://`. **alua replaces** Apache/`mod_dav_svn`. Upstream is **svnserve** (not HTTP-SVN). Not scheme A (TLS-only `svn://`).

| item | frozen (P0) |
|------|-------------|
| package | keep name **`svn_https`** |
| URL | `https://<edge-host>:8443/svn/<repo>/…` |
| repo root | svnserve `--root` = site path (operator-chosen) |
| svnserve | loopback only, default `127.0.0.1:3690` |
| auth | same users in svnserve `passwd`+`authz` and HTTPS Basic; forwarded to ra_svn; no anon; no separate alua user DB |
| accept | `co` / `up` / `ci` / `mkdir` / `delete` via public URL |
| out of scope | full DeltaV; direct site `svn://` publish URL; scheme A |
| phases | P0–P3 **done** (no Apache/`mod_dav_svn` on the edge) |

```
# end-state env (P2+)
export ALUA_SVN_LISTEN=8443
export ALUA_SVN_ADDR=<nic-ip>,127.0.0.1
export ALUA_SVN_SVNSERVE=127.0.0.1:3690
export ALUA_SVN_EDGE_HOST=<edge-host>:8443
alua pkg run svn_https

# client (self-signed)
svn co https://<edge-host>:8443/svn/<repo> ./wc \
  --username <user> --password '…' \
  --non-interactive --trust-server-cert-failures=unknown-ca,cn-mismatch
```

TLS: same as `alua serve` (`serve.resolve_tls` / pref `serve.crt|key`). Loopback access uses `https://localhost:8443/svn/…` (cert SAN includes `DNS:localhost`); other hosts install the edge `serve.crt` as svn `ssl-authority-files` to remove the unknown-CA warning.

| env (end-state) | meaning |
|-----------------|---------|
| `ALUA_SVN_LISTEN` | listen port (default 8443) |
| `ALUA_SVN_ADDR` | bind address list, comma/whitespace separated (default `0.0.0.0`); once set only listed addresses are bound, e.g. `<nic-ip>,127.0.0.1` (one NIC + loopback, other NICs not exposed) |
| `ALUA_SVN_SVNSERVE` | `host:port` of loopback svnserve (default `127.0.0.1:3690`) |
| `ALUA_SVN_REPO_ROOT` | repo parent dir (e.g. `svnserve --root` value); enables 404 for unknown repo vs 401 for auth failure |
| `ALUA_SVN_EDGE_HOST` | Destination/Location rewrite host[`:port`]; default = request `Host` |
| `ALUA_SVN_MAX_BODY` | max request body bytes (default 512MiB; klbweb layer; must be >= `ALUA_SVN_FILE_MAX_BYTES`) |
| `ALUA_SVN_FILE_MAX_BYTES` | max single PUT body bytes after svndiff expansion (default 512MiB; 0=unlimited) |
| `ALUA_SVN_TXN_MAX_BYTES` | max cumulative txn PUT bytes (default 4GiB; 0=unlimited) |
| `ALUA_SVN_CAT_MAX` | max single file cat in update/GET (default 64MiB; 0=unlimited) |
| `ALUA_SVN_TXN_TTL` | abandoned txn purge seconds (default 3600; 0=disable) |
| `ALUA_SVN_LOCK_DB` | optional path for persistent lock table (`repo`, `path`, `token`, `owner`, `created`, `expires`); unset = memory only |
| `ALUA_SVN_PROXY_DEBUG=1` | stderr debug |

Build **alua** without **ksvnra** (embedded, no svn_https C): `make -C klb/proj/alua ALUA_WITH_KSVNRA=0`; `svn_https` cannot run without it.

**Do not** use `ALUA_SVN_UPSTREAM` (deprecated; ignored with stderr warning; use dav+ksvnra).

### P2 implementation

`init.lua` serves HTTP-SVN **in-process** (`dav.lua` + preloaded `ksvnra` C ra_svn client → loopback svnserve). **Read** (`co` / `list`) and **HTTPv2 write** (`ci` / `mkdir` / `delete` / `move` via POST create-txn → MKCOL/PUT/DELETE/COPY/MOVE → MERGE → `ksvnra.commit`) are the main path. No python bridge. Classic DeltaV (`CHECKOUT`/`MKACTIVITY`/…) is **not** advertised and returns `501` + DAV error XML; `list-report` also stays `501` (`svn/list` is not advertised); `replay-report` stays `501`; `dated-rev-report` / `get-deleted-rev-report` are served; `DAV: 1,2` advertises only implemented capabilities; `GET` supports single and multiple `Range` (`multipart/byteranges`); common svn error codes (`160013`/`165000`/`170001`/`185005`/`185008`/`195012`) map to `<S:…>` DAV error tags with upstream messages; each request writes an access-log line.

**ksvnra source (P0 layout)**: `klb/src_apps/alua/svn_ra/` — `core/svn_ra_{md5,wire,session,log,commit}.c` + `svn_ra_klua/`; see the sources above.

## mcp

Optional **lib** package (listable): MCP (Model Context Protocol) interop. Public face = `require("klbagent.packages.mcp").server()` / `.client()` / `.http()` (getter functions; `base/` / `schema/` are internal, no sub-module re-export); CLI entry `alua mcp`.

| direction | command | transport |
|-----------|---------|-----------|
| server (export alua tools to MCP hosts) | `alua mcp serve [--profile P]` | stdio JSON-RPC 2.0 |
| client (consume external MCP tools) | `alua mcp probe --url URL` / `alua mcp list --url URL [--token T]` / `alua mcp call --url URL --tool NAME [--args JSON]` / `alua mcp listen --url URL` (GET SSE) / `alua mcp close --url URL` (DELETE) | Streamable HTTP (POST + GET SSE + DELETE; JSON or multi-frame SSE; `Mcp-Session-Id`) |

**AI help (docs.read):** `tools/mcp.md` (L3) · L2 `skills/mcp/SKILL.md` — Cursor / CodeBuddy / Trae / VS Code / Claude / Windsurf host JSON.  
AI host standard names (`host_tool=mcp-xx`, unknown=`mcp-unknown`) → `tools/mcp.md`.

Server methods: `initialize` / `ping` / `tools/list` / `tools/call` / `resources/list|read` / `resources/templates/list` / `prompts/list|get` / `logging/setLevel`; also `notifications/cancelled` / `notifications/progress` and `roots/list` (server→client). Resources use `alua://docs/<rel>` (via `docs.*`); prompts include built-in `alua-ai-bootstrap` / `alua-mcp-overview`. HTTP face: `alua serve` `POST|GET|DELETE /mcp` (Bearer / `X-Alua-Token` + optional `X-Alua-Ssid`; POST = JSON or multi-frame SSE, GET = SSE listen, DELETE = end session; loopback Host may use raw token, non-loopback plaintext → 401). Tool `inputSchema` from `catalog.args_schema` via `packages/mcp/schema/`. Handshake binds `core.session` attrs and `ssid` (XPC client); `ssid.whoami` echoes `host_tool` / session fingerprint.

stdio: `serve_entry(opts)` forks the read loop (embed); `serve(opts)` = `serve_entry` + `co_util.run_host_loop` (CLI → `hold` host pump; standalone → `klbenv.co_main_loop`). `alua mcp serve` calls `serve` until stdin EOF / `SIGTERM` → `kenv.exit`.

`serve` defaults to `--profile readonly`; call `runtime.ai_bootstrap` → `runtime.ai_load_ack` before business tools (the MCP channel is outside the stdio/serve ack gate). Host MCP config help → `docs.read {"path":"tools/mcp.md"}`.

```
alua mcp serve --profile net
alua mcp probe --url http://127.0.0.1:8931/mcp --json
alua mcp list --url http://127.0.0.1:8931/mcp --json
alua mcp call --url http://127.0.0.1:8931/mcp --tool demo.echo --args '{"x":1}'
alua mcp listen --url http://127.0.0.1:8931/mcp --timeout 30   # GET SSE
alua mcp close --url http://127.0.0.1:8931/mcp                 # DELETE session
```

## alua_ui

Optional **cli** package: **human** ops UI for local/remote alua config and status (product name **alua-ui**). With `kg2de.klbui` opens **in-process**; else **g2deapp** + `boot.lua`. Pages use `klbcore.klbui`.

```
alua ui
alua gui
alua pkg run alua_ui
```

Human entry: **`alua ui`** / **`alua gui`** (same as `alua pkg run alua_ui`). **M1–M6** done as **D3/D4**.

| path | role |
|------|------|
| `ui/main.lua` | `kg2de.klbui` host loop / optional `g2deapp` spawn |
| `ui/bridge.lua` | local `dispatch` / remote `remote.call` via `set_target` |
| `ui/app.lua` | multi-page nav (home / serve / runtime); page_tmpl host flow |
| `pages/page_tmpl.lua` | page module template |
| `pages/home.lua` | overview + target switch (local / peer / host) |
| `pages/serve.lua` | serve config (`tls_face` / scope) |
| `pages/runtime.lua` | `runtime.set_profile` (+ unsafe confirm) |
| `boot.lua` | `g2deapp` entry when in-process plugin missing |

## g2de_agent

Optional **tools** package for **g2de** engine debug control (`g2decore.agent` host). Register via `register_embed(catalog, dispatch)` or `alua pkg run g2de_agent` (registers tools only; host must be set for snapshot).

| tool | profile | summary |
|------|---------|---------|
| `g2de.versions` | readonly | kg2de / g2decore / klbagent version strings |
| `g2de.selfcheck` | readonly | embed host + g2de embed availability |
| `g2de.snapshot` | readonly | read-only agent/app state from host |
| `g2de.screenshot` | readonly | capture backbuffer TGA; `data_base64` + width/height for AI visual check |
| `g2de.input.state` | readonly | merged keyboard/mouse (inject + hardware) |
| `g2de.control.pause` | net | pause `on_frame` loop |
| `g2de.control.resume` | net | resume after pause |
| `g2de.control.step` | net | queue `n` frames while paused (async) |
| `g2de.control.inject_key` | net | inject key (`scancode` or `key` name) for `g2decore.input.key_down` |
| `g2de.control.inject_mouse` | net | inject mouse x/y/button for `g2decore.input.mouse_*` |
| `g2de.control.clear_input` | net | clear all injected key/mouse overrides |
| `g2de.control.tap_key` | net | key down → sync `step` frames → key up |
| `g2de.analyze` | net | one-shot: pause → inject → sync step → snapshot + screenshot + input_state |

Host callbacks (`set_host`): `versions`, `selfcheck`, `snapshot`, `screenshot`, `input_state`, `control_*` — provided by `g2de/bin/g2decore/agent/init.lua`. Package **does not** `require("g2decore")`. Embed default profile **`net`** (via `opts.agent.profile`).

**AI 验图推荐**: `g2de.analyze` with `{ step=1, keys=[{key="up",down=true}], screenshot=true }` — sync frames inside one HTTP call. Manual: `pause` → `inject_key` / `tap_key` → `screenshot`.
