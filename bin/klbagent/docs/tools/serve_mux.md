# serve same-port mux (HTTP / WS / SMP / MNP)

Main `alua --serve` listen uses `klblisten` probe. Preferred port `12817`; bare `alua --serve` auto-picks `12817`→`12810` then random `11000`–`12000` (→ `rules/ports.md`).

| Proto | How | Auth | Notes |
|-------|-----|------|-------|
| http / https | probe + `klbweb` | `X-Alua-Token` / Bearer (+ optional `x-alua-ssid`) | `/v1/call`, `/v1/health`; auth via `serve.security.auth` → ssid |
| ws / wss | HTTP Upgrade `/v1/ws` | same headers on handshake | text JSON envelope |
| smp | magic `*SMP` (non-RPC packtype) | first TEXT `alua.auth` | plaintext only (TLS→HTTP path) |
| mnp | magic `*MNP` (non-RPC packtype) | same as smp | plaintext only |
| smprpc | `*SMP` + first packtype RPC | first REQUEST `alua.auth` | → `dispatch`; see `serve_rpc_lpc.md` |
| mnprpc | `*MNP` + first packtype RPC | same as smprpc | → `dispatch` |

Token shapes: raw `ALUA_SERVE_TOKEN` (loopback only, must be TLS off-loopback) or ssid tickets `t.` (temp) / `p.` (proof) verified via `ssid` LPC (`co_verify_token`). ssid unavailable → `AUTH_ERR_SSID_UNAVAIL` (17) → HTTP 503 `serve.UNAVAILABLE`.

Proxy entry stays on `:12818` (SOCKS / CONNECT / alua-tunnel). Not merged into main port.

## SMP / MNP auth (P2)

1. Client connects with SMP or MNP frames.
2. First **TEXT** frame:
   - `head` = `alua.auth`
   - `body` = raw token / ssid ticket (`t.` / `p.`) **or** JSON `{"token":"<token>","alua_ssid":"<hex>"}` (ssid key also accepts `ssid`)
3. Server replies TEXT `alua.auth` with body `{"ok":true}` or `{"ok":false,"message":"..."}` (`serve.UNAVAILABLE` when ssid owner is unreachable).
4. On deny / wrong first head: connection closed.

## After auth

| Frame | Behavior |
|-------|----------|
| TEXT `head=alua.call` (or empty/`alua.auth` ignored) + JSON Request body | Same envelope as `POST /v1/call`; reply TEXT `alua.call` + Response JSON |
| TEXT other heads | Ignored (bypass) |
| BINARY | Ignored (bypass) |
| MEDIA | Freed with `free_media` (no media hub yet; keeps session) |

`remote.call` remains HTTP(S) only.

## Health / selfcheck (P3)

| Source | Fields |
|--------|--------|
| `GET /v1/health` | live: `mux` (bool), `transport` (array of wired protos), `tls_listen`, `port`, `port_mode` (`explicit`/`pool`/`random`) |
| `runtime.selfcheck` | capability: `serve.mux_capable`, `serve.transport_mux_tls` / `transport_mux_plain`, `serve.mux_active`, `serve.note` |

Typical mux+TLS live list: `https,http,ws,smp,mnp,smprpc,mnprpc`. Plain mux: `http` + the mux suite (no `https`).

Proxy entry stays on `:12818`. Native RPC details → `serve_rpc_lpc.md`.

## Longtest (P3)

Ephemeral same-port mixed smoke (ai temp):

```bash
/home/<user>/.tmp/klbagent/serve_mux_p3.sh
# or via suite:
cd /home/<user>/.tmp/klbagent/longtest && source env.sh && source lib/harness.sh && source tasks/20_serve_mux.sh
```

Covers: health `transport` + selfcheck caps; HTTP `remote.call` ping; SMP TEXT auth+call; smprpc auth+ping; WS `/v1/ws` ping — all on one listen port.

WS client in the smoke may use Python `websocket-client`. Native `klbws` must ignore `error/63` (`KLB_SOCKET_CONNECT`) and `70` (`WBUF_EMPTY`) before `open`/`text` — see `core/tools/remote/transport.lua` `ws_call_once` (v0.1.86+).
