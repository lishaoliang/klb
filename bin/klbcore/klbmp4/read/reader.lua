--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   reader.lua
-- @brief  klbmp4 只读对象 (read/ 角色层)
-- @note   由 klbcore.klbmp4.init 聚合; 帧为 lightuserdata(klb_buf_t*)
--   \n 帧格式: [klb_mnp_media_t][Annex-B 载荷]; 帧所有权归调用方, 须 `:free_frame`
-- @history 修改历史
--  \n 2026 创建文件
--]]


local kmp4 = require("kmp4")
local klbcode = require("klbcore.klbcode")
local base = klbcode.base
local net = klbcode.net
local util = require("klbcore.klbmp4.util")


local reader = {}


-- @brief 构造只读对象
-- @param [in]  mp4[userdata]			kmp4 只读句柄
-- @return [table]						只读对象
-- @note 对象方法: `:read` / `:info` / `:seek` / `:free_frame` / `:close` / `:is_open`
reader.new = function (mp4)
	local obj = { _mp4 = mp4 }

	-- @brief 读下一帧
	-- @return cobj[lightuserdata]		成功时为 klb_buf_t*; 失败/读完为 nil
	-- @return code[number(int)]			0 成功; 1 读完; -1 失败
	-- @return errstr[string]				描述; 成功为 ""
	-- @note 成功取得的帧须以 `:free_frame(cobj)` 释放
	function obj:read()
		if nil == self._mp4 then
			return util.fail(base.ERR, 'closed')
		end

		local cobj = nil
		local code = 0

		cobj, code = self._mp4:read()

		if nil == cobj then
			if net.DISCONNECT == code then
				return util.fail(net.DISCONNECT, 'eof')
			end

			return util.fail(base.ERR, 'read failed')
		end

		return cobj, klbcode.OK, ''
	end

	-- @brief 探测元信息
	-- @return info[table]					宽高/profile/level/样本数/帧率/时间基/时长; 失败为 nil
	-- @return code[number(int)]			0 成功; 非0 失败
	-- @return errstr[string]				描述; 成功为 ""
	-- @note 读侧数值取自 moov; 未知宽高/fps 为 0
	function obj:info()
		if nil == self._mp4 then
			return util.fail(base.ERR, 'closed')
		end

		local info = nil
		local code = 0

		info, code = self._mp4:info()

		if nil == info then
			return util.fail(code or base.ERR, 'info failed')
		end

		return info, klbcode.OK, ''
	end

	-- @brief 按时间定位读位置
	-- @param [in]  time_ms[number]			目标时间(ms)
	-- @return code[number(int)]			0 成功; 非0 失败
	-- @return errstr[string]				描述; 成功为 ""
	-- @note 定位到首个 time_ms >= 目标的样本; 超过末尾后 `:read` 返回 EOF
	--   \n 非关键帧感知; 需要从关键帧起解码时由调用方自行回退
	function obj:seek(time_ms)
		if nil == self._mp4 then
			return base.ERR, 'closed'
		end

		if 'number' ~= type(time_ms) then
			return base.INVAL, 'time_ms expected: number'
		end

		if klbcode.OK ~= self._mp4:seek(time_ms) then
			return base.ERR, 'seek failed'
		end

		return klbcode.OK, ''
	end

	-- @brief 释放 read 取得的帧
	-- @param [in]  cobj[lightuserdata]	klb_buf_t*; 可为 nil
	-- @return 无
	function obj:free_frame(cobj)
		if nil == cobj or nil == self._mp4 then
			return
		end

		self._mp4:free_frame(cobj)
	end

	-- @brief 关闭 (幂等)
	-- @return 无
	function obj:close()
		if nil ~= self._mp4 then
			self._mp4:close()
			self._mp4 = nil
		end
	end

	-- @brief 是否仍打开
	-- @return [boolean]
	function obj:is_open()
		return nil ~= self._mp4
	end

	return obj
end


-- @brief 打开 MP4 只读
-- @param [in]  path[string]			mp4 文件路径
-- @return [table]						只读对象; 失败为 nil
-- @return code[number(int)]			0 成功; 非0 为 E_* 错误码
-- @return errstr[string]				描述; 成功为 ""
-- @note 非分片单路 avc1; moov 可在头或尾; 其它子集打开失败
reader.open = function (path)
	if not util.check_path(path) then
		return util.fail(base.INVAL, 'path expected')
	end

	local mp4 = nil
	local code = klbcode.OK
	local err = nil

	mp4, code, err = kmp4.open_read(path)

	if nil == mp4 then
		return util.fail(code or base.NOENT, err or ('open read failed: ' .. path))
	end

	return reader.new(mp4), klbcode.OK, ''
end


return reader

-- end
