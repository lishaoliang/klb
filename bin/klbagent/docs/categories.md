# Feature categories (L2 index)

Category means what a tool does. Profile means whether it is allowed. They are independent.  
Load order: `bootstrap.md` (L0) → `rules/` (L1) → here / `skills/*/SKILL.md` (L2) → `tools/*` (L3).

| id | meaning | default min_profile | L2 skill |
|----|---------|---------------------|----------|
| runtime | version / ping / self-check | readonly | `skills/runtime/SKILL.md` |
| catalog | list tools / meta | readonly | (catalog tools; no long skill) |
| config | pref JSON read/write | readonly | (see `rules/core.md`) |
| fs | path / file (read-first; write needs write_fs / profile>=net) | readonly | `skills/fs/SKILL.md` |
| net | network | net | `skills/net/SKILL.md`, `skills/remote/SKILL.md` |
| device | device / serial | device | `skills/device/SKILL.md` |
| unsafe | high risk | unsafe | `skills/unsafe/SKILL.md` |
| log | structured log status / tail | readonly | `skills/log/SKILL.md` |
| g2de | g2de engine embed (snapshot / selfcheck / control) | readonly | optional package |

Optional host-facing (not a catalog category): MCP mount help → L2 `skills/mcp/SKILL.md` → L3 `tools/mcp.md` (Cursor / CodeBuddy / Trae / …).

Unknown category cannot be registered.
