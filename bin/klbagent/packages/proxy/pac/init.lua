--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   pac.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  简易 PAC 生成 (B-3)
-- @note   输出 FindProxyForURL; 默认指本机入口 SOCKS5+HTTP
-- @history 修改历史
--  \n [2026-10] 创建文件 (P3-1)
--]]


local config = require("klbagent.packages.proxy.config")


local M = {}


local function _listen_of(opts)
	local host = "127.0.0.1"
	local port = 12818
	local cfg = config.defaults()
	if type(cfg) == "table" and type(cfg.listen) == "table" then
		host = cfg.listen.host or host
		port = tonumber(cfg.listen.port) or port
	end

	if type(opts) == "table" then
		if type(opts.host) == "string" and opts.host ~= "" then
			host = opts.host
		end
		if opts.port ~= nil then
			port = tonumber(opts.port) or port
		end
		if type(opts.listen) == "table" then
			if type(opts.listen.host) == "string" and opts.listen.host ~= "" then
				host = opts.listen.host
			end
			if opts.listen.port ~= nil then
				port = tonumber(opts.listen.port) or port
			end
		end
	end

	return host, port
end


-- @brief 生成 PAC 脚本文本
-- @param [in]  opts[table|nil]	`{ host, port, listen, prefer[string] }`
--				prefer: `socks5`(默认) / `http` / `socks5-only` / `http-only`
-- @return [string] pac
-- @return [string|nil] err
M.generate = function (opts)
	if opts ~= nil and type(opts) ~= "table" then
		return nil, "bad opts"
	end

	local host, port = _listen_of(opts)
	local prefer = "socks5"
	if type(opts) == "table" and type(opts.prefer) == "string" and opts.prefer ~= "" then
		prefer = string.lower(opts.prefer)
	end

	local proxy_line = nil
	if prefer == "http" or prefer == "http-connect" or prefer == "http-only" then
		proxy_line = string.format("PROXY %s:%d", host, port)
		if prefer ~= "http-only" then
			proxy_line = proxy_line .. string.format("; SOCKS5 %s:%d; SOCKS %s:%d", host, port, host, port)
		end
	elseif prefer == "socks5-only" or prefer == "socks-only" then
		proxy_line = string.format("SOCKS5 %s:%d; SOCKS %s:%d", host, port, host, port)
	else
		-- socks5 优先, 回落 HTTP CONNECT
		proxy_line = string.format(
			"SOCKS5 %s:%d; SOCKS %s:%d; PROXY %s:%d",
			host, port, host, port, host, port
		)
	end

	local lines = {
		"// alua proxy PAC (packages/proxy/pac/init.lua)",
		"// Point browser PAC URL to this file, or paste into file:// / http:// hosting.",
		"function FindProxyForURL(url, host) {",
		"  if (isPlainHostName(host) ||",
		"      shExpMatch(host, \"localhost\") ||",
		"      shExpMatch(host, \"127.*\") ||",
		"      shExpMatch(host, \"10.*\") ||",
		"      shExpMatch(host, \"192.168.*\") ||",
		"      shExpMatch(host, \"172.16.*\") ||",
		"      shExpMatch(host, \"172.17.*\") ||",
		"      shExpMatch(host, \"172.18.*\") ||",
		"      shExpMatch(host, \"172.19.*\") ||",
		"      shExpMatch(host, \"172.2?.*\") ||",
		"      shExpMatch(host, \"172.30.*\") ||",
		"      shExpMatch(host, \"172.31.*\") ||",
		"      dnsDomainIs(host, \".local\"))",
		"    return \"DIRECT\";",
		string.format("  return \"%s\";", proxy_line),
		"}",
		"",
	}

	return table.concat(lines, "\n"), nil
end


return M

-- end
