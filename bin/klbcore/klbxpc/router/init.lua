--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @brief  klbxpc 方法表
-- @note   call=REQUEST; post=POST; notify=NOTIFY
-- @history 修改历史
--		[2026-11] 创建文件
--]]

local R = {}

local KINDS = {
	call = true,
	post = true,
	notify = true,
}

function R:add(kind, name, fn)
	if not KINDS[kind] then
		return false
	end

	if "string" ~= type(name) or "" == name then
		return false
	end

	if "function" ~= type(fn) then
		return false
	end

	self._map[kind][name] = fn
	return true
end

function R:find(kind, name)
	local bucket = self._map[kind]
	if not bucket then
		return nil
	end

	return bucket[name]
end

local router = {}

router.new = function ()
	local obj = {
		_map = {
			call = {},
			post = {},
			notify = {},
		},
	}

	setmetatable(obj, { __index = R })
	return obj
end

return router
