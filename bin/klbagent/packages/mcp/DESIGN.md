# mcp 包设计

> 路径: `klb/bin/klbagent/packages/mcp/DESIGN.md`
> 代码: `base/` / `server/` / `client/` / `schema/` / `http/`
> 用途: 本包设计（指纹/会话/模块落点/边界）; **非**协议译文、**非**宿主配置抄录。
> 相关: `PROTOCOL.md`（协议）; `CLIENTS.md`（宿主配置）

## 1. 定位

| 方向 | 入口 | 传输 |
| ---- | ---- | ---- |
| Server（导出本机 catalog 工具） | `alua mcp serve [--profile P]` | stdio JSON-RPC |
| Client（消费外部 MCP） | `alua mcp probe\|list\|call\|listen\|close --url …` | Streamable HTTP（POST + GET SSE + DELETE） |

锚定协议修订: **2025-06-18**（见 `PROTOCOL.md`）。默认档 `readonly`; 常用 `net` / `device` / `unsafe`。

**调用方不限于 AI**: alua 工具可被 AI 宿主、人类 CLI、脚本/CI、远端 peer、自研 MCP client 等调用。指纹/会话标签须能**识别并记录**这些类别（见 §2–§4）; 实现落点以 `via` + `caller_kind` 区分。

## 2. 目标

- 采集接入方**指纹**, 用于会话归因、排障与多调用方区分（含 **AI 与非 AI**）。
- 指纹只作**标签**: 不当鉴权依据, 不参与档位判定。
- 指纹属本机布局/用户环境边缘信息: 默认只落本地会话与本地日志, **禁止**进入对外文档与 Gitee 发布面。
- MCP 协议行为与 `PROTOCOL.md` 对齐; AI 宿主接入与 `CLIENTS.md` 可验; 非 AI 入口与 §3.2 信号表可验。

## 3. 调用方分类与指纹

### 3.1 类别（`caller_kind`）

| caller_kind | 含义 | 典型入口 |
| ---- | ---- | ---- |
| `ai_host` | AI IDE / Agent 拉起的 MCP 或等价宿主 | Cursor / CodeBuddy / Trae / Claude / VS Code Agent |
| `human_cli` | 人类在终端直接敲 | `alua call` / `alua status` / 交互 shell |
| `script` | 非交互脚本 / Makefile / 批处理 | cron、包装 sh/bat、测试驱动 |
| `ci` | 流水线环境 | CI 环境变量可辨时 |
| `remote_peer` | 远端 alua / HTTP serve 对端 | `alua serve` + `remote.call` 等 |
| `mcp_client` | 自研或第三方 **非 AI** MCP client（仍走 MCP 协议） | 自写 stdio/HTTP MCP 客户端 |
| `unknown` | 无法归类 | 信号不足 |

判定为粗标签, **可误判、可伪造**; 冲突时保留多信号, 主标签取置信最高者。

### 3.2 通道（`via`）

| via | 通道 | 指纹主来源 |
| ---- | ---- | ---- |
| `mcp` | `alua mcp serve` stdio | MCP `initialize` + 父进程/env + cwd |
| `call` | 一次性 `alua call` | argv/tty/env/父进程; 无 MCP 握手 |
| `stdio` | `alua stdio` 行协议 | 同 call + 连接段会话 |
| `serve` / `http` / `https` | `alua serve` HTTP(S) | `User-Agent` / token 侧写 / 对端地址哈希 |
| `remote` | `remote.call` 等 | peer id / 对端 hostinfo 摘要 |
| 其它 | lpc / mux 等 | 按现有 envelope `via` 扩展 |

**原则**: 凡经 `dispatch.call` 的工具调用, 最终日志/会话 attrs 须带 `via` + `caller_kind` + 可用指纹子集; MCP 包负责 `via=mcp` 面采集, 其它 via 由 core/cli/serve 同模型落盘。

### 3.3 可采指纹（MCP 面 + 共用宿主面）

| 层 | 字段 | 来源 | 适用 | 可靠性 |
| ---- | ---- | ---- | ---- | ---- |
| 握手 | `clientInfo.name` / `version` / `title` | MCP `initialize` | `via=mcp` | 高(可伪造) |
| 握手 | `protocolVersion` | MCP `initialize` | `via=mcp` | 高 |
| 握手 | `capabilities.*` 键集合 | MCP `initialize` | `via=mcp` | 中 |
| 行为 | `notifications/initialized` 时刻 | 通知帧 | `via=mcp` | 中 |
| 行为 | `logging/setLevel` level | 请求帧 | `via=mcp` | 中 |
| 行为 | `tools/list` 分页/重复; `tools/call` `_meta` 键名 | 请求帧 | `via=mcp` | 低 |
| 宿主 | ppid / 父进程名 / cwd 指纹 | `/proc` 等 | **全 via** | 中 |
| 宿主 | env 指纹(AI: `CURSOR_*`…; 非 AI: `CI`/`GITHUB_ACTIONS`/`GITLAB_CI`/`JENKINS_*`/`TERM`/`SSH_CLIENT` 等) | `os.getenv` | **全 via** | 中 |
| 终端 | stdin 是否 tty; 有无 `PS1`/`alua` 交互迹象 | isatty / env | `call`/`stdio` | 中 |
| 传输 | `User-Agent` / `Accept` / `MCP-Protocol-Version` / `Mcp-Session-Id` | HTTP 头 | MCP HTTP / serve | 高 |
| 反向 | `roots/list` 工作区路径 | server→client | `via=mcp` 且 client 声明 roots | 高(需实现) |
| 对端 | peer / remote 摘要 | serve / remote | `remote_peer` | 中 |

### 3.4 信号 → 标签粗映射

**AI 标准名**（字段 `host_tool` = `mcp-xx`; 配置 JSON 键仍为 **`alua`**, 见 `CLIENTS.md` §0.1）:

| 标准名 `host_tool` | 商品名 | 倾向信号 |
| ---- | ---- | ---- |
| `mcp-cursor` | Cursor | `CURSOR_*`; 父进程 / `clientInfo` 含 cursor |
| `mcp-codebuddy` | CodeBuddy | `CODEBUDDY_*`; `.codebuddy` |
| `mcp-trae` | Trae | `TRAE_*`; `.trae/mcp.json` |
| `mcp-vscode` | VS Code Agent | `VSCODE_*`（非 Copilot 名时） |
| `mcp-copilot` | GitHub Copilot | `clientInfo` / 名含 copilot |
| `mcp-claude` | Claude（伞名） | `CLAUDE_*`; 名含 claude |
| `mcp-claude-desktop` | Claude Desktop | 可选细分（名含 desktop） |
| `mcp-claude-code` | Claude Code | 可选细分（名含 code 且非笼统） |
| `mcp-windsurf` | Windsurf | 名含 windsurf / codeium |
| `mcp-unknown` | 未适配 / 未知 AI | `via=mcp` 且无上表命中; **仍可接 MCP** |

实现落点: `klbagent.core.ssid.fingerprint.resolve_host_tool` / `material.host_tool` / `ssid.whoami.host_tool`。

**AI 宿主**（`caller_kind=ai_host`, 多见于 `via=mcp`）: 上表除 `mcp-unknown` 外命中, 或 env 有 AI 信号。

**非 AI**（须同等记录）:

| 标签 | 倾向信号 |
| ---- | ---- |
| human_cli | tty + 无 AI/CI env; 父进程为 shell(bash/zsh/cmd/pwsh) |
| script | 非 tty; 父进程 sh/python/make; 或显式 `ALUA_CALLER_KIND=script` |
| ci | `CI=true` / `GITHUB_ACTIONS` / `GITLAB_CI` / `JENKINS_*` 等 |
| mcp_client | `via=mcp` 且 `host_tool=mcp-unknown` 且无可用 `clientInfo.name` |
| remote_peer | `via` 为 serve/remote 且有对端会话 |
| unknown | 其它 |

可选显式覆盖（仍只作标签）: `ALUA_CALLER_KIND` / `ALUA_CALLER_NAME`（`ALUA_CALLER_NAME` 取 `mcp-xx`; **勿**当鉴权）。过时 hint 值 `mcp` **忽略**, 改由本表判定。

## 4. 会话模型

MCP 面:

```
宿主或非 AI MCP client 拉起 `alua mcp serve`
  → 实例 id = host_tool + pid + 启动时刻 + cwd 指纹
  → core.session.begin（attrs: via=mcp, caller_kind, 指纹）
  → initialize（握手指纹写入 attrs; 修正 caller_kind）
  → notifications/initialized
  → tools/call 注入 session_id
  → （可选）idle / 再次 initialize → 新 segment
```

非 MCP 面（同模型, 无握手则跳过 initialize 段）:

```
alua call | stdio | serve
  → 若无会话则 begin（attrs: via, caller_kind, 宿主指纹）
  → dispatch.call 带 session_id / via
  → log 落盘同一字段集
```

- 无 MCP conversation id: 最高粒度 = 连接段 + 空闲分段。
- `ssid.whoami` 回显: `session_id` / `via` / `caller_kind` / `host_tool`（`mcp-xx`）/（mcp 时）`client_*` / `protocol_version` 等（已并入原 `runtime.session_whoami`）。

## 5. 模块落点（设计）

| 模块 | 职责 |
| ---- | ---- |
| `packages/mcp/init.lua` | **包对外入口** (包外须经此): `mcp.server()` / `mcp.client()` / `mcp.http()` (取面函数, 禁表代接) |
| `packages/mcp/server/init.lua` | server 对外面 (实体转发): `serve_entry` / `co_with_http_lock` / HTTP 会话辅助 / `notify_tools_list_changed` … |
| `packages/mcp/server/service.lua` | **内部**实现: JSON-RPC / tools / resources / prompts / 指纹; `serve_entry` fork 读循环 |
| `packages/mcp/http/init.lua` | http 对外面: `mount` / `co_handle` |
| `packages/mcp/http/handler.lua` | **内部**实现: Streamable HTTP POST/GET/DELETE |
| `packages/mcp/client/init.lua` | client 对外面: `new_context` / `initialize` / `list_tools` / `call_tool` / `ping` / `listen` / `close` |
| `packages/mcp/client/service.lua` | **内部**实现: HTTP JSON-RPC / SSE |
| `packages/mcp/base/` / `schema/` | **内部**; 仅 server/client 实现依赖 |
| `core/session` | 会话 id（CSPRNG）; `origin` / `via` / `caller_kind` / 指纹 attrs |
| `core/hostinfo` | `host_tool`（ppid / env / cwd / tty / CI 信号） |
| `core/cli` / `core/serve` / `core/stdio` | 非 MCP via 启动时 begin 会话并打标签 |
| `core/dispatch` | 统一带 `via`/`session_id` 记调用 |
| `core/log` | 日志字段: `session_id` / `via` / `caller_kind` / `tool` |
| `ssid.whoami` | 只读回显 ssid + 会话指纹摘要 |

生命周期与版本协商 → `PROTOCOL.md` §4。

## 6. 边界

- 客户端自报与环境字段**均可伪造**, 仅作标签; **禁止**用指纹做鉴权或提权。
- 高敏数据（如 `sampling` 提示词、完整 cwd 明文）一律不落盘或只存 hash; 路径/ppid 只存 hash 或工具名。
- 文档/示例**禁止**私人绝对路径、全日期、口令; 占位符见 `CLIENTS.md` §0。
- HTTP MCP: `alua serve` 同口 **`/mcp`**（POST JSON|SSE; GET SSE; DELETE; Bearer token; loopback 可 raw）。stdio 仍为宿主首选。
- **ssid**: MCP（含 `alua mcp serve` / `/mcp`）一律 `ssid.new_client({transport="xpc"})` → owner `alua.ssid`; **禁** `new_serve`。须本机已起 ssid XPC（通常随 `alua serve`）。
- `resources`: `alua://docs/<rel>` 映射 `docs.list`/`docs.read`; `prompts`: 内置 `alua-ai-bootstrap` / `alua-mcp-overview`。
- 非 AI 与 AI **同一套** attrs 字段; 不得因 `caller_kind!=ai_host` 跳过记录。
- `ai_load` ack 属**进程级**: `serve` 内任一调用方 ack 后全进程业务工具放行; `via=mcp` 通道门禁默认关（`channel=mcp`）, 依赖宿主按 instructions 自行 ack。ack 与指纹同属**标签/体验**, **不作**鉴权依据。
- 临时 token 仅**进程内**（serve 内存表）: 异进程 `ssid.ensure` 所得 token 在该 serve 侧无效; 签发须经该 serve 的 `/v1/call`（原始 token + TLS）或 `ssid.ensure` / MCP。

## 7. 文档分工

| 文档 | 职责 |
| ---- | ---- |
| `DESIGN.md` | 本文件: 目标、调用方分类、指纹、会话、落点、边界 |
| `PROTOCOL.md` | 官方协议 MUST/SHOULD 与本包对齐表 |
| `CLIENTS.md` | 各 **AI** 宿主配置落点与 JSON 示例; **`mcp-xx` 标准名表** |
| `docs/packages.md` § mcp | AI 可读 CLI 摘要（英文） |

变更规则: 改设计意图（含非 AI 指纹）→ `DESIGN.md`; 改协议行为 → `PROTOCOL.md`; 改 AI 接入示例 / 标准名 → `CLIENTS.md` §0.1; 新 AI 宿主 → 分配 `mcp-xx` + `CLIENTS.md` + 本文件 §3.4; 新非 AI 信号 → 本文件 §3.2–§3.4。
