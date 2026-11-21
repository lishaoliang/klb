--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  ssid.* 工具 (中层门面对外 catalog)
-- @note   一律 new_client(XPC → 本机 serve owner); 禁 new_serve; 须在 kco 协程内
-- @history 修改历史
--  \n [2026-11] 创建文件 (M8); 去掉主线程 owner 回退
--]]


-- @brief 取 ssid 客户端面 (跨进程 XPC)
-- @return face[table]
local function _ssid()
	local pkg = require("klbagent.core.ssid")
	return pkg.new_client({ transport = "xpc" })
end


local M = {}


local function _whoami(args)
	local ssid = _ssid()
	local info, err = ssid.co_whoami(args or { via = "call" })
	if info == nil then
		error(tostring(err or "whoami fail"))
	end

	-- 合并 core.session 指纹字段 (原 runtime.session_whoami 对外职责并入本工具)
	do
		local ok_s, session = pcall(require, "klbagent.core.session")
		if ok_s and type(session) == "table" and type(session.whoami) == "function" then
			local sinfo = session.whoami({})
			if type(sinfo) == "table" then
				if info.session_id == nil then
					info.session_id = sinfo.session_id
				end

				if info.protocol_version == nil then
					info.protocol_version = sinfo.protocol_version
				end

				if info.client_name == nil then
					info.client_name = sinfo.client_name
				end

				if info.via == nil then
					info.via = sinfo.via
				end

				if info.caller_kind == nil then
					info.caller_kind = sinfo.caller_kind
				end

				if info.host_tool == nil then
					info.host_tool = sinfo.host_tool
				end
			end
		end
	end

	return info
end


-- @brief 业务回显脱敏: 禁 token / token_ref; 仅 has_token 布尔
-- @param [in] info[table]
-- @return info[table]
local function _public_ssid_result(info)
	if type(info) ~= "table" then
		return info
	end

	local has = false
	if type(info.token) == "string" and info.token ~= "" then
		has = true
	elseif type(info.token_ref) == "string" and info.token_ref ~= "" then
		has = true
	elseif info.has_token == true or info.has_token_ref == true then
		has = true
	end

	info.token = nil
	info.token_ref = nil
	info.has_token = has
	return info
end


local function _ensure(args)
	local ssid = _ssid()
	local info, err = ssid.co_ensure(args or {})
	if info == nil then
		error(tostring(err or "ensure fail"))
	end

	return _public_ssid_result(info)
end


local function _status(args)
	return _ssid().co_status(args)
end


local function _begin_ctx(args)
	local ssid = _ssid()
	local ctx, err = ssid.co_begin_ctx(args or { via = "call" })
	if ctx == nil then
		error(tostring(err or "begin_ctx fail"))
	end

	return _public_ssid_result(ctx)
end


local function _gc(args)
	args = args or {}
	-- 默认 dry_run, 避免误清; 显式 dry_run=false 才删除
	if args.dry_run == nil then
		args.dry_run = true
	end

	local info, err = _ssid().co_gc(args)
	if info == nil then
		error(tostring(err or "gc fail"))
	end

	return info
end


M.register = function (catalog, dispatch_reg)
	local ok, err

	ok, err = catalog.register({
		name = "ssid.whoami",
		category = "runtime",
		title = "ssid whoami",
		summary = "Show alua_ssid / fingerprint summary (no secrets).",
		min_profile = "readonly",
		args_schema = {
			via = "string optional",
		},
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch_reg.register("ssid.whoami", _whoami)
	if not ok then
		return false, err
	end

	ok, err = catalog.register({
		name = "ssid.ensure",
		category = "runtime",
		title = "ssid ensure",
		summary = "Ensure temp session token in ssid memory; result has_token only (no token/token_ref).",
		min_profile = "net",
		args_schema = {
			alua_ssid = "string optional",
			profile = "string optional",
			force = "boolean optional",
		},
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch_reg.register("ssid.ensure", _ensure)
	if not ok then
		return false, err
	end

	ok, err = catalog.register({
		name = "ssid.begin_ctx",
		category = "runtime",
		title = "ssid begin_ctx",
		summary = "Begin request context (bind ssid); result has_token only (no token/token_ref).",
		min_profile = "readonly",
		args_schema = {
			via = "string optional",
			profile = "string optional",
		},
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch_reg.register("ssid.begin_ctx", _begin_ctx)
	if not ok then
		return false, err
	end

	ok, err = catalog.register({
		name = "ssid.status",
		category = "runtime",
		title = "ssid status",
		summary = "Redacted ssid counts / readiness.",
		min_profile = "readonly",
		args_schema = {},
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch_reg.register("ssid.status", _status)
	if not ok then
		return false, err
	end

	ok, err = catalog.register({
		name = "ssid.gc",
		category = "runtime",
		title = "ssid gc",
		summary = "GC ssid.clients.json by last_seen_ms; default dry_run=true; optional purge_temp_files for leftover temp.*.env; never touches machine_key.",
		min_profile = "net",
		args_schema = {
			dry_run = "boolean optional",
			max_age_ms = "number optional",
			purge_temp_files = "boolean optional",
		},
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch_reg.register("ssid.gc", _gc)
	if not ok then
		return false, err
	end

	return true
end


return M

-- end
