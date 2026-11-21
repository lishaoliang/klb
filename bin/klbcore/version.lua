--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   version.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbcore / klua 脚本线版本 (基础版本 + 发布年月)
-- @stability stable
-- @impl mature
-- @attention
--   \n @stability stable: AI 禁止擅自添加/删除/变更本文件对外接口 (含导出名, 参数, 返回与已文档含义); 须用户点名
--   \n @impl mature: 实现倾向冻结; 重构须同等谨慎, 不得破坏本文件导出签名与可观察语义
-- @note   与同目录 VERSION 单行一致; 升版规则同 klbagent; klua -v 读 VERSION
-- @history 修改历史
--  \n [2026-11] 增 klbtar; M1-M8 读侧
--  \n [2026-09] 创建文件
--]]


local M = {}


-- 基础语义版本 (主.次.修订); 功能改动后修订 +1
-- 段范围 (int32): 主 0-214 (3位), 次 0-999 (3位), 修订 0-9999 (4位)
M.base = "0.1.80"

-- 发布年月 YYYYMM (升版时改当月; 与 klb 核心 / klbagent 数值独立)
M.ym = "202611"

-- 完整: base.ym → klua -v 中 "klua <string> (klb ...)"
M.string = M.base .. "." .. M.ym

-- 数值: 主*10000000 + 次*10000 + 修订 (同 KLB_VERSION_NUMBER)
do
	local major, minor, patch = M.base:match("^(%d+)%.(%d+)%.(%d+)$")
	M.number = tonumber(major) * 10000000 + tonumber(minor) * 10000 + tonumber(patch)
end


return M
