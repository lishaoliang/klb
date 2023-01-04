--[[
-- Copyright (c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file  selector.lua
-- @brief 选择器
-- @note 参考 jQuery-选择器
---   https://www.w3school.com.cn/jquery/jquery_ref_selectors.asp
--]]
local table = require("table")
local kgui = require("kgui")


local selector = {}
local E = {}


local function FindWnds(t, wnd, cfg, sel_multi)
	if 'table' ~= type(wnd) then
		return
	end

	-- 按设置, 如果匹配则 按数组方式 放入 t
	if cfg['all'] then
		table.insert(t, wnd)
	elseif cfg['name'] and wnd['name'] == cfg['name'] then
		table.insert(t, wnd)
		if not sel_multi then return end
	elseif cfg['id'] and wnd['id'] == cfg['id'] then
		table.insert(t, wnd)
		if not sel_multi then return end
	elseif cfg['class'] and wnd['class'] == cfg['class'] then
		table.insert(t, wnd)
		if not sel_multi then return end
	elseif cfg['type'] and wnd['type'] == cfg['type'] then
		table.insert(t, wnd)
		if not sel_multi then return end
	end
	
	local child = wnd['child'] or {}

	-- 子窗口: 第1种表达方式
	for _, v in ipairs(wnd) do
		FindWnds(t, v, cfg, sel_multi)
		
		if not sel_multi and 1 <= #t then
			return
		end
	end

	local child = wnd['child'] or {}
	
	-- 子窗口: 第2种表达方式
	for _, v in ipairs(child) do
		FindWnds(t, v, cfg, sel_multi)
		
		if not sel_multi and 1 <= #t then
			return
		end
	end
	
	return 
end


selector.select = function (dlg, multi)	
	return function (s)
		-- 参考 https://www.w3school.com.cn/jquery/jquery_ref_selectors.asp	
		
		-- 规则: ''			按 'name' 选择所有, eg. 'name1'
		-- 规则: '#id'		按 'id' 选择所有, eg. '#aa'
		-- 规则: '.class'	按 'class' 选择所有, eg. '.abc'
		-- 规则: ':type'	按 'type' 选择所有, eg. ':kbutton'
		-- 规则: '*'		选择 'all' 所有控件(含对话框本身)	
		
		-- 单选/多选, 默认单选
		local sel_multi = ('boolean' == type(multi) and multi) or false
		
		-- 按规则 选取 窗口集
		local wnds = {}
		
		while true do	
			-- 按 'name' 选择所有
			local n = string.match(s, '^[^:#%.%*](.*)')
			if nil ~= n then
				FindWnds(wnds, dlg, {['name']=s}, sel_multi)
				break
			end
			
			-- 按 'id' 选择所有
			local id = string.match(s, '^#(.*)')
			if nil ~= id then
				FindWnds(wnds, dlg, {['id']=id}, sel_multi)
				break
			end
			
			-- 按 'class' 选择所有
			local c = string.match(s, '^%.(.*)')
			if nil ~= c then
				FindWnds(wnds, dlg, {['class']=c}, sel_multi)
				break
			end
			
			-- 按 'type' 选择所有
			local ts = string.match(s, '^:(.*)')
			if nil ~= ts then
				FindWnds(wnds, dlg, {['type']=ts}, sel_multi)
				break
			end
			
			-- 选择 'all' 所有控件
			local all = string.match(s, '^%*(.*)')
			if nil ~= all then
				FindWnds(wnds, dlg, {['all']=true}, sel_multi)
				break
			end
			
			break
		end
		
		if not sel_multi then
			assert(#wnds <= 1)
		end
		
		-- wnds 中第一个wnd的路径
		local function wnds_first_path()
			if 1 <= #wnds then
				local tmp_path = wnds[1]['path']
				return ('string' == type(tmp_path) and tmp_path) or ''
			end	
			return ''
		end
		
		local t = {
			['_dlg'] = dlg,
			['_wnds'] = wnds
		}
		
		-- 标准 get / set 控件属性方法
		t.get = function (...)
			return kgui.get(wnds_first_path(), ...)
		end
		
		t.set = function (...)
			for _, v in ipairs(wnds) do
				local path = ('string' == type(v['path']) and v['path']) or ''				
				kgui.set(path, ...)
			end
			
			return t
		end
		
		-- css 样式相关
		-- 参考: https://www.w3school.com.cn/jquery/jquery_ref_css.asp
		t.css = function (...)
			local args = {...}
			if 1 < #args then
				for _, v in ipairs(wnds) do
					local path = ('string' == type(v['path']) and v['path']) or ''				
					kgui.set(path, ...)
				end
			elseif 1 == #args then
				return kgui.get(wnds_first_path(), ...)
			end
			
			return t
		end
		
		t.position = function ()
			
		end
		
		t.height = function ()
			
		end
		
		t.width = function ()
			
		end
		
		-- 直接控制属性等
		return setmetatable(t, {
			__index = function (t, k)
				return function (...)
					local args = {...}
					local function wnd_insert_commond(wnd, func)
						if 'table' ~= type(wnd['_commonds']) then
							wnd['_commonds'] = {
								[k] = func
							}
						else
							wnd['_commonds'][k] = func
						end
					end
					
					if 'function' == type(args[1]) then
						for _, v in ipairs(wnds) do
							wnd_insert_commond(v, args[1])
						end
					else
						if 1 <= #args then
							for _, v in ipairs(wnds) do
								local path = v['path'] or ''				
								kgui.set(path, k, ...)
							end
							
							return t
						else
							return kgui.get(wnds_first_path(), k, ...)
						end
					end
				end
			end		
		})
	end
end


return selector
