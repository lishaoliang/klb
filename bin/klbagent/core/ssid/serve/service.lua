--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   service.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua.ssid LPC / XPC 服务注册 (独立于 alua.tools)
-- @stability internal
-- @impl evolving
-- @note   LPC/XPC 方法面分区 (const.LPC_METHODS / XPC_METHODS); 无 is_raw
--   \n 原始 token 仅 serve 本地维护; 客户端通道禁 reveal / 禁验 raw
--   \n 分发到 ssid.serve.co_* (owner 本地实现)
-- @history 修改历史
--  \n [2026-11] 创建文件 (M6/M7); 增 verify_token / begin_ctx
--  \n [2026-11] 迁入 serve/; 分发改打 ssid.serve
--  \n [2026-11] LPC/XPC 分注册; XPC 不置 trust / 无 begin_ctx
--  \n [2026-11] 分发改调 serve.co_*
--  \n [2026-11] unix path: Lua 拼 /tmp/alua.ssid.xpc.unix.pxxxx 再传入 C
--]]
local const = require("klbagent.core.ssid.base.const")
local xpcpath = require("klbagent.core.ssid.base.xpcpath")


local M = {}


local g_lpc_ready = false
local g_xpc_ready = false


-- 特例: 打断 service↔serve 循环依赖; require 须见顶部
local function _ssid_serve()
	return require("klbagent.core.ssid.serve")
end


-- @brief 统一方法分发
-- @param [in] name[string]
-- @param [in] opts[table|nil]
-- @param [in] trust[string]		lpc|xpc
local function _dispatch_method(name, opts, trust)
	if type(opts) ~= "table" then
		opts = {}
	end

	opts.trust = trust
	local ssid = _ssid_serve()

	if name == "verify_token" then
		local info, err, code = ssid.co_verify_token(opts.alua_ssid or opts.ssid, opts.token, opts)
		if info ~= nil then
			return true, { info = info, err = nil, code = code }
		end

		return true, { info = nil, err = err, code = code }
	elseif name == "list_tokens" then
		local info, err = ssid.co_list_tokens(opts)
		if info then
			return true, info
		end

		return false, err
	elseif name == "begin_ctx" then
		local ctx, err = ssid.co_begin_ctx(opts)
		if ctx then
			return true, ctx
		end

		return false, err
	elseif name == "whoami" then
		local info, err = ssid.co_whoami(opts)
		if info then
			return true, info
		end

		return false, err
	elseif name == "ensure" then
		local info, err = ssid.co_ensure(opts)
		if info then
			return true, info
		end

		return false, err
	elseif name == "get_token" then
		local info, err = ssid.co_get_token(opts)
		if info then
			return true, info
		end

		return false, err
	elseif name == "renew" then
		local info, err = ssid.co_renew(opts)
		if info then
			return true, info
		end

		return false, err
	elseif name == "destroy" then
		local ok, err = ssid.co_destroy(opts)
		if ok then
			return true, { ok = true }
		end

		return false, err
	elseif name == "status" then
		local info, err = ssid.co_status(opts)
		if info then
			return true, info
		end

		return false, err
	elseif name == "register" then
		local info, err = ssid.co_register(opts)
		if info then
			return true, info
		end

		return false, err
	elseif name == "resolve" then
		local info, err = ssid.co_resolve(opts)
		if info then
			return true, info
		end

		return false, err
	elseif name == "gc" then
		local info, err = ssid.gc(opts)
		if info then
			return true, info
		end

		return false, err
	end

	return false, "unknown method: " .. tostring(name)
end


-- @param [in] svc[table]
-- @param [in] methods[table]		方法名列表
-- @param [in] trust[string]		lpc|xpc
-- @param [in] allow_reveal[boolean]	仅 LPC 派发期可置 trust 读明文
local function _register_calls(svc, methods, trust, allow_reveal)
	for i = 1, #methods do
		local name = methods[i]
		svc:call(name, function (req)
			local opts = nil
			if type(req) == "table" and type(req.args) == "table" then
				opts = req.args[1]
			end

			-- 内部签发标记禁经 LPC/XPC 透传 (防伪造 auth.issue / revoke 授权)
			if type(opts) == "table" then
				opts.issued_by = nil
			end

			local ssid_m = _ssid_serve()
			local prev = ssid_m.is_trusted()
			if allow_reveal then
				ssid_m.set_trust(true)
			else
				ssid_m.set_trust(false)
				if type(opts) == "table" then
					opts.reveal = nil
				end
			end

			local ok_r, a, b = pcall(_dispatch_method, name, opts, trust)
			ssid_m.set_trust(prev)
			if not ok_r then
				return false, tostring(a)
			end

			return a, b
		end)
	end
end


M.ensure_lpc = function (module_name)
	if g_lpc_ready then
		return true
	end

	local ok_l, klblpc = pcall(require, "klbcore.klblpc")
	if not ok_l or type(klblpc) ~= "table" or type(klblpc.new) ~= "function" then
		return false, "klblpc unavailable"
	end

	local svc = klblpc.new({ name = module_name })
	if type(svc) ~= "table" then
		return false, "klblpc.new fail"
	end

	local ok_s, err_s = svc:setup({ name = module_name })
	if not ok_s then
		return false, "klblpc.setup fail: " .. tostring(err_s)
	end

	_register_calls(svc, const.LPC_METHODS, "lpc", true)

	local ok_v, err_v = svc:serve()
	if not ok_v then
		return false, "klblpc.serve fail: " .. tostring(err_v)
	end

	g_lpc_ready = true
	return true
end


-- @brief unix owner 是否已有存活对端 (open 成功即 connect 可达)
-- @param [in] module_name[string]
-- @param [in] path[string|nil]
-- @return owned[boolean]
local function _unix_xpc_owned(module_name, path)
	local ok_k, kxpc = pcall(require, "kxpc")
	if not ok_k or type(kxpc) ~= "table" or type(kxpc.open_unix) ~= "function" then
		return false
	end

	local path_arg = nil
	if type(path) == "string" and path ~= "" then
		path_arg = path
	end

	local ok_o, cli = pcall(kxpc.open_unix, module_name, path_arg)
	if not ok_o or cli == nil then
		return false
	end

	pcall(function ()
		cli:close()
	end)
	return true
end


M.ensure_xpc = function (module_name, opts)
	if g_xpc_ready then
		return true
	end

	if type(opts) ~= "table" then
		opts = {}
	end

	local ok_x, klbxpc = pcall(require, "klbcore.klbxpc")
	if not ok_x or type(klbxpc) ~= "table" or type(klbxpc.new) ~= "function" then
		return false, "klbxpc unavailable"
	end

	local transport = opts.transport or klbxpc.DEFAULT_TRANSPORT
	local cfger = require("klbcore.klbxpc.cfger")
	local path = opts.path
	if cfger.is_unix(transport) then
		-- Lua 指定路径; 禁空 path 回退 C 默认 /tmp/klb.xpc.<name>
		if type(path) ~= "string" or path == "" then
			local err_p = nil
			path, _, err_p = xpcpath.resolve(opts)
			if type(path) ~= "string" or path == "" then
				return false, err_p or "alua.ssid xpc path resolve fail"
			end
		end

		opts.path = path
	end

	if cfger.is_unix(transport) and _unix_xpc_owned(module_name, path) then
		return false, "alua.ssid xpc already owned"
	end

	local cfg = {
		name = module_name,
		transport = transport,
		path = path,
	}
	local svc = klbxpc.new(cfg)
	if type(svc) ~= "table" then
		return false, "klbxpc.new fail"
	end

	local ok_s, err_s = svc:setup(cfg)
	if not ok_s then
		return false, "klbxpc.setup fail: " .. tostring(err_s)
	end

	_register_calls(svc, const.XPC_METHODS, "xpc", false)

	local ok_v, err_v = svc:serve()
	if not ok_v then
		if cfger.is_unix(transport) and _unix_xpc_owned(module_name, path) then
			return false, "alua.ssid xpc already owned"
		end

		return false, "klbxpc.serve fail: " .. tostring(err_v)
	end

	g_xpc_ready = true
	return true
end


M.lpc_ready = function ()
	return g_lpc_ready
end


M.xpc_ready = function ()
	return g_xpc_ready
end


return M

-- end
