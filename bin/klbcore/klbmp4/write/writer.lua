--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   writer.lua
-- @brief  klbmp4 只写对象 (write/ 角色层)
-- @note   由 klbcore.klbmp4.init 聚合; 帧为 lightuserdata(klb_buf_t*)
--   \n 原子写: 先写 path..'.klbmp4.tmp', close 成功后才改名为目标
--   \n 路径策略: 默认递归创建父目录; 目标存在时可拒绝或经回调确认覆盖
-- @history 修改历史
--  \n 2026 创建文件
--  \n 2026 增加父目录自动创建与覆盖确认回调 (P1-11)
--]]


local kmp4 = require("kmp4")
local klbcode = require("klbcore.klbcode")
local base = klbcode.base
local util = require("klbcore.klbmp4.util")


local writer = {}


-- @brief 构造只写对象
-- @param [in]  mp4[userdata]			kmp4 只写句柄 (已指向临时文件)
-- @param [in]  path[string]			最终目标路径
-- @param [in]  tmp[string]				临时文件路径
-- @return [table]						只写对象
-- @note 对象方法: `:write` / `:info` / `:close` / `:is_open`
--   \n close 时由 C 层写出 ftyp + mdat + moov; 成功后才改名为目标
writer.new = function (mp4, path, tmp)
	local obj = { _mp4 = mp4, _path = path, _tmp = tmp }

	-- @brief 写一帧
	-- @param [in]  cobj[lightuserdata]	klb_buf_t* ([klb_mnp_media_t][Annex-B])
	-- @return code[number(int)]			0 成功; E_ARG 参数非法; E_WRITE 写入失败
	-- @return errstr[string]				描述; 成功为 ""
	-- @note 首帧须带 SPS 与 PPS; 帧由调用方释放, 本层不接管
	function obj:write(cobj)
		if nil == self._mp4 then
			return base.ERR, 'closed'
		end

		if nil == cobj or 'userdata' ~= type(cobj) then
			return base.INVAL, 'frame expected: lightuserdata'
		end

		if klbcode.OK ~= self._mp4:write(cobj) then
			return base.PACKAGE_IO, 'write failed'
		end

		return klbcode.OK, ''
	end

	-- @brief 探测元信息
	-- @return info[table]					宽高/profile/level/已写帧数/帧率/时间基/时长; 失败为 nil
	-- @return code[number(int)]			0 成功; 非0 失败
	-- @return errstr[string]				描述; 成功为 ""
	-- @note 写侧按已写帧统计; 时长按末帧间隔推算; 帧率无法计算为 0
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

	-- @brief 关闭并写出 moov, 成功后将临时文件改名为目标 (幂等)
	-- @return code[number(int)]			0 成功; E_WRITE 落盘失败; E_IO 改名失败
	-- @return errstr[string]				描述; 成功为 ""
	-- @note 落盘失败时删除临时文件, 原目标保持不变; 改名失败时保留临时文件供排查
	function obj:close()
		if nil == self._mp4 then
			return klbcode.OK, ''
		end

		local rc = self._mp4:close()

		self._mp4 = nil

		if klbcode.OK ~= rc then
			os.remove(self._tmp)

			return base.PACKAGE_IO, 'close failed: mp4 not written'
		end

		local ok, err = os.rename(self._tmp, self._path)

		if not ok then
			return base.IO, 'rename failed: ' .. tostring(err)
		end

		return klbcode.OK, ''
	end

	-- @brief 是否仍打开
	-- @return [boolean]
	function obj:is_open()
		return nil ~= self._mp4
	end

	return obj
end


-- @brief 打开 MP4 只写
-- @param [in]  path[string]			mp4 文件路径
-- @param [in]  opts[table]				[可选] overwrite: true 覆盖(默认) / false 存在即失败 /
--   \n function(path) 存在时回调, 返回 true 才覆盖
--   \n mkdir: 默认 true 自动递归创建父目录; false 不创建
-- @return [table]						只写对象; 失败为 nil
-- @return code[number(int)]			0 成功; 非0 为 E_* 错误码
-- @return errstr[string]				描述; 成功为 ""
-- @note 写入同目录临时文件 path..'.klbmp4.tmp'; 打开不截断原目标
writer.open = function (path, opts)
	if not util.check_path(path) then
		return util.fail(base.INVAL, 'path expected')
	end

	local overwrite = true
	local confirm = nil
	local mkdir = true

	if 'table' == type(opts) then
		if 'function' == type(opts.overwrite) then
			confirm = opts.overwrite
		elseif false == opts.overwrite then
			overwrite = false
		end

		if false == opts.mkdir then
			mkdir = false
		end
	end

	if mkdir and not util.ensure_parent(path) then
		return util.fail(base.NOENT, 'mkdir failed: ' .. path)
	end

	if util.file_exists(path) then
		if nil ~= confirm then
			if not confirm(path) then
				return util.fail(base.NOENT, 'exists: ' .. path)
			end
		elseif not overwrite then
			return util.fail(base.NOENT, 'exists: ' .. path)
		end
	end

	local tmp = path .. '.klbmp4.tmp'

	if util.file_exists(tmp) then
		os.remove(tmp)
	end

	local mp4 = nil
	local code = klbcode.OK
	local err = nil

	mp4, code, err = kmp4.open_write(tmp)

	if nil == mp4 then
		return util.fail(code or base.NOENT, err or ('open write failed: ' .. tmp))
	end

	return writer.new(mp4, path, tmp), klbcode.OK, ''
end


return writer

-- end
