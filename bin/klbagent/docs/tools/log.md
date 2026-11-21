# log tools

Structured JSONL via `klbagent.core.log`. Manage with `config.log` + `log.*`. Remote: `remote.call` → peer `log.*` (pull + push poll; no separate stream protocol).

| tool | profile | notes |
|------|---------|-------|
| `log.status` | readonly | cfg / path / size / `host_kind` / ring / `push_subs` |
| `log.tail` | readonly | JSONL file or `source=serve` text |
| `log.recent` | readonly | memory ring snapshot |
| `log.subscribe` | readonly | start push queue; returns `sub_id` (process-local; max 8) |
| `log.push_poll` | readonly | fetch+consume new rows for `sub_id` |
| `log.unsubscribe` | readonly | drop subscription |

## Config (`log.json`)

`config.set {module:"log", cfg:{…}, scope:"once|permanent"}`（默认 **permanent** 写盘；**once** 仅进程 live；写侧每次 `load` 吃 live）。

| key | meaning |
|-----|---------|
| `level` | stderr filter |
| `to_file` | write file; **embedded default false** |
| `file_level` | file threshold |
| `remote_level` | ring + push fanout + `log.tail`/`recent` minimum |
| `path` / `max_bytes` | file path / rotate |
| `ring_n` | ring capacity (0=off) |

## Push (P3)

Producer **pushes** matching rows into a per-`sub_id` queue when they pass `remote_level` and the sub filter. Debugger **polls** via `log.push_poll` (works over `remote.call` / stdio; no websocket).

```
# on peer (or via remote.call)
alua call log.subscribe {"module":"dispatch","level":"error","max_n":100}
# -> { sub_id:"s1", ... }

# generate activity, then:
alua call log.push_poll {"sub_id":"s1","lines":20}
# -> { lines:[...], pending, dropped }

alua call log.unsubscribe {"sub_id":"s1"}

# remote debugger
alua call remote.call {"host":"127.0.0.1","port":12817,"name":"log.subscribe","args":{"module":"serve"}}
alua call remote.call {"host":"127.0.0.1","port":12817,"name":"log.push_poll","args":{"sub_id":"s1","lines":50}}
```

Subscriptions live in the **serve/stdio process** only (not one-shot CLI).
