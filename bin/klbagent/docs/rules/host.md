# Host platforms (AI only)

Read **before** deploy / serve / remote decisions. Tool names stay the same on every OS; use `host_kind` + `arch` + `caps` to choose the path.

## Dialogue short names (workspace)

| say | means | host_kind |
|-----|-------|-----------|
| **alua** | Ubuntu/Linux peer (slave / private PC) | `linux` |
| **wsl alua** | host WIN-PC **WSL** alua | `wsl` |
| **win alua** | host WIN-PC **native** `alua.exe` (**alua-win**) | `windows` |

Bare **alua** ≠ WSL ≠ Win native. Do not invent a second tool vocabulary from these names.

```
alua call runtime.hostinfo {}
alua call runtime.selfcheck {}
```

Override morph with env `ALUA_HOST_KIND` (`wsl`|`linux`|`windows`|`macos`|`embedded`). Do **not** invent a second tool vocabulary per OS.

## Axes (orthogonal)

| axis | values | notes |
|------|--------|-------|
| `host_kind` | `wsl` / `linux` / `windows` / `macos` / `embedded` | morph; not a profile name |
| `arch` | e.g. `x86_64` / `aarch64` / `armv7l` / … | from `uname -m`; **≠** host_kind |
| `virt` | `none` / `vm` / `wsl2` / `qemu` / `container` | attribute under same host_kind |
| `network` | `lan_direct` / `nat_tunnel` / `nat_or_bridged` / `lan_or_serial` | connectivity hint |
| `caps.*` | booleans | gate deploy / suggest readonly |

**ARM**: `arch` may be `aarch64` or `arm*`. On Linux, ARM + busybox + no `systemctl` often → `host_kind=embedded`. PC ARM (e.g. aarch64 Ubuntu desktop) stays `linux` unless override. Cross-build / board images ≠ PC `linux` deploy flow.

## host_kind matrix

| host_kind | typical | network | deploy_pull / source | serve / restart | notes for AI |
|-----------|---------|---------|----------------------|-----------------|--------------|
| `linux` | Ubuntu PC / builder / VM | often `lan_direct` (VM may `nat_or_bridged`) | yes if rsync + curl\|wget (+ make for source) | `systemctl` or exec restart; install `/usr/local` | **main** peer for `remote.deploy` |
| `wsl` | WSL2 on PC-WIN | `nat_tunnel` | same as linux when tools present | same as linux; user service common | peer often via `ssh -L` / tunnel; **≠** native Win |
| `windows` | native Win (`alua.exe`) | `lan_direct` | **false** (hard) today | no systemctl; restart=`schtasks` / `deploy.win_install` | install `%LocalAppData%\Programs\klua` flat layout; see install/bootstrap below |
| `macos` | Darwin | `lan_direct` | **false** | limited | treat like non-linux deploy |
| `embedded` | board / Buildroot / OpenWrt / busybox ARM | `lan_or_serial` | **false** | no assume systemctl | prefer readonly; **remind only** for board serve; do not copy WSL deploy |

`virt=container` / `vm` keep `host_kind=linux` (or wsl); adjust network only.

## caps (decision)

| cap | meaning | when true |
|-----|---------|-----------|
| `http_listen` | can run `alua --serve` | usually true |
| `deploy_pull` | `deploy.self_install` / `pull_install` / peer of `remote.deploy` | **linux/wsl** + rsync + curl\|wget; forced false on windows/macos/embedded |
| `deploy_source` | `deploy.source_install` (local tree + make) | deploy_pull + make |
| `systemctl` | restart via systemd | if `systemctl` on PATH |
| `exec` | `unsafe.exec` available (still needs profile) | usually true |
| `recommend_readonly` | suggest readonly default | `embedded` |

If `caps.deploy_pull!=true`, do **not** call linux-style deploy; expect explicit error.

## Install layout

| side | layout | pack |
|------|--------|------|
| linux / wsl | `/usr/local/bin/alua` + `share/klua/klbagent` | stage tgz `usr/local` tree |
| windows | flat under `%LocalAppData%\Programs\klua` (`alua.exe`, dlls, `klbagent/`) | `klua-<ver>-setup.exe` (NSIS) from release stage |
| embedded | vendor prefix / image; do not assume `/usr/local` | board image / package |

## Bootstrap / upgrade

**正式自举升级物**：仅 **`klua_release` 发布包**（与人工同 stage；linux deploy=`*-usrlocal.tgz`）。流程：编 → `version_bump` → stage → pack → 再装。**本树升级 alua/klua/g2de/g2de_apps 默认本路径**；失败须用户点名其他途径。**≠** 开发树手拷 / 仅 `make install`。照做：`klua_release/docs/bootstrap-flow.md`（§0 / §9 WSL）。

| side | release pack | install path | restart |
|------|--------------|--------------|---------|
| linux / wsl | deploy：`*-linux-x64-usrlocal.tgz`；平铺：`*-linux-x64.tar.gz`（`wsl`=linux 别名） | `deploy.self_install` / `pull_install` / `remote.deploy` → `/usr/local`；once `deploy.setup_priv` | `schedule_restart`: 优先 systemctl（须 `sudo -S`/`ALUA_DEPLOY_SUDO`；`alua-deploy` dropin **不含** systemctl）；失败则人工/`Restart=on-failure`；无 systemctl 时 exec+kill |
| windows | `out/win/klua-<ver>-setup.exe` | `deploy.win_install` silent `/S` **full** (GUI defaults unchanged; **not** rsync+/usr/local) | `schedule_restart` → `schtasks` `/Run` `klua\alua-serve` |
| embedded | board image / vendor pack | not PC deploy tools | board-specific |

Lua-only hot path (dev): `deploy.scripts_install` + `runtime.reload`（无换 `.so`/`alua`）。正式升版仍以发布包为准。

## CRYPTO defaults (transport)

| morph | default CRYPTO (smp/mnp family) |
|-------|----------------------------------|
| PC (`linux` / `wsl` / `windows` / `macos`) | CHACHA20 / 44 |
| `embedded` | AES_CTR / 24 (lower CPU) |

TLS and CRYPTO are orthogonal. See `docs/tools/remote.md`.

## AI checklist

1. `runtime.hostinfo` → read `host_kind`, `arch`, `caps`, `network`.
2. Branch:
   - **linux/wsl** + `deploy_pull` → linux deploy/serve docs (`remote.md`).
   - **windows** → read **`rules/alua_win.md`**; native serve / flat install; do not use `/usr/local` or sudoers deploy.
   - **embedded** or ARM board-like → readonly; no WSL tunnel assumptions.
3. WSL peer from another host: prefer tunnel to forwarded port; direct WSL2 NAT often fails.
4. Never put `wsl` / `windows` into profile names; profiles stay `readonly|net|device|unsafe`.

Related: `rules/alua_win.md`, `rules/ports.md`, `docs/tools/runtime.md`, `docs/tools/remote.md`, `docs/tools/unsafe.md` (Win32 exec / POSIX-only `process_*`).
