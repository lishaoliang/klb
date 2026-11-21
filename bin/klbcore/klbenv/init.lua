--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbenv 脚本侧单 env 壳 (三钩子 + 入口参数 + 两种主循环入口)
-- @stability stable
-- @impl mature
-- @attention
--   \n @stability stable: AI 禁止擅自添加/删除/变更本文件对外接口 (含导出名, 参数, 返回与已文档含义); 须用户点名
--   \n @impl mature: 实现倾向冻结; 重构须同等谨慎, 不得破坏本文件导出签名与可观察语义
-- @note   粒度=单个 klua_env / 单线程; 非 C 进程壳 klbapp
--   \n 推荐标准入口: co_main_loop (无特殊用途即走此路径)
--   \n set_first_run / set_loop_run / set_end_run 后调 co_main_loop (或例外 main_loop)
--   \n main_loop: 同步钩子 (少见); co_main_loop: loop_run 托管在 kco 协程
--   \n 入口参数: set_args(t) 存储; get_args() 读取; 与 ksys.get_args (进程 argv) 独立
-- @history 修改历史
--  \n [2026-11] 创建文件; 目录由 klbapp 更名为 klbenv
--  \n [2026-11] 注明 co_main_loop 为推荐标准入口
--]]
local main_loop = require("klbcore.klbenv.main_loop")
local co_main_loop = require("klbcore.klbenv.co_main_loop")

local M = {}

-- 继承, 改写
M.first_run = function()

end


M.loop_run = function()

end

M.end_run = function()

end

--
M.G = {}

-- 入口参数 (应用侧; 表或任意值)
M.args = nil

--
M.kexit = function()
    M.first_run = nil
    M.loop_run = nil
    M.end_run = nil
    M.G = nil
    M.args = nil
end


M.set_first_run = function(func)
    M.first_run = func
end

M.set_loop_run = function(func)
    M.loop_run = func
end

M.set_end_run = function(func)
    M.end_run = func
end

-- @brief 设置入口参数
-- @param [in]     args[any]			通常为 table; 亦可存其它类型
-- @return 无
M.set_args = function(args)
    M.args = args
end

-- @brief 读取入口参数
-- @return args[any]					未设置时为 nil
M.get_args = function()
    return M.args
end

-- 主循环
M.main_loop = function()
    return main_loop(M.loop_run, M.first_run, M.end_run)
end

-- 主循环 (协程托管)
M.co_main_loop = function()
    return co_main_loop(M.loop_run, M.first_run, M.end_run)
end


return M

-- end
