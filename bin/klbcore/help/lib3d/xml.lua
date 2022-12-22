--[[
-- Copyright (c) 2022, GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file  xml.lua
-- @brief xml
--   \n require("klbcore.util.LuaXml")
--   \n C导出文件: ./klb/src_c/klua/LuaXML_130610/LuaXML_lib.c
--   \n Lua封装: ./klbcore/util/LuaXml.lua
--]]

xml = require("klbcore.util.LuaXml")


-------------------------------------------------------------------

local function new_xml()
	local xml = {}

	xml:tag = function (tag)
		
	end

	xml:append = function (tag)
		
	end
	
	xml:str = function (indent, tagValue)
	
	end

	xml:save = function (filename)
	
	end
	
	xml:find = function (tag, attributeKey, attributeValue)
	
	end
	
	return xml
end


-------------------------------------------------------------------
-- ./klb/src_c/klua/LuaXML_130610/LuaXML_lib.c



-- @brief 加载解析xml文件
-- @param [in]		filename[string]	xml文件路径
-- @return [table] xml
xml.load = function (filename)
	return new_xml()
end


-- @brief 解析xml文本
-- @param [in]		xml_text[string]	xml数据
-- @return [table] xml
xml.eval = function (xml_text)
	return new_xml()
end


xml.encode = function ()
	return ''
end


xml.registerCode = function ()
	
end


-------------------------------------------------------------------
-- ./klbcore/util/LuaXml.lua


-- @brief 新的一个xml对象
-- @param [in]		root[string]	根节点
-- @return [table] xml
xml.new = function (root)
	return new_xml()
end
