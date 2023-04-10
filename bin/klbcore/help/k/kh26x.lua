--[[
-- Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   kh26x.lua
-- @brief  C kh26x, 读取h26x文件: h264/h265
--   \n require("kh26x")
--   \n C导出文件: ./klb/src_c/klua/klua_format/klua_kh26x.c
-- @version 0.1
--]]

local kh26x = {}


-- @brief 加载/读取一个h26x(h264)文件
-- @param [in]	path[string]				h26x文件路径
-- @return h26x对象
kh26x.load = function (path)
	local h264 = {}
	
	
	-- @brief 读取帧数据
	-- @return 	cobj1[lightuserdata]		C指针: klb_buf_t*
	--			cobj2[lightuserdata]		C指针: klb_buf_t*
	h264:read = function ()
		return 1, 2
	end
	
	-- @brief 获取帧总数
	-- @return 	size[number(int)]			帧总数
	h264:size = function ()
		return 0
	end

	-- @brief 关闭
	-- @return 无
	h264:close = function ()
		return
	end
	
	return h264
end

return kh26x
