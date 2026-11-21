# mcp host config (AI L3)

Audience: **any AI** helping configure or verify alua as an MCP server in a host IDE.  
Package true source (dev): `packages/mcp/CLIENTS.md`. Protocol: `packages/mcp/PROTOCOL.md`.  
Entry: `alua mcp serve [--profile P]` (stdio JSON-RPC; default profile `readonly`). Client direction: `alua mcp probe|list|call|listen|close --url URL [--token T] [--timeout S] [--json]` (Streamable HTTP).

```
alua call docs.read {"path":"tools/mcp.md"}
alua call docs.read {"path":"packages.md"}
```

## When to read

| case | AI MUST |
|------|---------|
| User asks to **configure / add / fix MCP** for alua | Read this file; apply host-specific JSON; do **not** invent paths |
| MCP tools missing / host cannot start alua | Check PATH / `command`+`args` / top-key (`mcpServers` vs `servers`) |
| Already connected; only using tools | Prefer `runtime.ai_bootstrap` + `runtime.ai_load_ack`; this file optional |

Before business tools: `runtime.ai_bootstrap` → `runtime.ai_load_ack`.

## Rules (all hosts)

| rule | detail |
|------|--------|
| Config JSON key | Always **`alua`** under `mcpServers` (or VS Code `servers`). Do **not** use `mcp-xx` as the JSON key |
| `host_tool` standard name | `mcp-cursor` / `mcp-codebuddy` / `mcp-trae` / …; fingerprint / `ssid.whoami` only; **not** auth |
| Unknown host | Use § default (`mcp-unknown`); still OK to connect |
| `command` | No spaces inside `command`; put flags in `args` |
| PATH | Prefer `alua` on PATH; else absolute `<ALUA>` placeholder (never paste private home paths into shared docs) |
| Privacy | No private absolute paths / tokens / dates in examples |
| Transport | Preferred **stdio** `alua mcp serve`; HTTP face on **`alua serve`** `POST|GET|DELETE /mcp` (Bearer / `X-Alua-Token`, optional `X-Alua-Ssid`; POST = JSON or multi-frame SSE, GET = SSE listen, DELETE = end session; loopback raw token allowed, non-loopback plaintext → `401`; see CLIENTS.md) |

Optional label env: `ALUA_CALLER_NAME=mcp-cursor` (or codebuddy/trae/…).

## After standard install (checklist)

Premise: alua is already **standard-installed** (`alua` on PATH; optional packages kept). Do **not** invent a first-time OS install here.

| step | action | pass |
|------|--------|------|
| 1 | `alua -v` | version string prints |
| 2 | `alua pkg list` (or `alua pkg list --json`) | row **`mcp`** present (`tier=optional`) |
| 3 | Resolve host → write JSON from the matching section below (`command`=`alua`, `args`=`["mcp","serve",…]`) | file saved at host path |
| 4 | Reload MCP / reopen host window | host shows server **`alua`** healthy |
| 5 | `runtime.ai_bootstrap` → `runtime.ai_load_ack` → `runtime.ping` (or other readonly tool) | tools work |

If step 1 fails: stop; this doc assumes standard install. If step 2 lacks `mcp`: install was likely embedded/optional-stripped — report to user; do not fake HTTP MCP. If PATH lacks `alua` but binary exists: put absolute path in `command` (do not paste private paths into shared docs).

## Client direction (consume an external MCP server)

Same package, opposite direction: call a remote MCP server over Streamable HTTP.

| command | HTTP | result |
|---------|------|--------|
| `alua mcp probe --url URL` | `initialize` + `ping` | protocol version / `Mcp-Session-Id` |
| `alua mcp list --url URL` | `POST tools/list` | tool table (`--json` = machine) |
| `alua mcp call --url URL --tool NAME [--args JSON]` | `POST tools/call` | tool result |
| `alua mcp listen --url URL [--timeout S]` | `GET` (SSE) | server→client events (default 60s) |
| `alua mcp close --url URL` | `DELETE` | session ended |

Flags: `--token T` → `Authorization: Bearer`; `--timeout S` (default 15, `listen` 60); `--json` for AI/machine. Each command runs `initialize` internally first (`listen` / `close` therefore always carry a `Mcp-Session-Id`). Package face: `require("klbagent.packages.mcp").client()`.

## Default (unknown AI · `mcp-unknown`)

```json
{
  "mcpServers": {
    "alua": {
      "command": "alua",
      "args": ["mcp", "serve", "--profile", "net"]
    }
  }
}
```

Readonly (safer default):

```json
{
  "mcpServers": {
    "alua": {
      "command": "alua",
      "args": ["mcp", "serve"]
    }
  }
}
```

## Cursor (`mcp-cursor`)

| item | value |
|------|-------|
| user | `~/.cursor/mcp.json` (Win: `%USERPROFILE%\.cursor\mcp.json`) |
| workspace | `<project>/.cursor/mcp.json` |
| top key | `mcpServers` |
| UI | Settings → MCP |

Same JSON shape as § default. Workspace file may be shared; do not commit private paths/tokens. Built-in Cursor MCP namespaces ≠ this `alua` server.

## CodeBuddy (`mcp-codebuddy`)

Official: https://www.codebuddy.cn/docs/cli/mcp · IDE https://www.codebuddy.ai/docs/zh/ide/User-guide/MCP

| scope | path (high → low) |
|-------|-------------------|
| user (prefer) | `~/.codebuddy/.mcp.json` |
| user (legacy) | `~/.codebuddy/mcp.json`, `~/.codebuddy.json` |
| project (prefer) | `<project>/.mcp.json` |
| project (legacy) | `<project>/mcp.json` |
| local | user file `projects.<workspace_path>.mcpServers` |

Same-name priority: **local > project > user**. JSONC allowed.

```json
{
  "mcpServers": {
    "alua": {
      "type": "stdio",
      "command": "alua",
      "args": ["mcp", "serve", "--profile", "net"],
      "description": "alua / klbagent tools"
    }
  }
}
```

`type` optional if `command` present; **prefer explicit `stdio`**. UI: chat panel → CodeBuddy Settings → MCP → Add MCP. Env expand: `${VAR}` / `${VAR:-default}`.

Some CodeBuddy builds report `clientInfo.name=Copilot`; set `env.ALUA_CALLER_NAME=mcp-codebuddy` so `host_tool` stays the CodeBuddy standard name.

## Trae (`mcp-trae`)

Official: https://docs.trae.ai/ide/add-mcp-servers

| item | value |
|------|-------|
| global | Settings → MCP → add / raw JSON |
| project | `<project>/.trae/mcp.json` (enable **project MCP** first) |
| top key | `mcpServers` |
| infer | `command` → stdio; `url` → HTTP (usually no `type`) |
| vars | `${workspaceFolder}` |
| constraint | `command` must not contain spaces |

```json
{
  "mcpServers": {
    "alua": {
      "command": "alua",
      "args": ["mcp", "serve", "--profile", "net"]
    }
  }
}
```

Optional stdio timeouts via `env`:

```json
{
  "mcpServers": {
    "alua": {
      "command": "alua",
      "args": ["mcp", "serve", "--profile", "net"],
      "env": {
        "START_MCP_TIMEOUT_MS": "60000",
        "RUN_MCP_TIMEOUT_MS": "120000"
      }
    }
  }
}
```

Do **not** copy VS Code `.vscode/mcp.json` + top key `servers`. alua stdio is **local** Agent only. CN install dir may be `.trae-cn`; project file name still `.trae/mcp.json` per official docs.

## VS Code (`mcp-vscode`)

| item | value |
|------|-------|
| workspace | `<project>/.vscode/mcp.json` |
| user | user MCP settings (per VS Code / Agent docs) |
| top key | **`servers`** (not `mcpServers`) |
| note | Prefer `"type":"stdio"`; do **not** mix this shape into Cursor/Trae/CodeBuddy files |

```json
{
  "servers": {
    "alua": {
      "type": "stdio",
      "command": "alua",
      "args": ["mcp", "serve", "--profile", "net"]
    }
  }
}
```

## GitHub Copilot (`mcp-copilot`)

| item | value |
|------|-------|
| workspace | `<project>/.mcp.json` |
| user | `mcp-config.json` (portable / product docs) |
| top key | `mcpServers` |

```json
{
  "mcpServers": {
    "alua": {
      "command": "alua",
      "args": ["mcp", "serve", "--profile", "net"]
    }
  }
}
```

Same shape as § default. Prefer workspace file for project share; no private paths/tokens in committed JSON.

## Claude Desktop (`mcp-claude` / `mcp-claude-desktop`)

| item | value |
|------|-------|
| macOS | `~/Library/Application Support/Claude/claude_desktop_config.json` |
| Win | `%APPDATA%\Claude\claude_desktop_config.json` |
| top key | `mcpServers` |
| `host_tool` | default umbrella `mcp-claude`; fingerprint may refine `mcp-claude-desktop` |

```json
{
  "mcpServers": {
    "alua": {
      "command": "alua",
      "args": ["mcp", "serve", "--profile", "net"]
    }
  }
}
```

stdio only for this package serve. Restart Claude Desktop after editing the config file.

## Claude Code (`mcp-claude` / `mcp-claude-code`)

| item | value |
|------|-------|
| project | `<project>/.mcp.json` |
| user | per official (e.g. `~/.claude.json`) |
| top key | `mcpServers` |
| `host_tool` | umbrella `mcp-claude` or refine `mcp-claude-code` |

```json
{
  "mcpServers": {
    "alua": {
      "command": "alua",
      "args": ["mcp", "serve", "--profile", "net"]
    }
  }
}
```

Same shape as § default. Do not confuse with Claude Desktop path above.

## Windsurf (`mcp-windsurf`)

| item | value |
|------|-------|
| user | `~/.codeium/windsurf/mcp_config.json` (Win: `%USERPROFILE%\.codeium\windsurf\mcp_config.json`) |
| top key | `mcpServers` |
| remote | some hosts use `serverUrl` / `url` for HTTP MCP; this package serve remains **stdio** |

```json
{
  "mcpServers": {
    "alua": {
      "command": "alua",
      "args": ["mcp", "serve", "--profile", "net"]
    }
  }
}
```

## Unknown host (`mcp-unknown`)

No dedicated section: copy § Default. Still connectable; do not invent host-specific paths.

## Self-check

1. Shell: `alua -v`; **`alua pkg list` shows `mcp`**; host MCP list shows `alua` healthy; `tools/list` non-empty.
2. Call `runtime.ai_bootstrap` then `runtime.ai_load_ack`, then a readonly tool (e.g. `runtime.ping`).
3. Common fails: space in `command`; `alua` not on PATH; **mcp package omitted**; wrong top key; Trae project MCP switch off; logs on stdout.
4. Prefer the **After standard install** checklist when first wiring a host.

Deep package notes: `packages/mcp/CLIENTS.md` / `DESIGN.md` / `PROTOCOL.md` (source tree; not via `docs.read`).

## SSID / token flow (label only)

MCP transport identity is a **label**, never an authenticator.

| step | what happens | tool surface |
|------|--------------|--------------|
| `initialize` | fingerprint (`clientInfo` / parent proc / env) → `alua_ssid` (16 hex) → stored client row; session attrs get `via=mcp` + `host_tool=mcp-xx` | none |
| `notifications/initialized` | lifecycle gate opens; attrs get `mcp_initialized` | none |
| each `tools/call` | `begin_ctx` reuses the handshake `alua_ssid` (no token issuance on this path) | `ssid.whoami` / `ssid.status` |
| explicit token | temp token is issued **only** when asked via `ssid.ensure` / `ssid.begin_ctx` (min profile `net`) | those tools |

| rule | detail |
|------|--------|
| Not auth | `alua_ssid` / `host_tool` are labels; permission still comes from profile + `ALUA_SERVE_TOKEN` |
| Token plaintext | business tool results must **not** carry `token` / `token_ref`; only `has_token` (bool); plaintext only for in-process facade (`get_token_plain`); no `temp.*.env` |
| Facade | 对外仅 `ssid.*`（`auth.*` 工具已删除）；同 MCP 会话内复用 `via=mcp` / `alua_ssid` |
| Profile | raise via `runtime.set_profile`; `min_profile` of each tool still applies (`readonly` default) |
| Restart | `ssid.clients.json` / `ssid.machine.json` persist under the pref dir; GC via `ssid.gc` (dry-run default) |

# end
