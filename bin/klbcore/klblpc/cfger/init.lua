--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klblpc 模块名配置
-- @note   模块名须非空且长度 <= KLUA_LPC_NAME_LEN(15)
-- @history 修改历史
--		[2026-09] 创建文件
--]]


local cfger = {}


-- 与 klua_env.h KLUA_LPC_NAME_LEN 一致
cfger.NAME_MAX = 15


-- @brief 校验并规范化模块名
-- @param [in]      name[string]		模块名称
-- @return [string]						合法名称; 非法为 nil
cfger.parse_name = function (name)
	if "string" ~= type(name) then
		return nil
	end

	if "" == name then
		return nil
	end

	if cfger.NAME_MAX < #name then
		return nil
	end

	return name
end


-- @brief 解析 setup 配置
-- @param [in]      cfg[table]			[可选] `{ name }`
-- @return [table]						`{ name }`; 非法为 nil
-- @return ok[boolean]
cfger.parse_setup = function (cfg)
	if nil == cfg then
		return {
			name = "",
		}, true
	end

	if "table" ~= type(cfg) then
		return nil, false
	end

	if nil == cfg.name then
		return {
			name = "",
		}, true
	end

	local name = cfger.parse_name(cfg.name)
	if not name then
		return nil, false
	end

	return {
		name = name,					-- [必须] string    LPC 模块名, <=15
	}, true
end


return cfger
