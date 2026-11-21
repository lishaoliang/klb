--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  deploy.* 对端安装 (供 alua serve 装入 /usr/local)
-- @note   pull_install=URL; self_install=本机 path(.tgz|stage); 须 profile unsafe
--   \n source_install=本机源码树 make stage 后装入 (层 D)
--   \n scripts_install=仅 share/klua Lua + runtime.reload (层 E 脚本侧)
--   \n setup_priv=目标机一次配置限定 NOPASSWD (方案 1); 后续 sudo -n 通行
--   \n P2: expect_version/min_version 钉扎; 校验失败回滚且不 restart
--   \n P1: restart 经 serve.schedule_restart (proc/restart; systemctl/kill 或 exec / Win schtasks)
--   \n win_install: Win setup.exe 静默完全安装 (NSIS /S)
-- @history 修改历史
--  \n [2026-09] 创建文件
--  \n [2026-10] 按子命令拆分子模块
--]]


local stage = require("klbagent.core.tools.deploy.stage")
local install = require("klbagent.core.tools.deploy.install")
local win_install = require("klbagent.core.tools.deploy.win_install")
local setup_priv = require("klbagent.core.tools.deploy.setup_priv")


local M = {}


local MODULES = {
	install,
	win_install,
	setup_priv,
}


M.build_stage = stage.build_stage


-- @brief 注册 deploy 工具
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
