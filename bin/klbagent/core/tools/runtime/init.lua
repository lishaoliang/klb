--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  runtime.* 工具入口
-- @note   version/ping/selfcheck/hostinfo/cwd/env_*/tls_resign/reload/parallel/ai_bootstrap/set_profile
--   \n env overlay 供 exec 继承; parallel 为进程内编排; session 仅 core.session 内调 (无对外工具)
-- @history 修改历史
--  \n [2026-09] 创建文件; 迁入子目录 tools/runtime/init.lua; 补充 cwd / env_get / env_set; 补充 tls_resign (过期重签); 补充 session_* / parallel
--  \n [2026-10] 按子命令拆分子模块
--  \n [2026-11] 删除 runtime.session_* 对外工具 (宿主内调 core.session)
--]]


local env = require("klbagent.core.tools.runtime.env")
local probe = require("klbagent.core.tools.runtime.probe")
local tls = require("klbagent.core.tools.runtime.tls")
local reload = require("klbagent.core.tools.runtime.reload")
local parallel = require("klbagent.core.tools.runtime.parallel")
local ai_load = require("klbagent.core.tools.runtime.ai_load")
local profile = require("klbagent.core.tools.runtime.profile")


local M = {}


M.child_env = env.child_env


local MODULES = {
	probe,
	ai_load,
	env,
	tls,
	reload,
	parallel,
	profile,
}


-- @brief 注册 runtime 工具到 catalog/dispatch
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
