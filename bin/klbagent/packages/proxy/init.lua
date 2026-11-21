--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  proxy 包对外入口 (listen / config / chain / pac / stats)
-- @stability stable
-- @impl evolving
-- @attention
--   \n @stability stable: AI 禁止擅自添加/删除/变更本文件对外接口 (含导出名, 参数, 返回与已文档含义); 须用户点名
--   \n @impl evolving: 内部可重构 (relay/transport/hop_token 等), 不得破坏本文件导出签名与可观察语义
-- @note   kind=daemon; 包外须 require("klbagent.packages.proxy") 使用本入口
--   \n 例: local proxy = require("klbagent.packages.proxy"); proxy.listen().start(cfg)
--   \n 公开表禁止子模块代接: listen/config/chain/pac/stats 为取面函数
--   \n CLI: alua pkg run proxy [serve|check|pac] → M.entry
--   \n 不导出: relay/transport/hop_token/acl 及 klbcore 协议再导出 (内部; 包外直 require 子路径或 klbcore)
-- @history 修改历史
--  \n [2026-10] 创建文件 (桩)
--  \n [2026-11] 对外入口仅取面函数 + entry/selfcheck; 实现迁 service; entry 拆面
--]]


local svc = require("klbagent.packages.proxy.service")
local listen = require("klbagent.packages.proxy.entry")
local config = require("klbagent.packages.proxy.config")
local chain = require("klbagent.packages.proxy.chain")
local pac = require("klbagent.packages.proxy.pac")
local stats = require("klbagent.packages.proxy.stats")


local M = {}


--------------------------------------------------------------------------------------------
-- 包 CLI / 自检


-- @brief alua pkg run proxy …
-- @param [in] argv[table|nil]
-- @param [in] _manifest[table|nil]
-- @return ok[boolean]
-- @return code[number]
-- @return message[string|nil]
-- @stability stable
M.entry = function (argv, _manifest)
	return svc.entry(argv, _manifest)
end


-- @brief 包自检
-- @return info[table]
-- @stability stable
M.selfcheck = function ()
	return svc.selfcheck()
end


--------------------------------------------------------------------------------------------
-- 业务面 (取面函数)


-- @brief 取 listen 面 (入口 listen 循环)
-- @return listen[table]
-- @stability stable
-- @note start / start_embed / is_running / apply_live …
M.listen = function ()
	return listen
end


-- @brief 取 config 面
-- @return config[table]
-- @stability stable
M.config = function ()
	return config
end


-- @brief 取 chain 面 (多跳 / 探活)
-- @return chain[table]
-- @stability stable
M.chain = function ()
	return chain
end


-- @brief 取 PAC 面
-- @return pac[table]
-- @stability stable
M.pac = function ()
	return pac
end


-- @brief 取 stats 面 (运行态快照)
-- @return stats[table]
-- @stability stable
M.stats = function ()
	return stats
end


return M

-- end
