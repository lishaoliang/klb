--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  proxy listen 对外接口面
-- @stability stable
-- @impl evolving
-- @attention
--   \n @stability stable: AI 禁止擅自添加/删除/变更本文件对外接口 (含导出名, 参数, 返回与已文档含义); 须用户点名
--   \n @impl evolving: 实现见 service.lua; 不得破坏本文件导出签名与可观察语义
-- @note   经 require("klbagent.packages.proxy").listen; 实现 klbagent.packages.proxy.entry.service
--   \n 公开表禁止代接: 函数须实体转发
-- @history 修改历史
--  \n [2026-10] 创建文件 (原 entry.lua)
--  \n [2026-11] 入口仅保留对外导出; 实现迁 service.lua
--]]


local svc = require("klbagent.packages.proxy.entry.service")


local M = {}


-- @brief 已鉴权 mnps/smps 会话走 alua-tunnel 出口 (须 kco 协程内)
-- @param [in] serve[table]
-- @param [in] cfg[table|nil]
-- @param [in] opts[table|nil]
-- @return ok[boolean]
-- @return err[string|nil]
-- @stability stable
M.co_mnps_exit_authed = function (serve, cfg, opts)
	return svc.co_mnps_exit_authed(serve, cfg, opts)
end


-- @brief 嵌入式启动 (挂接 listen; 不阻塞泵)
-- @param [in] cfg[table|nil]
-- @param [in] opts[table|nil]
-- @return ok[boolean]
-- @return err[string|nil]
-- @stability stable
M.start_embed = function (cfg, opts)
	return svc.start_embed(cfg, opts)
end


-- @brief 泵送一次 (宿主主循环; 回 kenv.loop_once 建议休眠 ms)
-- @return ms[number]
-- @stability stable
M.pump_embed = function ()
	return svc.pump_embed()
end


-- @brief 阻塞启动 (start_embed + run_host_loop: CLI 下 hold / 独立进程自泵; embed 仍 pump_embed)
-- @param [in] cfg[table|nil]
-- @param [in] opts[table|nil]
-- @return ok[boolean]
-- @return err[string|nil]
-- @stability stable
M.start = function (cfg, opts)
	return svc.start(cfg, opts)
end


-- @brief 停止嵌入式入口
-- @return ok[boolean]
-- @return err[string|nil]
-- @stability stable
M.stop_embed = function ()
	return svc.stop_embed()
end


-- @brief 是否在跑
-- @return ok[boolean]
-- @stability stable
M.is_running = function ()
	return svc.is_running()
end


-- @brief 观测计数快照
-- @return stats[table]
-- @stability stable
M.stats = function ()
	return svc.stats()
end


-- @brief 当前配置快照 (无则 defaults)
-- @return cfg[table]
-- @stability stable
M.current_cfg = function ()
	return svc.current_cfg()
end


-- @brief 热更新运行中入口配置
-- @param [in] cfg[table]
-- @return ok[boolean]
-- @return err[string|nil]
-- @return restart_needed[boolean]
-- @stability stable
M.apply_live = function (cfg)
	return svc.apply_live(cfg)
end


return M

-- end
