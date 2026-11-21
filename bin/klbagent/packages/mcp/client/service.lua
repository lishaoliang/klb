--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   service.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  MCP client 实现 (Streamable HTTP JSON-RPC)
-- @stability internal
-- @impl evolving
-- @note   包外勿直 require; 对外面见 client/init.lua
--   \n POST + Accept JSON/SSE; Session/Version 头
-- @history 修改历史
--  \n [2026-10] 创建文件 (P1-1)
--  \n [2026-11] M6 指纹; 迁入 client/; 自 init 拆为 service
--]]


local cjson = require("cjson")
local http = require("klbagent.core.tools.net.http")
local netutil = require("klbagent.core.tools.net.util")
local version = require("klbagent.version")
local mcp_const = require("klbagent.packages.mcp.base.const")


local M = {}


M.PROTOCOL_VERSION = mcp_const.PROTOCOL_VERSION
M.SUPPORTED_PROTOCOL_VERSIONS = mcp_const.SUPPORTED_PROTOCOL_VERSIONS


local _seq = 0


-- @brief 去掉 Lua error 自带的 path:line 前缀 (对齐 dispatch._stable_tool_err)
-- @param [in] err[any]
-- @return msg[string]
local function _stable_err(err)
	local s = tostring(err)
	local msg = string.match(s, "^.-:%d+:%s*(.+)$")
	if type(msg) == "string" and msg ~= "" then
		return msg
	end

	return s
end


-- @brief 构造客户端上下文
-- @param [in] opts[table]			url/http(s); token/timeout_sec/max_bytes 可选
-- @return ctx[table|nil]
-- @return err[string|nil]
M.new_context = function (opts)
	opts = opts or {}
	local url = opts.url
	if type(url) ~= "string" or string.find(url, "^https?://") == nil then
		return nil, "need http(s) url"
	end

	if string.find(url, "[\r\n\0]") ~= nil then
		return nil, "bad url"
	end

	local ctx = {
		url = url,
		token = opts.token,
		timeout_sec = tonumber(opts.timeout_sec) or 15,
		max_bytes = tonumber(opts.max_bytes) or netutil.DEFAULT_MAX_BYTES,
		initialized = false,
		session_id = nil,
		protocol_version = nil,
		-- 传输面指纹 (M6; 仅标签)
		transport = {
			accept = "application/json, text/event-stream",
			user_agent = "alua-mcp/" .. tostring(version.string or "0"),
			protocol_version = nil,
			session_id = nil,
			session_checks = 0,
			session_mismatch = 0,
		},
	}

	return ctx
end


-- @brief 传输层 (测试可替换); args.method 默认 POST
-- @param [in] args[table]
-- @return result[table]
M._request = function (args)
	local method = "POST"
	if type(args) == "table" and type(args.method) == "string" and args.method ~= "" then
		method = string.upper(args.method)
	end

	return http.request(method, args)
end


-- @brief 解析 SSE 为多帧 JSON-RPC (按空行分事件)
-- @return frames[table]
-- @return err[string|nil]
local function _parse_sse_frames(body)
	local frames = {}
	local data_lines = {}
	local function _flush()
		if #data_lines == 0 then
			return
		end

		local ok, obj = pcall(cjson.decode, table.concat(data_lines, "\n"))
		data_lines = {}
		if ok and type(obj) == "table" then
			frames[#frames + 1] = obj
		end
	end

	for line in string.gmatch(body .. "\n", "([^\r\n]*)\r?\n") do
		if line == "" then
			_flush()
		else
			local data = string.match(line, "^data:%s*(.-)%s*$")
			if data ~= nil then
				data_lines[#data_lines + 1] = data
			end
		end
	end

	_flush()
	return frames
end


-- @brief SSE 响应体取值 (兼容旧: 取最后一帧; 多帧时优先带 id 的应答)
local function _parse_sse(body)
	local frames = _parse_sse_frames(body)
	if #frames == 0 then
		return nil, "bad sse json"
	end

	local reply = nil
	for i = 1, #frames do
		local f = frames[i]
		if type(f) == "table" and f.id ~= nil then
			reply = f
		end
	end

	return reply or frames[#frames]
end


-- @brief 解析响应体 (JSON 或 SSE)
local function _decode_body(result)
	local body = result.body or ""
	if body == "" then
		return nil
	end

	local ctype = netutil.header_value(result.headers or "", "Content-Type") or ""
	if string.find(ctype, "text/event-stream") ~= nil
		or string.find(body, "^event:") ~= nil
		or string.find(body, "^data:") ~= nil
	then
		return _parse_sse(body)
	end

	local ok, obj = pcall(cjson.decode, body)
	if not ok then
		return nil, "bad json response"
	end

	return obj
end


local function _common_headers(ctx, accept_override)
	local accept = accept_override or "application/json, text/event-stream"
	local ua = "alua-mcp/" .. tostring(version.string or "0")
	if type(ctx.transport) == "table" then
		if accept_override == nil and type(ctx.transport.accept) == "string"
			and ctx.transport.accept ~= "" then
			accept = ctx.transport.accept
		end

		if type(ctx.transport.user_agent) == "string" and ctx.transport.user_agent ~= "" then
			ua = ctx.transport.user_agent
		end
	end

	local headers = {
		["Accept"] = accept,
		["User-Agent"] = ua,
	}
	if type(ctx.token) == "string" and ctx.token ~= "" then
		headers["Authorization"] = "Bearer " .. ctx.token
	end

	if type(ctx.session_id) == "string" and ctx.session_id ~= "" then
		headers["Mcp-Session-Id"] = ctx.session_id
	end

	if type(ctx.protocol_version) == "string" and ctx.protocol_version ~= "" then
		headers["MCP-Protocol-Version"] = ctx.protocol_version
	end

	return headers, accept, ua
end


local function _apply_session_headers(ctx, result)
	local sid = netutil.header_value(result.headers or "", "Mcp-Session-Id")
	if type(sid) ~= "string" or sid == "" then
		return
	end

	if type(ctx.session_id) == "string" and ctx.session_id ~= "" then
		ctx.transport.session_checks = (tonumber(ctx.transport.session_checks) or 0) + 1
		if sid ~= ctx.session_id then
			ctx.transport.session_mismatch = (tonumber(ctx.transport.session_mismatch) or 0) + 1
		end
	end

	ctx.session_id = sid
	if type(ctx.transport) == "table" then
		ctx.transport.session_id = sid
	end
end


-- @brief 单次 JSON-RPC 调用
-- @param [in] ctx[table]
-- @param [in] method[string]
-- @param [in] params[table|nil]
-- @param [in] notify[boolean|nil]		true=通知 (无 id, 无响应体)
-- @return result[table|nil]
-- @return err[string|nil]
local function _rpc(ctx, method, params, notify)
	local msg = {
		jsonrpc = "2.0",
		method = method,
	}
	if params ~= nil then
		msg.params = params
	end

	if not notify then
		_seq = _seq + 1
		msg.id = _seq
	end

	local ok_e, body = pcall(cjson.encode, msg)
	if not ok_e or type(body) ~= "string" then
		return nil, "encode fail"
	end

	local headers, accept, ua = _common_headers(ctx)
	headers["Content-Type"] = "application/json"

	-- 传输面指纹快照 (发出侧; M6)
	if type(ctx.transport) ~= "table" then
		ctx.transport = {}
	end

	ctx.transport.accept = accept
	ctx.transport.user_agent = ua
	ctx.transport.protocol_version = ctx.protocol_version
	ctx.transport.session_id = ctx.session_id

	local ok_r, result = pcall(M._request, {
		method = "POST",
		url = ctx.url,
		headers = headers,
		body = body,
		timeout_sec = ctx.timeout_sec,
		max_bytes = ctx.max_bytes,
	})
	if not ok_r then
		-- Bug.2026-22: 稳定文案; 原文降级 log
		local raw = tostring(result)
		local stable = _stable_err(result)
		local ok_log, log = pcall(require, "klbagent.core.log")
		if ok_log and type(log) == "table" and type(log.write) == "function" then
			log.write("debug", "mcp client transport fail", {
				module = "mcp.client",
				err = stable,
				err_raw = raw,
			})
		end

		return nil, stable
	end

	if type(result) ~= "table" then
		return nil, "bad http result"
	end

	local status = tonumber(result.status) or 0
	if status >= 400 then
		return nil, "http " .. tostring(status)
	end

	_apply_session_headers(ctx, result)

	if notify then
		return true
	end

	local obj, err = _decode_body(result)
	if obj == nil then
		return nil, err or "empty response"
	end

	if obj.error ~= nil then
		local m = obj.error.message or "mcp error"
		return nil, tostring(m)
	end

	return obj.result or {}
end


-- @brief initialize + notifications/initialized (幂等)
-- @param [in] ctx[table]
-- @param [in] opts[table|nil]			protocol_version 可选
-- @return info[table|nil]
-- @return err[string|nil]
M.initialize = function (ctx, opts)
	opts = opts or {}
	if ctx.initialized then
		return {
			protocolVersion = ctx.protocol_version,
		}
	end

	local res, err = _rpc(ctx, "initialize", {
		protocolVersion = opts.protocol_version or M.PROTOCOL_VERSION,
		capabilities = {},
		clientInfo = {
			name = "alua",
			version = version.string,
		},
	})
	if res == nil then
		return nil, err
	end

	ctx.protocol_version = res.protocolVersion or M.PROTOCOL_VERSION
	ctx.initialized = true
	_rpc(ctx, "notifications/initialized", nil, true)

	return res
end


-- @brief 列出远端工具 (初始化后 tools/list)
-- @param [in] ctx[table]
-- @return tools[table|nil]
-- @return err[string|nil]
M.list_tools = function (ctx)
	local info, err = M.initialize(ctx)
	if info == nil then
		return nil, err
	end

	local res, err2 = _rpc(ctx, "tools/list", {})
	if res == nil then
		return nil, err2
	end

	return res.tools or {}, nil
end


-- @brief 调用远端工具
-- @param [in] ctx[table]
-- @param [in] name[string]
-- @param [in] args[table|nil]
-- @return result[table|nil]
-- @return err[string|nil]
M.call_tool = function (ctx, name, args)
	if type(name) ~= "string" or name == "" then
		return nil, "need tool name"
	end

	if args == nil then
		args = {}
	end

	local info, err = M.initialize(ctx)
	if info == nil then
		return nil, err
	end

	return _rpc(ctx, "tools/call", {
		name = name,
		arguments = args,
	})
end


-- @brief ping (初始化后)
-- @param [in] ctx[table]
-- @return result[table|nil]
-- @return err[string|nil]
M.ping = function (ctx)
	local info, err = M.initialize(ctx)
	if info == nil then
		return nil, err
	end

	return _rpc(ctx, "ping", {})
end


-- @brief DELETE 结束远端 MCP 会话
-- @param [in] ctx[table]
-- @return ok[boolean]
-- @return err[string|nil]
M.close = function (ctx)
	if type(ctx) ~= "table" or type(ctx.url) ~= "string" then
		return false, "need ctx"
	end

	if type(ctx.session_id) ~= "string" or ctx.session_id == "" then
		return false, "need session_id"
	end

	local headers = _common_headers(ctx, "application/json")
	local ok_r, result = pcall(M._request, {
		method = "DELETE",
		url = ctx.url,
		headers = headers,
		timeout_sec = ctx.timeout_sec,
		max_bytes = ctx.max_bytes,
	})
	if not ok_r then
		return false, _stable_err(result)
	end

	local status = tonumber(result and result.status) or 0
	if status >= 400 then
		return false, "http " .. tostring(status)
	end

	ctx.session_id = nil
	ctx.initialized = false
	if type(ctx.transport) == "table" then
		ctx.transport.session_id = nil
	end

	return true
end


-- @brief GET 订阅 server→client SSE (阻塞至超时或收满)
-- @param [in] ctx[table]				须已 initialize 且有 session_id
-- @param [in] opts[table|nil]			timeout_sec / max_events
-- @return events[table|nil]			JSON-RPC 对象列表
-- @return err[string|nil]
M.listen = function (ctx, opts)
	opts = opts or {}
	if type(ctx) ~= "table" or type(ctx.url) ~= "string" then
		return nil, "need ctx"
	end

	local info, err = M.initialize(ctx)
	if info == nil then
		return nil, err
	end

	if type(ctx.session_id) ~= "string" or ctx.session_id == "" then
		return nil, "need session_id"
	end

	local timeout_sec = tonumber(opts.timeout_sec) or ctx.timeout_sec or 15
	local max_events = tonumber(opts.max_events) or 32
	local headers = _common_headers(ctx, "text/event-stream")
	-- 与 server GET 对齐: 服务端到点结束 SSE, 避免 client 先 timeout
	headers["MCP-SSE-Timeout"] = tostring(math.floor(timeout_sec))
	-- SSE 长连接: force_curl (klbhttp 对半开流收尾不稳定)
	local ok_r, result = pcall(M._request, {
		method = "GET",
		url = ctx.url,
		headers = headers,
		timeout_sec = timeout_sec + 5,
		max_bytes = tonumber(opts.max_bytes) or ctx.max_bytes,
		force_curl = true,
	})
	if not ok_r then
		return nil, _stable_err(result)
	end

	if type(result) ~= "table" then
		return nil, "bad http result"
	end

	local status = tonumber(result.status) or 0
	if status >= 400 then
		return nil, "http " .. tostring(status)
	end

	_apply_session_headers(ctx, result)
	local body = result.body or ""
	local frames = _parse_sse_frames(body)
	if max_events > 0 and #frames > max_events then
		local trimmed = {}
		for i = 1, max_events do
			trimmed[i] = frames[i]
		end

		frames = trimmed
	end

	return frames
end


return M

-- end

