--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   profile.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua serve 子模块: 本进程权限档 (readonly|net|device|unsafe)
-- @note   仅进程内覆盖 profiles, 禁止写共享 pref (勿踩本机 alua call)
--   \n @stability internal: 实现文件, 契约面见 core/serve/init.lua
-- @history 修改历史
--  \n [2026-11] 创建文件; 从 core/serve/init.lua 拆出 apply_profile / set_live_profile
--]]
local profiles = require("klbagent.core.profiles")
local log = require("klbagent.core.log")
local state = require("klbagent.core.serve.base.state")


local M = {}


-- @brief 应用权限档 (仅进程内覆盖, 不写 pref)
-- @param [in] profile[string]  readonly|net|device|unsafe
-- @return ok[boolean]
-- @return err[string|nil]
M.apply_profile = function (profile)
	local ok, err = profiles.set_override(profile)
	if not ok then
		return false, err or "set profile override fail"
	end

	return true
end


-- @brief 运行中调整 serve/embed 进程权限档 (不写 pref)
-- @param [in] profile[string]  readonly|net|device|unsafe
-- @return ok[boolean]
-- @return err[string|nil]
M.set_live_profile = function (profile)
	if type(profile) ~= "string" or profile == "" then
		return false, "bad profile"
	end

	profile = string.lower(profile)
	local ok, err = M.apply_profile(profile)
	if not ok then
		return false, err
	end

	if type(state.serve_opts) == "table" then
		state.serve_opts.profile = profile
	end

	if type(state.embed_opts) == "table" then
		state.embed_opts.profile = profile
	end

	log.write("info", "serve set_live_profile", {
		module = "serve",
		profile = profile,
	})

	return true
end


return M

-- end
