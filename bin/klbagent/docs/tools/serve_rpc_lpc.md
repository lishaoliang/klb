# alua RPC / LPC（dispatch 适配）

Cross-machine **RPC** and in-process **LPC** share one tool core: `catalog` + `schema` + `dispatch` + `profiles`.
Adapters only map frames ↔ `{ name, args, id|sequence, via }`.

| Path | Layer | Notes |
|------|-------|-------|
| HTTP `/v1/call`, WS `/v1/ws` | existing | Main contract; unchanged |
| SMP/MNP TEXT `alua.call` | mux P2 bypass | Same JSON envelope; `via=smp_text` |
| `smprpc` / `mnprpc` | **R1** | Native `klbrpc` frames on main port; `via=smprpc`/`mnprpc` |
| `klblpc` module `alua.tools` | **L1** | In-process; `lpc.call` or `klblpc.co_call` → dispatch |
| `alua_lpc` in `run_file` worker | **L2** | Worker-only helper; `readonly_only` |

Do **not** replace `alua --serve` with `klbrpc.serve` (would drop `/v1/call`, token headers, WS).

## Auth (RPC R1)

Same admission as HTTP (`serve.security.auth` → ssid): raw `ALUA_SERVE_TOKEN` (loopback / TLS) or ssid ticket `t.` (temp) / `p.` (proof).

1. Client connects with first frame packtype `RPC_LUA` / `RPC_JSON` (probe → `smprpc` / `mnprpc`).
2. First **REQUEST** method must be `alua.auth` (head name only; no `auth.*` catalog tool).
3. Args: token string, or table `{ token = "<token>", alua_ssid = "<hex>" }` (ssid key also accepts `ssid`).
4. Response: `true` on ok; `false, "<message>"` on deny (connection should stop); ssid owner unreachable → `serve.UNAVAILABLE`.
5. After auth: method name = catalog tool name; first table arg (or `{}`) → `dispatch.call` args.

Plain SMP/MNP TEXT auth (`alua.auth` TEXT head) remains for the bypass path.

## Client smoke (R1)

```lua
local klbsmp = require("klbcore.klbsmp")
local c = klbsmp.connect_rpc("127.0.0.1", 12817)
assert(c)
local ok = c:co_call("alua.auth", os.getenv("ALUA_SERVE_TOKEN"))
-- then:
local r = c:co_call("runtime.ping", {})
c:disconnect()
```

Or URL one-shot after a long-lived session is established; one-shot `klbrpc.co_call` alone cannot auth then call in one disconnect.

`remote.call` stays **HTTP(S) only**. Use **`remote.rpc_call`** for `smprpc`/`mnprpc` (R2).

```
alua call remote.rpc_call {"host":"127.0.0.1","port":12817,"name":"runtime.ping","args":{},"transport":"smprpc"}
```

## LPC L1

Module name: **`alua.tools`** (started with registry / `alua --serve`).

| Client | Call |
|--------|------|
| Tool | `lpc.call {"name":"runtime.ping","args":{}}` |
| Raw | `klblpc.co_call("alua.tools", "tool", "<tool>", args[, opts])` |

## LPC L2 (`run_file` worker)

Worker scripts may:

```lua
local alua_lpc = require("alua_lpc")
local ok, res = alua_lpc.call("runtime.ping", {})
```

Calls use `readonly_only=true` (tools with `min_profile` above readonly are denied). Main env pumps `kenv.loop_once` while waiting for the worker.

| Stage | Status |
|-------|--------|
| C0 / R1 | **done** |
| R2 `remote.rpc_call` | **done**（v0.1.83） |
| L1 `alua.tools` / `lpc.call` | **done**（v0.1.83） |
| L2 `alua_lpc` in worker | **done**（v0.1.83） |
| L3 klist 协作 | 直调 `klist` + `klblpc.post_to`（不经 `klblpc.queue`） |

Proxy entry `:12818` stays separate. See also `serve_mux.md`.
