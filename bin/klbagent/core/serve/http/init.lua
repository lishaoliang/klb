--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua serve/http 子模块: v1 路由挂载 (/v1/health /v1/call /v1/ws /mcp)
-- @note   处理函数按入口分文件: call.lua / health.lua / ws.lua
--   \n MCP Streamable HTTP 由 packages/mcp/http 挂载, 门禁回调在本文件 (本机 Host 放行免 TLS)
--   \n @stability internal: 实现文件, 契约面见 core/serve/init.lua
-- @history 修改历史
--  \n [2026-11] 创建文件; 从 core/serve/init.lua 拆出 _mount_v1_routes
--]]
local auth = require("klbagent.core.serve.security.auth")
local call = require("klbagent.core.serve.http.call")
local health = require("klbagent.core.serve.http.health")
local ws = require("klbagent.core.serve.http.ws")


local M = {}


-- @brief 挂载 v1 路由到 klbweb app
-- @param [in] app[table]				klbweb app
M.mount_v1_routes = function (app)
	app:get("/v1/health", health.handle_health)
	app:post("/v1/call", call.handle_call)

	-- P1: WS 同源 token + 文本 envelope (与 /v1/call 同)
	app:ws("/v1/ws", function(req, res, next)
		if not auth.token_ok(req) then
			res:co_status(401, "text/plain", "bad token")
			return
		end

		next()
	end, function(req, socket)
		ws.handle_ws(req, socket)
	end)

	-- P4-1: MCP Streamable HTTP (POST/GET/DELETE /mcp; 同口 Bearer token)
	do
		local ok_m, mcp = pcall(require, "klbagent.packages.mcp")
		local mcp_http = nil
		if ok_m and type(mcp) == "table" and type(mcp.http) == "function" then
			local ok_h, http_mod = pcall(mcp.http)
			if ok_h and type(http_mod) == "table" then
				mcp_http = http_mod
			end
		end

		if type(mcp_http) == "table" and type(mcp_http.mount) == "function" then
			mcp_http.mount(app, function (req, res)
				local headers = req and req.headers
				local tok = auth.token_from_headers(headers)
				local sid = auth.ssid_from_headers(headers)
				-- 本机 Host 放行 raw token 免 TLS, 便于宿主 url=http://127.0.0.1:<PORT>/mcp
				local host = ""
				if type(headers) == "table" then
					host = headers["host"] or headers["Host"] or ""
				end

				host = string.lower(tostring(host))
				local loopback = string.match(host, "^127%.0%.0%.1") ~= nil
					or string.match(host, "^localhost") ~= nil
					or string.match(host, "^%[::1%]") ~= nil
				local tls_hint = loopback and true or auth.tls_of(req)
				local info, err_v, err_code = auth.verify_token(tok, tls_hint, sid)
				if info == nil then
					auth.deny_http(res, err_v, err_code)
					return false
				end

				-- 只验门禁, 不 bind raw: MCP 面业务走 ssid begin_ctx(skip_token)
				return true
			end)
		end
	end
end


return M

-- end
