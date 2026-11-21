--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klblpc 方法表
-- @note   call 对应 REQUEST; post 对应 POST; notify 与 post 同路径(无回应)
-- @history 修改历史
--		[2026-09] 创建文件
--]]


--------------------------------------------------------------------------------------------
-- 路由对象

local R = {}


local KINDS = {
	call = true,
	post = true,
	notify = true,
}


-- @brief 注册或覆盖一个方法
-- @param [in]      kind[string]		`call` / `post` / `notify`
-- @param [in]      name[string]		方法名, 即载荷首个参数
-- @param [in]      fn[function]		处理函数 `fn(req)`
-- @return ok[boolean]					true 成功; 参数非法为 false
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


-- @brief 查找方法
-- @param [in]      kind[string]		`call` / `post` / `notify`
-- @param [in]      name[string]		方法名
-- @return [function]					已注册函数; 没有时为 nil
function R:find(kind, name)
	local bucket = self._map[kind]
	if not bucket then
		return nil
	end

	return bucket[name]
end


--------------------------------------------------------------------------------------------
-- 工厂

local router = {}


-- @brief 新建一张空方法表
-- @return [table]						路由对象
router.new = function ()
	local obj = {
		_map = {
			call = {},
			post = {},
			notify = {},
		},
	}

	setmetatable(obj, {
		__index = R,
	})

	return obj
end


return router
