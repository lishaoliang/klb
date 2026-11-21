# MCP 协议与实现要求

> 路径: `klb/bin/klbagent/packages/mcp/PROTOCOL.md`
> 代码: `base/` / `server/` / `client/` / `schema/` / `http/`
> 用途: 官方 MCP 协议与实现要求摘要, 供本包对齐; **非**完整规范译文。
> 锚定修订: **2025-06-18**（本包 `PROTOCOL_VERSION`）

## 1. 权威出处

| 项 | URL |
| ---- | ---- |
| 规范首页 | https://modelcontextprotocol.io/specification/ |
| 锚定修订 (2025-06-18) | https://modelcontextprotocol.io/specification/2025-06-18/ |
| 较新修订 (2025-11-25) | https://modelcontextprotocol.io/specification/2025-11-25/ |
| 生命周期 | https://modelcontextprotocol.io/specification/2025-06-18/basic/lifecycle |
| 传输 | https://github.com/modelcontextprotocol/modelcontextprotocol/blob/main/docs/specification/2025-06-18/basic/transports.mdx |
| Tools | https://github.com/modelcontextprotocol/modelcontextprotocol/blob/main/docs/specification/2025-06-18/server/tools.mdx |
| Schema 真源 | 规范仓库 TypeScript `schema.ts`（JSON Schema 由其生成） |
| 文档索引 | https://modelcontextprotocol.io/llms.txt |

已见修订日期（不完全）: `2024-11-05` → `2025-03-26` → `2025-06-18` → `2025-11-25`（另有更新草案）。实现以**协商出的** `protocolVersion` 为准。

### 1.1 锚定版 vs 2025-11-25（差异摘要 · P3-3）

> 本包仍锚定 `2025-06-18`; 宿主若报 `2025-11-25` 且不在 `SUPPORTED_PROTOCOL_VERSIONS`, 回退锚定版（不断连）。升级本包版本前须先扩本表与 `PROTOCOL_VERSION`。

| 主题 | 2025-06-18（锚定） | 2025-11-25（较新） | 本包动作 |
| ---- | ---- | ---- | ---- |
| `protocolVersion` 字面 | `2025-06-18` | `2025-11-25` | 未入支持集 → 回退锚定 |
| Base / lifecycle / tools | MUST 同族 | 同族演进 | 保持现行 |
| Streamable HTTP | Server 可提供 HTTP 面 | 同; 细节修订 | serve `/mcp` POST JSON|SSE; GET SSE; DELETE |
| `tools/list` 分页 | `cursor` / `nextCursor` | 同 | 无 cursor=全量; 有 cursor=分页 |
| `outputSchema` | 可选 | 可选; 有则 structuredContent 须符合 | 有 `result_schema` 时导出 |
| `listChanged` | 可声明 | 同 | tools=`true`; reload 后通知 |
| resources / prompts | MAY | MAY | list 空占位; read/get 待内容 |

## 2. 角色与分层

| 角色 | 含义 |
| ---- | ---- |
| **Host** | 启动连接的 LLM 应用（IDE / Agent 等） |
| **Client** | Host 内的 MCP 连接器 |
| **Server** | 对外提供 context / capabilities 的服务 |

协议层（规范要求）:

| 组件 | 要求 |
| ---- | ---- |
| **Base Protocol** | **MUST** 支持（JSON-RPC 消息） |
| **Lifecycle** | **MUST** 支持（initialize / initialized / 能力协商） |
| Authorization | HTTP 传输 **SHOULD** 遵循; **stdio 不应**套 HTTP Auth, 凭证取自环境 |
| Server Features | `tools` / `resources` / `prompts` 等 **MAY** 按需实现 |
| Client Features | `roots` / `sampling` / `elicitation` 等 **MAY** |
| Utilities | `logging` / `completions` / `ping` 等横切能力 |

## 3. JSON-RPC 消息（强制）

所有消息 **MUST** 符合 JSON-RPC 2.0; 编码 **MUST** 为 UTF-8。

### 3.1 Request

```json
{"jsonrpc":"2.0","id":1,"method":"initialize","params":{}}
```

- `id` **MUST** 为 `string` 或 `number`; **MUST NOT** 为 `null`（严于裸 JSON-RPC）。
- 同一会话内, 请求方 **MUST NOT** 复用已用过的 `id`。

### 3.2 Response

成功:

```json
{"jsonrpc":"2.0","id":1,"result":{}}
```

失败:

```json
{"jsonrpc":"2.0","id":1,"error":{"code":-32602,"message":"..."}}
```

- 成功响应 **MUST** 带与请求相同的 `id` 与 `result`。
- 错误响应 **MUST** 带 `error.code`（整数）与 `error.message`; **MUST NOT** 同时设 `result` 与 `error`。
- 无法解析请求 `id` 时, 错误响应的 `id` 可为缺省。

### 3.3 Notification

```json
{"jsonrpc":"2.0","method":"notifications/initialized"}
```

- **MUST NOT** 带 `id`。
- 接收方 **MUST NOT** 回响应。

### 3.4 标准错误码（JSON-RPC）

| 码 | 名 |
| ---- | ---- |
| `-32700` | Parse error |
| `-32600` | Invalid request |
| `-32601` | Method not found |
| `-32602` | Invalid params |
| `-32603` | Internal error |

MCP 可另保留应用区间（不同修订略有差异; 如资源未找到等历史码）。业务工具失败优先走 **Tool Result** 的 `isError: true`, 而非一律 JSON-RPC error。

## 4. 生命周期（强制）

顺序 **MUST**:

1. Client → Server: `initialize`（带 `protocolVersion` / `capabilities` / `clientInfo`）
2. Server → Client: `InitializeResult`（`protocolVersion` / `capabilities` / `serverInfo`, 可选 `instructions`）
3. Client → Server: `notifications/initialized`
4. 进入 Operation; 结束时靠传输层断开（无专用 shutdown RPC）

约束:

- `initialize` **MUST** 为双方首个交互（可例外允许 `ping`）。
- Client 在收到 `initialize` 响应前, **SHOULD NOT** 发其它请求（ping 除外）。
- Server 在收到 `notifications/initialized` 前, **SHOULD NOT** 发其它请求（ping / logging 除外）。
- 版本: Client 声明其支持版本; Server 若支持则回相同版本, 否则回自有支持版本; Client 不支持则 **SHOULD** 断开。
- 双方 Operation 期间 **MUST** 只使用已协商能力。
- 实现 **SHOULD** 为请求设超时; 超时 **SHOULD** 发 cancellation 并停止等待。

### 4.1 能力键（摘要）

| 侧 | 键 | 含义 |
| ---- | ---- | ---- |
| Client | `roots` | 提供文件系统根 |
| Client | `sampling` | 接受 server 侧 LLM sampling 请求 |
| Client | `elicitation` | 接受 elicitation |
| Client | `experimental` | 实验能力 |
| Server | `tools` | 可调用工具（可含 `listChanged`） |
| Server | `resources` | 可读资源（可含 `subscribe` / `listChanged`） |
| Server | `prompts` | 提示模板（可含 `listChanged`） |
| Server | `logging` | 结构化日志 |
| Server | `completions` | 参数补全 |
| Server | `experimental` | 实验能力 |

## 5. 传输

现行标准传输两种; Client **SHOULD** 尽量支持 **stdio**。

### 5.1 stdio（本包 server 主路径）

| 规则 | 级别 |
| ---- | ---- |
| Client 以子进程启动 Server | 规范描述 |
| 一帧 = 一条 JSON-RPC; **换行分隔**; 帧内 **MUST NOT** 含嵌入换行 | **MUST** |
| `stdout` **MUST NOT** 输出非 MCP 消息 | **MUST** |
| `stdin` **MUST NOT** 写入非 MCP 消息 | **MUST** |
| `stderr` **MAY** 打 UTF-8 日志; Client **MAY** 忽略 | **MAY** |
| 关闭: Client 关 stdin → 等退出 → `SIGTERM` → `SIGKILL` | **SHOULD** |

### 5.2 Streamable HTTP（本包 client 主路径; 替代 2024-11-05 的 HTTP+SSE）

| 规则 | 级别 |
| ---- | ---- |
| 单一 MCP endpoint, 支持 POST 与 GET | **MUST** |
| Client POST 发消息; `Accept` 含 `application/json` **与** `text/event-stream` | **MUST** |
| POST body = **单条** JSON-RPC request / notification / response | **MUST** |
| notification/response 被接受 → HTTP **202** 无 body | **MUST** |
| request → `application/json` **或** SSE `text/event-stream`; Client **MUST** 两者都支持 | **MUST** |
| GET 可开 SSE 听 server→client; 不支持则 **405** | **MUST** |
| 校验 `Origin` 防 DNS rebinding; 本机 **SHOULD** 只绑 `127.0.0.1` | **MUST** / **SHOULD** |
| 会话: `InitializeResult` 响应可带 `Mcp-Session-Id`; 之后请求 **MUST** 带回 | **MAY** / **MUST** |
| 后续 HTTP 请求带 `MCP-Protocol-Version: <协商版本>` | **MUST**（HTTP） |
| 无版本头且无其它依据时, Server **SHOULD** 假定 `2025-03-26` | **SHOULD** |
| 不支持的 `MCP-Protocol-Version` → HTTP **400** | **MUST** |
| 显式结束会话: DELETE + `Mcp-Session-Id`（Server 可 **405**） | **SHOULD** / **MAY** |

`Mcp-Session-Id` 字符: 可见 ASCII `0x21`–`0x7E`; **SHOULD** 全局唯一且密码学安全。

### 5.3 自定义传输

**MAY** 自定, 但 **MUST** 保持 JSON-RPC 格式与生命周期; **SHOULD** 文档化建连与交换模式。

## 6. Server: Tools（本包核心面）

声明能力:

```json
{"capabilities":{"tools":{"listChanged":false}}}
```

| 方法 | 作用 |
| ---- | ---- |
| `tools/list` | 列举工具; 支持分页 `cursor` / `nextCursor` |
| `tools/call` | 调用; params: `name` + `arguments` |
| `notifications/tools/list_changed` | 列表变更通知（声明了 `listChanged` 时 **SHOULD**） |

### 6.1 Tool 定义字段

| 字段 | 要求 |
| ---- | ---- |
| `name` | 唯一标识 |
| `title` | 可选显示名 |
| `description` | 人类可读说明 |
| `inputSchema` | JSON Schema（期望参数） |
| `outputSchema` | 可选; 有则 Server **MUST** 使 `structuredContent` 符合之 |
| `annotations` | 可选行为标注; Client **MUST** 视为不可信（除非信任该 Server） |

JSON Schema 方言: 缺 `$schema` 时默认 **JSON Schema 2020-12**; 实现 **MUST** 至少支持 2020-12。

### 6.2 CallToolResult

- 非结构化: `content[]`（`text` / `image` / `audio` / `resource_link` / embedded `resource` 等）。
- 结构化: `structuredContent` 对象; 有结构化时 **SHOULD** 同时在 `content` 放序列化 JSON 文本（兼容）。
- 工具执行失败: **result** 内 `isError: true`（仍是成功 JSON-RPC 帧）。
- 未知工具 / 非法参数等: JSON-RPC **error**（如 `-32602`）。

安全（规范措辞）:

- Server **MUST** 校验输入、访问控制、限流、净化输出。
- Client **SHOULD** 敏感操作人工确认、展示入参、校验结果、设超时、审计日志。

## 7. 其它方法面（摘要）

| 域 | 典型方法 | 备注 |
| ---- | ---- | ---- |
| Resources | `resources/list` / `resources/read` / templates / subscribe | 声明 `resources` 后 |
| Prompts | `prompts/list` / `prompts/get` | 声明 `prompts` 后 |
| Logging | `logging/setLevel`; server 侧日志通知 | 声明 `logging` |
| Utilities | `ping` → `{}` | 生命周期例外允许 |
| Client→Server 反向 | `roots/list`; sampling / elicitation | 依赖 Client capabilities |
| 取消 / 进度 | cancellation / `notifications/progress` + `progressToken` | 超时与长任务 |

HTTP Auth: 仅面向 HTTP 传输; stdio **SHOULD NOT** 套同一套, 改从环境取凭据。

通用字段 `_meta`: 保留给协议级元数据; 前缀含 `modelcontextprotocol` / `mcp` 作为第二标签的 DNS 前缀为保留。

## 8. 与本包实现对齐

| 项 | 本包现状 (`packages/mcp`) |
| ---- | ---- |
| 锚定版本 | `2025-06-18`（`base/const.lua`; server/client 再导出） |
| Server 传输 | **stdio** 换行帧; 另挂 `alua serve` **/mcp**（POST JSON|SSE; GET SSE; DELETE） |
| Client 传输 | Streamable HTTP POST + GET listen + DELETE close; 解析 JSON / 多帧 SSE |
| 强制面 | `initialize` / `ping` / `tools/list` / `tools/call` |
| resources/prompts | `resources/list|read`（`alua://docs/*`）; `prompts/list|get`（内置 2 条）; `logging/setLevel` 旁路 |
| Tool 结果 | `content[text]` + 可选 `structuredContent`; 失败 `isError` |
| inputSchema | `schema/`: `args_schema` → JSON Schema object + `$schema` 2020-12 |
| outputSchema | catalog `result_schema` 存在时导出 (同 schema 转换) |
| listChanged | tools=`true`; `runtime.reload` 成功后 `notifications/tools/list_changed` |
| tools/list | 无 cursor=全量; 有 cursor 则分页 (页长 64 / `nextCursor`); 独立 `title`/`annotations` |
| request id | 同会话段拒绝重复 id |
| 取消/进度 | `notifications/cancelled`; `notifications/progress` + `_meta.progressToken` |
| 反向能力 | 登记 Client `roots`/`sampling`/`elicitation`; 有 roots 则异步 `roots/list`; 缺能力降级 |
| 通知 | 无 `id` 不回包; 旁路记录 method/时刻; `initialized` 开门禁 |
| Auth | stdio 无 HTTP Auth; HTTP client 可选 `token`; CLI `--timeout` |

**`alua serve` 面 token 口径**（实测 `0.1.195.202611`）:

| 面 | 明文传输 + 原始 token | 说明 |
| ---- | ---- | ---- |
| `/v1/call` / `/v1/health` / `ssid.ensure` | **拒**: `DENIED raw token requires tls`（code 20） | 原始 token 只走 TLS 面 |
| `/mcp` + **loopback** Host | **放行**（本机自举例外, 非信任提升） | 同口另一套 token 口径 |
| `/mcp` + 局域网 IP | **401** | 非 loopback 明文一律拒 |

临时 token 仅**签发它的 serve 进程内**有效（内存表）: 异进程 `ssid.ensure` 所得 token 在该 serve 侧 `401`; 须经该 serve 的 `/v1/call`（原始 token + TLS）或 MCP/`ssid.ensure` 签发。

指纹/会话设计 → `DESIGN.md`。

## 9. 接入检查清单（实现者）

**Server (stdio) 最低:**

1. UTF-8 换行分隔 JSON-RPC; stdout 纯净。
2. 首包处理 `initialize`, 回 `protocolVersion` / `capabilities` / `serverInfo`。
3. 接受 `notifications/initialized`（不回包）。
4. 声明并实现所需能力（本包至少 `tools`）。
5. `tools/list` + `tools/call` 形态符合 §6; 错误分流（协议 vs `isError`）。
6. 标准错误码可用。

**Client (HTTP) 最低:**

1. POST + 双 `Accept`; 兼容 JSON 与 SSE。
2. `initialize` 后发 `notifications/initialized`。
3. 保存并回传 `Mcp-Session-Id`; 后续带 `MCP-Protocol-Version`。
4. 仅使用协商能力与版本。

## 10. 维护

- 官方修订升级时: 先对照本文件 §1–§7, 再改 `PROTOCOL_VERSION` 与 server/client。
- 勿把客户端自报指纹当鉴权（见 `DESIGN.md` §6）。
- 本文是摘要; 冲突以官方规范页与 `schema.ts` 为准。
- 各 AI 宿主配置落点与 JSON 示例 → `CLIENTS.md`；AI 经 MCP 读帮助 → `docs/tools/mcp.md`。
- 指纹/会话设计 → `DESIGN.md`。
