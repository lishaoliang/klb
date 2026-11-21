# fs tools

| tool | profile | notes |
|------|---------|-------|
| `fs.stat` | readonly | exists/mode/size |
| `fs.list` | readonly | dir entries; `max_entries` / `max_depth` |
| `fs.read` | readonly | file bytes; `max_bytes` / `offset` |
| `fs.grep` | readonly | literal (default) or `regex=true`; optional `recursive` |
| `fs.write` | **net+** (`write_fs`) | `content` + `mode` overwrite\|append |
| `fs.mkdir` | **net+** | `parents=true` optional |
| `fs.remove` | **net+** | dir needs `recursive=true` |
| `fs.copy` / `fs.move` | **net+** | `src`/`dst`; dst must not exist |

```
alua call fs.list {"path":"/tmp","max_entries":50}
alua call fs.read {"path":"/etc/hostname","max_bytes":4096}
alua call config.set {"module":"runtime","cfg":{"profile":"net","io":"stdio"}}
alua call fs.write {"path":"/tmp/alua_demo.txt","content":"hello\n"}
```
