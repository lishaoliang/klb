# alua ports (L1 · AI only · all platforms)

Port map for **alua serve** and future fixed services. Same on windows / linux / wsl / macos / embedded.

## Serve auto-pick (no explicit port)

| step | ports | rule |
|------|-------|------|
| 1. pool (forward) | `12817` → `12816` → … → `12810` (8) | try bind in order |
| 2. random | `11000`–`12000` inclusive | if pool all busy; max **3** tries |
| 3. fail | — | exit if still unbound |

| CLI | behavior |
|-----|----------|
| `alua --serve [profile]` | auto-pick (pool then random) |
| `alua --serve <port> [profile]` | **only** that port; no fallback |

On success: write actual port to pref **`serve.port`**; stderr listen line includes `mode=explicit|pool|random`; `GET /v1/health` returns `port` + `port_mode`.

## Fixed / reserved

| port / range | kind | owner |
|--------------|------|-------|
| `12810`–`12817` | **serve_pool** | `alua --serve` auto only; **business must not** claim |
| `12818` | **fixed** | proxy entry / embed default (`EMBED_DEFAULT_PORT`) |
| `8080` | **fixed** (user-named) | `alua setup web` config page (`packages/alua_web`); default `plain+tls` same port |
| `≥12819` | **fixed reserve** | future business; claim lowest free, register in design ledger |
| `11000`–`12000` | **serve random** | only after pool exhausted; not for fixed services |
| `22817` (ops) | **ops** | WSL tunnel example; not product serve pool |

## Claiming a new fixed port

1. Do **not** use `12810`–`12817` or the random band.
2. Prefer next free `≥12819` (skip `12818`).
3. Record: `port | kind=fixed | owner | since | notes` in this file.
4. Fixed service on conflict: **fail** (no serve-style random).

## Must / must-not

| MUST | MUST NOT |
|------|----------|
| Prefer reading `serve.port` / health `port` when peer may have drifted | Assume listen is always `12817` after bare `alua --serve` |
| Keep systemd/unit with **explicit** `12817` when the unit must stay pinned | Put proxy or new fixed tools on serve_pool ports |
| Explicit CLI port = sticky, no silent remapping | Use `11000`–`12000` for new fixed product listeners |

Related: `rules/core.md`, `rules/alua_win.md`, `tools/serve_mux.md`, `tools/remote.md`.
