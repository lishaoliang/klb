--[[
-- Copyright (c) 2022, GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file  selector.lua
-- @brief 选择器
-- @note 参考 jQuery-选择器
---   https://www.w3school.com.cn/jquery/jquery_ref_selectors.asp
--]]
local kgui = require("kgui")

local selector = {}
local E = {}


local function find_path_by(wnd, id)
	if 'table' ~= type(wnd) then
		return '', wnd
	end
	
	if id == wnd['id'] then
		return wnd['path'], wnd
	end	
	
	local child = wnd['child'] or {}

	-- 子窗口: 第1种表达方式
	for _, v in ipairs(wnd) do
		local path, w = find_path_by(v, id)
		if '' ~= path then
			return path, w
		end
	end

	local child = wnd['child'] or {}
	
	-- 子窗口: 第2种表达方式
	for _, v in ipairs(child) do
		local path, w = find_path_by(v, id)
		if '' ~= path then
			return path, w
		end
	end
	
	return '', w
end


selector.select = function (dlg)
	
	local f = function (s)
		local path, wnd = find_path_by(dlg, s)
		
		local t = {
			['_dlg'] = dlg,
			['_path'] = path,
			['_wnd'] = wnd
		}
		
		t.get = function (...)
			return kgui.get(t['_path'], ...)
		end
		
		t.set = function (...)
			return kgui.set(t['_path'], ...)
		end
			
		return setmetatable(t, {
			__index = function (t, k)
				return function (...)
					local args = {...}
					
					if 'function' == type(args[1]) then
						local func = args[1]
						if 'table' ~= wnd['_commonds'] then
							wnd['_commonds'] = {
								[k] = func
							}
						else
							wnd[_commonds][k] = func
						end
					else
						if 1 <= #args then
							kgui.set(t['_path'], k, ...)
							return t
						else
							return kgui.get(t['_path'], k, ...)
						end
					end
				end
			end
		})
	end
	
	return f
end

return selector
