# unsafe tools

Require `runtime.profile=unsafe`.

| tool | notes |
|------|-------|
| `unsafe.status` | lists unsafe tools |
| `unsafe.run_file` | run `.lua` in **kthread** isolated `klua_env`; `path` + optional `argv` / `timeout_sec` |
| `unsafe.exec` | external command: `cmd` **or** `argv` (mutually exclusive); optional `cwd` / `env` / `timeout_sec` / `max_bytes` |
| `unsafe.process_list` | list processes: POSIX `/proc`; Win32 `tasklist` (`ppid`/`cmd` via `wmic` when available); optional `name_substr` / `pid` / `max_entries` |
| `unsafe.process_kill` | kill by `pid` (>1); `signal`=`TERM`\|`KILL` (default TERM); Win32 `taskkill` (`/F` for KILL) |

## run_file isolation

`unsafe.run_file` / `alua run` load the script in a **worker thread** with its own `klua_env` (own `_G` / `package.loaded`). Not a multi-process sandbox. One job destroys its env when done; max concurrent `run_file` is limited (busy → tool failed).

Result always includes `isolated=true` when the worker path is used. Optional `timeout_sec` (default 60, max 300): main side polls a temp JSON result; on timeout it requests worker stop (cooperative; hard OS-thread kill is out of scope).

### Exit hooks (permission split)

Before the user chunk runs, the worker intercepts exit APIs so they **never** kill the whole process from inside the worker:

| API | Worker behavior | `alua call` / `serve` | `alua run` (one-shot CLI) |
|-----|-----------------|------------------------|---------------------------|
| `os.exit(code)` | intercept; result `ok` + `exit_intercepted` + `exit_via=os.exit` + `exit_code` | JSON only; process stays | process exits with `exit_code` |
| `ksys.exit` / `kenv.exit` | end worker env; `exit_via=ksys.exit\|kenv.exit`; `exit_code=0` | JSON only | does **not** raise to process exit |

`error` / `assert` → tool failed (code 40). `unsafe.exec` child-process exit only affects that child.

## exec / process

`unsafe.exec` result: `mode` (`cmd`|`argv`), `exit_code`, `timed_out`, `stdout`, `stderr`, truncation flags. Uses `timeout` when available (exit 124 => timed_out). Merges `runtime.env_set` overlay plus optional `env` table.

- **`argv`**: no shell; POSIX runs `env … prog args…`; Win quotes each element. **Prefer** for fixed programs.
- **`cmd`**: shell string; POSIX uses `sh -c`; Win passes through `cmd.exe`. Use for pipes/redirection.

Win32 branch: `cmd.exe` semantics — no `env`/`timeout` wrapper (`timed_out` always false, `timeout_sec` reported only), `env` unset degrades to empty value, `cwd` via `cd /d &&`. `unsafe.process_list` uses `tasklist /FO CSV /NH` (`ppid`/`cmd` from `wmic … /format:csv` when available, else `cmd` = image name); `unsafe.process_kill` uses `taskkill /PID` (`/F` for `KILL`). Same response shape as POSIX.

```
alua call config.set {"module":"runtime","cfg":{"profile":"unsafe","io":"stdio"}}
alua run /tmp/task.lua arg1 arg2
alua call unsafe.run_file {"path":"/tmp/task.lua","argv":["a","b"],"timeout_sec":30}
alua call unsafe.exec {"argv":["uname","-a"],"timeout_sec":10}
alua call unsafe.exec {"cmd":"make -j4","cwd":"/path/to/tree","timeout_sec":120}
alua call unsafe.process_list {"name_substr":"alua","max_entries":50}
alua call unsafe.process_kill {"pid":12345,"signal":"TERM"}
```
