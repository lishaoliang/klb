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
-- ./klb/src_c/klua/LuaXML_130610/LuaXML_lib.c



-- @brief 加载解析xml文件
-- @param [in]		filename[string]	xml文件路径
-- @return [table] xml
xml.load = function (filename)
	return xml
end


-- @brief 解析xml文本
-- @param [in]		xml_text[string]	xml数据
-- @return [table] xml
xml.eval = function (xml_text)
	return xml
end


xml.encode = function ()
	return ''
end


xml.registerCode = function ()
	
end


-------------------------------------------------------------------
-- ./klbcore/util/LuaXml.lua


-- @brief 新的一个xml
-- @param [in]		arg[string]	根节点
-- @return [table] xml
xml.new = function (arg)
	return xml 
end


xml.tag = function (var, tag)

end


xml.append = function (var, tag)
	
end


xml.str = function (var, indent, tagValue)
	
end


-- @brief 保存xml文件
-- @param [in]		var[table]			xml
-- @param [in]		filename[string]	文件路径
-- @return [nil, table] [boolean] 几个返回值集合
-- @note 注意事项,参见xxx
-- @see [string][table] = x_xx(string, string, table)
xml.save = function (var, filename)
	
end


xml.find = function (var, tag, attributeKey, attributeValue)
	
end
