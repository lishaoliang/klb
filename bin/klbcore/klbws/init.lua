--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbws init.lua
-- @note   WebSocket 协议脚本封装; 内部使用 kws
--   \n 服务端心跳默认开 (C 30s ping / 10s 超时); Lua 侧暂无开关; 客户端无自动心跳
--   \n 不协商 Sec-WebSocket-Extensions (含 permessage-deflate); RSV 非 0 会 1002 拆连
--   \n 单条接收硬顶 1MiB; 超限 1009 拆连, Lua 仅见 close
-- @history 修改历史
--		[2026-09] 创建文件
--		[2026-10] is_upgrade / switching_protocols / adopt (HTTP Upgrade 细节归本层)
--		[2026-10] accept_tcp / as_byte_stream (proxy L0-10)
--]]
local kws = require("kws")
local kproxy_io = require("klbcore.klbproxy.io")
local wsclienter = require("klbcore.klbws.client.wsclienter")
local wsclientrpcer = require("klbcore.klbws.client.wsclientrpcer")
local wslistener = require("klbcore.klbws.serve.wslistener")
local wsserver = require("klbcore.klbws.serve.wsserver")


local klbws = {}


--------------------------------------------------------------------------------------------
-- HTTP Upgrade 辅助 (脚本层细节; 产品只调本 API)


local function path_only(target)
	if type(target) ~= "string" or target == "" then
		return "/"
	end

	local q = string.find(target, "?", 1, true)
	if q ~= nil then
		return string.sub(target, 1, q - 1)
	end

	return target
end


local function parse_headers(hdr)
	local headers = {}
	for line in string.gmatch(hdr, "[^\r\n]+") do
		local k, v = string.match(line, "^([^:]+):%s*(.*)$")
		if k ~= nil then
			headers[string.lower(k)] = v
		end
	end
	return headers
end


-- @brief 是否 WebSocket HTTP Upgrade 请求
-- @param [in]  hdr[string]		完整 HTTP 请求头 (含请求行)
-- @return [boolean] ok
-- @return [table|nil] info		`{ path, key, protocol, headers }`
klbws.is_upgrade = function (hdr)
	if type(hdr) ~= "string" or hdr == "" then
		return false, nil
	end

	local first = string.match(hdr, "^([^\r\n]+)")
	if first == nil then
		return false, nil
	end

	local method, target = string.match(first, "^(%S+)%s+(%S+)")
	if method == nil or string.upper(method) ~= "GET" then
		return false, nil
	end

	local headers = parse_headers(hdr)
	local upgrade = headers["upgrade"] or ""
	if string.lower(upgrade) ~= "websocket" then
		return false, nil
	end

	local key = headers["sec-websocket-key"]
	if type(key) ~= "string" or key == "" then
		return false, nil
	end

	local protocol = headers["sec-websocket-protocol"] or ""

	return true, {
		path = path_only(target),
		key = key,
		protocol = protocol,
		headers = headers,
	}
end


-- @brief 组装 101 Switching Protocols 应答原文
-- @param [in]  key[string]			Sec-WebSocket-Key
-- @param [in]  opts[table|nil]		`{ protocol[string] }`
-- @return [string|nil] rsp			失败为 nil
klbws.switching_protocols = function (key, opts)
	if type(key) ~= "string" or key == "" then
		return nil
	end

	local accept = kws.accept(key)
	if type(accept) ~= "string" or accept == "" then
		return nil
	end

	local lines = {
		"HTTP/1.1 101 Switching Protocols",
		"Upgrade: websocket",
		"Connection: Upgrade",
		"Sec-WebSocket-Accept: " .. accept,
	}

	if type(opts) == "table" and type(opts.protocol) == "string" and opts.protocol ~= "" then
		lines[#lines + 1] = "Sec-WebSocket-Protocol: " .. opts.protocol
	end

	lines[#lines + 1] = ""
	return table.concat(lines, "\r\n") .. "\r\n"
end


-- @brief 由已回 101 的 listen_socket 接管为 WS 服务连接
-- @param [in]  listen_sock[lightuserdata]	kws.adopt 同源 (klb_netlisten_socket_t*)
-- @param [in]  opts[table|nil]				透传 kws.adopt (ping_*)
-- @return [table|nil] serve				wsserver; 失败 nil
-- @return [number|nil] code
klbws.adopt = function (listen_sock, opts)
	if listen_sock == nil or type(kws.adopt) ~= "function" then
		return nil, 1
	end

	local raw, code = kws.adopt(listen_sock, opts)
	if raw == nil then
		return nil, code
	end

	return wsserver.new(raw), nil
end


-- @brief 将 wsserver 包装为字节流 (send/recv; leftover 统一)
-- @param [in]  serve[table]	wsserver (send_binary / co_recv)
-- @return [table|nil] stream
klbws.as_byte_stream = function (serve)
	if serve == nil or type(serve.send_binary) ~= "function" or type(serve.co_recv) ~= "function" then
		return nil
	end

	local data_leftover = ""
	local stream = {
		kind = "ws-session",
		_serve = serve,
		_ws = serve,
	}

	stream.idle_touch_ms = function ()
		local s = stream._serve
		if s ~= nil and type(s.last_recv_ms) == "function" then
			local ok, v = pcall(function ()
				return s:last_recv_ms()
			end)
			if ok and type(v) == "number" and v > 0 then
				return v
			end
		end
		return stream._idle_touch_ms
	end

	stream.close = function ()
		if serve ~= nil and type(serve.disconnect) == "function" then
			pcall(function ()
				serve:disconnect()
			end)
		end
		stream._serve = nil
	end

	stream.send = function (_, data)
		if stream._serve == nil then
			return false, "closed"
		end
		local rc = stream._serve:send_binary(data or "")
		return 0 == rc
	end

	stream.recv = function (_, max_n)
		if stream._serve == nil then
			return nil, "closed"
		end

		max_n = tonumber(max_n) or 4096
		if #data_leftover > 0 then
			local out = data_leftover
			if max_n > 0 and #out > max_n then
				data_leftover = string.sub(out, max_n + 1)
				return string.sub(out, 1, max_n)
			end
			data_leftover = ""
			return out
		end

		for _ = 1, 32 do
			local msg, head, body = stream._serve:co_recv()
			if msg == "binary" or msg == "text" then
				body = body or ""
				if max_n > 0 and #body > max_n then
					data_leftover = string.sub(body, max_n + 1)
					body = string.sub(body, 1, max_n)
				end
				return body
			elseif msg == "open" or msg == "pong" then
				-- C 吞心跳
			elseif msg == "close" or msg == "error" or msg == "exit" then
				return nil, tostring(msg)
			end
			_ = head
		end

		return nil, "timeout"
	end

	return stream
end


-- @brief kproxy.tcp: 发 101 + steal + adopt → 字节流
-- @param [in]  sock[userdata]	kproxy.tcp (须 steal_listen_socket)
-- @param [in]  key[string]		Sec-WebSocket-Key
-- @param [in]  opts[table|nil]	`{ protocol }` 透传 switching_protocols / adopt
-- @return [table|nil] stream
-- @return [string|nil] err
klbws.accept_tcp = function (sock, key, opts)
	if sock == nil then
		return nil, "need sock"
	end
	if type(key) ~= "string" or key == "" then
		return nil, "need key"
	end
	if type(sock.steal_listen_socket) ~= "function" then
		return nil, "need kproxy.tcp steal_listen_socket"
	end

	local rsp = klbws.switching_protocols(key, opts)
	if type(rsp) ~= "string" or rsp == "" then
		return nil, "switching_protocols fail"
	end

	local ok_s = kproxy_io.send(sock, rsp)
	if not ok_s then
		return nil, "101 send fail"
	end

	local listen_sock, err_st = sock:steal_listen_socket()
	if listen_sock == nil then
		return nil, tostring(err_st or "steal fail")
	end

	local serve, code = klbws.adopt(listen_sock, opts)
	if serve == nil then
		return nil, "klbws.adopt fail code=" .. tostring(code)
	end

	local stream = klbws.as_byte_stream(serve)
	if stream == nil then
		return nil, "as_byte_stream fail"
	end

	return stream, nil
end


--------------------------------------------------------------------------------------------
-- 客户端

-- @brief 新建一个客户端
-- @param [in]      cfg[table]			客户端配置
-- @return [table]	客户端模块
klbws.new_client = function (cfg)
	return wsclienter.new(cfg)
end


-- @brief 连接到目标
-- @param [in]      host[string]		主机
-- @param [in]      port[number(int)]	端口
-- @param [in]      path[string]		[可选] 握手路径, 默认 `/`; 也可为 opts table
-- @param [in]      opts[table]			[可选] 连接选项; 见内联 opts 注释
-- @return [table]	客户端模块
klbws.connect = function (host, port, path, opts)
	-- opts = {
	--   tls[boolean]			[可选] 是否 TLS, 默认 `false`
	--   protocol[string]		[可选] 子协议 Sec-WebSocket-Protocol
	--   headers[table]		[可选] 额外握手头, 键值均为 string
	-- }
	local client = wsclienter.new()

	local ret, code = client:connect(host, port, path, opts)
	if 0 ~= ret then
		return nil, code
	end

	return client
end


--------------------------------------------------------------------------------------------
-- 服务端

-- @brief 新建一个监听模块
-- @param [in]      cfg[table]			监听模块配置
-- @return [table]	监听模块
klbws.new_listen = function (cfg)
	return wslistener.new(cfg)
end


-- @brief 开始 监听模块
-- @param [in]      port[number(int)]	端口
-- @param [in]      cfg[table]			[可选]监听模块配置; 见内联 opts 注释
-- @return [table]	监听模块; 失败为 nil
-- @return [number]	失败时 klb_netcode_e
klbws.listen = function (port, cfg)
	-- cfg = {
	--   tls[boolean]			[可选] 是否 TLS, 默认 `false`
	--   cert[string]			tls 时必填, 证书 PEM
	--   key[string]			tls 时必填, 私钥 PEM
	--   path[string]			[可选] 只接受该路径, 忽略 query
	--   protocol[string]		[可选] 选定的子协议
	-- }
	local l = wslistener.new(cfg)
	local ok, code = l:open(port)
	if not ok then
		return nil, code
	end

	return l
end


-- @brief 由 Sec-WebSocket-Key 计算 Sec-WebSocket-Accept
-- @param [in]      key[string]			客户端 Key
-- @return [string]	Accept; 失败为空串
klbws.accept_key = function (key)
	if "string" ~= type(key) or "" == key then
		return ""
	end

	local accept = kws.accept(key)
	if "string" ~= type(accept) then
		return ""
	end

	return accept
end


-- @brief 把已写完 101 的 HTTP 服务连接升为 WebSocket
-- @param [in]      http_serve[table]	klbhttp 服务连接, 须有 :upgrade
-- @return [table]	WebSocket 服务连接; 失败为 nil
-- @note 调用前 101 须已发完. 成功后不要再使用原 HTTP 连接
klbws.from_http = function (http_serve)
	if "table" ~= type(http_serve) or "function" ~= type(http_serve.upgrade) then
		return nil
	end

	local conn = http_serve:upgrade()
	if not conn then
		return nil
	end

	return wsserver.new(conn)
end


--------------------------------------------------------------------------------------------
-- 客户端 RPC

-- @brief 新建一个RPC客户端
-- @param [in]      cfg[table]			客户端配置
-- @return [table]	客户端模块
klbws.new_client_rpc = function (cfg)
	return wsclientrpcer.new_rpc(cfg)
end

-- @brief 连接到目标
-- @param [in]      host[string]		主机
-- @param [in]      port[number(int)]	端口
-- @param [in]      opts[table]			[可选] `{ tls, path, protocol }`; path 省略为 `/rpc`
-- @return [table]	客户端模块; 连接失败为 nil
klbws.connect_rpc = function (host, port, opts)
	local client = wsclientrpcer.new_rpc()

	if 0 ~= client:connect_rpc(host, port, opts) then
		return nil
	end

	return client
end


--------------------------------------------------------------------------------------------
-- 客户端 RPC 一次性接口

-- @brief 发起一次 POST-RPC
-- @param [in]      url[string]			目标, eg. 'wsrpc://127.0.0.1:3459/rpc'
-- @return [string] 消息
klbws.co_post_rpc = function (url, ...)
	local r = wsclientrpcer.new_rpc()
	if 0 ~= r:connect(url) then
		return
	end

	r:post(...)
	r:co_wait()
	r:disconnect()
	return 'success'
end

-- @brief 发起一次 NOTIFY-RPC
-- @param [in]      url[string]			目标, eg. 'wsrpc://127.0.0.1:3459/rpc'
-- @return [string] 消息
klbws.co_notify_rpc = function (url, ...)
	local r = wsclientrpcer.new_rpc()
	if 0 ~= r:connect(url) then
		return
	end

	r:notify(...)
	r:co_wait()
	r:disconnect()
	return 'success'
end

-- @brief 发起一次 CALL-RPC
-- @param [in]      url[string]			目标, eg. 'wsrpc://127.0.0.1:3459/rpc'
-- @return [string] 消息
--			[...] 回应数据
klbws.co_call_rpc = function (url, ...)
	local ReturnCoCall = function (r, ...)
		r:disconnect()
		return 'success', ...
	end

	local r = wsclientrpcer.new_rpc()
	if 0 ~= r:connect(url) then
		return
	end

	return ReturnCoCall(r, r:co_call(...))
end

--------------------------------------------------------------------------------------------
-- 服务端 RPC


-- @brief 新建一个监听模块
-- @param [in]      cfg[table]			监听模块配置
-- @return [table]	监听模块
klbws.new_listen_rpc = function (cfg)
	return wslistener.new_rpc(cfg)
end


-- @brief 开始 监听模块
-- @param [in]      port[number(int)]	端口
-- @param [in]      cfg[table]			[可选]监听模块配置; tls/cert/key/path/protocol/rpctype
-- @return [table]	监听模块; 打开失败为 nil
-- @return [number]	失败时 klb_netcode_e
klbws.listen_rpc = function (port, cfg)
	local l = wslistener.new_rpc(cfg)
	local ok, code = l:open(port)
	if not ok then
		return nil, code
	end

	return l
end


--------------------------------------------------------------------------------------------
-- 一些常量

-------------------------------------------------------
-- 数据组织方式
klbws.RPC_LUA			= 'LUA'
klbws.RPC_JSON			= 'JSON'


-------------------------------------------------------
-- RPC 方法
klbws.RPC_POST 			= 'POST'
klbws.RPC_NOTIFY 		= 'NOTIFY'
klbws.RPC_REQUEST 		= 'REQUEST'
klbws.RPC_RESPONSE 		= 'RESPONSE'


return klbws
