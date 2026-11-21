--[[
-- Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   kmp4.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  C kmp4, 读写 MP4 文件 (单路 avc1 子集)
--   \n require("kmp4")
--   \n C导出文件: ./klb/src_packages/klbmp4/mp4_klua/klbmp4_klua.c
--   \n 帧格式: lightuserdata(klb_buf_t*) = [klb_mnp_media_t][Annex-B 载荷]
--   \n 读侧帧须 free_frame; 写首帧须带 SPS/PPS
-- @version 0.2
--]]

local kmp4 = {}


-- @brief 打开 MP4 只读
-- @param [in]	path[string]				mp4 文件路径
-- @return 	mp4[userdata|nil]			成功为对象; 失败 nil
--			code[number(int)]			成功为 0; 失败为错误码 (见 klbmp4_errcode_e)
--			err[string|nil]				失败时错误信息
kmp4.open_read = function (path)
	local mp4 = {}


	-- @brief 读下一帧
	-- @return 	cobj[lightuserdata|nil]		C指针: klb_buf_t*
	--			code[number(int)]			0 成功; 1 读完; -1 失败
	mp4:read = function ()
		return nil, 1
	end

	-- @brief 探测元信息
	-- @return 	info[table|nil]				width/height/profile/level/sample_count/fps/timescale/duration_ms
	--			code[number(int)]			0 成功; 非0 失败
	mp4:info = function ()
		return nil, -1
	end

	-- @brief 按时间定位 (首个 time_ms >= 目标的样本)
	-- @param [in]	time_ms[number]				目标时间 (毫秒)
	-- @return 	code[number(int)]			0 成功; -1 失败
	mp4:seek = function (time_ms)
		return -1
	end

	-- @brief 释放 read 得到的帧
	-- @param [in]	cobj[lightuserdata]		klb_buf_t*
	-- @return 无
	mp4:free_frame = function (cobj)
		return
	end

	-- @brief 取帧载荷 (去掉 klb_mnp_media_t 头)
	-- @param [in]	cobj[lightuserdata]		klb_buf_t*
	-- @return payload[string]	码流 (Annex-B)
	-- @return size[number(int)]	载荷字节数
	mp4:frame_payload = function (cobj)
		return '', 0
	end

	-- @brief 取帧元信息
	-- @param [in]	cobj[lightuserdata]		klb_buf_t*
	-- @return [table] pts_us[number(int)], key[boolean]
	mp4:frame_info = function (cobj)
		return { pts_us = 0, key = false }
	end

	-- @brief 关闭
	-- @return code[number(int)]			0 成功
	mp4:close = function ()
		return 0
	end

	return mp4
end


-- @brief 打开 MP4 只写
-- @param [in]	path[string]				mp4 文件路径
-- @return 	mp4[userdata|nil]			成功为对象; 失败 nil
--			code[number(int)]			成功为 0; 失败为错误码 (见 klbmp4_errcode_e)
--			err[string|nil]				失败时错误信息
-- @note 关闭时写出 ftyp + mdat + moov
kmp4.open_write = function (path)
	local mp4 = {}


	-- @brief 写一帧
	-- @param [in]	cobj[lightuserdata]		klb_buf_t* ([klb_mnp_media_t][Annex-B])
	-- @return 	code[number(int)]			0 成功; 非0 失败
	-- @note 首帧须带 SPS 与 PPS; time 单位同 read (媒体毫秒 x1000)
	mp4:write = function (cobj)
		return -1
	end

	-- @brief 探测元信息 (写侧按已写帧统计)
	-- @return 	info[table|nil]				width/height/profile/level/sample_count/fps/timescale/duration_ms
	--			code[number(int)]			0 成功; 非0 失败
	mp4:info = function ()
		return nil, -1
	end

	-- @brief 关闭 (写出 moov)
	-- @return code[number(int)]			0 成功; -1 写侧落盘失败
	mp4:close = function ()
		return 0
	end

	return mp4
end

return kmp4
