--[[
-- Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   krtp.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  C krtp, RTP 打包
--   \n require("krtp")
--   \n C导出文件: ./klb/src_packages/klbrtp/rtp_klua/klbrtp_klua.c
--   \n 现状: 仅 packetize 桩函数, 恒返回 nil, KLB_NETCODE_EOPNOTSUPP(-95)
-- @version 0.1
--]]

local krtp = {}


-- @brief RTP 打包 (未实现)
-- @param [in] ...	[任意]	当前桩实现忽略入参
-- @return nil, code[number(int)]	恒为 nil, -95 (KLB_NETCODE_EOPNOTSUPP)
-- @note 描述串用 knetcode.strerror(code)
krtp.packetize = function (...)
	return nil, -95
end


return krtp
