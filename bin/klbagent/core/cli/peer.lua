--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   peer.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua peer — list/ping/approve/revoke; 工具结果→kv 人读
-- @note   alua peer list|ping|approve|revoke … [--json]
-- @history 修改历史
--  \n [2026-10] 创建文件 (自 cli/init 拆出)
--]]
local codes = require("klbagent.code")
local fmt = require("klbagent.core.cli.fmt")


local M = {}


M.help = function ()
	print("Usage: alua peer list [--json]")
	print("       alua peer ping|approve|revoke <id> [--json]")
	fmt.print_kv("list", "peers.json summary")
	fmt.print_kv("ping", "probe peer (pending ok)")
	fmt.print_kv("approve", "trust ceremony → trusted")
	fmt.print_kv("revoke", "revoke trust")
	fmt.print_kv("format", "--json|json|-j = JSON(AI)")
end


local function _print_list_text(result)
	local counts = type(result) == "table" and result.counts or {}
	local peers = type(result) == "table" and result.peers or {}
	fmt.print_kv("peers", string.format("total=%s trusted=%s pending=%s revoked=%s",
		tostring(counts.total or 0),
		tostring(counts.trusted or 0),
		tostring(counts.pending or 0),
		tostring(counts.revoked or 0)))
	for i = 1, #peers do
		local p = peers[i]
		print(string.format("  %-10s state=%s  %s  %s:%s",
			tostring(p.id or "?"),
			tostring(p.state or ""),
			tostring(p.scheme or ""),
			tostring(p.host or ""),
			tostring(p.port or "")))
	end
end


local function _print_op_text(result, id)
	if type(result) ~= "table" then
		fmt.print_kv("peer", tostring(id))
		fmt.print_kv("result", tostring(result))
		return
	end

	fmt.print_kv("peer", tostring(result.peer or id))
	if result.state ~= nil then
		fmt.print_kv("state", tostring(result.state))
	end

	if result.host ~= nil then
		fmt.print_kv("endpoint", string.format("%s:%s  scheme=%s",
			tostring(result.host),
			tostring(result.port or ""),
			tostring(result.scheme or "")))
	end

	if type(result.message) == "string" and result.message ~= "" then
		fmt.print_kv("message", result.message)
	end
end


-- @brief CLI 入口
-- @param [in] ...		list|ping|approve|revoke …
-- @return ok[boolean]
-- @return code[number]
M.run = function (...)
	local want_help, want_json, rest = fmt.take_cli_flags({ ... })
	if want_help then
		M.help()
		return true, codes.OK
	end

	local sub = rest[1]
	if type(sub) ~= "string" or sub == "" then
		M.help()
		fmt.print_json(codes.make_error(codes.cli.BAD_REQUEST, "peer: need list|ping|approve|revoke"))
		return false, codes.cli.BAD_REQUEST
	end

	if sub == "list" then
		return fmt.emit_tool("peer.list", {}, want_json, _print_list_text)
	end

	local id = rest[2]
	if type(id) ~= "string" or id == "" then
		fmt.print_json(codes.make_error(codes.cli.BAD_REQUEST, "peer: need id (linux|wsl|windows)"))
		return false, codes.cli.BAD_REQUEST
	end

	local tool = nil
	if sub == "ping" then
		tool = "peer.ping"
	elseif sub == "approve" then
		tool = "peer.approve"
	elseif sub == "revoke" then
		tool = "peer.revoke"
	else
		fmt.print_json(codes.make_error(codes.cli.BAD_REQUEST, "peer: unknown subcommand"))
		return false, codes.cli.BAD_REQUEST
	end

	return fmt.emit_tool(tool, { id = id }, want_json, function (result)
		_print_op_text(result, id)
	end)
end


return M
