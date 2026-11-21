--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  catalog.* 元工具
-- @note   catalog.tools 列出已注册工具 (可按 category 过滤)
-- @history 修改历史
--  \n [2026-09] 创建文件
--]]


local M = {}


-- @brief 注册 catalog 元工具
-- @return ok[boolean]
-- @return err[string|nil]
M.register = function (catalog, dispatch)
	local function _tools(args)
		local opts = {}
		if type(args) == "table" and type(args.category) == "string" and args.category ~= "" then
			opts.category = args.category
		end

		return {
			tools = catalog.list(opts),
			categories = catalog.categories(),
		}
	end

	local ok, err = catalog.register({
		name = "catalog.tools",
		category = "catalog",
		title = "list registered tools",
		summary = "List tool contracts (summary/args_schema/docs). Optional category filter.",
		args_schema = {
			category = "string optional category id",
		},
		examples = {
			{ args = {} },
			{ args = { category = "runtime" } },
		},
		docs = "categories.md",
		side_effects = {},
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("catalog.tools", _tools)
	if not ok then
		return false, err
	end

	return true
end


return M
