--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   bridge.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua_ui 工具桥 (本机 dispatch / 远程 remote.call)
-- @note   页面 commands 经此调用; 禁止直写 pref; token 不进控件明文
-- @history 修改历史
--  \n [2026-10] 创建文件 (桩); 本机 dispatch; set_target + remote.call
--]]
local dispatch = require("klbagent.core.dispatch")
local catalog = require("klbagent.core.catalog")
local registry = require("klbagent.core.registry")


local M = {}


local s_tools = false


-- 目标: local | peer | host
M.target = {
	mode = "local",
	host = "",
	port = 0,
	peer = "",
	scheme = "",
}


-- @brief 确保工具已登记
-- @return ok[boolean]
-- @return err[string|nil]
M.ensure_tools = function ()
	if s_tools then
		return true
	end

	local ok, err = registry.load_all(catalog, dispatch)
	if not ok then
		return false, tostring(err or "registry.load_all")
	end

	s_tools = true
	return true
end


-- @brief 目标可读摘要 (无 token)
-- @return text[string]
M.target_label = function ()
	local t = M.target
	if t.mode == "local" then
		return "local"
	end

	if t.mode == "peer" then
		return "peer=" .. tostring(t.peer or "")
	end

	if t.mode == "host" then
		local host = tostring(t.host or "")
		local port = tonumber(t.port)
		if type(port) ~= "number" then
			error("alua.core.serve.UNAVAILABLE: need port")
		end
		local sch = t.scheme
		if type(sch) == "string" and sch ~= "" then
			return sch .. "://" .. host .. ":" .. tostring(port)
		end

		return host .. ":" .. tostring(port)
	end

	return "?"
end


-- @brief 设置调用目标
-- @param [in] opts[table|nil]  mode=local|peer|host; peer; host; port; scheme
-- @return ok[boolean]
-- @return err[string|nil]
M.set_target = function (opts)
	opts = opts or {}
	local mode = opts.mode or "local"

	if mode == "local" then
		M.target.mode = "local"
		M.target.host = ""
		M.target.port = 0
		M.target.peer = ""
		M.target.scheme = ""
		return true
	end

	if mode == "peer" then
		local peer = opts.peer or opts.id
		if type(peer) ~= "string" or peer == "" then
			return false, "need peer id"
		end

		M.target.mode = "peer"
		M.target.peer = peer
		M.target.host = ""
		M.target.port = 0
		M.target.scheme = type(opts.scheme) == "string" and opts.scheme or ""
		return true
	end

	if mode == "host" then
		local host = opts.host
		if type(host) ~= "string" or host == "" then
			return false, "need host"
		end

		local port = opts.port
		if type(port) ~= "number" or port < 1 or 65535 < port then
			error("alua.core.serve.UNAVAILABLE: need port")
		end

		M.target.mode = "host"
		M.target.host = host
		M.target.port = port
		M.target.peer = ""
		M.target.scheme = type(opts.scheme) == "string" and opts.scheme or ""
		return true
	end

	return false, "bad mode (local|peer|host)"
end


-- @brief 调用工具 (本机或 remote.call 转发)
-- @param [in] name[string]
-- @param [in] args[table|nil]
-- @return ok[boolean]
-- @return result[table|nil]
-- @return err[table|string|nil]
M.call = function (name, args)
	if type(name) ~= "string" or name == "" then
		return false, nil, "need tool name"
	end

	local ok_tools, err_tools = M.ensure_tools()
	if not ok_tools then
		return false, nil, err_tools
	end

	args = args or {}

	if M.target.mode == "local" then
		local ok, res = dispatch.call({
			name = name,
			args = args,
		})
		if not ok then
			return false, nil, res
		end

		return true, res, nil
	end

	-- 远程: 经 remote.call; token 由 peer / serve.env / ALUA_SERVE_TOKEN 解析, 不由 UI 传入
	local remote_args = {
		name = name,
		args = args,
	}

	if M.target.mode == "peer" then
		remote_args.peer = M.target.peer
		if M.target.scheme ~= "" then
			remote_args.scheme = M.target.scheme
		end
	else
		remote_args.host = M.target.host
		remote_args.port = M.target.port
		if M.target.scheme ~= "" then
			remote_args.scheme = M.target.scheme
		end
	end

	local ok, res = dispatch.call({
		name = "remote.call",
		args = remote_args,
	})
	if not ok then
		return false, nil, res
	end

	return true, res, nil
end


local function _err_msg(err)
	if type(err) == "table" then
		return tostring(err.message or err.name or "fail")
	end

	return tostring(err or "fail")
end


-- @brief 总览快照
-- @return ok[boolean]
-- @return snap[table|nil]
-- @return err[any]
M.overview = function ()
	local snap = {
		target = M.target_label(),
		mode = M.target.mode,
		ping = nil,
		selfcheck = nil,
		version = nil,
		runtime = nil,
		ok = false,
		message = "",
	}

	local ok_p, ping, err_p = M.call("runtime.ping", {})
	if ok_p then
		snap.ping = ping
	else
		snap.message = "runtime.ping: " .. _err_msg(err_p)
		snap.err = err_p
		return true, snap, nil
	end

	local ok_s, sc, err_s = M.call("runtime.selfcheck", {})
	if ok_s then
		snap.selfcheck = sc
	else
		snap.message = "runtime.selfcheck: " .. _err_msg(err_s)
		snap.err = err_s
		return true, snap, nil
	end

	local ok_v, ver, err_v = M.call("runtime.version", {})
	if ok_v then
		snap.version = ver
	else
		snap.message = "runtime.version: " .. _err_msg(err_v)
		snap.err = err_v
	end

	local ok_c, cfg, err_c = M.call("config.get", { module = "runtime" })
	if ok_c then
		snap.runtime = cfg
	else
		snap.message = "config.get runtime: " .. _err_msg(err_c)
		snap.err = err_c
	end

	snap.ok = true
	if snap.message == "" then
		snap.message = "ok"
	end

	return true, snap, nil
end


-- @brief serve 配置快照
-- @return ok[boolean]
-- @return snap[table|nil]
M.serve_snap = function ()
	local ok, res, err = M.call("config.get", { module = "serve" })
	if not ok then
		return false, {
			ok = false,
			message = _err_msg(err),
			err = err,
		}
	end

	local cfg = res and res.cfg or {}
	return true, {
		ok = true,
		cfg = cfg,
		tls_face = cfg.tls_face or "-",
		path = res.path,
		live = res.live,
		message = "ok",
	}
end


-- @brief 写 serve 配置
-- @param [in] patch[table]
-- @param [in] scope[string|nil]  once|permanent
-- @return ok[boolean]
-- @return result[table|nil]
-- @return err[any]
M.serve_set = function (patch, scope)
	return M.call("config.set", {
		module = "serve",
		cfg = patch or {},
		scope = scope or "permanent",
	})
end


-- @brief runtime 档位快照
-- @return ok[boolean]
-- @return snap[table|nil]
M.runtime_snap = function ()
	local ok, res, err = M.call("config.get", { module = "runtime" })
	local active = nil

	local ok_sc, sc = M.call("runtime.selfcheck", {})
	if ok_sc and type(sc) == "table" then
		active = sc.profile or sc.active_profile
		if type(sc.runtime) == "table" then
			active = active or sc.runtime.profile
		end
	end

	if not ok then
		return false, {
			ok = false,
			message = _err_msg(err),
			active = active,
			err = err,
		}
	end

	local cfg = res and res.cfg or {}
	return true, {
		ok = true,
		cfg = cfg,
		pref_profile = cfg.profile or "-",
		active = active or cfg.profile or "-",
		path = res.path,
		message = "ok",
	}
end


-- @brief 调整档位
-- @param [in] profile[string]
-- @param [in] scope[string|nil]
-- @param [in] confirm[boolean|nil]
-- @return ok[boolean]
-- @return result[table|nil]
-- @return err[any]
M.set_profile = function (profile, scope, confirm)
	local args = {
		profile = profile,
		scope = scope or "once",
	}
	if confirm == true then
		args.confirm = true
	end

	return M.call("runtime.set_profile", args)
end


return M

-- end
