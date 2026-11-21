--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua-tunnel 帧编解码 (优先 ktunnel; Lua 回退)
-- @note   头 6 字节: magic0 magic1 ver type len_be16; 其后 payload
-- @history 修改历史
--  \n [2026-10] 创建文件 (N3-1); N3-2 T_DATA / try_decode; wire 下沉 ktunnel
--]]


local kproxy_io = require("klbcore.klbproxy.io")


local M = {}


local function _load_wire()
	local ok, mod = pcall(require, "ktunnel")
	if ok and type(mod) == "table" and type(mod.encode) == "function" then
		return mod
	end
	return nil
end


local W = _load_wire()


M.MAGIC0 = (W and W.MAGIC0) or 0xA7
M.MAGIC1 = (W and W.MAGIC1) or 0x01
M.VER = (W and W.VER) or 0x01
M.PAYLOAD_MAX = 65535

M.T_HELLO = (W and W.T_HELLO) or 0x01
M.T_HELLO_OK = (W and W.T_HELLO_OK) or 0x02
M.T_HELLO_NAK = (W and W.T_HELLO_NAK) or 0x03
M.T_CONNECT = (W and W.T_CONNECT) or 0x10
M.T_CONNECT_OK = (W and W.T_CONNECT_OK) or 0x11
M.T_CONNECT_NAK = (W and W.T_CONNECT_NAK) or 0x12
M.T_DATA = (W and W.T_DATA) or 0x20


local function _byte(n)
	return string.char((tonumber(n) or 0) % 256)
end


local function _u16be(n)
	n = tonumber(n) or 0
	if n < 0 then
		n = 0
	end
	if n > 65535 then
		n = 65535
	end
	return string.char(math.floor(n / 256) % 256, n % 256)
end


local function _recv_exact(sock, n, prefix)
	local buf = prefix or ""
	while #buf < n do
		local chunk, err = kproxy_io.recv(sock, n - #buf)
		if type(chunk) ~= "string" or #chunk == 0 then
			return nil, err or "eof"
		end
		buf = buf .. chunk
	end
	return buf
end


-- @brief 编码一帧
-- @param [in]  typ[number]
-- @param [in]  payload[string|nil]
-- @return [string|nil] wire; 超 PAYLOAD_MAX 时 nil
-- @return [string|nil] err
M.encode = function (typ, payload)
	payload = payload or ""
	if type(payload) ~= "string" then
		payload = tostring(payload)
	end

	if #payload > M.PAYLOAD_MAX then
		return nil, "payload too large"
	end

	if W ~= nil then
		local out, err = W.encode(typ, payload)
		if type(out) == "string" then
			return out
		end
		return nil, tostring(err or "encode fail")
	end

	return _byte(M.MAGIC0) .. _byte(M.MAGIC1) .. _byte(M.VER) .. _byte(typ)
		.. _u16be(#payload) .. payload
end


-- @brief 解码一帧
-- @param [in]  sock[any]		send/recv
-- @param [in]  prefix[string|nil]	已读前缀 (可含半帧)
-- @return [boolean] ok
-- @return [table|string] `{ type, payload, rest }` 或 err
M.decode = function (sock, prefix)
	local hdr, err = _recv_exact(sock, 6, prefix)
	if hdr == nil then
		return false, err or "hdr fail"
	end

	if string.byte(hdr, 1) ~= M.MAGIC0 or string.byte(hdr, 2) ~= M.MAGIC1 then
		return false, "bad magic"
	end

	local ver = string.byte(hdr, 3)
	if ver ~= M.VER then
		return false, "bad ver"
	end

	local typ = string.byte(hdr, 4)
	local len = string.byte(hdr, 5) * 256 + string.byte(hdr, 6)
	local payload = ""
	if len > 0 then
		payload, err = _recv_exact(sock, len, "")
		if payload == nil then
			return false, err or "payload fail"
		end
	end

	return true, { type = typ, payload = payload, ver = ver }
end


-- @brief 从缓冲尝试解一帧 (不读 sock)
-- @param [in]  buf[string]
-- @return [boolean] ok		true=完整帧; false=不足或坏帧
-- @return [table|string] msg 或 err (`short` 表示继续等)
-- @return [string] rest	剩余缓冲
M.try_decode = function (buf)
	if W ~= nil then
		return W.try_decode(buf or "")
	end

	buf = buf or ""
	if #buf < 6 then
		return false, "short", buf
	end

	if string.byte(buf, 1) ~= M.MAGIC0 or string.byte(buf, 2) ~= M.MAGIC1 then
		return false, "bad magic", buf
	end

	local ver = string.byte(buf, 3)
	if ver ~= M.VER then
		return false, "bad ver", buf
	end

	local typ = string.byte(buf, 4)
	local len = string.byte(buf, 5) * 256 + string.byte(buf, 6)
	if #buf < 6 + len then
		return false, "short", buf
	end

	local payload = ""
	if len > 0 then
		payload = string.sub(buf, 7, 6 + len)
	end

	local rest = ""
	if #buf > 6 + len then
		rest = string.sub(buf, 7 + len)
	end

	return true, { type = typ, payload = payload, ver = ver }, rest
end


-- @brief HELLO payload: token 字符串 (可空)
M.encode_hello = function (token)
	if W ~= nil and type(W.encode_hello) == "function" then
		local out, err = W.encode_hello(token or "")
		if type(out) == "string" then
			return out
		end
		return nil, tostring(err or "encode fail")
	end
	return M.encode(M.T_HELLO, token or "")
end


-- @brief CONNECT payload: host_len(1) + host + port_be16
M.encode_connect = function (host, port)
	if W ~= nil and type(W.encode_connect) == "function" then
		local out = W.encode_connect(host, port)
		if type(out) == "string" then
			return out
		end
	end

	host = tostring(host or "")
	if #host > 255 then
		host = string.sub(host, 1, 255)
	end
	return M.encode(M.T_CONNECT, _byte(#host) .. host .. _u16be(port))
end


-- @brief 解析 CONNECT payload
-- @return host, port, err
M.parse_connect_payload = function (payload)
	if W ~= nil and type(W.parse_connect_payload) == "function" then
		return W.parse_connect_payload(payload)
	end

	if type(payload) ~= "string" or #payload < 3 then
		return nil, nil, "bad connect payload"
	end

	local n = string.byte(payload, 1)
	if #payload < 1 + n + 2 then
		return nil, nil, "short connect payload"
	end

	local host = string.sub(payload, 2, 1 + n)
	local port = string.byte(payload, 2 + n) * 256 + string.byte(payload, 3 + n)
	if host == "" then
		return nil, nil, "empty host"
	end

	return host, port, nil
end


return M

-- end
