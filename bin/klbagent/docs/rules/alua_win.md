# alua-win contract (L1 · AI only)

**alua-win** = native Windows `alua.exe` (`host_kind=windows`) that can run **`alua --serve`**, install via release pack, and (later) self-upgrade / optional resident.  
Dialogue short name: **win alua** (≠ bare **alua** = Ubuntu/Linux; ≠ **wsl alua**).  
三端（**alua** / **wsl alua** / **win alua**）互操作 **默认 alua**（`remote.*`）；业务 **SSH**/scp/rsync **须点名**。  
**≠** WSL `alua` / `alua-serve.service` (may coexist). **≠** embedded Windows Service / Registry tools (**wont**).

## Build product

| item | value |
|------|-------|
| Toolchain | mingw-w64 cross (`make mingw` / `MY_HOST=mingw`); MSVC later |
| Binary | `klb/lib/mingw/alua.exe` |
| DLL | `klb/lib/mingw/libklb.dll` (+ plugins as staged) |
| Scripts | `klb/bin/klbagent` (+ `klbcore`) copied into install/stage flat next to exe |
| Entry | same as Linux: only boots `klbagent` |

```
# on Linux builder with mingw-w64
cd klb && make mingw
# expect: lib/mingw/alua.exe lib/mingw/libklb.dll
```

## Install layout

| item | value |
|------|-------|
| Default root | `%LocalAppData%\Programs\klua` (per-user; no admin / no UAC) |
| Shape | **flat**: `alua.exe`, `klua.exe`, dlls, `klbagent/`, `klbcore/`, `plugins/` |
| **not** | `/usr/local` tree; Linux `rsync` deploy |
| Pack | `klua_release`: `stage/stage.sh win [--no-g2de-apps]` → `windows/pack.sh` (NSIS) → `klua-<ver>-setup.exe` (build on Linux/WSL; 默认全量，可裁 g2de_apps) |
| PATH | installer adds install dir to user PATH |
| Background serve | Installer optional (**default checked**, can uncheck): selected → **immediate** Task Scheduler resident `klua\alua-serve` via `wscript //B` + `alua-serve-hidden.vbs` (**no console/cmd window**). Start/stop **only** via Task Scheduler or uninstall. **≠** Win Service. Token → `%APPDATA%\klua\alua\serve.env` if missing. |
| GUI install | Unchanged: wizard pages; user may uncheck apps / background serve |
| Bootstrap silent | `setup.exe /S` (NSIS) = **full** install: fixed `%LocalAppData%\Programs\klua`, all sections (core + apps if staged + background serve). Tool: `deploy.win_install`. Inno parity: `/VERYSILENT /SUPPRESSMSGBOXES /TYPE=full /TASKS=aluaServe` |

## Token / pref / serve

| item | value |
|------|-------|
| Token | `ALUA_SERVE_TOKEN` **required** for all serve profiles (same as other hosts) |
| Pref | under user data (same klbagent pref paths; Windows path rules) |
| **ai_tmp** | `%TEMP%\.tmp`（常为 `%LocalAppData%\Temp\.tmp`；对标 Linux `~/.tmp`）；`alua status` 展示并确保目录存在；子目录 `<topic>/` |
| Default / pool | same as all platforms: preferred `12817`; auto pool `12817`→`12810`; random `11000`–`12000` → `rules/ports.md` |
| AI L0 gate | serve/stdio: `runtime.ai_bootstrap` then `runtime.ai_load_ack` (same protocol) |
| Profiles | `readonly\|net\|device\|unsafe` — **not** tied to `host_kind` |
| Profile set | `runtime.set_profile`：`scope=once` live only；`scope=permanent` → `runtime.json` + live；bare `alua --serve`（无 argv 档）重启跟 pref；raise `unsafe` needs `confirm=true` |
| Pref set | `config.set`：`scope=once\|permanent`（默认 permanent）；log/serve/proxy/peers live；`tls_face`/proxy listen 变口可能 `restart_needed` |

## Coexist with WSL

| side | role |
|------|------|
| **WSL** `alua serve` | Linux tree, build/deploy, tunnel to from-machine |
| **Win** `alua.exe serve` | Native Win paths, COM/UART on host, Win-only tools |
| Ports | Both use the **same** auto pool; second process binds next free (forward). Pin with explicit port if needed. Read pref `serve.port` / health `port`. |
| Client | From-machine `remote.call` → Win LAN IP:**actual** port with same schemes (`mnps` default) |

## Caps (windows)

| cap | today |
|-----|--------|
| `deploy_pull` / `deploy_source` | **false** (hard) |
| Linux `deploy.*` / `remote.deploy` | **do not** use |
| Upgrade path | **done (D28)**: `deploy.win_install` → silent `/S` full setup.exe + `schedule_restart`=`schtasks`; **≠** linux `usrlocal.tgz` / `deploy_pull` |
| Resident | Installer optional section → Task Scheduler `klua\alua-serve` (immediate; no Start Menu controls). Further ops → **M10+** |

## AI must / must-not

| MUST | MUST NOT |
|------|----------|
| `runtime.hostinfo` → confirm `host_kind=windows` | Assume `/usr/local` or `sudoers` deploy |
| Use flat install root + `alua.exe` | Treat WSL serve as covering Win capabilities |
| Same tool names; branch on caps | Embed Win Service / Registry tools in alua |
| Read this file + `rules/host.md` before Win deploy/serve | Copy WSL systemd unit as the Win resident plan |

## Open phases (after this contract)

| ID | goal |
|----|------|
| **D24/D25** | contract + mingw/`setup.exe` pack (**done**) |
| **D28** | Win self-install: NSIS `/S` full + `deploy.win_install` + Win `schtasks` restart (**done**) |
| **D29** | Real PC smoke: from-machine ping/selfcheck/`host_kind=windows` (**done**；CLI/VERSION 与 live serve 可能短暂不一致，须 restart) |
| **M10+** | Resident ops + peer mesh |

Related: `rules/host.md`, `tools/remote.md`, `tools/unsafe.md` (Win32 exec), `skills/remote/SKILL.md`.
