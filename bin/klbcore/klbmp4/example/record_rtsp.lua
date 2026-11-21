--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   record_rtsp.lua
-- @brief  klbmp4 录制最小示例: klbrtsp 取帧 -> kmp4 落盘
-- @note   须在 kco 协程内调用; 仅单路 H.264 (video-only) 流
--   \n 用法:
--   \n   local kco = require("kco")
--   \n   local rec = require("klbcore.klbmp4.example.record_rtsp")
--   \n   kco.fork(function ()
--   \n       local ok, n, why = rec.record("rtsp://127.0.0.1:8554/live", "/tmp/out.mp4",
--   \n           { max_frames = 100 })
--   \n   end)
--   \n 帧面: krtsp 回 klb_buf_t* ([klb_mnp_media_t][Annex-B]); kmp4.write 内部拷贝, 写完即可 free_media
--   \n 阻塞: 每 30 帧 kco.co_sleep(1) 让出; close 一次落盘, 大文件建议独立 kco 协程
-- @history 修改历史
--  \n 2026 创建文件 (P1-9)
--]]


local kco = require("kco")
local klbrtsp = require("klbcore.klbrtsp")
local klbmp4 = require("klbcore.klbmp4")


local record_rtsp = {}


-- @brief 录制 RTSP 视频到 MP4
-- @param [in]  url[string]				rtsp 地址
-- @param [in]  path[string]			输出 mp4 路径; 父目录默认自动创建
-- @param [in]  opts[table]				[可选] max_frames: 上限帧数(0 表示不限);
--   \n transport: 'tcp'(默认) / 'udp'; progress: function(frames) 每写一帧回调, 返回 false 提前结束
-- @return ok[boolean]					成功为 true; 失败为 nil
-- @return frames_or_code[number]		成功为已写帧数; 失败为错误码
-- @return why_or_err[string]			成功为结束原因('exit'/'max_frames'/'progress'); 失败为描述
-- @note 须在 kco 协程内; 仅 H.264 视频帧可写, 音频帧会写失败(多轨见 P1-3)
--   \n 写失败时已写帧仍会落盘, 该 MP4 可能不完整
record_rtsp.record = function (url, path, opts)
	if 'string' ~= type(url) or '' == url then
		return nil, klbmp4.E_ARG, 'url expected'
	end

	if 'string' ~= type(path) or '' == path then
		return nil, klbmp4.E_ARG, 'path expected'
	end

	opts = ('table' == type(opts)) and opts or {}

	local max_frames = tonumber(opts.max_frames) or 0
	local progress = ('function' == type(opts.progress)) and opts.progress or nil
	local client = klbrtsp.new_client({ transport = opts.transport })

	client:set_timeout(10000, 10000, 30000)

	local ok, code, err = client:co_connect(url)

	if not ok then
		client:disconnect()

		return nil, code, err or 'connect failed'
	end

	local w, wc, we = klbmp4.open_write(path)

	if nil == w then
		client:disconnect()

		return nil, wc, we
	end

	local frames = 0
	local why = 'exit'

	while true do
		local msg, data = client:co_recv()

		if nil == msg or 'exit' == msg then
			break
		end

		if 'media' == msg then
			local rc = w:write(data)

			-- kmp4.write 已拷贝帧数据, 这里即可归还 krtsp
			client:free_media(data)

			if 0 ~= rc then
				client:disconnect()
				w:close()

				return nil, rc, 'write failed: frame ' .. tostring(frames)
			end

			frames = frames + 1

			if nil ~= progress and false == progress(frames) then
				why = 'progress'
				break
			end

			if 0 < max_frames and frames >= max_frames then
				why = 'max_frames'
				break
			end

			-- 阻塞 IO 让出, 避免饿死其它协程 (P1-8)
			if 0 == (frames % 30) then
				kco.co_sleep(1)
			end
		end
	end

	client:disconnect()

	local crc, cerr = w:close()

	if 0 ~= crc then
		return nil, crc, cerr
	end

	return true, frames, why
end


return record_rtsp

-- end
