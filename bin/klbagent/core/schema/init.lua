--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  Request / tool 参数校验
-- @note   args_schema 值为说明串; 含 optional 则可选; 前缀 string/table/array/number/boolean 作类型提示
-- @history 修改历史
--  \n [2026-09] 创建文件
--]]
local catalog = require("klbagent.core.catalog")


local M = {}


local function _hint_type(hint)
	if type(hint) ~= "string" then
		return "any", true
	end

	local optional = string.find(hint, "optional", 1, true) ~= nil
	local typ = "any"
	if string.find(hint, "^string") ~= nil then
		typ = "string"
	elseif string.find(hint, "^table") ~= nil then
		typ = "table"
	elseif string.find(hint, "^array") ~= nil then
		typ = "array"
	elseif string.find(hint, "^number") ~= nil then
		typ = "number"
	elseif string.find(hint, "^boolean") ~= nil then
		typ = "boolean"
	end

	return typ, optional
end


local function _type_ok(typ, v)
	if typ == "any" then
		return true
	end

	if typ == "string" then
		return type(v) == "string"
	end

	if typ == "number" then
		return type(v) == "number"
	end

	if typ == "boolean" then
		return type(v) == "boolean"
	end

	if typ == "table" or typ == "array" then
		return type(v) == "table"
	end

	return true
end


-- @brief 校验 Request 外形
-- @param [in] req[table]			须含 name[string]; 可选 args[table] / id
-- @return ok[boolean]
-- @return err[string|nil]
M.validate_request = function (req)
	if type(req) ~= "table" then
		return false, "request must be table"
	end

	if type(req.name) ~= "string" or req.name == "" then
		return false, "need name"
	end

	if req.args ~= nil and type(req.args) ~= "table" then
		return false, "args must be table"
	end

	return true
end


-- @brief 按 catalog.args_schema 校验工具参数
-- @param [in] tool_name[string]
-- @param [in] args[table|nil]
-- @return ok[boolean]
-- @return err[string|nil]
M.validate_args = function (tool_name, args)
	if type(tool_name) ~= "string" or tool_name == "" then
		return false, "bad tool name"
	end

	local a = args
	if a == nil then
		a = {}
	end

	if type(a) ~= "table" then
		return false, "args must be table"
	end

	local desc = catalog.get(tool_name)
	if desc == nil then
		return true
	end

	local schema = desc.args_schema
	if type(schema) ~= "table" then
		return true
	end

	for key, hint in pairs(schema) do
		if type(key) == "string" then
			local typ, optional = _hint_type(hint)
			local v = a[key]
			if v == nil then
				if not optional then
					return false, "missing arg: " .. key
				end
			else
				if not _type_ok(typ, v) then
					return false, "bad arg type: " .. key
				end
			end
		end
	end

	return true
end


return M
