--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb 路由表
-- @note   METHOD + path; 精确优先, 再 `:name` / 末尾 `*`; 可挂路由级 mw
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local compile = require("klbcore.klbweb.router.compile")


--------------------------------------------------------------------------------------------
-- 路由对象

local R = {}

-- @brief 添加一条路由
-- @param [in]      method[string]		HTTP 方法; eg. `POST`
-- @param [in]      path[string]		URL 路径; eg. `/api/v1/:id` 或 `/files/*`
-- @param [in]      fn[function|table]	处理函数, 或函数数组 (末项为 handler, 前为路由级 mw)
-- @return ok[boolean]					true 成功; 参数非法为 false
function R:add(method, path, fn)
	if "string" ~= type(method) or "string" ~= type(path) then
		return false
	end

	if "" == method or "" == path then
		return false
	end

	local fns = compile.pack_fns(fn)
	if not fns or 0 == #fns then
		return false
	end

	method = string.upper(method)
	path = compile.normalize_path(path)

	if compile.is_pattern(path) then
		local pat, names = compile.compile_path(path)
		if not pat then
			return false
		end

		for i = 1, #self._pats do
			if self._pats[i].method == method and self._pats[i].path == path then
				self._pats[i].fns = fns
				self._pats[i].pat = pat
				self._pats[i].names = names
				return true
			end
		end

		self._pats[#self._pats + 1] = {
			method = method,
			path = path,
			pat = pat,
			names = names,
			fns = fns,
		}

		return true
	end

	local key = method .. "\t" .. path
	if self._map[key] then
		self._map[key] = fns
		return true
	end

	self._map[key] = fns
	self._list[#self._list + 1] = {
		method = method,
		path = path,
		fns = fns,
	}

	return true
end


-- @brief 匹配路由
-- @param [in]      method[string]		HTTP 方法
-- @param [in]      path[string]		URL 路径 (已规范化)
-- @return fns[table]					函数数组; 未命中为 nil
-- @return params[table]				路径参数; 精确匹配为 `{}`
-- @note 先精确, 再按注册序匹配 `:name` / `*`
function R:match(method, path)
	if "string" ~= type(method) or "string" ~= type(path) then
		return nil, {}
	end

	method = string.upper(method)
	path = compile.normalize_path(path)

	local fns = self._map[method .. "\t" .. path]
	if fns then
		return fns, {}
	end

	for i = 1, #self._pats do
		local e = self._pats[i]
		if e.method == method then
			local caps = { string.match(path, e.pat) }
			if 0 < #caps then
				local params = {}
				for j = 1, #e.names do
					params[e.names[j]] = caps[j] or ""
				end

				return e.fns, params
			end
		end
	end

	return nil, {}
end


-- @brief 收集某 path 已注册的 METHOD (含 pattern 命中)
-- @param [in]      path[string]		URL 路径
-- @return methods[table]				大写方法数组; 无为 `{}`
function R:methods_of(path)
	local set = {}
	if "string" ~= type(path) then
		return {}
	end

	path = compile.normalize_path(path)

	for i = 1, #self._list do
		if self._list[i].path == path then
			set[self._list[i].method] = true
		end
	end

	for i = 1, #self._pats do
		local e = self._pats[i]
		local caps = { string.match(path, e.pat) }
		if 0 < #caps then
			set[e.method] = true
		end
	end

	if set["GET"] then
		set["HEAD"] = true
	end

	local methods = {}
	for m, _ in pairs(set) do
		methods[#methods + 1] = m
	end

	table.sort(methods)
	return methods
end


--------------------------------------------------------------------------------------------
-- 工厂

local router = {}

-- @brief 新建路由表
-- @return [table]	路由对象
router.new = function ()
	local obj = {
		_list = {},
		_map = {},
		_pats = {},
	}

	return setmetatable(obj, { __index = R })
end


return router
