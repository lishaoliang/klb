--[[
-- Copyright (c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbrtsp init.lua
-- @note   rtsp协议
-- @history 修改历史
--]]
local rtspclienter = require("klbcore.klbrtsp.client.rtspclienter")


local klbrtsp = {}



-- @brief 新建一个客户端
-- @param [in]      cfg[table]		客户端配置
-- @return [table]
klbrtsp.new_client = function (cfg)
	return rtspclienter.new(cfg)
end


return klbrtsp
