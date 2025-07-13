--[[
-- Copyright (c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   wnder.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  窗口
--]]

local wnder = {}
local E = {}

local wnd = {}


-- 设置/获取 静态TIP
function wnd:tip(s)
	if self._kwnd then
		return self._kwnd:tip(s)
	end
	
	return ''
end

-- 设置/获取 动态TIP
function wnd:tip_dynamic(s)
	if self._kwnd then
		return self._kwnd:tip_dynamic(s)
	end
	
	return ''
end

-- 更新TIP
function wnd:tip_update()
	if self._kwnd then
		self._kwnd:tip_update()
	end
end


-- 新建 封装 操作窗口 的接口
wnder.new = function (kwnd)
	local obj = {
		_kwnd = kwnd
	}

	setmetatable(obj, {
		__index = wnd,
		__tostring = function(self)
			return self._kwnd
        end
	})
	
	return obj
end

return wnder
