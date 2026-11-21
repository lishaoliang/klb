--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  AI L0 session/process ack 硬门禁 (serve/stdio 默认开)
-- @note   白名单工具可在未 ack 时调用; ALUA_AI_LOAD_GATE=0 关; =1 强制开; ALUA_AI_LOAD_ACK=1 逃生已 ack
--]]


local session = require("klbagent.core.session")


local M = {}


-- auto | on | off
local _mode = "auto"
local _channel = "cli"
local _acked = false


local WHITELIST = {
	["runtime.ai_bootstrap"] = true,
	["runtime.ai_load_ack"] = true,
	["runtime.ping"] = true,
	["runtime.version"] = true,
	["runtime.set_profile"] = true,
	["runtime.selfcheck"] = true,
	["runtime.hostinfo"] = true,
	["runtime.cwd"] = true,
	["runtime.parallel"] = true,
	["docs.list"] = true,
	["docs.read"] = true,
	["catalog.tools"] = true,
	["catalog.categories"] = true,
	["lpc.call"] = true,
}


local function _env_gate()
	local v = os.getenv("ALUA_AI_LOAD_GATE")
	if type(v) ~= "string" or v == "" then
		return nil
	end

	v = string.lower(v)
	if v == "0" or v == "off" or v == "false" or v == "no" then
		return false
	end

	if v == "1" or v == "on" or v == "true" or v == "yes" or v == "force" then
		return true
	end

	return nil
end


local function _env_pre_ack()
	local v = os.getenv("ALUA_AI_LOAD_ACK")
	if type(v) ~= "string" or v == "" then
		return false
	end

	v = string.lower(v)
	return (v == "1" or v == "on" or v == "true" or v == "yes")
end


-- @brief 标记当前通道 (cli / stdio / serve)
-- @param [in] channel[string]
M.set_channel = function (channel)
	if type(channel) == "string" and channel ~= "" then
		_channel = channel
	end
end


-- @brief 是否启用门禁
-- @return enabled[boolean]
M.enabled = function ()
	local eg = _env_gate()
	if eg == false then
		return false
	end

	if eg == true then
		return true
	end

	-- auto: serve / stdio 开; 单次 cli call 关
	if _channel == "serve" or _channel == "stdio" then
		return true
	end

	return false
end


local function _session_acked(session_id)
	if type(session_id) ~= "string" or session_id == "" then
		return false
	end

	local info = session.get(session_id)
	if info == nil or type(info.attrs) ~= "table" then
		return false
	end

	return info.attrs.ai_load_ack == true or info.attrs.ai_load_ack == "true"
end


-- @brief 当前是否已 ack (进程或 session)
-- @param [in] session_id[string|nil]
-- @return acked[boolean]
M.is_acked = function (session_id)
	if _env_pre_ack() then
		return true
	end

	if _acked then
		return true
	end

	return _session_acked(session_id)
end


-- @brief 置 ack
-- @param [in] opts[table|nil]  session_id?
-- @return info[table]
M.ack = function (opts)
	opts = opts or {}
	_acked = true

	local sid = opts.session_id
	if type(sid) == "string" and sid ~= "" then
		session.set_attrs(sid, { ai_load_ack = true })
	end

	return {
		ok = true,
		ack = true,
		channel = _channel,
		gate_enabled = M.enabled(),
		session_id = sid,
	}
end


-- @brief 工具是否允许在未 ack 时调用
-- @param [in] name[string]
-- @return ok[boolean]
M.whitelisted = function (name)
	if type(name) ~= "string" then
		return false
	end

	if WHITELIST[name] == true then
		return true
	end

	-- catalog.* / docs.* 前缀放行
	if string.sub(name, 1, 8) == "catalog." then
		return true
	end

	if string.sub(name, 1, 5) == "docs." then
		return true
	end

	return false
end


-- @brief 门禁检查; 允许则 true
-- @param [in] name[string]
-- @param [in] session_id[string|nil]
-- @return ok[boolean]
-- @return err_msg[string|nil]
M.check = function (name, session_id)
	if not M.enabled() then
		return true
	end

	if M.is_acked(session_id) then
		return true
	end

	if M.whitelisted(name) then
		return true
	end

	return false, "ai_load ack required; call runtime.ai_bootstrap then runtime.ai_load_ack"
end


-- @brief 状态表 (selfcheck / 调试)
M.status = function ()
	return {
		enabled = M.enabled(),
		acked = M.is_acked(nil),
		channel = _channel,
		mode = _mode,
		env_gate = os.getenv("ALUA_AI_LOAD_GATE") or "",
		env_pre_ack = _env_pre_ack(),
	}
end


return M

-- end
