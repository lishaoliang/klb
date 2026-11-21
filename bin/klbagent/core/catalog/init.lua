--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  Agent 工具清单 (含基础功能分类与 AI 契约字段)
-- @note   category 与 profiles 正交; 未知分类拒绝注册
-- @history 修改历史
--  \n [2026-09] 创建文件; 迁入子目录 catalog/init.lua
--]]


local M = {}


-- 基础功能分类表 (固定)
local CATEGORY_LIST = {
	{ id = "runtime", title = "runtime", min_profile = "readonly" },
	{ id = "catalog", title = "catalog", min_profile = "readonly" },
	{ id = "config", title = "config", min_profile = "readonly" },
	{ id = "fs", title = "fs", min_profile = "readonly" },
	{ id = "net", title = "net", min_profile = "net" },
	{ id = "device", title = "device", min_profile = "device" },
	{ id = "unsafe", title = "unsafe", min_profile = "unsafe" },
	{ id = "log", title = "log", min_profile = "readonly" },
	{ id = "g2de", title = "g2de engine embed", min_profile = "readonly" },
}


local CATEGORY_MAP = {}
for i = 1, #CATEGORY_LIST do
	local c = CATEGORY_LIST[i]
	CATEGORY_MAP[c.id] = c
end


-- name -> desc
local tools_by_name = {}
-- 注册顺序
local tools_order = {}


local function _copy_category(c)
	return {
		id = c.id,
		title = c.title,
		min_profile = c.min_profile,
	}
end


local function _copy_table(t)
	if type(t) ~= "table" then
		return {}
	end

	local out = {}
	for k, v in pairs(t) do
		if type(v) == "table" then
			out[k] = _copy_table(v)
		else
			out[k] = v
		end
	end

	return out
end


local function _copy_desc(d)
	return {
		name = d.name,
		category = d.category,
		title = d.title,
		summary = d.summary,
		min_profile = d.min_profile,
		side_effects = _copy_table(d.side_effects),
		args_schema = _copy_table(d.args_schema),
		examples = _copy_table(d.examples),
		docs = d.docs,
	}
end


-- @brief 是否合法分类 id
-- @param [in] id[string]
-- @return ok[boolean]
M.is_category = function (id)
	return type(id) == "string" and CATEGORY_MAP[id] ~= nil
end


-- @brief 基础分类表副本
-- @return list[table]					数组; 项含 id/title/min_profile
M.categories = function ()
	local out = {}
	for i = 1, #CATEGORY_LIST do
		out[i] = _copy_category(CATEGORY_LIST[i])
	end

	return out
end


-- @brief 注册工具描述
-- @param [in] desc[table]				name/category 必填
-- @return ok[boolean]
-- @return err[string|nil]
M.register = function (desc)
	if type(desc) ~= "table" then
		return false, "desc not table"
	end

	local name = desc.name
	if type(name) ~= "string" or name == "" then
		return false, "bad name"
	end

	local category = desc.category
	if not M.is_category(category) then
		return false, "bad category"
	end

	local cat = CATEGORY_MAP[category]
	local title = desc.title
	if type(title) ~= "string" then
		title = ""
	end

	local summary = desc.summary
	if type(summary) ~= "string" then
		summary = ""
	end

	local min_profile = desc.min_profile
	if type(min_profile) ~= "string" or min_profile == "" then
		min_profile = cat.min_profile
	end

	local side_effects = desc.side_effects
	if type(side_effects) ~= "table" then
		side_effects = {}
	end

	local args_schema = desc.args_schema
	if type(args_schema) ~= "table" then
		args_schema = {}
	end

	local examples = desc.examples
	if type(examples) ~= "table" then
		examples = {}
	end

	local docs = desc.docs
	if type(docs) ~= "string" then
		docs = ""
	end

	-- 可选: MCP outputSchema 源 (packages/mcp P4-4)
	local result_schema = desc.result_schema
	if type(result_schema) ~= "table" then
		result_schema = nil
	end

	local entry = {
		name = name,
		category = category,
		title = title,
		summary = summary,
		min_profile = min_profile,
		side_effects = side_effects,
		args_schema = args_schema,
		examples = examples,
		docs = docs,
		result_schema = result_schema,
	}

	if tools_by_name[name] == nil then
		tools_order[#tools_order + 1] = name
	end

	tools_by_name[name] = entry
	return true
end


-- @brief 列出工具
-- @param [in] opts[table|nil]			[可选] opts.category 过滤
-- @return tools[table]					描述数组; 无匹配为 {}
M.list = function (opts)
	local filter = nil
	if type(opts) == "table" and type(opts.category) == "string" and opts.category ~= "" then
		filter = opts.category
		if not M.is_category(filter) then
			return {}
		end
	end

	local out = {}
	for i = 1, #tools_order do
		local name = tools_order[i]
		local d = tools_by_name[name]
		if d ~= nil then
			if filter == nil or d.category == filter then
				out[#out + 1] = _copy_desc(d)
			end
		end
	end

	return out
end


-- @brief 按名查工具描述
-- @param [in] name[string]
-- @return desc[table|nil]
M.get = function (name)
	if type(name) ~= "string" or name == "" then
		return nil
	end

	local d = tools_by_name[name]
	if d == nil then
		return nil
	end

	return _copy_desc(d)
end


-- @brief 清空登记 (测试/重载用)
M.clear = function ()
	tools_by_name = {}
	tools_order = {}
end


return M
