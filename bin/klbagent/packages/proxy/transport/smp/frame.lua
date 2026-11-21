--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   frame.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  代理 SMP/MNP 产品头名 + 魔数薄封装 (真源 klbsmp/klbmnp)
-- @note   业务头名供 auth/tunnel; 魔数分类经 klbcore; 勿在此拼 wire
-- @history 修改历史
--  \n [2026-10] 创建文件 (N5 P2 / M1); 帧编解码下沉 C (ksmp/kmnp); 本文件仅 demux 魔数; MAGIC/classify 迁 klbsmp/klbmnp
--]]


local klbsmp = require("klbcore.klbsmp")
local klbmnp = require("klbcore.klbmnp")


local M = {}


M.MAGIC_SMP = klbsmp.MAGIC
M.MAGIC_MNP = klbmnp.MAGIC
M.HEAD_AUTH = "alua.auth"
M.HEAD_TUNNEL = "alua-tunnel"


-- @brief 是否 SMP/MNP 魔数首字节
function M.is_magic0(first)
	return klbsmp.is_magic0(first) or klbmnp.is_magic0(first)
end


-- @brief 按 4 字节魔数分类 (窥探用; 不解码载荷)
-- @return [string] "smp"|"mnp"|"unknown"
function M.classify_magic(buf)
	local k = klbsmp.classify_magic(buf)
	if k == "smp" then
		return k
	end
	return klbmnp.classify_magic(buf)
end


return M

-- end
