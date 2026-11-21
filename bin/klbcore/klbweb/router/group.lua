--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   group.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb 路由组
-- @note   带前缀的登记器. 写入父站点同一栈与路由表. 不 listen, 未命中不产生 404
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local compile = require("klbcore.klbweb.router.compile")


--------------------------------------------------------------------------------------------
-- 内部

local function normalize_prefix(prefix)
	if "string" ~= type(prefix) or "" == prefix then
		return nil
	end

	prefix = compile.normalize_path(prefix)
	if compile.is_pattern(prefix) then
		return nil
	end

	return prefix
end


-- @brief 把组内相对 path 拼到前缀上
-- @return path[string]		失败为 nil
local function join_path(prefix, path)
	if "string" ~= type(path) or "" == path then
		return nil
	end

	path = string.gsub(path, "\\", "/")
	local rel = string.gsub(path, "^/+", "")
	if "" == rel then
		return prefix
	end

	if "/" == prefix then
		return compile.normalize_path("/" .. rel)
	end

	return compile.normalize_path(prefix .. "/" .. rel)
end


-- @brief 拼接静态前缀 (组前缀 / 中间件前缀 / 静态挂载 / 嵌套组)
local function join_static(prefix, path)
	local full = join_path(prefix, path)
	if not full then
		return nil
	end

	if compile.is_pattern(full) then
		return nil
	end

	return full
end


--------------------------------------------------------------------------------------------
-- 组对象

local G = {}

local function new_group(app, prefix)
	local obj = {
		_app = app,
		_prefix = prefix,
	}

	return setmetatable(obj, { __index = G })
end


-- @brief 注册组内中间件
-- @param [in]      path_or_fn[string|function]		相对前缀, 或处理函数
-- @param [in]      fn[function]					[可选] 前缀形式时的处理函数
-- @return ok[boolean]								true 成功; 参数非法为 false
-- @note `fn(req, res, next)`. 前缀须为静态路径
function G:use(path_or_fn, fn)
	if "function" == type(path_or_fn) then
		return self._app:use(self._prefix, path_or_fn)
	end

	if "string" == type(path_or_fn) and "function" == type(fn) then
		local full = join_static(self._prefix, path_or_fn)
		if not full then
			return false
		end

		return self._app:use(full, fn)
	end

	return false
end


-- @brief 注册组内路由; 可 `route(method, path, mw, handler)`
-- @param [in]      method[string]		HTTP 方法
-- @param [in]      path[string]		相对组前缀的 URL 路径
-- @param [in]      ...[function]		路由级 mw..., 末项 handler
-- @return ok[boolean]					true 成功; 参数非法为 false
-- @note path 可含 `:name` / 末尾 `*`. 对外路径为前缀加 path
function G:route(method, path, ...)
	local full = join_path(self._prefix, path)
	if not full then
		return false
	end

	return self._app:route(method, full, ...)
end


-- @brief 注册组内 GET
function G:get(path, ...)
	return self:route("GET", path, ...)
end


-- @brief 注册组内 POST
function G:post(path, ...)
	return self:route("POST", path, ...)
end


-- @brief 注册组内 PUT
function G:put(path, ...)
	return self:route("PUT", path, ...)
end


-- @brief 注册组内 DELETE
function G:delete(path, ...)
	return self:route("DELETE", path, ...)
end


-- @brief 注册组内 PATCH
function G:patch(path, ...)
	return self:route("PATCH", path, ...)
end


-- @brief 注册组内 HEAD
function G:head(path, ...)
	return self:route("HEAD", path, ...)
end


-- @brief 注册组内 OPTIONS
function G:options(path, ...)
	return self:route("OPTIONS", path, ...)
end


-- @brief 注册组内 WebSocket
-- @param [in]      path[string]		相对组前缀的 URL 路径
-- @param [in]      ...[function|table]	路由级 mw..., 可选 `{ protocol = "..." }`, 末项 handler
-- @return ok[boolean]					true 成功; 参数非法为 false
function G:ws(path, ...)
	local full = join_path(self._prefix, path)
	if not full then
		return false
	end

	return self._app:ws(full, ...)
end


-- @brief 嵌套路由组
-- @param [in]      prefix[string]		相对本组件的静态前缀
-- @return group[table]					成功为组对象; 前缀非法为 false
function G:group(prefix)
	local full = join_static(self._prefix, prefix)
	if not full then
		return false
	end

	return new_group(self._app, full)
end


-- @brief 在组前缀下挂静态目录
-- @param [in]      prefix[string]		相对本组件的 URL 前缀
-- @param [in]      dir[string]			本地目录
-- @param [in]      opts[table]			[可选] 同站点 :static
-- @return ok[boolean]					true 成功; 参数非法为 false
function G:static(prefix, dir, opts)
	local full = join_static(self._prefix, prefix)
	if not full then
		return false
	end

	return self._app:static(full, dir, opts)
end


--------------------------------------------------------------------------------------------
-- 工厂

local group = {}

-- @brief 新建路由组
-- @param [in]      app[table]			父站点
-- @param [in]      prefix[string]		URL 前缀; 静态路径
-- @return group[table]					成功为组对象; 前缀非法为 false
group.new = function (app, prefix)
	if "table" ~= type(app) then
		return false
	end

	local full = normalize_prefix(prefix)
	if not full then
		return false
	end

	return new_group(app, full)
end


return group
