--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   client/xpcclienter.lua
-- @brief  klbxpc 长期客户端
-- @note   须先 open 对端模块名; 方法 :post / :request / :close;
--         可选 opts={transport=,path=} 或 transport 字符串
-- @history 修改历史
--		[2026-11] 创建文件
--]]
local kxpc = require("kxpc")
local cfger = require("klbcore.klbxpc.cfger")


local xpcclienter = {}


xpcclienter.new_client = function (peer_name, opts)
	if "string" ~= type(peer_name) or "" == peer_name then
		return nil
	end

	if nil == opts then
		return kxpc.open(peer_name)
	end

	if "string" == type(opts) then
		local tr = cfger.parse_transport(opts)
		if not tr then
			return nil
		end

		if cfger.is_unix(tr) then
			return kxpc.open_unix(peer_name)
		end

		if cfger.is_npipe(tr) then
			return kxpc.open_npipe(peer_name)
		end

		if cfger.is_sysmq(tr) then
			return kxpc.open_sysmq(peer_name)
		end

		return kxpc.open(peer_name)
	end

	if "table" ~= type(opts) then
		return nil
	end

	local parsed, ok = cfger.parse_setup({
		name = peer_name,
		transport = opts.transport,
		path = opts.path,
	})
	if not ok or not parsed then
		return nil
	end

	if cfger.is_unix(parsed.transport) then
		return kxpc.open_unix(peer_name, cfger.path_arg(parsed.path))
	end

	if cfger.is_npipe(parsed.transport) then
		return kxpc.open_npipe(peer_name, cfger.path_arg(parsed.path))
	end

	if cfger.is_sysmq(parsed.transport) then
		return kxpc.open_sysmq(peer_name, cfger.path_arg(parsed.path))
	end

	return kxpc.open(peer_name)
end


return xpcclienter
