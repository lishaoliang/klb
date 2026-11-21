--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   token_env.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  ssid serve 临时 token 内存表 (进程内; 重启丢失须重新 ensure)
-- @stability internal
-- @impl evolving
-- @note   require("klbagent.core.ssid.serve.token_env")
--   \n 不再读写 pref 下 temp.<id>.env; token_ref 形如 mem:<id>
--   \n 兼容读旧 file:temp.<id>.env / file:<id> 键名 (仅内存命中, 不读盘)
--]]
local M = {}


-- [safe_client_id] = token
local _mem = {}


local function _safe_client_id(client_id)
	local s = string.gsub(tostring(client_id), "[^%w%.%-_]", "-")
	s = string.gsub(s, "%-+", "-")
	s = string.gsub(s, "^%-", "")
	s = string.gsub(s, "%-$", "")
	if s == "" then
		return nil
	end

	return s
end


-- @brief 从 token_ref 解析内存键 (禁路径穿越)
-- @param [in] token_ref[string]
-- @return safe_id[string|nil]
local function _id_from_ref(token_ref)
	if type(token_ref) ~= "string" or token_ref == "" then
		return nil
	end

	local id = string.match(token_ref, "^mem:(.+)$")
	if type(id) ~= "string" or id == "" then
		id = string.match(token_ref, "^file:temp%.(.+)%.env$")
	end

	if type(id) ~= "string" or id == "" then
		id = string.match(token_ref, "^file:(.+)$")
	end

	if type(id) ~= "string" or id == "" then
		return nil
	end

	if string.find(id, "%.%.", 1, true) or string.find(id, "/", 1, true)
		or string.find(id, "\\", 1, true) then
		return nil
	end

	return _safe_client_id(id)
end


-- @brief 写入临时 token (仅内存)
-- @param [in] client_id[string]
-- @param [in] token[string]
-- @return token_ref[string|nil]		mem:<id>
-- @return err[string|nil]
M.save = function (client_id, token)
	local safe = _safe_client_id(client_id)
	if safe == nil then
		return nil, "bad client_id for save"
	end

	if type(token) ~= "string" or token == "" then
		return nil, "missing token"
	end

	_mem[safe] = token
	return "mem:" .. safe, nil
end


-- @brief 由 token_ref 读临时 token 明文 (仅内存)
-- @param [in] token_ref[string]
-- @return token[string|nil]
M.read = function (token_ref)
	local safe = _id_from_ref(token_ref)
	if safe == nil then
		return nil
	end

	local tok = _mem[safe]
	if type(tok) == "string" and tok ~= "" then
		return tok
	end

	return nil
end


-- @brief 删除临时 token (仅内存)
-- @param [in] token_ref[string|nil]
M.unlink = function (token_ref)
	local safe = _id_from_ref(token_ref)
	if safe == nil then
		return
	end

	_mem[safe] = nil
end


return M

-- end
