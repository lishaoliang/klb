--[[
-- Copyright (c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbrtsp init.lua
-- @note   rtsp协议
-- @history 修改历史
--		[2025-10] 添加基础
--]]
local rtspclienter = require("klbcore.klbrtsp.client.rtspclienter")
local rtsplistener = require("klbcore.klbrtsp.serve.rtsplistener")

local klbrtsp = {}


-- @brief 新建一个客户端
-- @param [in]      cfg[table]		客户端配置
-- @return [table]	客户端模块
klbrtsp.new_client = function (cfg)
	return rtspclienter.new(cfg)
end


-- @brief 新建一个监听模块
-- @param [in]      cfg[table]		监听模块配置
-- @return [table]	监听模块
klbrtsp.new_listen = function (cfg)
	return rtsplistener.new(cfg)
end


-- @brief 开始监听模块
-- @param [in]      port[number(int)]	端口
-- @param [in]      cfg[table]			[可选]监听模块配置
-- @return [table]	监听模块; 失败为 nil
-- @return [number]	失败时 klb_netcode_e
klbrtsp.listen = function (port, cfg)
	local l = rtsplistener.new(cfg)
	local ok, code = l:open(port)
	if not ok then
		return nil, code
	end

	return l
end


return klbrtsp
