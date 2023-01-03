--[[
-- Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   wsdl.lua
-- @brief  C wsdl, windows SDL, wlua专用, 支持win平台相关的解码显示等
--   \n require("wsdl")
--   \n C导出文件: ./klb/wlua/wsdl/wsdl.cpp
-- @version 0.1
--]]

local wsdl = {}


-- @brief 获取SDL版本
-- @return [string] SDL版本字符串
wsdl.get_version = function ()
	return 'V2.0.1'
end


-- @brief 获取SDL版本
-- @return [string] SDL的git版本字符串
wsdl.get_revision = function ()
	return 'xxxxxx'
end


-- @brief 获取可执行文件的路径
-- @return [string] 可执行文件路径
wsdl.get_base_path = function ()
	return '/home/aaa/bbb'
end


-- @brief 获取应用程序配置路径
-- @param [in]      org[string]		组织
-- @param [in]      app[string]		程序
-- @return [string] 应用程序配置路径
wsdl.get_pref_path = function (org, app)
	return 'C:\Users\Administrator\AppData\Roaming\klb\test'
end


-- @brief 打开窗口
-- @param [in]      font_path[string]	字库路径: eg. './res/font/simsun.ttc'
-- @param [in]      w[number(int)]		窗口宽
-- @param [in]      h[number(int)]		窗口高
-- @param [in]      title[string]		窗口标题
-- @return [string] 应用程序配置路径
wsdl.open_wnd = function (font_path, w, h, title)
	return 
end


-- @brief 关闭窗口
wsdl.close_wnd = function ()
	return
end


-- @brief 送音视频解码显示
-- @param [in]      chnn[number(int)]		通道号
-- @param [in]      sidx[number(int)]		流序号
-- @param [in]      media[lightuserdata]	C指针: klb_buf_t*
-- @return 无
wsdl.push_media = function (chnn, sidx, media)
	return
end


-- @brief 设置视频显示位置
-- @param [in]      idx[number(int)]		视频序号
-- @param [in]      x[number(int)]			x位置
-- @param [in]      y[number(int)]			y位置
-- @param [in]      w[number(int)]			宽
-- @param [in]      h[number(int)]			高
-- @return 无
wsdl.set_video_pos = function (idx, x, y, w, h)
	return
end


return wsdl
