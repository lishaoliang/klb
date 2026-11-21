--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   package.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua_web 可选包清单 (alua 配置页 www)
-- @note   tier=optional; kind=daemon (pkg run 常驻); 页面 /api/v1/*; 端口组 http/https
-- @history 修改历史
--  \n [2026-10] 创建文件 (桩); serve 入口 + /api/v1/* + 静态页
--]]
return {
	name = "alua_web",
	version = "0.1.2",
	tier = "optional",
	kind = "daemon",
	summary = "Config web page for alua (port group http/https; /api/v1/*)",
	main = "klbagent.packages.alua_web",
}

-- end
