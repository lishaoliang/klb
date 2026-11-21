--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   package.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  proxy 可选包清单 (本地代理入口 + 多级链)
-- @note   tier=optional; kind=daemon
--   \n 目录: entry/ config/ chain/ pac/ stats/ acl/ relay/ transport/ bench/
--   \n 对外入口: require("klbagent.packages.proxy") → .listen() / .config() / .chain() / .pac() / .stats() / .entry / .selfcheck
--   \n 不导出: relay/transport/hop_token/acl (内部)
-- @history 修改历史
--  \n [2026-10] 创建文件 (桩)
--  \n [2026-11] 根/entry 禁代接: 取面函数 + 实体转发; CLI 迁 service
--  \n [2026-11] hop 鉴权对齐 ssid 证明票 (p. + alua_ssid); 删 new_serve / temp 文件读写
--]]

return {
	name = "proxy",
	version = "0.1.71",
	tier = "optional",
	kind = "daemon",
	summary = "Local SOCKS5/HTTP CONNECT/forward/direct + pump/chain/PAC; alua-tunnel PSK; same-port demux",
	main = "klbagent.packages.proxy",
}

-- end
