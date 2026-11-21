# L1 rules index (AI only)

Read **after** `docs/bootstrap.md` (L0). Keep this tree short; no L3 bodies here.

| order | path | topic |
|-------|------|-------|
| 1 | `rules/core.md` | profiles, error codes, docs rules, must-not |
| 2 | `rules/host.md` | windows / linux / wsl / macos / embedded + arch (ARM) |
| 3 | `rules/alua_win.md` | **alua-win** contract (native Win serve/install; vs WSL) |
| 4 | `rules/ports.md` | serve pool / random / fixed port map (all platforms) |
| 5 | `rules/peers.md` | peer mesh: peers.json / approve / `peer=` |
| 6 | `rules/auth.md` | 原始/临时 token；对外仅 `ssid.*`；`core.auth` 仅供 ssid |

```
alua call docs.read {"path":"rules/core.md"}
alua call docs.read {"path":"rules/host.md"}
alua call docs.read {"path":"rules/alua_win.md"}
alua call docs.read {"path":"rules/ports.md"}
alua call docs.read {"path":"rules/peers.md"}
alua call docs.read {"path":"rules/auth.md"}
```

Then choose an L2 skill under `docs/skills/*/SKILL.md` or `alua list <category>`.
