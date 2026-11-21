--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  remote.* 工具入口
-- @note   call / pull / push / deploy 见同目录子模块
-- @history 修改历史
--  \n [2026-09] 创建文件; 增加 remote.deploy (本机 stage + HTTP 供对端 pull_install)
--  \n [2026-10] 按子命令拆分子模块
--]]


local call = require("klbagent.core.tools.remote.call")
local rpc_call = require("klbagent.core.tools.remote.rpc_call")
local transfer = require("klbagent.core.tools.remote.transfer")
local deploy = require("klbagent.core.tools.remote.deploy")


local M = {}


local MODULES = {
	call,
	rpc_call,
	transfer,
	deploy,
}


-- @brief 注册 remote 工具
-- @return ok[boolean]
-- @return err[string|nil]
M.register = function (catalog, dispatch_reg)
	for i = 1, #MODULES do
		local ok, err = MODULES[i].register(catalog, dispatch_reg)
		if not ok then
			return false, err
		end
	end

	return true
end


return M

-- end
