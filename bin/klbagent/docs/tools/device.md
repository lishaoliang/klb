# device tools

Require `runtime.profile=device` (or higher).

| tool | notes |
|------|-------|
| `device.status` | kuart binding + tool list + `open_handles` / `open_count` |
| `device.uart.list` | candidate ports (Linux `/dev/ttyUSB*`/`ttyACM*`/`ttyAMA*`; `include_legacy` adds `ttyS*`; Windows `COMn`) |
| `device.uart.open` | open port; returns string `handle`; optional `name` → `uart-<name>`; same `path`/`name` exclusive unless `steal=true` |
| `device.uart.write` | write `data` string/bytes |
| `device.uart.read` | read up to `max_bytes` with `timeout_ms` (default 200) |
| `device.uart.flush` | flush RX/TX |
| `device.uart.close` | close handle |
| `device.uart.handles` | list process-local open handles (includes `name` when set) |
| `device.uart.transact` | optional write, then read until `until` / `pattern` / `min_bytes` (or timeout / max) |

## Binding

C module `require("kuart")` (`klua_platform/klua_kuart.c`): Linux termios + Windows COM (`CreateFile` / DCB).

Baud: positive integer up to 12M. Linux maps known `Bxxxx` (incl. `B1500000` / `B2000000` … `B4000000`); other rates use `TCGETS2` + `BOTHER` + `c_ispeed`/`c_ospeed`. Windows passes non-CBR rates via DCB `BaudRate`. Hardware/driver may still reject unsupported rates at open.

Handle is a **process-local string id**, kept for the life of the process.

| entry | keep-alive across tool calls |
|-------|------------------------------|
| one-shot `alua call` | **no** |
| `alua stdio` / `alua --serve` | **yes** |

## Stable errors (P2)

Tool failures use message prefix **`uart:<tag>`** (no Lua file path). Common tags:

| tag | meaning |
|-----|---------|
| `kuart_unavailable` | C binding missing |
| `need_path` / `need_handle` / `need_data` | missing args |
| `path_busy` / `name_busy` | exclusive lock |
| `unknown_handle` | bad/closed handle |
| `open_fail` / `read_fail` / `write_fail` / `flush_fail` | I/O |
| `bad_name` / `bad_until` / `bad_pattern` / `need_wait` | arg validation |

Via `remote.call`, peer failures also carry:

| field | meaning |
|-------|---------|
| `hop` | always `remote` |
| `peer_tool` | remote tool name |
| `peer_code` / `peer_name` | peer envelope code |
| `uart_err` | parsed `<tag>` when message is `uart:*` |

Key ops (`open`/`write`/`close`/`transact` + fails) write `log` with `module=device.uart`.

## Named sessions and multi-port (P3)

- Auto handle: `uart-1`, `uart-2`, …  
- Named: `open` with `"name":"console"` → handle `uart-console` (reuse across calls in the same serve/stdio process).  
- Multiple ports: open different `path`s at once; each has its own handle. Same path stays exclusive.  
- `steal=true` closes existing same path **or** same name before reopen.

```
alua call device.uart.open {"path":"/dev/ttyUSB0","name":"console","baud":115200}
alua call device.uart.open {"path":"/dev/ttyUSB1","name":"gps","baud":9600}
alua call device.uart.handles {}
```

## Relay / dual-machine topology (P3)

```
Cursor / AI
  → 开发机 alua (profile ≥ net)
      → remote.call  (HTTP(S), token)
          → 调试 PC alua serve (profile ≥ device)
              → device.uart.* → USB-UART → 板
```

Rules:

1. USB must be on the machine that runs `alua serve`.  
2. Caller ≥ **net**; peer serve ≥ **device**; `ALUA_SERVE_TOKEN` both sides.  
3. Same tool names; handle lives only on the **peer** process.  
4. Prefer `scheme=http` for lab loopback smokes; production may use https + pin.

```
# peer
export ALUA_SERVE_TOKEN=...
alua --serve 12827 device

# client
alua call config.set {"module":"runtime","cfg":{"profile":"net","io":"stdio"}}
alua call remote.call {"host":"127.0.0.1","port":12827,"scheme":"http","name":"device.uart.open","args":{"path":"/dev/ttyUSB0","name":"console","baud":115200}}
alua call remote.call {"host":"127.0.0.1","port":12827,"scheme":"http","name":"device.uart.transact","args":{"handle":"uart-console","data":"help\r\n","until":">","timeout_ms":2000}}
```

## Board protocol adapters (P3 boundary)

**Not shipped by default.** Product-specific prompt/regex/line protocols (bootloader menus, AT, custom framing) stay as call-site `transact` args or a future optional package. To add a named adapter, point a scene (board + protocol) and request implementation — do not treat Modbus/RS485 product stacks as alua transport (still **wont**).

## transact

Need at least one of: `until` (plain substring), `pattern` (Lua pattern), `min_bytes`.

| field | meaning |
|-------|---------|
| `data` | optional write before read loop |
| `flush_before` | flush RX/TX before write |
| `timeout_ms` | overall deadline (default 2000) |
| `max_bytes` | buffer cap (default 4096) |
| `matched` / `reason` / `truncated` | wait outcome |

## Examples (local)

```
alua call config.set {"module":"runtime","cfg":{"profile":"device","io":"stdio"}}
alua call device.uart.list {}
alua call device.uart.open {"path":"/dev/ttyUSB0","name":"console","baud":115200}
alua call device.uart.open {"path":"/dev/ttyUSB0","name":"console","baud":1500000}
alua call device.uart.transact {"handle":"uart-console","data":"\r\n","until":"#","timeout_ms":2000}
alua call device.uart.close {"handle":"uart-console"}
```

Windows path example: `"path":"COM3"` (COM10+ uses `\\\\.\\COMn` internally).
