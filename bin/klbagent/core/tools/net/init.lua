--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  net.* 工具入口
-- @note   status / http_get / http_post / session_* 见同目录子模块
-- @history 修改历史
--  \n [2026-09] 创建文件; klbhttp 自带后端; curl 可选回退
--  \n [2026-10] 按子命令拆分子模块
--]]


local status = require("klbagent.core.tools.net.status")
local http = require("klbagent.core.tools.net.http")
local session = require("klbagent.core.tools.net.session")


local M = {}


local MODULES = {
	status,
	http,
	session,
}


-- @brief 注册 net 工具
-- @return ok[boolean]
-- @return err[string|nil]
M.register = function (catalog, dispatch)
	for i = 1, #MODULES do
		local ok, err = MODULES[i].register(catalog, dispatch)
		if not ok then
			return false, err
		end
	end

	return true
end


return M

-- end
