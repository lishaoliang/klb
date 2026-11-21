# alua AI bootstrap (L0 · MUST)

Audience: **any AI** calling alua. Not human product docs.  
**MUST** load this file **or** `runtime.ai_bootstrap` **before** other business tools.

```
alua call runtime.ai_bootstrap {}
# or: alua call docs.read {"path":"bootstrap.md"}
alua call runtime.selfcheck {}
```

## Load order (L0 → L1 → L2 → L3)

| level | meaning | where |
|-------|---------|-------|
| **L0** | this protocol | `docs/bootstrap.md` / `runtime.ai_bootstrap` |
| **L1** | hard rules | `docs/rules/` (`index.md` then listed files) |
| **L2** | skill index | `docs/skills/*/SKILL.md` + `alua categories` / `catalog.tools` |
| **L3** | skill body | `docs/tools/*.md` (on demand) |

Do **not** dump all L3 into context every turn.

## After L0 (required next)

1. `docs.read {"path":"rules/index.md"}` then L1 files (at least `rules/core.md`; deploy/serve → `rules/host.md`).
2. Pick L2: `alua categories` / `skills/<name>/SKILL.md`.
3. Need detail → L3 via `docs.read` on paths from the SKILL.

## Host reflection (by capability)

| host capability | AI MUST |
|-----------------|---------|
| Can persist rules/skills (soft enforce) | Prefer **symlink / junction / short pointer** to installed `klbagent/docs` (or `bootstrap.md`). **No** full copy of L3 bodies. |
| Session-only / no persist | Each session: `runtime.ai_bootstrap` + `docs.read` as needed. |
| No write / sandbox | Read-only via alua; still run reconcile self-check. |

Do **not** treat host IDE skills (e.g. editor-only trees) as alua runtime source of truth.

## Reconcile + prompt user (MUST)

1. Call `runtime.ai_bootstrap` and/or `runtime.selfcheck` (optional args below).
2. Read `ai_load.status` / `user_prompt` / `docs_revision` / `fingerprint`.
3. If status is **`stale`** or **`missing`**: **MUST** tell the **user** the `user_prompt` text and ask them to deploy/update alua or recreate the soft-link/pointer. Do not silently ignore.
4. If `host_capable=true`, ensure soft-link/pointer exists or prompt user to create it.

```
# pin host copy / link revision after you store it locally
alua call runtime.selfcheck {"host_docs_revision":"<docs_revision>","host_fingerprint":"<fingerprint>","host_capable":true}
```

| status | meaning | AI action |
|--------|---------|-----------|
| `ok` | docs present; host pin matches or unchecked | continue |
| `stale` | host pin ≠ published docs | **prompt user** with `user_prompt` |
| `missing` | L0/L1 files absent | **prompt user** with `user_prompt` |

## Hard rules (preview; full text in L1)

- Tool **names** are the same on every OS; branch on `host_kind` / `arch` / `caps` (`rules/host.md`).
- **profiles** (`readonly|net|device|unsafe`) = permission; **≠** `host_kind`.
- No path traversal in `docs.read` (`..` / absolute denied).
- **No popups**: never open UI dialogs / modal prompts, and never block waiting for interactive input; state the need in text or JSON instead (`rules/core.md`).

## Ack hard gate (serve / stdio)

On `alua --serve` / `alua stdio` (or `ALUA_AI_LOAD_GATE=1`), business tools need ack after L0:

```
alua call runtime.ai_bootstrap {}
alua call runtime.ai_load_ack {}
```

Whitelist while pending: bootstrap / ack / ping / version / selfcheck / hostinfo / cwd / parallel / docs.* / catalog.* / lpc.call.  
Escape: `ALUA_AI_LOAD_GATE=0` disables; `ALUA_AI_LOAD_ACK=1` pretends already acked. One-shot `alua call` defaults gate **off**.

## Quick entry

```
alua call runtime.ai_bootstrap {}
alua call runtime.selfcheck {}
alua call runtime.hostinfo {}
alua categories
```

Related: `rules/index.md`, `categories.md`, `packages.md` (optional; not L0).  
Host MCP config (Cursor / CodeBuddy / Trae / …): L2 `skills/mcp/SKILL.md` → L3 `tools/mcp.md`.
