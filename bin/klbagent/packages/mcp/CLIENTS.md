# MCP 宿主客户端配置参考

> 路径: `klb/bin/klbagent/packages/mcp/CLIENTS.md`
> 代码: `server/`（`alua mcp serve`）; CLI `core/cli/mcp.lua`
> 用途: 各 AI 宿主如何挂载本包 **stdio MCP server**; 与 `PROTOCOL.md`（协议）分工。
> 相关: `PROTOCOL.md` / `DESIGN.md` / `klbagent/docs/packages.md` § mcp
> **AI 帮助面**（`docs.read` 可达）: `klbagent/docs/tools/mcp.md` + `docs/skills/mcp/SKILL.md`（含 CodeBuddy / Trae / Cursor 等）; 改宿主配置示例时 **两处同步**

## 0. 前提

| 项 | 说明 |
| ---- | ---- |
| 入口 | `alua mcp serve [--profile P]` |
| 传输 | **stdio** JSON-RPC（换行帧）; 宿主拉起子进程 |
| 默认档 | 未传 `--profile` → `readonly` |
| 常用档 | `readonly` / `net` / `device` / `unsafe` |
| PATH | `command` 优先用已在 PATH 的 `alua`; 否则写可执行文件**绝对路径**（勿写私人目录真值入库; 本文一律占位） |
| 占位 | `<ALUA>` = alua 可执行路径或 `alua`; `<PROFILE>` = 档位, 缺省可省略 `--profile` |
| 隐私 | 配置示例**禁止**写入私人绝对路径 / 全日期 / 口令; token 用宿主密钥机制或本机自填 |

业务工具前须按 server `instructions` 调 `runtime.ai_bootstrap` → `runtime.ai_load_ack`（MCP 通道不走 serve/stdio 硬门禁, 但 catalog 仍要求 ack）。

### 0.1 AI 标准名（`mcp-xx`）与配置键

| 层 | 约定 |
| ---- | ---- |
| **配置 JSON 键**（`mcpServers` / VS Code `servers`） | **固定用 `alua`**（本包服务名）; **不要**改成 `mcp-xx` |
| **AI 标准名** `host_tool` | `mcp-xx`; 指纹 / `ssid.whoami` / 可选 `ALUA_CALLER_NAME` 使用; **不当鉴权** |
| 未适配 / 未知 AI | 标准名 **`mcp-unknown`**; 仍可抄 §1 接入 MCP |

| 标准名 | 宿主 | 本文 |
| ---- | ---- | ---- |
| `mcp-cursor` | Cursor | §2 |
| `mcp-codebuddy` | CodeBuddy | §3 |
| `mcp-trae` | Trae | §4 |
| `mcp-vscode` | VS Code Agent | §5 |
| `mcp-copilot` | GitHub Copilot | §6 |
| `mcp-claude` | Claude Desktop / Code（伞名） | §7 / §8 |
| `mcp-claude-desktop` | Claude Desktop（可选细分） | §7 |
| `mcp-claude-code` | Claude Code（可选细分） | §8 |
| `mcp-windsurf` | Windsurf | §9 |
| `mcp-unknown` | 未适配 / 未知 AI | §1 / §10 |

可选覆盖（标签）: 环境变量 `ALUA_CALLER_NAME=mcp-cursor`（等）; 与 `DESIGN.md` §3 一致。

---

## 1. 默认推荐配置（未单独适配的 AI · `mcp-unknown`）

凡支持 **Claude/Cursor 系** `mcpServers` + stdio（`command` / `args` / 可选 `env`）的宿主, **优先抄本节**。未进专节的宿主标准名记 **`mcp-unknown`**, **不**挡 MCP。

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

只读档（更安全的默认）:

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

`alua` 不在 PATH 时:

```json
{
  "mcpServers": {
    "alua": {
      "command": "<ALUA>",
      "args": ["mcp", "serve", "--profile", "net"]
    }
  }
}
```

要点:

- `command` **不要**含空格（部分宿主会整段当可执行名解析失败）。
- 参数全部进 `args` 数组; 不要写成 `"command": "alua mcp serve"`。
- `stdout` 只能是协议帧; 勿把日志打到 stdout（本包日志走 stderr / 自有 log）。
- 改配置后按宿主要求 **重载 MCP / 重开窗口**。

HTTP 远程（宿主支持 `url`; 对接本机 **`alua serve`** 的 Streamable HTTP `/mcp`）:

```json
{
  "mcpServers": {
    "alua-http": {
      "url": "http://127.0.0.1:<PORT>/mcp",
      "headers": {
        "Authorization": "Bearer <ALUA_SERVE_TOKEN>"
      }
    }
  }
}
```

说明:

- 首选仍是 **stdio** `alua mcp serve`（上表）。
- HTTP 面挂在 **`alua serve`** 同口 `POST|GET|DELETE /mcp`（须有效 Bearer token; GET=SSE 听推送; POST 可 JSON 或 SSE 多帧）。
- `Host` 为本机（`127.0.0.1` / `localhost`）时允许 raw `ALUA_SERVE_TOKEN` 走明文 HTTP; 非本机仍须 TLS 面。
- CLI: `alua mcp probe|list|call|listen|close --url http://127.0.0.1:<PORT>/mcp --token <TOK>`。

---

## 2. Cursor（标准名 `mcp-cursor`）

| 项 | 值 |
| ---- | ---- |
| 标准名 | `mcp-cursor` |
| 用户级 | `~/.cursor/mcp.json`（Win: `%USERPROFILE%\.cursor\mcp.json`） |
| 工作区级 | `<项目根>/.cursor/mcp.json` |
| 顶键 | `mcpServers` |
| UI | Settings → MCP; 可查看启停与工具列表 |
| 台账 | 本地运维台账记「用户 MCP」是否已配 |

推荐（与 §1 同形）:

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

注意:

- 工作区文件可随项目共享; **勿**把含私人路径 / token 的条目提交到公开仓。
- Cursor 亦可走 HTTP MCP（若填 `url`）; 接本包仍以 **stdio** 为准。
- 本会话内置的 `cursor-ide-browser` / `cursor-computer-use` 是 Cursor 自带 MCP, **≠** 本包 `alua`。

---

## 3. CodeBuddy（腾讯云代码助手 · 标准名 `mcp-codebuddy`）

官方: https://www.codebuddy.cn/docs/cli/mcp 、IDE 指南 https://www.codebuddy.ai/docs/zh/ide/User-guide/MCP

| 项 | 值 |
| ---- | ---- |
| 标准名 | `mcp-codebuddy` |

| 作用域 | 路径（优先级高→低） |
| ---- | ---- |
| user（推荐） | `~/.codebuddy/.mcp.json` |
| user（旧） | `~/.codebuddy/mcp.json`（废弃）、`~/.codebuddy.json` |
| project（推荐） | `<项目根>/.mcp.json` |
| project（旧） | `<项目根>/mcp.json` |
| local | 写入 user 文件的 `projects.<workspace_path>.mcpServers` |

同名服务优先级: **local > project > user**。支持 JSONC（注释 / 尾逗号）。

推荐 stdio:

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

`type` 可省略（有 `command` 则推断 stdio）; **建议显式写**。

HTTP 形态（外部 MCP, 非本包 serve）:

```json
{
  "mcpServers": {
    "other": {
      "type": "http",
      "url": "http://127.0.0.1:<PORT>/mcp",
      "headers": {
        "Authorization": "Bearer <TOKEN>"
      }
    }
  }
}
```

UI: 对话面板 → CodeBuddy Settings → MCP → Add MCP。可用 `${VAR}` / `${VAR:-default}` 展开环境变量（字段见官方文档）。

**指纹注意**: 实测部分 CodeBuddy 构建 `clientInfo.name` 为 `Copilot`, 未设覆盖时 `host_tool` 可能落 `mcp-copilot` / `mcp-unknown`。推荐在 `env` 显式设标准名:

```json
{
  "mcpServers": {
    "alua": {
      "type": "stdio",
      "command": "alua",
      "args": ["mcp", "serve", "--profile", "net"],
      "env": {
        "ALUA_CALLER_NAME": "mcp-codebuddy"
      }
    }
  }
}
```

---

## 4. Trae（TraeCode · 标准名 `mcp-trae`）

官方: https://docs.trae.ai/ide/add-mcp-servers

| 项 | 值 |
| ---- | ---- |
| 标准名 | `mcp-trae` |
| 全局 | 设置 → MCP → 添加 / 手动添加; 或「原始配置（JSON）」写入宿主 mcp.json |
| 项目级 | `<项目根>/.trae/mcp.json`（须先打开 **启用项目级 MCP**） |
| 顶键 | `mcpServers` |
| 传输推断 | 有 `command` → stdio; 有 `url` → HTTP（通常**不必**写 `type`） |
| 变量 | 支持 `${workspaceFolder}` |
| 约束 | `command` **不能含空格** |

推荐 stdio（项目级示例）:

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

超时（stdio, 经 `env`）:

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

注意:

- **不要**照搬 VS Code 的 `.vscode/mcp.json` + 顶键 `servers`; Trae 用 `mcpServers`。
- Agent 窗口可区分「本地 / 云端」运行环境; 本包 alua 仅适合 **本地** stdio。
- 国区安装目录名可能是 `.trae-cn` 等; **项目级文件名仍以官方 `.trae/mcp.json` 为准**, 全局以 Settings UI / 原始 JSON 为准。

---

## 5. VS Code（标准名 `mcp-vscode`）

| 项 | 值 |
| ---- | ---- |
| 标准名 | `mcp-vscode` |
| 工作区 | `<项目根>/.vscode/mcp.json` |
| 用户级 | 用户 MCP 配置（见 VS Code / Agent 文档） |
| 顶键 | **`servers`**（非 `mcpServers`） |
| 差异 | stdio 项建议 `"type":"stdio"`; **勿**与 Cursor/Trae/CodeBuddy 的 `mcpServers` 混贴 |
| 核对 | 对照 VS Code Copilot/Agent MCP 文档: 顶键确为 `servers` + `type:stdio`（与 Cursor 系 `mcpServers` 不同） |

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

---

## 6. GitHub Copilot（标准名 `mcp-copilot`）

| 项 | 值 |
| ---- | ---- |
| 标准名 | `mcp-copilot` |
| 工作区 | `<项目根>/.mcp.json` |
| 用户级 | `mcp-config.json`（portable / 产品文档） |
| 顶键 | `mcpServers` |

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

与 §1 同形。项目共享可用工作区文件; **勿**提交私人路径 / token。

---

## 7. Claude Desktop（标准名 `mcp-claude` / `mcp-claude-desktop`）

| 项 | 值 |
| ---- | ---- |
| 标准名 | 默认伞名 `mcp-claude`; 指纹可细分 `mcp-claude-desktop` |
| macOS | `~/Library/Application Support/Claude/claude_desktop_config.json` |
| Win | `%APPDATA%\Claude\claude_desktop_config.json` |
| 顶键 | `mcpServers` |
| 传输 | 接本包仍以 **stdio** 为准 |

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

改配置后按 Claude Desktop 要求重启应用。

---

## 8. Claude Code（标准名 `mcp-claude` / `mcp-claude-code`）

| 项 | 值 |
| ---- | ---- |
| 标准名 | 伞名 `mcp-claude` 或细分 `mcp-claude-code` |
| 项目级 | `<项目根>/.mcp.json` |
| 用户级 | 见官方（如 `~/.claude.json`） |
| 顶键 | `mcpServers` |

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

与 §1 同形; **勿**与 §7 Claude Desktop 配置路径混淆。

---

## 9. Windsurf（标准名 `mcp-windsurf`）

| 项 | 值 |
| ---- | ---- |
| 标准名 | `mcp-windsurf` |
| 用户级 | `~/.codeium/windsurf/mcp_config.json`（Win: `%USERPROFILE%\.codeium\windsurf\mcp_config.json`） |
| 顶键 | `mcpServers` |
| 远程 | 部分宿主可用 `serverUrl` / `url` 接 HTTP MCP; 本包 serve 仍为 **stdio** |

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

---

## 10. 未知宿主（`mcp-unknown`）

无专节细节: 抄 **§1**。仍可接入 MCP, **不**挡用; 勿臆造宿主路径。

---

## 11. 标准安装后清单 + 接入自检

**前提**: 本机已 **标准安装** alua（`alua` 在 PATH；未裁 optional 包）。**不**在此写从零 OS 安装。

| 步 | 动作 | 合格 |
| ---- | ---- | ---- |
| 1 | `alua -v` | 打印版本 |
| 2 | `alua pkg list`（或 `--json`） | 有 **`mcp`** 行 |
| 3 | 按宿主专节写 JSON（`command`=`alua`, `args` 含 `mcp` `serve`） | 落盘到宿主路径 |
| 4 | 重载 MCP / 重开窗口 | 宿主出现可用的 **`alua`** |
| 5 | `runtime.ai_bootstrap` → `runtime.ai_load_ack` → `runtime.ping` | 工具可用 |

接入自检补充:

1. 可选: `alua mcp serve --profile readonly`（手动喂一行 initialize）。
2. 宿主 `tools/list` 可见 catalog 工具。
3. 失败常见因: `command` 含空格、PATH 无 `alua`、**mcp 包被裁**、日志打到 stdout、顶键抄错（`servers` vs `mcpServers`）、Trae 未开项目级开关。

AI 帮助面同步: `docs/tools/mcp.md` § After standard install / Self-check。  
协议与错误码 → `PROTOCOL.md`。会话指纹设计 → `DESIGN.md`。

## 12. 维护

### 12.1 新宿主接入顺序（强制）

1. **未专节前**：一律抄 **§1**，标准名 **`mcp-unknown`**（可接 MCP，不挡用）。
2. **专节 + 新 `mcp-xx`**：**须用户点名**；再写入 §0.1 枚举、本节专节、`DESIGN.md` §3.4、`fingerprint.HOST_TOOL_KNOWN`；并同步 `docs/tools/mcp.md`。
3. 配置 JSON 键始终 **`alua`**；勿用标准名当 `mcpServers` 键。

### 12.2 其它

- 新增时按「标准名 / 落点 / 顶键 / 与 §1 差异 / 示例」补专节。
- **配置键保持 `alua`**; 标准名只进指纹/文档/台账, 不改 JSON 服务键。
- 示例保持占位符; 勿写入私人机路径或全日期。
- CLI 变更时同步 §0 与各节 `args`。
