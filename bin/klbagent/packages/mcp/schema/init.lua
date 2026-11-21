--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbagent args_schema → MCP inputSchema (JSON Schema)
-- @stability stable
-- @impl evolving
-- @attention
--   \n @stability stable: AI 禁止擅自添加/删除/变更本文件对外接口 (含导出名, 参数, 返回与已文档含义); 须用户点名
--   \n @impl evolving: 内部可重构, 不得破坏本文件导出签名与可观察语义
-- @note   args_schema 值为说明串; 前缀作类型提示, 含 optional 为可选
--   \n require("klbagent.packages.mcp.schema") → 本目录 init.lua
-- @history 修改历史
--  \n [2026-10] 创建文件 (P1-1)
--  \n [2026-11] 迁入 schema/
--]]


local M = {}


-- @brief 解析提示串: JSON 类型 + 是否可选
-- @param [in] hint[string|any]
-- @return typ[string]					JSON Schema type; any=不写
-- @return optional[boolean]
local function _hint(hint)
	if type(hint) ~= "string" then
		return "any", true
	end

	local optional = string.find(hint, "optional", 1, true) ~= nil
	local typ = "any"
	if string.find(hint, "^string") ~= nil then
		typ = "string"
	elseif string.find(hint, "^number") ~= nil then
		typ = "number"
	elseif string.find(hint, "^boolean") ~= nil then
		typ = "boolean"
	elseif string.find(hint, "^array") ~= nil then
		typ = "array"
	elseif string.find(hint, "^table") ~= nil then
		typ = "object"
	end

	return typ, optional
end


-- @brief args_schema → JSON Schema object
-- @param [in] args_schema[table|nil]
-- @return schema[table]
M.from_args_schema = function (args_schema)
	local properties = {}
	local required = {}

	if type(args_schema) == "table" then
		for key, hint in pairs(args_schema) do
			if type(key) == "string" then
				local typ, optional = _hint(hint)
				local prop = {}
				if typ ~= "any" then
					prop.type = typ
				end

				if type(hint) == "string" and hint ~= "" then
					prop.description = hint
				end

				properties[key] = prop
				if not optional then
					required[#required + 1] = key
				end
			end
		end
	end

	table.sort(required)

	local out = {
		["$schema"] = "https://json-schema.org/draft/2020-12/schema",
		type = "object",
		properties = properties,
	}
	if #required > 0 then
		out.required = required
	end

	return out
end


return M

-- end
