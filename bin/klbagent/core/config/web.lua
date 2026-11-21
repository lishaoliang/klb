--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   web.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua web 配置页非密配置模块 (web.json)
-- @note   listen=端口组; 每项 port + tls/plain (+cert/key); 默认 8080 同端口 HTTP+HTTPS
--   \n 端口组形态由 klbweb cfger 合并; 同端口多项合并为 tls+plain
--   \n acl=黑白名单 (allow / deny CIDR + default); 规则规范见 config.web_acl
-- @history 修改历史
--  \n [2026-10] 创建文件 (桩); 端口组逐项 normalize; 默认落定 8080 双协议
--]]
local store = require("klbagent.core.config.store")
local web_acl = require("klbagent.core.config.web_acl")


local M = {}


M.FILE = "web.json"


-- 协议标记 → 端口项 (CLI 与页面共用; 键为小写)
M.PROTO = {
	http = { tls = false, plain = true },
	https = { tls = true, plain = false },
	["http+https"] = { tls = true, plain = true },
	both = { tls = true, plain = true },
	["plain+tls"] = { tls = true, plain = true },
}


-- @brief 单个端口项规范
-- @param [in] src[table]
-- @return item[table|nil]			非法为 nil
local function _item(src)
	if type(src) ~= "table" then
		return nil
	end

	local port = tonumber(src.port)
	if port == nil or port < 1 or 65535 < port then
		return nil
	end

	port = math.floor(port)

	local tls = src.tls == true
	local plain = src.plain == true
	if not tls and not plain then
		plain = true		-- 两项都未给按 HTTP
	end

	local cert = src.cert
	if type(cert) ~= "string" then
		cert = ""
	end

	local key = src.key
	if type(key) ~= "string" then
		key = ""
	end

	return {
		port = port,		-- 端口
		tls = tls,			-- HTTPS
		plain = plain,		-- HTTP
		cert = cert,		-- 证书 (空=复用 serve TLS)
		key = key,			-- 私钥 (空=复用 serve TLS)
	}
end


-- @brief 缺省
-- @return cfg[table]
M.defaults = function ()
	return {
		addr = "127.0.0.1",		-- 绑定地址; 局域网须显式 0.0.0.0
		listen = {				-- 端口组; 每项独立协议面
			_item({ port = 8080, tls = true, plain = true, cert = "", key = "" }),
			-- {}, ...			同结构项
		},
		acl = web_acl.defaults(),	-- 黑白名单; 默认全放行
	}
end


-- @brief 规范: 只保留已知键; 非法端口项丢弃, 全空回默认
-- @param [in] cfg[table]
-- @return cfg[table]
M.normalize = function (cfg)
	local def = M.defaults()
	if type(cfg) ~= "table" then
		return def
	end

	local addr = cfg.addr
	if type(addr) ~= "string" or addr == "" then
		addr = def.addr
	end

	local out = {}
	local src = cfg.listen
	if type(src) == "table" then
		for i = 1, #src do
			local one = _item(src[i])
			if one ~= nil then
				out[#out + 1] = one
			end
		end
	end

	if #out == 0 then
		out = def.listen
	end

	return {
		addr = addr,
		listen = out,
		acl = web_acl.normalize(cfg.acl),
	}
end


-- @brief 构造端口项 (协议标记; 非法为 nil)
-- @param [in] port[number]
-- @param [in] proto[string]
-- @param [in] cert[string]
-- @param [in] key[string]
-- @return item[table|nil]
M.item = function (port, proto, cert, key)
	local p = M.PROTO[string.lower(tostring(proto or "http+https"))]
	if p == nil then
		return nil
	end

	return _item({
		port = port,
		tls = p.tls,
		plain = p.plain,
		cert = cert or "",
		key = key or "",
	})
end


-- @brief 配置文件路径
-- @return path[string]
M.path = function ()
	return store.path(M.FILE)
end


-- @brief 读取 web.json
-- @return cfg[table]
M.load = function ()
	return store.load(M)
end


-- @brief 写入 web.json
-- @param [in] cfg[table]
-- @return ok[boolean]
M.save = function (cfg)
	return store.save(M, cfg)
end


return M

-- end
