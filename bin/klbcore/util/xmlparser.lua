--[[
-- Copyright (c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file  xmlparser.lua
-- @brief xml parser
--   受限转换
--   转换规则参考: http://web.chacuo.net/charsetjson2xml
--   规则:
--     1. 添加根目录 "root"
--     2. 数组以 "item" 为关键字
--     3. 仅支持类似json结构
--]]
xml = require("klbcore.util.LuaXml")
local tableex = require("klbcore.util.tableex")


local xmlparser = {}


local KEY_root = 'root'
local KEY_array = 'item'
local KEY_type = 'type'


-- @brief 设置信息
--  是否添加 'type'
xmlparser.cfg = {
	['boolean'] = true,
	['number'] = true,
	['string'] = false,
	
	['array'] = false,
	['object'] = false
}


function table_to_xml(node, t)
	if 'table' ~= type(t) then
		return
	end

	local function append_leaf_node(node, k, v)
		local leaf_node = node:append(k)
		
		local type_v = type(v)
		if xmlparser.cfg['boolean'] and 'boolean' == type_v then
			leaf_node[KEY_type] = 'boolean'
		elseif xmlparser.cfg['number'] and 'number' == type_v then
			leaf_node[KEY_type] = 'number'
		elseif xmlparser.cfg['string'] then
			leaf_node[KEY_type] = 'string'
		end
		
		leaf_node[1] = tostring(v)
	end

	if tableex.is_array(t) then
		-- 数组
		if xmlparser.cfg['array'] then
			node[KEY_type] = 'array'	-- 类型
		end	
		
		for _, v in ipairs(t) do
			if 'table' == type(v) then
				table_to_xml(node:append(KEY_array), v)	-- 分支节点
			else
				append_leaf_node(node, KEY_array, v) 				-- 叶子节点
			end
		end
	else
		-- 非数组: KV结构
		if xmlparser.cfg['object'] then
			node[KEY_type] = 'object'	-- 类型
		end		
		
		for k, v in pairs(t) do
			if 'string' == type(k) then
				if 'table' == type(v) then
					table_to_xml(node:append(k), v)		-- 分支节点
				else
					append_leaf_node(node, k, v)					-- 叶子节点
				end
			end
		end
	end
end


-- @brief 将table转换为xml
-- @param [in]      t[table]			标准table
-- @param [in]      root[nil,string]	[可选]根节点名称; 默认 'root'
-- @return [table] xml对象
function xmlparser.to_xml(t, root)	
	local r = ('string' == type(root) and root) or KEY_root
	r = ('' == r and KEY_root) or r
	
	local x = xml.new(r)
	
	table_to_xml(x, t)
	
	return x
end


-- @brief 将table转换为xml字符串
-- @param [in]      t[table]			标准table
-- @param [in]      root[nil,string]	[可选]根节点名称; 默认 'root'
-- @return [string] xml字符串
function xmlparser.to_xml_str(t, root)
	local x = xmlparser.to_xml(t, root)

	return x:str()
end


function xml_to_table(t, node, is_array)
	if 'table' ~= type(node) then
		return ''
	end
	
	local function type_node(v)
		-- leaf
		local len = #v
		if len < 1 then
			return 'leaf'
		elseif len == 1 then
			if 'table' ~= type(v[1]) then
				return 'leaf'
			end
		end
		
		for _, v in ipairs(v) do
			if 'item' ~= v[0] then
				return 'object'
			end
		end
		
		return 'array'
	end
	
	local k = node:tag()

	local tn = type_node(node)
	
	if 'leaf' == tn then
		-- 叶子节点
		local node_type = ('string' == type(node['type']) and node['type']) or '' -- xml提供的节点类型
		if 1 == #node then
			local v = ''
			if 'boolean' == node_type then
				if 'true' == node[1] then
					v = true
				else
					v = false
				end
			elseif 'number' == node_type then
				v = tonumber(node[1])
			else
				v = tostring(node[1])
			end
			
			if is_array then
				table.insert(t, v)
			else
				t[k] = v
			end
		else			
			if is_array then
				table.insert(t, {})
			else
				t[k] = {}
			end
		end
	elseif 'array' == tn then
		-- 数组
		for _, v in ipairs(node) do			
			if 'table' == type(v) then
				if 'leaf' == type_node(v) then
					xml_to_table(t, v, true)
				else
					local t1 = {}
					local k1 = xml_to_table(t1, v)
					table.insert(t, t1)
				end
			end
		end
	else
		-- object {...}
		for _, v in ipairs(node) do
			if 'table' == type(v) then
				if 'leaf' == type_node(v) then
					xml_to_table(t, v)
				else
					local t1 = {}
					local k1 = xml_to_table(t1, v)
					
					t[k1] = t1
				end
			end
		end
	end
	
	return k
end


-- @brief 将xml转换为table
-- @param [in]      x[table,string]		xml对象或xml字符串
-- @return [table] {} 标准table
function xmlparser.to_table(x)	
	local t = {}	
	
	if 'table' == type(x) then
		xml_to_table(t, x)
	elseif 'string' == type(x) then
		local new_x = xml.eval(x)
		if 'table' == type(new_x) then
			xml_to_table(t, new_x)
		end	
	end
	
	return t
end


return xmlparser

--[[

json 与 xml 互转参考: http://web.chacuo.net/charsetjson2xml

转换规则 eg.
{
    funcname = 'get_video',
    result = {
        code = 0,
        fmt = 'h264',
        rc_mode = 'vbr',
        wh = '1920x1080',
        frame_rate = 25,
        bitrate = 4096,
		
        range = {
            fmt = {'h264','h265'},
            rc_mode = {'vbr','cbr'},
            wh = {'1920x1080','960x540','758x432'},
            frame_rate = {min=1,max=25},
            bitrate = {min=128,max=65536},
        }
    }
}

转换为:

<root>
    <funcname>get_video</funcname>
    <result>
        <code>0</code>
        <fmt>h264</fmt>
        <rc_mode>vbr</rc_mode>
        <wh>1920x1080</wh>
        <frame_rate>25</frame_rate>
        <bitrate>4096</bitrate>
        <range>
			
			<fmt>
				<item>h264</item>
				<item>h265</item>
			</fmt>
			
			<rc_mode>
				<item>vbr</item>
				<item>cbr</item>
			</rc_mode>
			
			<wh>
				<item>1920x1080</item>
				<item>960x540</item>
				<item>960x540</item>
			</wh>
			
            <frame_rate>
                <min>1</min>
                <max>25</max>
            </frame_rate>
			
            <bitrate>
                <min>128</min>
                <max>65536</max>
            </bitrate>
        </range>
    </result>
</root>

--]]
