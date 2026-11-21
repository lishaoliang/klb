--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   util.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  fs.* 公共辅助
-- @note   路径校验 / 截断 / join / register 辅助; base64 走 C kcrypto
-- @history 修改历史
--  \n [2026-10] 从 init.lua 拆出; base64 编解码 (ws 面二进制 payload / Bug.2026-9)
--]]


local kcrypto = require("kcrypto")


local M = {}


M.DEFAULT_MAX_BYTES = 1048576
M.MAX_MAX_BYTES = 8 * 1048576
M.DEFAULT_MAX_ENTRIES = 1000
M.MAX_MAX_ENTRIES = 10000
M.DEFAULT_MAX_MATCHES = 100
M.MAX_MAX_MATCHES = 2000
M.DEFAULT_MAX_DEPTH = 0
M.MAX_MAX_DEPTH = 8


-- @brief 原始字节 → base64 ASCII (供 JSON/WS 文本帧)
function M.b64_encode(s)
	if type(s) ~= "string" then
		s = ""
	end

	return kcrypto.base64_encode(s)
end


-- @brief base64 ASCII → 原始字节; 非法输入返回 nil
function M.b64_decode(s)
	if type(s) ~= "string" then
		return nil
	end

	-- 兼容 WS/JSON 面可能夹带空白
	s = string.gsub(s, "[^A-Za-z0-9+/=]", "")
	if s ~= "" and (#s % 4) ~= 0 then
		return nil
	end

	local out = kcrypto.base64_decode(s)
	if out == nil then
		return nil
	end

	return out
end


function M.path_ok(path)
	if type(path) ~= "string" or path == "" then
		return false
	end

	if string.find(path, "[\r\n\0]") ~= nil then
		return false
	end

	return true
end


function M.clamp_max_bytes(v)
	local n = tonumber(v)
	if n == nil or n <= 0 then
		return M.DEFAULT_MAX_BYTES
	end

	n = math.floor(n)
	if n > M.MAX_MAX_BYTES then
		return M.MAX_MAX_BYTES
	end

	return n
end


function M.clamp_max_entries(v)
	local n = tonumber(v)
	if n == nil or n <= 0 then
		return M.DEFAULT_MAX_ENTRIES
	end

	n = math.floor(n)
	if n > M.MAX_MAX_ENTRIES then
		return M.MAX_MAX_ENTRIES
	end

	return n
end


function M.clamp_max_matches(v)
	local n = tonumber(v)
	if n == nil or n <= 0 then
		return M.DEFAULT_MAX_MATCHES
	end

	n = math.floor(n)
	if n > M.MAX_MAX_MATCHES then
		return M.MAX_MAX_MATCHES
	end

	return n
end


function M.clamp_max_depth(v)
	local n = tonumber(v)
	if n == nil or n < 0 then
		return M.DEFAULT_MAX_DEPTH
	end

	n = math.floor(n)
	if n > M.MAX_MAX_DEPTH then
		return M.MAX_MAX_DEPTH
	end

	return n
end


function M.join(a, b)
	if a == "/" then
		return "/" .. b
	end

	if string.sub(a, -1) == "/" then
		return a .. b
	end

	return a .. "/" .. b
end


function M.reg(catalog, dispatch, desc, fn)
	local ok, err = catalog.register(desc)
	if not ok then
		return false, err
	end

	ok, err = dispatch.register(desc.name, fn)
	if not ok then
		return false, err
	end

	return true
end


return M

-- end
