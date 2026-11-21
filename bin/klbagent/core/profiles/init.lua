--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  Agent 权限档
-- @note   readonly < net < device < unsafe; 与功能分类正交; write_fs 档位同 net
-- @history 修改历史
--  \n [2026-09] 创建文件; 迁入子目录 profiles/init.lua; 增加 write_fs 副作用标签
--  \n [2026-10] min_of: 两档取更严 (临时 token 裁剪 / D52)
--]]


local M = {}


-- serve 进程内覆盖; 非空时优先于 pref runtime.json (隔离 CLI)
local _override = nil


local RANK = {
	readonly = 1,
	net = 2,
	device = 3,
	unsafe = 4,
}


local TAG_RANK = {
	write_pref = 1,
	write_fs = 2,
	net = 2,
	device = 3,
	unsafe = 4,
}


-- @brief 是否合法权限档
-- @param [in] name[string]
-- @return ok[boolean]
M.is_profile = function (name)
	return type(name) == "string" and RANK[name] ~= nil
end


-- @brief 默认档
-- @return name[string]
M.default = function ()
	return "readonly"
end


-- @brief 设置进程内 profile 覆盖 (alua serve 用; 不写 pref)
-- @param [in] name[string|nil]		合法档名; nil/"" 清除覆盖
-- @return ok[boolean]
-- @return err[string|nil]
M.set_override = function (name)
	if name == nil or name == "" then
		_override = nil
		return true
	end

	if not M.is_profile(name) then
		return false, "bad profile"
	end

	_override = name
	return true
end


-- @brief 当前进程内覆盖 (无则 nil)
-- @return name[string|nil]
M.get_override = function ()
	return _override
end


-- @brief 当前生效档: override > pref runtime.json > default
-- @return name[string]
M.active = function ()
	if type(_override) == "string" and M.is_profile(_override) then
		return _override
	end

	local config = require("klbagent.core.config")
	local cfg = config.runtime.load()
	local p = cfg.profile
	if not M.is_profile(p) then
		return M.default()
	end

	return p
end


-- @brief 档位等级 (越大权限越高)
-- @param [in] name[string]
-- @return rank[number]
M.rank = function (name)
	if not M.is_profile(name) then
		return 0
	end

	return RANK[name]
end


-- @brief 两档取更严 (rank 更小者); 非法则回退合法侧 / default
-- @param [in] a[string]
-- @param [in] b[string]
-- @return name[string]
M.min_of = function (a, b)
	local ok_a = M.is_profile(a)
	local ok_b = M.is_profile(b)
	if ok_a and ok_b then
		if RANK[a] <= RANK[b] then
			return a
		end

		return b
	end

	if ok_a then
		return a
	end

	if ok_b then
		return b
	end

	return M.default()
end


-- @brief 当前档是否满足工具 min_profile
-- @param [in] active[string]
-- @param [in] need[string]
-- @return ok[boolean]
M.allows = function (active, need)
	if not M.is_profile(active) then
		return false
	end

	if type(need) ~= "string" or need == "" then
		need = M.default()
	end

	if not M.is_profile(need) then
		return false
	end

	return RANK[active] >= RANK[need]
end


-- @brief 当前档是否允许 side_effects
-- @param [in] profile[string]
-- @param [in] side_effects[table]
-- @return ok[boolean]
M.allow = function (profile, side_effects)
	if not M.is_profile(profile) then
		return false
	end

	if type(side_effects) ~= "table" then
		return true
	end

	local need = 1
	for i = 1, #side_effects do
		local tag = side_effects[i]
		local r = TAG_RANK[tag]
		if r == nil then
			-- 未知或非字符串副作用按最高档
			r = RANK.unsafe
		end

		if type(r) == "number" and r > need then
			need = r
		end
	end

	return RANK[profile] >= need
end


return M
