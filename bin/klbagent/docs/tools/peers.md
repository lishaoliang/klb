# peer.* / peers.json (mesh)

Named peer registry under pref `peers.json`. **Not** canceling tokens — approve fills pin + marks `trusted`; `remote.*` may use `peer=<id>` instead of host/token.

## peer id

| Dialogue | Config / `peer=` |
|----------|------------------|
| win | **windows** |
| wsl | **wsl** |
| alua / slave | **linux** |

Do **not** store `win` in JSON (normalized to `windows` on write).

## States

| state | Meaning |
|-------|---------|
| `pending` | Registered; not default for `remote.* peer=` |
| `trusted` | Approved; `peer=` allowed |
| `revoked` | Rejected until re-approve (`force=true`) |

## token_ref（内部配置；业务回显用 `token_set`）

`peers.json` 可存引用句柄（**勿**写长 token）。`peer.list` 对外只回显 **`token_set` 布尔**，不回显 `token_ref`。日常业务鉴权仍走 **ssid**（行内 `token_ref` 不作业务证）。

| Form | Resolves |
|------|----------|
| `file:serve.env` | Local pref `serve.env` / `ALUA_SERVE_TOKEN` |
| `file:peers.<id>.env` | Pref file (e.g. `peers.windows.env`) |
| `env:ALUA_SERVE_TOKEN` | Process env |
| `peer:<id>` | Alias → `file:peers.<id>.env` |

## CLI

```
alua peer list
alua peer ping windows
alua peer approve windows
alua peer revoke windows
```

## Tools

| Tool | Profile | Notes |
|------|---------|-------|
| `peer.list` | readonly | Counts + rows；`token_set` 布尔（无 `token_ref`） |
| `peer.ping` | net | pending OK |
| `peer.approve` | net | Fingerprint + `runtime.hostinfo` → trusted |
| `peer.revoke` | net | state=revoked |

## remote.* with peer=

```
alua call remote.call {"peer":"windows","name":"runtime.ping","args":{}}
```

Requires `state=trusted`. Explicit `host`/`token`/`cert_sha256` still override when set.

Register rows via `config.set` module=`peers` (or edit `peers.json`).

## Out of scope (later)

Hop relay `via=` / hop≤2 — mesh P2; not this slice.
