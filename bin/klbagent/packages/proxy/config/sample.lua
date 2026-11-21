--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   sample.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  proxy 配置样例 (N0 读盘; alua pkg run proxy serve config ...)
-- @note   复制后改 chains/routes; 亦可等价 JSON
--]]

return {
	listen = {
		host = "127.0.0.1",
		port = 12818,
		protocols = { "socks5", "http-connect", "alua-tunnel" },
		-- backlog = 128,
		-- port_fallback = 8, -- 占用时试 port .. port+N (避开 listens/mnps)
		-- 与 mnps 并存时: mnps 默认 bind port+1; 或显式 listen.mnps_port
		-- mnps_port = 12819,
	},
	-- 多监听器 (M4): 附加口; 与主 listen 同 schema; 口/mnps_port 须唯一
	-- listens = {
	-- 	{ id = "http-only", host = "127.0.0.1", port = 12828,
	-- 	  protocols = { "http-connect" } },
	-- 	{ id = "socks-only", host = "127.0.0.1", port = 12829,
	-- 	  protocols = { "socks5" } },
	-- },
	auth = {
		mode = "none",
		-- mode = "userpass", user = "alice", pass = "secret",
		-- mode = "userpass", users = { { user = "a", pass = "1" }, { user = "b", pass = "2" } },
		-- mode = "bearer", token = "shared-token",
	},
	-- 全阶段超时 (ms); 省略则用 defaults
	-- timeouts = {
	-- 	connect_ms = 15000,
	-- 	probe_ms = 5000,
	-- 	idle_ms = 100000,
	-- 	poll_ms = 40,
	-- 	accept_ms = 200,
	-- 	hop_idle_ms = 3600000, -- proxy↔alua 暖会话空闲自断 (1h; 0=关池)
	-- },
	-- hop_pool = { max = 2 }, -- mnp/smp 鉴权暖会话槽
	-- security = {
	-- 	allow_from = { "127.0.0.1", "192.0.2.0/24" },
	-- 	quiet_reject = true,
	-- 	auth_fail = { max = 8, window_ms = 60000, ban_ms = 300000 },
	-- },
	-- check = {
	-- 	probe_targets = { { host = "example.com", port = 80 } },
	-- 	latency = { ok_ms = 500, slow_ms = 2000 },
	-- },
	no_proxy = { "127.0.0.1", "localhost", ".local" },
	-- chains key = 随机 6 位 id; 角色用 default/debug 引用
	-- 临时切链: alua call proxy.use {"chain":"debug"} (入口重启回 default)
	-- 旧写法 chains.default 会在 load 时自动迁到随机 id
	-- default = "a1b2c3",
	-- debug = "d4e5f6",
	chains = {
		-- a1b2c3 = {
		-- 	{ type = "socks5", host = "127.0.0.1", port = 1080 },
		-- },
		-- alua↔alua: mnps=TLS+默认CRYPTO; 亦可 mnp+crypto={format=XOR|CHACHA20|...}
		-- d4e5f6 = {
		-- 	{ type = "mnps", host = "127.0.0.1", port = 12819, token = "shared-token" },
		-- },
		-- e7f8g9 = {
		-- 	{ type = "http-connect", host = "proxy.corp", port = 8080 },
		-- },
	},
	routes = {
		-- { match = ".corp.example", chain = "e7f8g9" },
		-- { match = "default", chain = "default" }, -- chain 可用角色名, 解析到 default 字段
	},
	-- 目的地策略 (默认 blacklist + 空 deny = 全部代理; 同站 deny>allow)
	-- policy = {
	-- 	mode = "blacklist", -- 或 whitelist
	-- 	deny = { ".local", "intranet.example" }, -- 黑: 直连 (deny_action)
	-- 	allow = {}, -- 白模式才用
	-- 	deny_action = "direct", -- 或 reject
	-- 	miss_action = "direct", -- 白模式未命中 allow
	-- },
}

-- end
