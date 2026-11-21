# alua / klbagent docs (AI only)

**MUST start here → L0:**

```
alua call docs.read {"path":"bootstrap.md"}
```

Then L1 `rules/index.md` → L2 `skills/*/SKILL.md` → L3 `tools/*.md` on demand.

| level | path |
|-------|------|
| L0 | `bootstrap.md` |
| L1 | `rules/` |
| L2 | `skills/*/SKILL.md`, `categories.md` |
| L3 | `tools/*.md`, optional `skills/*/reference.md` |

Also: `alua call docs.list {}` / `alua docs [path]`.  
Optional packages: `packages.md` (not L0). Service shapes (systemd units): `units.md`.
Platforms: `rules/host.md` (stub: `host.md`).

Do **not** treat this README as the full protocol; **bootstrap.md** is authoritative for load order.
