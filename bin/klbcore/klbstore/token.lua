--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
--
-- @file   token.lua
-- @brief  klbstore 上传/下载令牌签发与校验 (HMAC-SHA256)
-- @stability experimental
-- @impl evolving
-- @note   令牌形如 `base64url(payload) "." hex(hmac)`; payload 为 JSON
--   \n payload: `{ s=作用域, f=文件标识, o=操作, e=过期毫秒 }`
--   \n 失败形状: nil, code; code 见 klbcode (INVAL / ACCES)
--]]


local cjson = require("cjson.safe")

local klbcode = require("klbcore.klbcode")
local base = klbcode.base


local token = {}


--------------------------------------------------------------------------------------------
-- SHA-256 / HMAC

local K = {
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
}


local function rrotate(x, n)
	return ((x >> n) | (x << (32 - n))) & 0xffffffff
end


-- @brief SHA-256; 返回 32 字节二进制
local function sha256_bin(msg)
	local bytes = { string.byte(msg, 1, #msg) }
	local bit_len = #msg * 8

	bytes[#bytes + 1] = 0x80
	while 56 ~= (#bytes % 64) do
		bytes[#bytes + 1] = 0
	end

	local hi = (bit_len >> 32) & 0xffffffff
	local lo = bit_len & 0xffffffff
	for i = 3, 0, -1 do
		bytes[#bytes + 1] = (hi >> (8 * i)) & 0xff
	end
	for i = 3, 0, -1 do
		bytes[#bytes + 1] = (lo >> (8 * i)) & 0xff
	end

	local h0 = 0x6a09e667
	local h1 = 0xbb67ae85
	local h2 = 0x3c6ef372
	local h3 = 0xa54ff53a
	local h4 = 0x510e527f
	local h5 = 0x9b05688c
	local h6 = 0x1f83d9ab
	local h7 = 0x5be0cd19

	local off = 1
	while off <= #bytes do
		local w = {}
		for i = 0, 15 do
			local j = off + i * 4
			w[i] = ((bytes[j] << 24) | (bytes[j + 1] << 16) | (bytes[j + 2] << 8) | bytes[j + 3]) & 0xffffffff
		end

		for i = 16, 63 do
			local s0 = rrotate(w[i - 15], 7) ~ rrotate(w[i - 15], 18) ~ (w[i - 15] >> 3)
			local s1 = rrotate(w[i - 2], 17) ~ rrotate(w[i - 2], 19) ~ (w[i - 2] >> 10)
			w[i] = (w[i - 16] + s0 + w[i - 7] + s1) & 0xffffffff
		end

		local a, b, c, d, e, f, g, h = h0, h1, h2, h3, h4, h5, h6, h7
		for i = 0, 63 do
			local S1 = rrotate(e, 6) ~ rrotate(e, 11) ~ rrotate(e, 25)
			local ch = (e & f) ~ ((~e) & g)
			local t1 = (h + S1 + ch + K[i + 1] + w[i]) & 0xffffffff
			local S0 = rrotate(a, 2) ~ rrotate(a, 13) ~ rrotate(a, 22)
			local maj = (a & b) ~ (a & c) ~ (b & c)
			local t2 = (S0 + maj) & 0xffffffff

			h = g
			g = f
			f = e
			e = (d + t1) & 0xffffffff
			d = c
			c = b
			b = a
			a = (t1 + t2) & 0xffffffff
		end

		h0 = (h0 + a) & 0xffffffff
		h1 = (h1 + b) & 0xffffffff
		h2 = (h2 + c) & 0xffffffff
		h3 = (h3 + d) & 0xffffffff
		h4 = (h4 + e) & 0xffffffff
		h5 = (h5 + f) & 0xffffffff
		h6 = (h6 + g) & 0xffffffff
		h7 = (h7 + h) & 0xffffffff

		off = off + 64
	end

	local out = {}
	for _, x in ipairs({ h0, h1, h2, h3, h4, h5, h6, h7 }) do
		out[#out + 1] = string.char((x >> 24) & 0xff, (x >> 16) & 0xff, (x >> 8) & 0xff, x & 0xff)
	end

	return table.concat(out)
end


-- @brief HMAC-SHA256; 返回 32 字节二进制
local function hmac_sha256(key, msg)
	local bs = 64
	if bs < #key then
		key = sha256_bin(key)
	end

	key = key .. string.rep("\0", bs - #key)

	local ipad = {}
	local opad = {}
	for i = 1, bs do
		local b = string.byte(key, i)
		ipad[i] = string.char(b ~ 0x36)
		opad[i] = string.char(b ~ 0x5c)
	end

	return sha256_bin(table.concat(opad) .. sha256_bin(table.concat(ipad) .. msg))
end


--------------------------------------------------------------------------------------------
-- base64url / hex

local B64URL = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"


local function b64url_encode(s)
	local t = {}
	local n = #s
	local i = 1

	while i <= n do
		local remain = n - i + 1
		local a = string.byte(s, i)
		local b = 0
		local c = 0

		if 2 <= remain then
			b = string.byte(s, i + 1)
		end

		if 3 <= remain then
			c = string.byte(s, i + 2)
		end

		local n2 = ((a & 3) << 4) | (b >> 4)
		local n3 = ((b & 15) << 2) | (c >> 6)

		t[#t + 1] = string.sub(B64URL, (a >> 2) + 1, (a >> 2) + 1)
		t[#t + 1] = string.sub(B64URL, n2 + 1, n2 + 1)

		if 2 == remain then
			t[#t + 1] = string.sub(B64URL, n3 + 1, n3 + 1)
		elseif 3 <= remain then
			t[#t + 1] = string.sub(B64URL, n3 + 1, n3 + 1)
			t[#t + 1] = string.sub(B64URL, (c & 63) + 1, (c & 63) + 1)
		end

		i = i + 3
	end

	return table.concat(t)
end


-- @brief base64url 解码; 非法为 nil
local function b64url_decode(s)
	if "string" ~= type(s) then
		return nil
	end

	local map = {}
	for i = 1, #B64URL do
		map[string.sub(B64URL, i, i)] = i - 1
	end

	local out = {}
	local buf = 0
	local bits = 0

	for i = 1, #s do
		local ch = string.sub(s, i, i)
		if "=" == ch then
			break
		end

		local v = map[ch]
		if nil == v then
			return nil
		end

		buf = (buf << 6) | v
		bits = bits + 6
		if 8 <= bits then
			bits = bits - 8
			out[#out + 1] = string.char((buf >> bits) & 0xff)
		end
	end

	return table.concat(out)
end


local function hex(s)
	return (string.gsub(s, ".", function (c)
		return string.format("%02x", string.byte(c))
	end))
end


--------------------------------------------------------------------------------------------
-- 令牌

local function now_ms()
	return os.time() * 1000
end


-- @brief 常量时间比较
local function bool_eq(a, b)
	if "string" ~= type(a) or "string" ~= type(b) then
		return false
	end

	local la = #a
	local lb = #b
	local n = la
	if lb > n then
		n = lb
	end

	local diff = la ~ lb
	for i = 1, n do
		local ca = (i <= la) and string.byte(a, i) or 0
		local cb = (i <= lb) and string.byte(b, i) or 0
		diff = diff | (ca ~ cb)
	end

	return 0 == diff
end


-- @brief 签发令牌
-- @param [in]  secret[string]			签名密钥; 非空
-- @param [in]  claims[table]			`{ scope, file_id, op, ttl_ms }`; 均可选
-- @param [in]  ttl_ms[number|nil]		有效期毫秒; 缺省用 claims.ttl_ms, 再缺省 1 小时
-- @return [string] 令牌; 失败 nil
-- @return [number|nil] 失败时 base.INVAL
-- @stability experimental
token.sign = function (secret, claims, ttl_ms)
	if "string" ~= type(secret) or "" == secret then
		return nil, base.INVAL
	end

	if "table" ~= type(claims) then
		claims = {}
	end

	local ttl = tonumber(ttl_ms) or tonumber(claims.ttl_ms) or 3600000
	if 0 >= ttl then
		return nil, base.INVAL
	end

	local payload = {
		s = claims.scope,
		f = claims.file_id,
		o = claims.op,
		e = now_ms() + ttl,
	}

	local body = b64url_encode(cjson.encode(payload))
	if "string" ~= type(body) or "" == body then
		return nil, base.INVAL
	end

	return body .. "." .. hex(hmac_sha256(secret, body))
end


-- @brief 校验令牌
-- @param [in]  secret[string]			签名密钥; 非空
-- @param [in]  tok[string]				令牌
-- @param [in]  opts[table|nil]			`{ scope, op }` 可选附加约束
-- @return [table] payload `{ s, f, o, e }`; 失败 nil
-- @return [number|nil] 失败时 base.ACCES / E_ARG
-- @stability experimental
token.verify = function (secret, tok, opts)
	if "string" ~= type(secret) or "" == secret then
		return nil, base.INVAL
	end

	if "string" ~= type(tok) or "" == tok then
		return nil, base.ACCES
	end

	local body, sig = string.match(tok, "^([^.]+)%.([0-9a-fA-F]+)$")
	if nil == body or nil == sig then
		return nil, base.ACCES
	end

	if not bool_eq(string.lower(sig), hex(hmac_sha256(secret, body))) then
		return nil, base.ACCES
	end

	local text = b64url_decode(body)
	if "string" ~= type(text) or "" == text then
		return nil, base.ACCES
	end

	local payload = cjson.decode(text)
	if "table" ~= type(payload) then
		return nil, base.ACCES
	end

	local exp = tonumber(payload.e)
	if nil == exp or exp <= now_ms() then
		return nil, base.ACCES
	end

	if "table" == type(opts) then
		if nil ~= opts.scope and opts.scope ~= payload.s then
			return nil, base.ACCES
		end

		if nil ~= opts.op and opts.op ~= payload.o then
			return nil, base.ACCES
		end
	end

	return payload
end


return token

-- end
