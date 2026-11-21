--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   state.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua serve 子模块: 进程内共享状态 (serve opts / embed / tools)
-- @note   各子模块 require 同一份表; 取代 init.lua 内的 g_* 上值
--   \n 仅存状态, 不做业务判断; 生命周期随进程 / 本 klua_env
--   \n @stability internal: 实现文件, 勿 require 子路径
-- @history 修改历史
--  \n [2026-11] 创建文件; 从 core/serve/init.lua 拆出进程内状态
--]]
local M = {}


-- 当前 serve 监听参数 (port / port_mode / profile; 供 restart / health)
M.serve_opts = nil

-- 宿主嵌入 serve (非阻塞; g2decore.agent.pump 泵送)
M.embed_app = nil
M.embed_opts = nil
M.embed_token = nil
M.embed_service = "alua"

-- tools 是否已注册 (本 env 只做一次; serve 入口不依赖调用方预注册)
M.tools_loaded = false


return M

-- end
