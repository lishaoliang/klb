--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   mcp.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua mcp — serve|list|call|probe
-- @note   serve=stdio MCP server (blocks); 其余走 HTTP MCP client
-- @history 修改历史
--  \n [2026-10] 创建文件 (P1-1)
--  \n [2026-11] serve 走 mcp.server.serve (serve_entry + co_util.run_host_loop)
--]]


local cjson = require("cjson")
local codes = require("klbagent.code")
local profiles = require("klbagent.core.profiles")
local fmt = require("klbagent.core.cli.fmt")
local mcp = require("klbagent.packages.mcp")
local mcp_server = mcp.server()
local mcp_client = mcp.client()


local M = {}


M.help = function ()
	print("Usage: alua mcp serve [--profile P]")
	print("       alua mcp list --url URL [--token T] [--timeout S] [--json]")
	print("       alua mcp call --url URL --tool NAME [--args JSON] [--token T] [--timeout S] [--json]")
	print("       alua mcp probe --url URL [--token T] [--timeout S] [--json]")
	print("       alua mcp listen --url URL [--token T] [--timeout S] [--json]")
	print("       alua mcp close --url URL [--token T] [--timeout S] [--json]")
	fmt.print_kv("serve", "MCP server over stdio; tools map to klbagent catalog")
	fmt.print_kv("list", "remote MCP tools/list")
	fmt.print_kv("call", "remote MCP tools/call")
	fmt.print_kv("listen", "GET SSE subscribe server->client (needs session after probe/list)")
	fmt.print_kv("close", "DELETE end MCP HTTP session")
	fmt.print_kv("timeout", "HTTP client timeout seconds (default 15; listen default 60)")
	fmt.print_kv("url", "http(s) streamable endpoint (POST/GET/DELETE JSON-RPC)")
end


-- @brief 解析 --key value / --key=value; 其余为位置参数
-- @return opts[table]
-- @return pos[table]
local function _parse_opts(argv)
	local opts = {}
	local pos = {}
	local i = 1
	while i <= #argv do
		local a = argv[i]
		if string.sub(a, 1, 2) == "--" then
			local key = string.sub(a, 3)
			local val = nil
			local eq = string.find(key, "=", 1, true)
			if eq ~= nil then
				val = string.sub(key, eq + 1)
				key = string.sub(key, 1, eq - 1)
			elseif argv[i + 1] ~= nil and string.sub(argv[i + 1], 1, 2) ~= "--" then
				val = argv[i + 1]
				i = i + 1
			end

			key = string.gsub(key, "%-", "_")
			opts[key] = val or true
		else
			pos[#pos + 1] = a
		end

		i = i + 1
	end

	return opts, pos
end


local function _fail(msg)
	fmt.print_json(codes.make_error(codes.cli.BAD_REQUEST, msg))
	return false, codes.cli.BAD_REQUEST
end


-- @brief serve 子命令
local function _run_serve(argv)
	local opts = _parse_opts(argv)
	local profile = opts.profile
	if profile == true or profile == nil then
		profile = "readonly"
	end

	if not profiles.is_profile(profile) then
		return _fail("bad profile: " .. tostring(profile))
	end

	-- serve = serve_entry + run_host_loop (CLI hold / 独立自泵 co_main_loop)
	local ok, err = mcp_server.serve({
		profile = profile,
	})
	if not ok then
		io.stderr:write("alua mcp serve: " .. tostring(err) .. "\n")
		return false, codes.cli.TOOL_FAILED
	end

	return true, codes.OK
end


-- @brief 构造 client 上下文 (url/token/timeout)
local function _client_ctx(opts)
	if type(opts.url) ~= "string" or opts.url == "" then
		return nil, "need --url"
	end

	local ctx, err = mcp_client.new_context({
		url = opts.url,
		token = (type(opts.token) == "string" and opts.token) or nil,
		timeout_sec = tonumber(opts.timeout) or nil,
	})
	if ctx == nil then
		return nil, err
	end

	return ctx
end


local function _run_list(argv, want_json)
	local opts = _parse_opts(argv)
	local ctx, err = _client_ctx(opts)
	if ctx == nil then
		return _fail(err)
	end

	local tools, err2 = mcp_client.list_tools(ctx)
	if tools == nil then
		return _fail("mcp list fail: " .. tostring(err2))
	end

	if want_json then
		fmt.print_json({
			ok = true,
			url = opts.url,
			tools = tools,
		})
		return true, codes.OK
	end

	fmt.print_kv("url", opts.url)
	fmt.print_kv("tools", tostring(#tools))
	for i = 1, #tools do
		local t = tools[i]
		print(string.format("  %-32s %s", tostring(t.name or "?"), tostring(t.description or "")))
	end

	return true, codes.OK
end


local function _run_call(argv, want_json)
	local opts, pos = _parse_opts(argv)
	local tool = opts.tool
	if tool == true then
		tool = pos[1]
	end

	if type(tool) ~= "string" or tool == "" then
		tool = pos[1]
	end

	if type(tool) ~= "string" or tool == "" then
		return _fail("need --tool NAME")
	end

	local args = {}
	if type(opts.args) == "string" and opts.args ~= "" then
		local ok_j, obj = pcall(cjson.decode, opts.args)
		if not ok_j or type(obj) ~= "table" then
			return _fail("bad --args json")
		end

		args = obj
	end

	local ctx, err = _client_ctx(opts)
	if ctx == nil then
		return _fail(err)
	end

	local result, err2 = mcp_client.call_tool(ctx, tool, args)
	if result == nil then
		return _fail("mcp call fail: " .. tostring(err2))
	end

	if want_json then
		fmt.print_json({
			ok = true,
			url = opts.url,
			tool = tool,
			result = result,
		})
	else
		fmt.print_json(result)
	end

	return true, codes.OK
end


local function _run_probe(argv, want_json)
	local opts = _parse_opts(argv)
	local ctx, err = _client_ctx(opts)
	if ctx == nil then
		return _fail(err)
	end

	local result, err2 = mcp_client.ping(ctx)
	if result == nil then
		return _fail("mcp probe fail: " .. tostring(err2))
	end

	if want_json then
		fmt.print_json({
			ok = true,
			url = opts.url,
			session = ctx.session_id,
			protocol = ctx.protocol_version,
			result = result,
		})
	else
		fmt.print_kv("url", opts.url)
		fmt.print_kv("protocol", tostring(ctx.protocol_version or ""))
		fmt.print_kv("session", tostring(ctx.session_id or ""))
		fmt.print_kv("ping", "ok")
	end

	return true, codes.OK
end


local function _run_listen(argv, want_json)
	local opts = _parse_opts(argv)
	local ctx, err = _client_ctx(opts)
	if ctx == nil then
		return _fail(err)
	end

	-- 先握手拿 session, 再 GET SSE
	local info, err_i = mcp_client.initialize(ctx)
	if info == nil then
		return _fail("mcp listen init fail: " .. tostring(err_i))
	end

	local events, err2 = mcp_client.listen(ctx, {
		timeout_sec = tonumber(opts.timeout) or 60,
	})
	if events == nil then
		return _fail("mcp listen fail: " .. tostring(err2))
	end

	if type(events) ~= "table" then
		events = {}
	end

	local n = #events
	if want_json then
		local out_events = events
		if n == 0 and cjson.empty_array ~= nil then
			out_events = cjson.empty_array
		end

		fmt.print_json({
			ok = true,
			url = opts.url,
			session = ctx.session_id,
			events = out_events,
			count = n,
		})
		return true, codes.OK
	end

	fmt.print_kv("url", opts.url)
	fmt.print_kv("session", tostring(ctx.session_id or ""))
	fmt.print_kv("events", tostring(n))
	for i = 1, n do
		local ev = events[i]
		print(string.format("  [%d] %s", i, tostring(ev.method or ev.id or "?")))
	end

	return true, codes.OK
end


local function _run_close(argv, want_json)
	local opts = _parse_opts(argv)
	local ctx, err = _client_ctx(opts)
	if ctx == nil then
		return _fail(err)
	end

	local info, err_i = mcp_client.initialize(ctx)
	if info == nil then
		return _fail("mcp close init fail: " .. tostring(err_i))
	end

	local ok, err2 = mcp_client.close(ctx)
	if not ok then
		return _fail("mcp close fail: " .. tostring(err2))
	end

	if want_json then
		fmt.print_json({
			ok = true,
			url = opts.url,
			closed = true,
		})
	else
		fmt.print_kv("url", opts.url)
		fmt.print_kv("closed", "ok")
	end

	return true, codes.OK
end


-- @brief CLI 入口
-- @param [in] ...			serve|list|call|probe|listen|close
-- @return ok[boolean]
-- @return code[number]
M.run = function (...)
	local want_help, want_json, rest = fmt.take_cli_flags({ ... })
	if want_help then
		M.help()
		return true, codes.OK
	end

	local sub = rest[1]
	local argv = { table.unpack(rest, 2) }
	if sub == "serve" then
		return _run_serve(argv)
	end

	if sub == "list" then
		return _run_list(argv, want_json)
	end

	if sub == "call" then
		return _run_call(argv, want_json)
	end

	if sub == "probe" then
		return _run_probe(argv, want_json)
	end

	if sub == "listen" then
		return _run_listen(argv, want_json)
	end

	if sub == "close" then
		return _run_close(argv, want_json)
	end

	M.help()
	fmt.print_json(codes.make_error(codes.cli.BAD_REQUEST, "alua mcp serve|list|call|probe|listen|close"))
	return false, codes.cli.BAD_REQUEST
end


return M

-- end
