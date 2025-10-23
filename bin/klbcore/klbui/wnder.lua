--[[
-- Copyright (c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   wnder.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  窗口
--]]

local wnder = {}
local E = {}

local window = {}


-- 设置/获取 样式
function window:style(n)
	if self._kwnd then
		return self._kwnd:style(n)
	end
	
	return 0
end


-- 设置/获取 显示状态
function window:show(is_show)
	if self._kwnd then
		return self._kwnd:show(is_show)
	end
	
	return false
end


-- 设置/获取 隐藏状态
function window:hide(is_hide)
	if self._kwnd then
		return self._kwnd:hide(is_hide)
	end
end


-- 设置/获取 静态TIP
function window:tip(s)
	if self._kwnd then
		return self._kwnd:tip(s)
	end
	
	return ''
end

-- 设置/获取 动态TIP
function window:tip_dynamic(s)
	if self._kwnd then
		return self._kwnd:tip_dynamic(s)
	end
	
	return ''
end

-- 更新TIP
function window:tip_update()
	if self._kwnd then
		self._kwnd:tip_update()
	end
end


-- 基于父窗口移动到指定的相对坐标
function window:move(x, y)
	if self._kwnd then
		self._kwnd:move(x, y)
	end
end

-- 重新设置大小
function window:resize(w, h)
	if self._kwnd then
		self._kwnd:resize(w, h)
	end
end

-- 刷新
function window:refresh()
	if self._kwnd then
		self._kwnd:refresh()
	end
end

-- 设置参数
function window:set(...)
	if self._kwnd then
		return self._kwnd:set(...)
	end
	
	return 0
end

-- 获取参数
function window:get(...)
	if self._kwnd then
		return self._kwnd:get(...)
	end
	
	return {}
end

-- 获取GUI的当前 系统滴答数(单位毫秒ms)
function window:tick_count()
	if self._kwnd then
		return self._kwnd:tick_count()
	end
	
	return 0
end


-- 新建 封装 操作窗口 的接口
wnder.new = function (kwnd)
	local obj = {
		_kwnd = kwnd
	}

	setmetatable(obj, {
		__index = window,
		__tostring = function(self)
			return self._kwnd
		end
	})
	
	return obj
end

return wnder
