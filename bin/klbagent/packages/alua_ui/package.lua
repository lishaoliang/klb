--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   package.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua_ui 可选包清单 (人用 alua 运维 UI; 产品记名 alua-ui)
-- @note   tier=optional; kind=cli; CLI: alua ui|gui; 启动依赖 g2deapp + kg2de.klbui
-- @history 修改历史
--  \n [2026-10] 创建文件 (桩); CLI 入口 alua ui|gui; 宿主 / 本机桥 / 总览页; 远程目标 / serve / runtime; 页面对齐 page_tmpl; 删除 pages.common
--]]

return {
	name = "alua_ui",
	version = "0.1.5",
	tier = "optional",
	kind = "cli",
	summary = "Human ops UI for local/remote alua config and status (needs kg2de.klbui)",
	main = "klbagent.packages.alua_ui",
}

-- end
