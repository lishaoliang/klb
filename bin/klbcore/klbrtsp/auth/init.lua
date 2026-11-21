--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @brief  RTSP Basic / Digest (MD5, RFC 2617)
-- @note   内部实现; 供 client / serve 使用
-- @history 修改历史
--		[2026-09] 添加
--]]
local stringex = require("klbcore.util.stringex")

local rtspauth = {}


local b64chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'
local b64map = {}

for i = 1, string.len(b64chars) do
	b64map[string.sub(b64chars, i, i)] = i - 1
end


local S = {
	7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
	5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
	4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
	6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21,
}

local K = {
	0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
	0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
	0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
	0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
	0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
	0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
	0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
	0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
	0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
	0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
	0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
	0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
	0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
	0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
	0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
	0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
}


local function bnot(x)
	return (~x) & 0xffffffff
end


local function rotl(x, n)
	local v = x & 0xffffffff
	return ((v << n) | (v >> (32 - n))) & 0xffffffff
end


local function md5_hex(s)
	local bytes = { string.byte(s, 1, string.len(s)) }
	local bit_len = string.len(s) * 8
	local a0 = 0x67452301
	local b0 = 0xefcdab89
	local c0 = 0x98badcfe
	local d0 = 0x10325476
	local lo = bit_len & 0xffffffff
	local hi = (bit_len >> 32) & 0xffffffff
	local off = 1

	bytes[#bytes + 1] = 0x80

	while 56 ~= (#bytes % 64) do
		bytes[#bytes + 1] = 0
	end

	for i = 0, 3 do
		bytes[#bytes + 1] = (lo >> (8 * i)) & 0xff
	end

	for i = 0, 3 do
		bytes[#bytes + 1] = (hi >> (8 * i)) & 0xff
	end

	while off <= #bytes do
		local M = {}
		local A = a0
		local B = b0
		local C = c0
		local D = d0

		for i = 0, 15 do
			local o = off + i * 4
			M[i] = bytes[o] | (bytes[o + 1] << 8) | (bytes[o + 2] << 16) | (bytes[o + 3] << 24)
		end

		for i = 0, 63 do
			local F = 0
			local g = 0
			local sum = 0

			if 15 >= i then
				F = (B & C) | (bnot(B) & D)
				g = i
			elseif 31 >= i then
				F = (D & B) | (bnot(D) & C)
				g = (5 * i + 1) % 16
			elseif 47 >= i then
				F = B ~ C ~ D
				g = (3 * i + 5) % 16
			else
				F = C ~ (B | bnot(D))
				g = (7 * i) % 16
			end

			F = F & 0xffffffff
			sum = (A + F + K[i + 1] + M[g]) & 0xffffffff
			A = D
			D = C
			C = B
			B = (B + rotl(sum, S[i + 1])) & 0xffffffff
		end

		a0 = (a0 + A) & 0xffffffff
		b0 = (b0 + B) & 0xffffffff
		c0 = (c0 + C) & 0xffffffff
		d0 = (d0 + D) & 0xffffffff
		off = off + 64
	end

	local function le(x)
		return string.format('%02x%02x%02x%02x', x & 0xff, (x >> 8) & 0xff, (x >> 16) & 0xff, (x >> 24) & 0xff)
	end

	return le(a0) .. le(b0) .. le(c0) .. le(d0)
end


local function b64enc(s)
	local t = {}
	local n = string.len(s)
	local i = 1

	while i <= n do
		local remain = n - i + 1
		local a = string.byte(s, i)
		local b = 0
		local c = 0
		local n2 = 0
		local n3 = 0

		if 2 <= remain then
			b = string.byte(s, i + 1)
		end

		if 3 <= remain then
			c = string.byte(s, i + 2)
		end

		n2 = ((a & 3) << 4) | (b >> 4)
		n3 = ((b & 15) << 2) | (c >> 6)

		t[#t + 1] = string.sub(b64chars, (a >> 2) + 1, (a >> 2) + 1)
		t[#t + 1] = string.sub(b64chars, n2 + 1, n2 + 1)

		if 1 == remain then
			t[#t + 1] = '='
			t[#t + 1] = '='
		elseif 2 == remain then
			t[#t + 1] = string.sub(b64chars, n3 + 1, n3 + 1)
			t[#t + 1] = '='
		else
			t[#t + 1] = string.sub(b64chars, n3 + 1, n3 + 1)
			t[#t + 1] = string.sub(b64chars, ((c & 63) + 1), ((c & 63) + 1))
		end

		i = i + 3
	end

	return table.concat(t)
end


local function b64dec(s)
	local t = {}
	local n = string.len(s)
	local i = 1

	while i <= n do
		local c1 = string.sub(s, i, i)
		local c2 = ''
		local c3 = ''
		local c4 = ''
		local n1 = nil
		local n2 = nil
		local n3 = nil
		local n4 = nil

		if '' == c1 or '=' == c1 then
			break
		end

		c2 = string.sub(s, i + 1, i + 1)
		c3 = string.sub(s, i + 2, i + 2)
		c4 = string.sub(s, i + 3, i + 3)
		n1 = b64map[c1]
		n2 = b64map[c2]

		if nil == n1 or nil == n2 then
			return ''
		end

		t[#t + 1] = string.char((n1 << 2) | (n2 >> 4))

		if '' ~= c3 and '=' ~= c3 then
			n3 = b64map[c3]
			if nil == n3 then
				return ''
			end

			t[#t + 1] = string.char(((n2 & 15) << 4) | (n3 >> 2))

			if '' ~= c4 and '=' ~= c4 then
				n4 = b64map[c4]
				if nil == n4 then
					return ''
				end

				t[#t + 1] = string.char(((n3 & 3) << 6) | n4)
			end
		end

		i = i + 4
	end

	return table.concat(t)
end


local function qesc(s)
	return (string.gsub(s, '([\\"])', '\\%1'))
end


local function parse_params(s)
	local t = {}
	local i = 1
	local n = string.len(s)

	while i <= n do
		local ch = string.sub(s, i, i)
		local eq = nil
		local key = ''
		local val = ''

		while i <= n do
			ch = string.sub(s, i, i)
			if ' ' ~= ch and ',' ~= ch and '\t' ~= ch then
				break
			end
			i = i + 1
		end

		if i > n then
			break
		end

		eq = string.find(s, '=', i, true)
		if nil == eq then
			break
		end

		key = string.lower(stringex.trim(string.sub(s, i, eq - 1)))
		i = eq + 1

		while i <= n and ' ' == string.sub(s, i, i) do
			i = i + 1
		end

		if '"' == string.sub(s, i, i) then
			local buf = {}
			i = i + 1
			while i <= n do
				ch = string.sub(s, i, i)
				if '\\' == ch and i < n then
					buf[#buf + 1] = string.sub(s, i + 1, i + 1)
					i = i + 2
				elseif '"' == ch then
					i = i + 1
					break
				else
					buf[#buf + 1] = ch
					i = i + 1
				end
			end
			val = table.concat(buf)
		else
			local comma = string.find(s, ',', i, true)
			if nil == comma then
				val = stringex.trim(string.sub(s, i))
				i = n + 1
			else
				val = stringex.trim(string.sub(s, i, comma - 1))
				i = comma + 1
			end
		end

		if '' ~= key then
			t[key] = val
		end
	end

	return t
end


local function split_scheme(s)
	local scheme, rest = string.match(s or '', '^%s*(%a[%w%-]*)%s+(.*)$')
	if nil == scheme then
		return '', ''
	end
	return string.lower(scheme), rest
end


local function want_qop(qop)
	if 'string' ~= type(qop) or '' == qop then
		return false
	end

	for item in string.gmatch(qop, '[^,]+') do
		if 'auth' == string.lower(stringex.trim(item)) then
			return true
		end
	end

	return false
end


local function hexeq(a, b)
	return string.lower(a or '') == string.lower(b or '')
end


-- 由 Authorization 头取认证方案: 'basic' / 'digest' / nil
rtspauth.scheme_of = function (authorization)
	if 'string' ~= type(authorization) or '' == authorization then
		return nil
	end

	local scheme = split_scheme(authorization)

	if 'basic' == scheme or 'digest' == scheme then
		return scheme
	end

	return nil
end


-- 生成带时间戳的 nonce: <8 位十六进制时间><随机串>; 供服务端轮换
rtspauth.gen_nonce = function (rand_hex)
	local t = os.time() % 0x100000000
	return string.format('%08x%s', t, rand_hex or '')
end


-- 校验 nonce 时效(秒); 过期或格式非法返回 false
rtspauth.nonce_valid = function (nonce, ttl_sec)
	if 'string' ~= type(nonce) or string.len(nonce) < 8 then
		return false
	end

	local t = tonumber(string.sub(nonce, 1, 8), 16)

	if nil == t then
		return false
	end

	if (os.time() - t) > (ttl_sec or 300) then
		return false
	end

	return true
end


-- 新建 nc 重放防护上下文(nc 须单调递增)
rtspauth.new_nc_ctx = function ()
	return { nc = 0 }
end


-- 校验 nc 单调递增(防重放); 合法则记录并返回 true. ctx 为 nil 时不拦
rtspauth.check_nc = function (ctx, nc_hex)
	if 'table' ~= type(ctx) then
		return true
	end

	local nc = tonumber(nc_hex or '', 16)

	if nil == nc then
		return false
	end

	if nc <= (ctx.nc or 0) then
		return false
	end

	ctx.nc = nc
	return true
end


-- 由 WWW-Authenticate 生成 Authorization 头的值. 无用户名时返回 nil
rtspauth.authorization = function (method, uri, user, pass, www, ctx)
	local scheme, rest = '', ''
	local p = nil

	if 'string' ~= type(user) or '' == user then
		return nil
	end

	if 'string' ~= type(www) or '' == www then
		return nil
	end

	if 'table' ~= type(ctx) then
		ctx = {}
	end

	scheme, rest = split_scheme(www)

	if 'basic' == scheme then
		return 'Basic ' .. b64enc(user .. ':' .. (pass or ''))
	end

	if 'digest' ~= scheme then
		return nil
	end

	p = parse_params(rest)

	if 'md5' ~= string.lower(p.algorithm or 'md5') then
		return nil
	end

	local realm = p.realm or ''
	local nonce = p.nonce or ''
	local ha1 = md5_hex(user .. ':' .. realm .. ':' .. (pass or ''))
	local ha2 = md5_hex(method .. ':' .. uri)
	local resp = ''
	local line = ''

	if want_qop(p.qop) then
		local nc = ''
		local cnonce = ctx.cnonce

		ctx.auth_nc = (ctx.auth_nc or 0) + 1
		nc = string.format('%08x', ctx.auth_nc)

		if 'string' ~= type(cnonce) or '' == cnonce then
			cnonce = string.format('%08x%08x', ctx.auth_nc, os.time() % 0x100000000)
		end

		resp = md5_hex(ha1 .. ':' .. nonce .. ':' .. nc .. ':' .. cnonce .. ':auth:' .. ha2)
		line = string.format(
			'Digest username="%s", realm="%s", nonce="%s", uri="%s", response="%s", algorithm=MD5, qop=auth, nc=%s, cnonce="%s"',
			qesc(user), qesc(realm), qesc(nonce), qesc(uri), resp, nc, qesc(cnonce))
	else
		resp = md5_hex(ha1 .. ':' .. nonce .. ':' .. ha2)
		line = string.format(
			'Digest username="%s", realm="%s", nonce="%s", uri="%s", response="%s", algorithm=MD5',
			qesc(user), qesc(realm), qesc(nonce), qesc(uri), resp)
	end

	return line
end


-- 校验 Authorization. 通过返回 true
-- nc_ctx 为 nc 重放防护上下文(可为 nil); qop=auth 时校验 nc 单调递增
rtspauth.check = function (method, uri, user, pass, realm, nonce, authorization, nc_ctx)
	local scheme, rest = '', ''
	local p = nil
	local ha1 = ''
	local ha2 = ''
	local expect = ''

	if 'string' ~= type(authorization) or '' == authorization then
		return false
	end

	scheme, rest = split_scheme(authorization)

	if 'basic' == scheme then
		return b64dec(stringex.trim(rest)) == (user .. ':' .. (pass or ''))
	end

	if 'digest' ~= scheme then
		return false
	end

	p = parse_params(rest)

	if p.username ~= user then
		return false
	end

	if (p.realm or '') ~= (realm or '') then
		return false
	end

	if (p.nonce or '') ~= (nonce or '') then
		return false
	end

	if (p.uri or '') ~= (uri or '') then
		return false
	end

	if 'md5' ~= string.lower(p.algorithm or 'md5') then
		return false
	end

	ha1 = md5_hex(user .. ':' .. (realm or '') .. ':' .. (pass or ''))
	ha2 = md5_hex(method .. ':' .. (uri or ''))

	if 'auth' == string.lower(p.qop or '') then
		expect = md5_hex(ha1 .. ':' .. (nonce or '') .. ':' .. (p.nc or '') .. ':' .. (p.cnonce or '') .. ':auth:' .. ha2)

		if not hexeq(expect, p.response) then
			return false
		end

		-- qop=auth: 校验 nc 单调递增, 拒绝重放
		return rtspauth.check_nc(nc_ctx, p.nc)
	elseif nil ~= p.qop and '' ~= p.qop then
		return false
	else
		expect = md5_hex(ha1 .. ':' .. (nonce or '') .. ':' .. ha2)
	end

	return hexeq(expect, p.response)
end


return rtspauth
