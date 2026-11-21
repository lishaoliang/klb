--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   service.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  MCP server 实现 (stdio JSON-RPC; tools/resources/prompts)
-- @stability internal
-- @impl evolving
-- @note   包外勿直 require; 对外面见 server/init.lua
--   \n 每行一个 JSON-RPC; stdout 仅协议帧
--   \n ssid 一律 new_client({transport="xpc"}); 禁 new_serve
-- @history 修改历史
--  \n [2026-10] 创建文件 (P1-1)
--  \n [2026-11] 自 server/init 拆出实现
--  \n [2026-11] serve→serve_entry; fork 读循环, 禁入口内 while 卡死
--]]


local cjson = require("cjson")
local kenv = require("kenv")
local kco = require("kco")
local catalog = require("klbagent.core.catalog")
local dispatch = require("klbagent.core.dispatch")
local profiles = require("klbagent.core.profiles")
local ai_load_gate = require("klbagent.core.ai_load_gate")
local session = require("klbagent.core.session")
local version = require("klbagent.version")
local mcp_const = require("klbagent.packages.mcp.base.const")
local mcp_schema = require("klbagent.packages.mcp.schema")


local M = {}


M.PROTOCOL_VERSION = mcp_const.PROTOCOL_VERSION
M.SUPPORTED_PROTOCOL_VERSIONS = mcp_const.SUPPORTED_PROTOCOL_VERSIONS


local PARSE_ERROR = mcp_const.PARSE_ERROR
local INVALID_REQUEST = mcp_const.INVALID_REQUEST
local METHOD_NOT_FOUND = mcp_const.METHOD_NOT_FOUND
local INVALID_PARAMS = mcp_const.INVALID_PARAMS
local INTERNAL_ERROR = mcp_const.INTERNAL_ERROR
local IDLE_SEGMENT_MS = mcp_const.IDLE_SEGMENT_MS
local TOOLS_PAGE_SIZE = mcp_const.TOOLS_PAGE_SIZE

-- 进度通知前向声明 (_write 之后赋值; _tools_call 闭包引用)
local _notify_progress


-- initialize 握手指纹上下文: tools/call begin_ctx / ssid.* 复用 (host_tool=mcp-xx)
-- 注意: HTTP 多会话时会切换指向; 见 activate_http_session
local g_mcp_ssid = nil
local g_http_sessions = {}
local g_http_busy = false


local function _now_ms()
	return math.floor(os.time() * 1000)
end


-- @brief 取 ssid XPC 客户端 (跨进程 → owner alua.ssid)
-- @return client[table|nil]
local function _ssid_xpc()
	local ok_s, pkg = pcall(require, "klbagent.core.ssid")
	if not (ok_s and type(pkg) == "table" and type(pkg.new_client) == "function") then
		return nil
	end

	return pkg.new_client({ transport = "xpc" })
end


local function _log_bypass(method, extra)
	local ok_log, log = pcall(require, "klbagent.core.log")
	if not (ok_log and type(log) == "table" and type(log.write) == "function") then
		return
	end

	local fields = {
		module = "mcp.server",
		method = method,
		via = "mcp",
		session_id = g_mcp_ssid.session_id,
		host_tool = g_mcp_ssid.host_tool,
		caller_kind = g_mcp_ssid.caller_kind,
		alua_ssid = g_mcp_ssid.alua_ssid,
		segment = g_mcp_ssid.segment,
		ms = _now_ms(),
	}
	if type(extra) == "table" then
		for k, v in pairs(extra) do
			if type(k) == "string" then
				fields[k] = v
			end
		end
	end

	log.write("info", "mcp bypass", fields)
end


local function _negotiate_protocol_version(want)
	return mcp_const.negotiate_protocol_version(want)
end


-- @brief 新建连接段 session (initialize 或 idle 切段)
-- @note protocol_version 记协商值; 客户端请求值另落 client_protocol_version
local function _begin_segment(params, reason)
	local client_name = nil
	local client_want = nil
	if type(params) == "table" then
		client_want = params.protocolVersion or params.protocol_version
		local ci = params.clientInfo or params.client_info
		if type(ci) == "table" then
			client_name = ci.name
		end
	end

	local protocol_version = g_mcp_ssid.protocol_version
	if type(protocol_version) ~= "string" or protocol_version == "" then
		protocol_version = _negotiate_protocol_version(client_want)
	end

	g_mcp_ssid.segment = (tonumber(g_mcp_ssid.segment) or 0) + 1
	g_mcp_ssid.last_activity_ms = _now_ms()
	g_mcp_ssid.seen_ids = {}
	g_mcp_ssid.cancelled_ids = {}
	g_mcp_ssid.current_req_id = nil
	g_mcp_ssid.reverse_pending = {}
	g_mcp_ssid.roots = nil
	g_mcp_ssid.reverse_err = nil

	local info = session.begin({
		attrs = {
			via = "mcp",
			caller_kind = g_mcp_ssid.caller_kind,
			host_tool = g_mcp_ssid.host_tool,
			alua_ssid = g_mcp_ssid.alua_ssid,
			client_name = client_name,
			protocol_version = protocol_version,
			client_protocol_version = tostring(client_want or ""),
			segment = g_mcp_ssid.segment,
			init_count = g_mcp_ssid.init_count,
			segment_reason = reason or "initialize",
		},
	})
	if type(info) == "table" and type(info.id) == "string" then
		g_mcp_ssid.session_id = info.id
		pcall(session.set_current, info.id)
		local ok_log, log = pcall(require, "klbagent.core.log")
		if ok_log and type(log) == "table" and type(log.write) == "function" then
			log.write("info", "mcp session begin", {
				module = "mcp.server",
				session_id = info.id,
				via = "mcp",
				host_tool = g_mcp_ssid.host_tool,
				caller_kind = g_mcp_ssid.caller_kind,
				alua_ssid = g_mcp_ssid.alua_ssid,
				client_name = client_name,
				segment = g_mcp_ssid.segment,
				init_count = g_mcp_ssid.init_count,
				reason = reason or "initialize",
			})
		end
	end
end


-- @brief 活动触达; 空闲超时则切段 (M7)
local function _touch_activity(params)
	local now = _now_ms()
	local last = tonumber(g_mcp_ssid.last_activity_ms) or 0
	if last > 0 and (now - last) >= IDLE_SEGMENT_MS then
		_begin_segment(params or g_mcp_ssid.mcp_init, "idle")
	else
		g_mcp_ssid.last_activity_ms = now
	end
end


-- @brief 生命周期门禁 (M1): initialized 前仅放行 initialize/ping/logging/*
local function _lifecycle_allows(method)
	if method == "initialize" then
		return true
	end

	if method == "notifications/initialized" then
		return g_mcp_ssid.handshake_done == true
	end

	if g_mcp_ssid.initialized == true then
		return true
	end

	if method == "ping" then
		return true
	end

	if string.sub(method, 1, 8) == "logging/" then
		return true
	end

	return false
end


-- HTTP 捕获缓冲: 非 nil 时 _write 写入表而非 stdout (P4-1)
local g_capture = nil


-- @brief HTTP 会话出站通知入队 (供 GET SSE)
local function _enqueue_sse(st, obj)
	if type(st) ~= "table" or type(obj) ~= "table" then
		return
	end

	if type(st.sse_q) ~= "table" then
		st.sse_q = {}
	end

	st.sse_q[#st.sse_q + 1] = obj
end


local function _write(obj)
	if type(g_capture) == "table" then
		g_capture[#g_capture + 1] = obj
		return
	end

	-- HTTP 会话: 非捕获期写入 GET SSE 队列 (stdio 无 http_sid)
	if type(g_mcp_ssid) == "table" and type(g_mcp_ssid.http_sid) == "string"
		and g_mcp_ssid.http_sid ~= "" then
		_enqueue_sse(g_mcp_ssid, obj)
		return
	end

	local ok, text = pcall(cjson.encode, obj)
	if not ok or type(text) ~= "string" then
		return
	end

	io.write(text)
	io.write("\n")
	io.flush()
end


-- @brief 新建 MCP 会话状态表 (stdio 单例 / HTTP 每 Mcp-Session-Id)
local function _new_state()
	return {
		via = "mcp",
		mcp_init = nil,
		alua_ssid = nil,
		host_tool = nil,
		session_id = nil,
		caller_kind = nil,
		handshake_done = false,
		initialized = false,
		init_count = 0,
		segment = 0,
		last_activity_ms = 0,
		protocol_version = nil,
		log_level = nil,
		seen_ids = {},
		cancelled_ids = {},
		current_req_id = nil,
		client_caps = nil,
		reverse_pending = {},
		reverse_seq = 0,
		roots = nil,
		reverse_err = nil,
		http_sid = nil,
		sse_q = {},
	}
end


g_mcp_ssid = _new_state()


-- @brief 进度通知 (P2-7; 绑定前向声明)
_notify_progress = function (token, progress, total)
	if token == nil then
		return
	end

	local params = {
		progressToken = token,
		progress = progress,
	}
	if type(total) == "number" then
		params.total = total
	end

	_write({
		jsonrpc = "2.0",
		method = "notifications/progress",
		params = params,
	})
end


-- @brief Client 是否声明反向能力 (P2-2)
local function _client_has_cap(name)
	local caps = g_mcp_ssid.client_caps
	if type(caps) ~= "table" or type(name) ~= "string" or name == "" then
		return false
	end

	local v = caps[name]
	if v == nil or v == false then
		return false
	end

	return true
end


-- @brief 方法 → 所需 Client capability
local function _reverse_cap_for(method)
	if method == "roots/list" then
		return "roots"
	end

	if method == "sampling/createMessage" then
		return "sampling"
	end

	if method == "elicitation/create" then
		return "elicitation"
	end

	return nil
end


-- @brief 向 Client 发反向 JSON-RPC (异步; 无能力则失败降级)
-- @return id[string|nil]
-- @return err[string|nil]
local function _reverse_request(method, params)
	local need = _reverse_cap_for(method)
	if need == nil then
		return nil, "unknown reverse method"
	end

	if not _client_has_cap(need) then
		g_mcp_ssid.reverse_err = "client capability missing: " .. need
		_log_bypass(method, {
			note = "reverse degrade",
			capability = need,
		})
		return nil, g_mcp_ssid.reverse_err
	end

	g_mcp_ssid.reverse_seq = (tonumber(g_mcp_ssid.reverse_seq) or 0) + 1
	local id = "rev-" .. tostring(g_mcp_ssid.reverse_seq)
	if type(g_mcp_ssid.reverse_pending) ~= "table" then
		g_mcp_ssid.reverse_pending = {}
	end

	g_mcp_ssid.reverse_pending[id] = {
		method = method,
		ms = _now_ms(),
	}
	local body = {
		jsonrpc = "2.0",
		id = id,
		method = method,
	}
	if params ~= nil then
		body.params = params
	else
		body.params = {}
	end

	_write(body)
	return id
end


-- @brief 处理 Client 对反向请求的应答
local function _handle_reverse_result(id, result, err_obj)
	local key = id
	if type(key) == "number" then
		key = tostring(key)
	end

	local pend = nil
	if type(g_mcp_ssid.reverse_pending) == "table" then
		pend = g_mcp_ssid.reverse_pending[key]
		g_mcp_ssid.reverse_pending[key] = nil
	end

	if type(pend) ~= "table" then
		_log_bypass("reverse.result", {
			note = "unknown reverse id",
			id = id,
		})
		return
	end

	if type(err_obj) == "table" then
		g_mcp_ssid.reverse_err = tostring(err_obj.message or "reverse error")
		_log_bypass(pend.method, {
			note = "reverse failed",
			err = g_mcp_ssid.reverse_err,
		})
		return
	end

	if pend.method == "roots/list" then
		g_mcp_ssid.roots = result
		g_mcp_ssid.reverse_err = nil
		_log_bypass(pend.method, {
			note = "roots ok",
		})
		return
	end

	_log_bypass(pend.method, {
		note = "reverse ok",
	})
end


local function _reply_result(id, result)
	_write({
		jsonrpc = "2.0",
		id = id,
		result = result,
	})
end


local function _reply_error(id, code, message)
	_write({
		jsonrpc = "2.0",
		id = id,
		error = {
			code = code,
			message = tostring(message or "error"),
		},
	})
end


-- @brief JSON-RPC id → 稳定键 (仅 string|number)
local function _id_key(id)
	local t = type(id)
	if t == "string" or t == "number" then
		return t .. ":" .. tostring(id)
	end

	return nil
end


-- @brief 从 side_effects/category 估 annotations (P2-6; Client 须视为不可信)
local function _tool_annotations(desc)
	local ann = nil
	if type(desc.category) == "string" and desc.category == "readonly" then
		ann = {
			readOnlyHint = true,
		}
	end

	local se = desc.side_effects
	if type(se) == "table" then
		for i = 1, #se do
			local v = se[i]
			if v == "unsafe" or v == "write_fs" then
				if ann == nil then
					ann = {}
				end

				ann.destructiveHint = true
			end
		end
	end

	return ann
end


-- @brief 工具描述 → MCP tool (独立 title; schema 2020-12)
local function _tool_entry(desc)
	local entry = {
		name = desc.name,
		description = desc.summary or "",
		inputSchema = mcp_schema.from_args_schema(desc.args_schema),
	}
	if type(desc.title) == "string" and desc.title ~= "" then
		entry.title = desc.title
	end

	-- P4-4: catalog 声明 result_schema 时导出 outputSchema
	if type(desc.result_schema) == "table" then
		entry.outputSchema = mcp_schema.from_args_schema(desc.result_schema)
	end

	local ann = _tool_annotations(desc)
	if type(ann) == "table" then
		entry.annotations = ann
	end

	return entry
end


-- @brief tools/list
-- @note 无 cursor: 全量 (兼容不跟 nextCursor 的宿主); 有 cursor: 按 TOOLS_PAGE_SIZE 分页
-- @return result[table|nil]
-- @return code[number|nil]
-- @return err[string|nil]
local function _tools_list(params)
	local list = catalog.list()
	local offset = 0
	local paginate = false
	if type(params) == "table" and type(params.cursor) == "string" and params.cursor ~= "" then
		local n = tonumber(params.cursor)
		if n == nil or n < 0 or n ~= math.floor(n) then
			return nil, INVALID_PARAMS, "bad cursor"
		end

		offset = n
		paginate = true
	end

	local tools = {}
	local last
	if paginate then
		last = math.min(#list, offset + TOOLS_PAGE_SIZE)
	else
		last = #list
	end

	for i = offset + 1, last do
		tools[#tools + 1] = _tool_entry(list[i])
	end

	local out_tools = tools
	if #tools == 0 and cjson.empty_array ~= nil then
		out_tools = cjson.empty_array
	end

	local out = {
		tools = out_tools,
	}
	if paginate and last < #list then
		out.nextCursor = tostring(last)
	end

	return out
end


-- @brief 取 params._meta.progressToken
local function _progress_token(params)
	if type(params) ~= "table" then
		return nil
	end

	local meta = params._meta
	if type(meta) ~= "table" then
		meta = params.meta
	end

	if type(meta) ~= "table" then
		return nil
	end

	local tok = meta.progressToken
	if tok == nil then
		tok = meta.progress_token
	end

	return tok
end


-- @brief 当前请求是否已 cancelled
local function _is_cancelled()
	local key = _id_key(g_mcp_ssid.current_req_id)
	if key == nil then
		return false
	end

	local tab = g_mcp_ssid.cancelled_ids
	return type(tab) == "table" and tab[key] == true
end


-- @brief 调本机工具 → MCP CallToolResult
local function _tools_call(params)
	if type(params) ~= "table" then
		return nil, INVALID_PARAMS, "params must be object"
	end

	if _is_cancelled() then
		return nil, INVALID_REQUEST, "request cancelled"
	end

	local name = params.name
	if type(name) ~= "string" or name == "" then
		return nil, INVALID_PARAMS, "need tool name"
	end

	local args = params.arguments
	if args == nil then
		args = {}
	end

	if type(args) ~= "table" then
		return nil, INVALID_PARAMS, "arguments must be object"
	end

	local ptok = _progress_token(params)
	if ptok ~= nil then
		_notify_progress(ptok, 0, 1)
	end

	-- 工具调用前 begin_ctx (XPC; 复用 initialize 的 mcp_init/alua_ssid)
	-- skip_token: MCP 面只借身份, 不在每次调用隐式签发/落临时 env
	local ssid_mod = nil
	do
		local ssid = _ssid_xpc()
		if type(ssid) == "table" and type(ssid.co_begin_ctx) == "function" then
			ssid_mod = ssid
			local profile = "net"
			if type(profiles.get_override) == "function" then
				local p = profiles.get_override()
				if type(p) == "string" and p ~= "" then
					profile = p
				end
			end

			local ctx_ok, ctx = pcall(ssid.co_begin_ctx, {
				via = "mcp",
				mcp_init = g_mcp_ssid.mcp_init,
				alua_ssid = g_mcp_ssid.alua_ssid,
				profile = profile,
				skip_token = true,
			})
			if ctx_ok and type(ctx) == "table" then
				if type(ctx.alua_ssid) == "string" and ctx.alua_ssid ~= "" then
					g_mcp_ssid.alua_ssid = ctx.alua_ssid
				end

				if type(ctx.host_tool) == "string" then
					g_mcp_ssid.host_tool = ctx.host_tool
				end
			end
		end
	end

	-- ssid.* 工具默认带上 MCP 握手身份, 避免 via=call 另推 SSID
	if type(name) == "string" and string.sub(name, 1, 5) == "ssid." then
		if args.via == nil then
			args.via = "mcp"
		end

		if args.mcp_init == nil then
			args.mcp_init = g_mcp_ssid.mcp_init
		end

		if args.alua_ssid == nil then
			args.alua_ssid = g_mcp_ssid.alua_ssid
		end
	end

	-- remote.*: 复用 MCP alua_ssid, 令 token 内沉绑定同一客户端 (M3)
	if type(name) == "string" and string.sub(name, 1, 7) == "remote." then
		if args.via == nil then
			args.via = "mcp"
		end

		if args.alua_ssid == nil and type(g_mcp_ssid.alua_ssid) == "string"
			and g_mcp_ssid.alua_ssid ~= "" then
			args.alua_ssid = g_mcp_ssid.alua_ssid
		end

		-- 预 ensure (XPC → owner; 远端签发仍由 resolve_call_token 对 host 完成)
		do
			local ssid = _ssid_xpc()
			if type(ssid) == "table" and type(ssid.co_ensure) == "function"
				and type(args.alua_ssid) == "string" and args.alua_ssid ~= "" then
				pcall(ssid.co_ensure, {
					via = "mcp",
					alua_ssid = args.alua_ssid,
					mcp_init = g_mcp_ssid.mcp_init,
					profile = "net",
				})
			end
		end
	end

	if _is_cancelled() then
		if ssid_mod ~= nil and type(ssid_mod.clear_request) == "function" then
			pcall(ssid_mod.clear_request)
		end

		return nil, INVALID_REQUEST, "request cancelled"
	end

	local ok, result = dispatch.call({
		name = name,
		args = args,
		via = "mcp",
		session_id = g_mcp_ssid.session_id,
	})

	-- 清本协程身份绑定: 免下一次调用继承上次 local 绑定
	if ssid_mod ~= nil and type(ssid_mod.clear_request) == "function" then
		pcall(ssid_mod.clear_request)
	end

	if ptok ~= nil then
		_notify_progress(ptok, 1, 1)
	end

	local payload = result
	if not ok then
		if type(result) ~= "table" then
			payload = {
				ok = false,
				message = tostring(result),
			}
		end
	else
		payload = {
			ok = true,
			result = result,
		}
	end

	local ok_e, text = pcall(cjson.encode, payload)
	if not ok_e or type(text) ~= "string" then
		text = "{\"ok\":false,\"message\":\"encode fail\"}"
	end

	local out = {
		content = {
			{
				type = "text",
				text = text,
			},
		},
		isError = not ok,
	}
	if ok and type(result) == "table" then
		out.structuredContent = result
	end

	return out
end


local METHOD_HANDLERS = {}


METHOD_HANDLERS["initialize"] = function (params)
	local want = nil
	if type(params) == "table" and type(params.protocolVersion) == "string" then
		want = params.protocolVersion
	end

	local negotiated = _negotiate_protocol_version(want)

	-- MCP 握手自动 register (指纹含 clientInfo → host_tool=mcp-xx / mcp-unknown)
	g_mcp_ssid.mcp_init = params
	g_mcp_ssid.alua_ssid = nil
	g_mcp_ssid.host_tool = nil
	g_mcp_ssid.session_id = nil
	g_mcp_ssid.caller_kind = nil
	g_mcp_ssid.handshake_done = false
	g_mcp_ssid.initialized = false
	g_mcp_ssid.protocol_version = negotiated
	g_mcp_ssid.init_count = (tonumber(g_mcp_ssid.init_count) or 0) + 1
	-- P2-2: 登记 Client capabilities (roots/sampling/elicitation)
	g_mcp_ssid.client_caps = nil
	if type(params) == "table" and type(params.capabilities) == "table" then
		g_mcp_ssid.client_caps = params.capabilities
	end

	do
		local ssid = _ssid_xpc()
		if type(ssid) == "table" and type(ssid.co_register) == "function" then
			local ok_r, info = pcall(ssid.co_register, {
				via = "mcp",
				mcp_init = params,
			})
			if ok_r and type(info) == "table" then
				if type(info.alua_ssid) == "string" then
					g_mcp_ssid.alua_ssid = info.alua_ssid
				end

				if type(info.host_tool) == "string" then
					g_mcp_ssid.host_tool = info.host_tool
				end

				if type(info.caller_kind) == "string" then
					g_mcp_ssid.caller_kind = info.caller_kind
				end
			end
		end
	end

	-- 每次 initialize 新 segment (含再次握手; M7)
	local reason = "initialize"
	if g_mcp_ssid.init_count > 1 then
		reason = "reinitialize"
	end

	_begin_segment(params, reason)
	g_mcp_ssid.handshake_done = true

	return {
		protocolVersion = negotiated,
		capabilities = {
			tools = {
				-- P4-3: 声明 listChanged; 热更后发 notifications/tools/list_changed
				listChanged = true,
			},
			resources = {
				subscribe = false,
				listChanged = false,
			},
			prompts = {
				listChanged = false,
			},
			logging = {},
		},
		serverInfo = {
			name = "alua",
			title = "alua (klbagent)",
			version = version.string,
		},
		instructions = "alua MCP server: tools map to klbagent catalog; call runtime.ai_bootstrap then runtime.ai_load_ack before business tools; host MCP config (Cursor/CodeBuddy/Trae/...) → docs.read path=tools/mcp.md.",
	}
end


METHOD_HANDLERS["ping"] = function ()
	_touch_activity(nil)
	return {}
end


METHOD_HANDLERS["tools/list"] = function (params)
	_touch_activity(params)
	return _tools_list(params)
end


METHOD_HANDLERS["tools/call"] = function (params)
	_touch_activity(params)
	return _tools_call(params)
end


-- P4-2: docs 作为 MCP resources (uri=alua://docs/<rel>)
local function _docs_resource_uri(rel)
	return "alua://docs/" .. tostring(rel)
end


local function _docs_rel_from_uri(uri)
	if type(uri) ~= "string" then
		return nil
	end

	local rel = string.match(uri, "^alua://docs/(.+)$")
	if rel == nil or rel == "" then
		return nil
	end

	if string.find(rel, "%.%.") ~= nil or string.sub(rel, 1, 1) == "/" then
		return nil
	end

	return rel
end


-- @brief MCP 内调工具 (XPC begin_ctx skip_token, 与 tools/call 同)
local function _mcp_tool_call(name, args)
	local ssid_mod = nil
	do
		local ssid = _ssid_xpc()
		if type(ssid) == "table" and type(ssid.co_begin_ctx) == "function" then
			ssid_mod = ssid
			local profile = "net"
			if type(profiles.get_override) == "function" then
				local p = profiles.get_override()
				if type(p) == "string" and p ~= "" then
					profile = p
				end
			end

			pcall(ssid.co_begin_ctx, {
				via = "mcp",
				mcp_init = g_mcp_ssid.mcp_init,
				alua_ssid = g_mcp_ssid.alua_ssid,
				profile = profile,
				skip_token = true,
			})
		end
	end

	local ok, result = dispatch.call({
		name = name,
		args = args or {},
		via = "mcp",
		session_id = g_mcp_ssid.session_id,
	})

	if ssid_mod ~= nil and type(ssid_mod.clear_request) == "function" then
		pcall(ssid_mod.clear_request)
	end

	return ok, result
end


METHOD_HANDLERS["resources/list"] = function ()
	_touch_activity(nil)
	local resources = {}
	local ok, result = _mcp_tool_call("docs.list", {})
	if ok and type(result) == "table" and type(result.files) == "table" then
		for i = 1, #result.files do
			local rel = result.files[i]
			if type(rel) == "string" and rel ~= "" then
				resources[#resources + 1] = {
					uri = _docs_resource_uri(rel),
					name = rel,
					title = rel,
					mimeType = "text/markdown",
				}
			end
		end
	end

	local out_res = resources
	if #resources == 0 and cjson.empty_array ~= nil then
		out_res = cjson.empty_array
	end

	return {
		resources = out_res,
	}
end


METHOD_HANDLERS["resources/templates/list"] = function ()
	_touch_activity(nil)
	local empty = {}
	if cjson.empty_array ~= nil then
		empty = cjson.empty_array
	end

	return {
		resourceTemplates = empty,
	}
end


METHOD_HANDLERS["resources/read"] = function (params)
	_touch_activity(params)
	if type(params) ~= "table" then
		return nil, INVALID_PARAMS, "params must be object"
	end

	local uri = params.uri
	local rel = _docs_rel_from_uri(uri)
	if rel == nil then
		return nil, INVALID_PARAMS, "bad or unsupported uri"
	end

	local ok, result = _mcp_tool_call("docs.read", {
		path = rel,
	})
	if not ok then
		local msg = "resource not found"
		if type(result) == "table" and type(result.message) == "string" then
			msg = result.message
		end

		return nil, INVALID_PARAMS, msg
	end

	local text = ""
	if type(result) == "table" and type(result.text) == "string" then
		text = result.text
	end

	return {
		contents = {
			{
				uri = _docs_resource_uri(rel),
				mimeType = "text/markdown",
				text = text,
			},
		},
	}
end


-- P4-2: 内置 prompts (无外部内容依赖)
local MCP_PROMPTS = {
	{
		name = "alua-ai-bootstrap",
		title = "AI L0 bootstrap",
		description = "Call runtime.ai_bootstrap then runtime.ai_load_ack before business tools",
	},
	{
		name = "alua-mcp-overview",
		title = "MCP overview",
		description = "Read alua MCP host setup guidance (docs tools/mcp.md)",
	},
}


METHOD_HANDLERS["prompts/list"] = function ()
	_touch_activity(nil)
	local prompts = {}
	for i = 1, #MCP_PROMPTS do
		local p = MCP_PROMPTS[i]
		prompts[#prompts + 1] = {
			name = p.name,
			title = p.title,
			description = p.description,
		}
	end

	return {
		prompts = prompts,
	}
end


METHOD_HANDLERS["prompts/get"] = function (params)
	_touch_activity(params)
	if type(params) ~= "table" then
		return nil, INVALID_PARAMS, "params must be object"
	end

	local name = params.name
	if type(name) ~= "string" or name == "" then
		return nil, INVALID_PARAMS, "need prompt name"
	end

	local found = nil
	for i = 1, #MCP_PROMPTS do
		if MCP_PROMPTS[i].name == name then
			found = MCP_PROMPTS[i]
			break
		end
	end

	if found == nil then
		return nil, INVALID_PARAMS, "prompt not found"
	end

	local text
	if name == "alua-ai-bootstrap" then
		text = "Before any business alua tool, call runtime.ai_bootstrap then runtime.ai_load_ack. Follow returned L0 rules."
	else
		text = "Read MCP host help via resources/read uri=alua://docs/tools/mcp.md (or docs.read path=tools/mcp.md). Prefer stdio `alua mcp serve`; HTTP is POST /mcp on alua serve with Bearer token."
	end

	return {
		description = found.description,
		messages = {
			{
				role = "user",
				content = {
					type = "text",
					text = text,
				},
			},
		},
	}
end


METHOD_HANDLERS["logging/setLevel"] = function (params)
	local level = nil
	if type(params) == "table" then
		level = params.level
	end

	g_mcp_ssid.log_level = level
	_log_bypass("logging/setLevel", {
		level = level,
	})
	_touch_activity(nil)
	return {}
end


-- @brief 处理 notifications/* (无响应; 旁路记录 + initialized 门禁)
local function _handle_notification(method, params)
	_log_bypass(method, {
		has_params = type(params) == "table",
	})

	if method == "notifications/initialized" then
		if g_mcp_ssid.handshake_done ~= true then
			_log_bypass(method, {
				note = "initialized before handshake",
			})
			return
		end

		g_mcp_ssid.initialized = true
		g_mcp_ssid.last_activity_ms = _now_ms()
		if type(g_mcp_ssid.session_id) == "string" and g_mcp_ssid.session_id ~= "" then
			pcall(session.set_attrs, g_mcp_ssid.session_id, {
				mcp_initialized = true,
				client_caps_roots = _client_has_cap("roots"),
				client_caps_sampling = _client_has_cap("sampling"),
				client_caps_elicitation = _client_has_cap("elicitation"),
			})
		end

		-- P2-2: 有 roots 则异步 roots/list; 无能力则降级记 err
		if _client_has_cap("roots") then
			_reverse_request("roots/list", {})
		else
			g_mcp_ssid.reverse_err = "client capability missing: roots"
			_log_bypass("roots/list", {
				note = "reverse degrade",
				capability = "roots",
			})
		end

		-- sampling / elicitation: 仅登记, 缺省不主动发 (失败降级已由 _reverse_request)
		if not _client_has_cap("sampling") then
			_log_bypass("sampling/createMessage", {
				note = "reverse degrade",
				capability = "sampling",
			})
		end

		if not _client_has_cap("elicitation") then
			_log_bypass("elicitation/create", {
				note = "reverse degrade",
				capability = "elicitation",
			})
		end

		return
	end

	-- P2-7: 客户端取消进行中的请求
	if method == "notifications/cancelled" then
		local rid = nil
		if type(params) == "table" then
			rid = params.requestId
			if rid == nil then
				rid = params.request_id
			end
		end

		local key = _id_key(rid)
		if key ~= nil then
			if type(g_mcp_ssid.cancelled_ids) ~= "table" then
				g_mcp_ssid.cancelled_ids = {}
			end

			g_mcp_ssid.cancelled_ids[key] = true
			_log_bypass(method, {
				request_id = rid,
			})
		end
	end
end


-- @brief 处理单条 JSON-RPC 消息
local function _handle_msg(msg)
	if type(msg) ~= "table" then
		_reply_error(nil, PARSE_ERROR, "invalid json")
		return
	end

	local id = msg.id
	local method = msg.method

	-- P2-2: Client 应答反向请求 (有 id, 无 method)
	if (type(method) ~= "string" or method == "") and id ~= nil
		and (msg.result ~= nil or msg.error ~= nil) then
		_handle_reverse_result(id, msg.result, msg.error)
		return
	end

	if type(method) ~= "string" or method == "" then
		if id ~= nil then
			_reply_error(id, INVALID_REQUEST, "missing method")
		end

		return
	end

	-- notifications/*: 无 id 正常; 带 id 也不回 (容错)
	if string.sub(method, 1, 14) == "notifications/" then
		if not _lifecycle_allows(method) then
			_log_bypass(method, {
				note = "lifecycle denied",
			})
			return
		end

		_handle_notification(method, msg.params)
		return
	end

	-- notification: 无 id; 不回复 (非 notifications/ 前缀的容错)
	if id == nil then
		_log_bypass(method, {
			note = "notify no-id",
		})
		return
	end

	if not _lifecycle_allows(method) then
		_reply_error(id, INVALID_REQUEST, "not initialized: " .. method)
		return
	end

	-- P2-8: 同会话段拒绝重复 request id
	local id_key = _id_key(id)
	if id_key ~= nil then
		if type(g_mcp_ssid.seen_ids) ~= "table" then
			g_mcp_ssid.seen_ids = {}
		end

		if g_mcp_ssid.seen_ids[id_key] == true then
			_log_bypass(method, {
				note = "duplicate request id",
				id = id,
			})
			_reply_error(id, INVALID_REQUEST, "duplicate request id")
			return
		end

		g_mcp_ssid.seen_ids[id_key] = true
	end

	local fn = METHOD_HANDLERS[method]
	if fn == nil then
		_reply_error(id, METHOD_NOT_FOUND, "method not found: " .. method)
		return
	end

	g_mcp_ssid.current_req_id = id
	local ok, result, code, err = pcall(fn, msg.params)
	g_mcp_ssid.current_req_id = nil
	if not ok then
		_reply_error(id, INTERNAL_ERROR, tostring(result))
		return
	end

	if result == nil then
		_reply_error(id, code or INVALID_PARAMS, err or "bad params")
		return
	end

	_reply_result(id, result)
end


-- @brief 处理单行原文 (含 batch)
local function _handle_line(line)
	local ok_j, obj = pcall(cjson.decode, line)
	if not ok_j then
		_reply_error(nil, PARSE_ERROR, "parse error")
		return
	end

	if type(obj) == "table" and obj[1] ~= nil then
		for i = 1, #obj do
			_handle_msg(obj[i])
		end

		return
	end

	_handle_msg(obj)
end


-- @brief 工具列表变更通知 (P4-3; runtime.reload 成功后可调)
-- @return sent[boolean]
M.notify_tools_list_changed = function ()
	local obj = {
		jsonrpc = "2.0",
		method = "notifications/tools/list_changed",
		params = {},
	}
	local sent = false
	if type(g_mcp_ssid) == "table" and g_mcp_ssid.initialized == true then
		_write(obj)
		sent = true
	end

	-- 扇出到其它已初始化 HTTP 会话的 GET SSE 队列
	for _, st in pairs(g_http_sessions) do
		if type(st) == "table" and st.initialized == true and st ~= g_mcp_ssid then
			_enqueue_sse(st, obj)
			sent = true
		end
	end

	return sent
end


-- @brief 取出 HTTP 会话 SSE 队列 (不持锁长时间)
-- @param [in] http_sid[string]
-- @param [in] max_n[number|nil]
-- @return events[table]
M.take_sse_events = function (http_sid, max_n)
	local st = g_http_sessions[http_sid]
	if type(st) ~= "table" or type(st.sse_q) ~= "table" then
		return {}
	end

	local n = tonumber(max_n) or #st.sse_q
	if n < 1 then
		return {}
	end

	local out = {}
	local q = st.sse_q
	local take = math.min(n, #q)
	for i = 1, take do
		out[i] = q[i]
	end

	if take >= #q then
		st.sse_q = {}
	else
		local rest = {}
		for i = take + 1, #q do
			rest[#rest + 1] = q[i]
		end

		st.sse_q = rest
	end

	return out
end


-- @brief 会话是否仍登记
-- @param [in] http_sid[string]
-- @return ok[boolean]
M.has_http_session = function (http_sid)
	return type(http_sid) == "string" and type(g_http_sessions[http_sid]) == "table"
end


-- @brief 生成 HTTP Mcp-Session-Id (可见 ASCII)
local function _new_http_sid()
	return string.format("mcp-%x-%d", os.time(), math.random(100000, 999999))
end


-- @brief 激活 HTTP 会话状态为当前 g_mcp_ssid
-- @param [in] http_sid[string|nil]		nil=新建
-- @return sid[string|nil]
-- @return err[string|nil]
M.activate_http_session = function (http_sid)
	if type(http_sid) == "string" and http_sid ~= "" then
		local st = g_http_sessions[http_sid]
		if type(st) ~= "table" then
			return nil, "unknown session"
		end

		g_mcp_ssid = st
		return http_sid
	end

	local sid = _new_http_sid()
	local st = _new_state()
	st.http_sid = sid
	g_http_sessions[sid] = st
	g_mcp_ssid = st
	return sid
end


-- @brief 结束 HTTP 会话
-- @param [in] http_sid[string]
-- @return ok[boolean]
M.end_http_session = function (http_sid)
	if type(http_sid) ~= "string" or http_sid == "" then
		return false
	end

	g_http_sessions[http_sid] = nil
	if type(g_mcp_ssid) == "table" and g_mcp_ssid.http_sid == http_sid then
		g_mcp_ssid = _new_state()
	end

	return true
end


-- @brief 串行化 HTTP MCP 请求 (保护全局会话指针)
-- @param [in] fn[function]
-- @return ...
-- @note 须在 kco 协程内 (co_sleep 等锁)
M.co_with_http_lock = function (fn)
	local spins = 0
	while g_http_busy do
		spins = spins + 1
		if spins > 2000 then
			return nil, nil, "mcp http busy"
		end

		kco.co_sleep(5)
	end

	g_http_busy = true
	local ok, a, b, c = pcall(fn)
	g_http_busy = false
	if not ok then
		return nil, nil, tostring(a)
	end

	return a, b, c
end


-- @brief 捕获 _write 输出并处理一行 JSON-RPC
-- @param [in] line[string]
-- @return frames[table]				写出的 JSON-RPC 对象列表
M.handle_line_captured = function (line)
	g_capture = {}
	_handle_line(line)
	local frames = g_capture
	g_capture = nil
	return frames
end


-- @brief stdio 读循环 (须在 kco 协程内; EOF/SIGTERM 后置位退出)
-- @note SIGTERM: klua_env 信号处理 close(stdin) 解除 io.read (Bug.2026-1-mcp)
local function _co_stdio_loop()
	while not kenv.is_exit() do
		-- SIGTERM 关 stdin 后 read 可能抛错或回 nil
		local ok_r, line = pcall(io.read, "*l")
		if (not ok_r) or line == nil then
			break
		end

		line = string.match(line, "^%s*(.-)%s*$") or ""
		if line ~= "" then
			_handle_line(line)
		end
	end

	-- EOF / 读失败: 置位 env 退出, 由调用方 main_loop 收尾
	if not kenv.is_exit() then
		kenv.exit()
	end
end


-- @brief stdio MCP 嵌入式入口: 挂接后 fork 读循环并立即返回 (不自泵)
-- @param [in] opts[table|nil]			opts.profile: readonly|net|device|unsafe
-- @return ok[boolean]
-- @return err[string|nil]
-- @note 禁入口内 while is_exit 卡死; 长期读 stdin 在 kco.fork 内
--   \n 阻塞常驻请用 M.serve (serve_entry + co_util.run_host_loop)
M.serve_entry = function (opts)
	opts = opts or {}
	local profile = opts.profile or "readonly"
	if not profiles.is_profile(profile) then
		return false, "bad profile"
	end

	profiles.set_override(profile)
	-- 独立通道: 不触发 stdio/serve 的 ai_load 门禁 (MCP 宿主自行按 instructions 调 ack)
	ai_load_gate.set_channel("mcp")
	g_mcp_ssid = _new_state()

	kco.fork(_co_stdio_loop)
	return true
end


-- @brief stdio MCP 阻塞常驻: serve_entry + run_host_loop
-- @param [in] opts[table|nil]			同 serve_entry
-- @return ok[boolean]
-- @return err[string|nil]
-- @note CLI 宿主下 hold; 独立 env 自调 klbenv.co_main_loop; EOF/SIGTERM → kenv.exit
M.serve = function (opts)
	local ok, err = M.serve_entry(opts)
	if not ok then
		return false, err
	end

	local ok_u, co_util = pcall(require, "klbagent.core.co_util")
	if not ok_u or type(co_util) ~= "table" or type(co_util.run_host_loop) ~= "function" then
		return false, "co_util.run_host_loop unavailable"
	end

	local ok_h, err_h = pcall(co_util.run_host_loop)
	if not ok_h then
		return false, tostring(err_h)
	end

	return true
end


return M

-- end
