# skill: unsafe (L2)

| field | value |
|-------|-------|
| title | exec / run_file / process |
| summary | unsafe.exec, run_file (kthread isolate), process_list/kill (POSIX) |
| when_to_read | shell commands, arbitrary lua, process control |
| category | unsafe |
| l3_paths | `tools/unsafe.md` |

Needs `runtime.profile=unsafe`. Win32 exec ≠ POSIX process_*.

```
alua call docs.read {"path":"tools/unsafe.md"}
```
