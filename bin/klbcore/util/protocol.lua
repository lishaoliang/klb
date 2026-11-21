--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   protocol.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  网络协议类型常量 (对齐 C klb_protocol_e)
-- @note   真源: klb/inc/klbnet/klb_netconn.h; 数值须与 C 枚举保持一致
-- @stability experimental
-- @impl evolving
-- @history 修改历史
--  \n [2026-11] 创建文件
--]]


local protocol = {}


-- 基础协议
protocol.UNKOWN			= 0		-- unkown (与 C 拼写一致)
protocol.HTTP			= 1		-- http
protocol.WS				= 2		-- websocket
protocol.RTSP			= 3		-- rtsp
protocol.RTMP			= 4		-- rtmp

-- 私有帧协议
protocol.MNP			= 31	-- mnp
protocol.SMP			= 32	-- smp

-- HTTP / WS 衍生
protocol.HTTP_FLV		= 51	-- http flv
protocol.HTTP_MNP		= 52	-- http mnp
protocol.HTTP_SMP		= 53	-- http smp
protocol.WS_MNP			= 54	-- websocket mnp
protocol.WS_SMP			= 55	-- websocket smp

-- RPC
protocol.RPC_MNP_LUA	= 81	-- RPC mnp-lua
protocol.RPC_MNP_JSON	= 82	-- RPC mnp-json
protocol.RPC_SMP_LUA	= 83	-- RPC smp-lua
protocol.RPC_SMP_JSON	= 84	-- RPC smp-json
protocol.RPC_HTTP_LUA	= 85	-- RPC http-lua
protocol.RPC_HTTP_JSON	= 86	-- RPC http-json
protocol.RPC_WS_LUA		= 87	-- RPC ws-lua
protocol.RPC_WS_JSON	= 88	-- RPC ws-json

protocol.MAX			= 89	-- 内置协议上界 (不含)
protocol.user			= 2000	-- 用户自定义起始


local TEXT = {
	[protocol.UNKOWN]			= "unkown",
	[protocol.HTTP]				= "http",
	[protocol.WS]				= "ws",
	[protocol.RTSP]				= "rtsp",
	[protocol.RTMP]				= "rtmp",
	[protocol.MNP]				= "mnp",
	-- SMP 与 MNP 同值, 不单独占 TEXT
	[protocol.HTTP_FLV]			= "http-flv",
	[protocol.HTTP_MNP]			= "http-mnp",
	[protocol.HTTP_SMP]			= "http-smp",
	[protocol.WS_MNP]			= "ws-mnp",
	[protocol.WS_SMP]			= "ws-smp",
	[protocol.RPC_MNP_LUA]		= "rpc-mnp-lua",
	[protocol.RPC_MNP_JSON]		= "rpc-mnp-json",
	[protocol.RPC_SMP_LUA]		= "rpc-smp-lua",
	[protocol.RPC_SMP_JSON]		= "rpc-smp-json",
	[protocol.RPC_HTTP_LUA]		= "rpc-http-lua",
	[protocol.RPC_HTTP_JSON]	= "rpc-http-json",
	[protocol.RPC_WS_LUA]		= "rpc-ws-lua",
	[protocol.RPC_WS_JSON]		= "rpc-ws-json",
}


-- @brief 协议号转短名
-- @param [in]  code[number]	协议号
-- @return [string]			短名; 未知为 "protocol-<n>"
protocol.str = function (code)
	local s = TEXT[code]
	if nil ~= s then
		return s
	end
	return "protocol-" .. tostring(code)
end


return protocol

-- end
