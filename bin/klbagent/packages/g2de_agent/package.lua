--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   package.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  g2de_agent 可选包清单 (g2de 引擎 AI 调试控制)
-- @note   tier=optional; kind=tools; 宿主 g2decore.agent 进程内 register_embed
-- @history 修改历史
--  \n 2026 创建文件
--]]

return {
	name = "g2de_agent",
	version = "0.1.2",
	tier = "optional",
	kind = "tools",
	summary = "g2de engine embed: g2de.* snapshot/selfcheck/control for in-process AI debug",
	main = "klbagent.packages.g2de_agent",
}

-- end
