--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   knetcode.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  C knetcode 帮助桩 (运行时 C 仅导出 strerror)
--   \n require("knetcode") -- 运行时: C strerror
--   \n 常量真源: require("klbcore.klbcode.netcode") / klbcore.klbcode.net
--   \n C导出文件: ./klb/src_c/klua/klua_net/klua_knetcode.c
-- @version 0.7
-- @history 修改历史
--  \n [2026-11] 常量改从 klbcode.net 再导出; 避免与 C 仅 strerror 双源
--]]


local net = require("klbcore.klbcode.netcode")


local knetcode = {}


-- 常量: 与 klb_netcode_e / klbcode.net 同名同值
for k, v in pairs(net) do
	if "function" ~= type(v) then
		knetcode[k] = v
	end
end


-- @brief 将 klb_netcode_e 转为描述串
-- @param [in] code[number(int)]	错误码
-- @return [string]	静态描述串; 带 "[klbnet] " 前缀; 未知为 "Unknown error"
-- @note 运行时由 C klua_open_knetcode 覆盖本桩
knetcode.strerror = function (code)
	return "[klbnet] Unknown error"
end


return knetcode
