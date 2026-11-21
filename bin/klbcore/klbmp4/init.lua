--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @brief  klbmp4: MP4 单路 avc1 读写脚本包装层 (对外入口 / 聚合)
-- @note   require("klbcore.klbmp4"); C 绑定 kmp4
--   \n 分层: init 仅聚合与契约; 角色实现见 read/reader.lua、write/writer.lua
--   \n 常量真源: klbcore.klbcode; 内部小工具: util/init.lua
--   \n 帧格式: lightuserdata(klb_buf_t*) = [klb_mnp_media_t][Annex-B 载荷]
--   \n 帧来源: 只能取自 `:read` 或 C 层生成的 klb_buf_t*; 脚本无法自建帧
--   \n 帧生命周期: 读侧取得的帧所有权归调用方, 用完须 `:free_frame`, 本层不接管
--   \n B 帧语义: C 层不解析 ctts; 读侧按样本存储(解码)序输出; 写侧按调用方交付顺序记为显示序
--   \n 参数集: 写侧首帧只取第一个 SPS/PPS, 同帧其余参数集丢弃; 后续帧携带的参数集忽略
--   \n 原子写: open_write 写入同目录临时文件, close 成功后改名为目标, 失败保留原目标
--   \n 错误码: E_* 映射 klbcore.klbcode; 与 C klbmp4_errcode_e / klb_basecode 对齐
--   \n 探测: 只读/只写对象均有 `:info`; 只读对象另有 `:seek(time_ms)`
--   \n 阻塞 IO: C 侧 fopen/fread/fwrite 不让出协程; open_read / close 建议放独立 kco 协程
--   \n  读侧 moov 一次读入(≤32MB), 写侧 close 一次落盘; 大文件建议独立 kco 协程
--   \n 路径策略: open_write 默认递归创建父目录; opts.overwrite 可为 true/false/function
--   \n 录制示例: `klbcore.klbmp4.example.record_rtsp` (P1-9)
-- @history 修改历史
--  \n 2026 创建文件
--  \n 2026 写路径原子化; close 返回状态; 补充帧/B 帧/参数集声明
--  \n 2026 按 klbhttp 分层原则拆分: common / read / write
--  \n 2026 增加 E_* 错误码; 对象增加 info / seek
--  \n 2026 补阻塞 IO 说明; open_write 自动建目录与覆盖确认; 增加录制示例
--]]


local klbcode = require("klbcore.klbcode")
local base = klbcode.base
local net = klbcode.net
local reader = require("klbcore.klbmp4.read.reader")
local writer = require("klbcore.klbmp4.write.writer")


local klbmp4 = {}


--------------------------------------------------------------------------------------------
-- 返回码 (真源 klbcore.klbcode)

klbmp4.OK = klbcode.OK				-- 成功
klbmp4.EOF = net.DISCONNECT			-- 读侧已读完 (值 1, 与 C read EOF 对齐)
klbmp4.ERR = base.ERR				-- 通用失败

-- 打开 / 读写失败码 (映射 klbcode; 与 C klbmp4_errcode_e 同值)
klbmp4.E_ARG = base.INVAL			-- 参数非法
klbmp4.E_OPEN = base.NOENT			-- 打开文件失败
klbmp4.E_FORMAT = base.CODEC_PROTO	-- 非 MP4 / 顶层 box 非法
klbmp4.E_MOOV = base.PACKAGE_BAD	-- 无 moov / moov 损坏
klbmp4.E_TRACK = base.CODEC_NOSUP	-- 无 vide 轨 / 非 avc1 / 样本表非法
klbmp4.E_LIMIT = base.LIMIT			-- 超过硬上限
klbmp4.E_IO = base.IO				-- 文件读写失败
klbmp4.E_WRITE = base.PACKAGE_IO	-- 写侧未写出


--------------------------------------------------------------------------------------------
-- 模块级

-- @brief 打开 MP4 只读
-- @param [in]  path[string]			mp4 文件路径
-- @return [table]						只读对象; 失败为 nil
-- @return code[number(int)]			0 成功; 非0 为 E_* 错误码
-- @return errstr[string]				描述; 成功为 ""
-- @note 非分片单路 avc1; moov 可在头或尾; 其它子集打开失败
--   \n 对象方法见 read/reader.lua: read / info / seek / free_frame / close / is_open
klbmp4.open_read = function (path)
	return reader.open(path)
end


-- @brief 打开 MP4 只写
-- @param [in]  path[string]			mp4 文件路径
-- @param [in]  opts[table]				[可选] overwrite: true 覆盖(默认) / false 存在即失败 /
--   \n function(path) 存在时回调, 返回 true 才覆盖
--   \n mkdir: 默认 true 自动递归创建父目录; false 不创建
-- @return [table]						只写对象; 失败为 nil
-- @return code[number(int)]			0 成功; 非0 为 E_* 错误码
-- @return errstr[string]				描述; 成功为 ""
-- @note 写入同目录临时文件 path..'.klbmp4.tmp'; close 成功后才改名为 path
--   \n 打开不截断原目标; 未 close 或 close 失败时原目标保持不变
--   \n close 一次写出全部样本(阻塞), 大文件建议在独立 kco 协程内调用
--   \n 对象方法见 write/writer.lua: write / info / close / is_open
klbmp4.open_write = function (path, opts)
	return writer.open(path, opts)
end


return klbmp4

-- end
