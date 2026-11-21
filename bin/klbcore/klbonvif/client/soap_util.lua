--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   soap_util.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  ONVIF SOAP 信封 / UsernameToken / 轻量 XML 解析
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local krand = require("krand")


local soap_util = {}


--------------------------------------------------------------------------------------------
-- base64 / sha1 (PasswordDigest)


local b64chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"


local b64enc = function (s)
	local t = {}
	local n = string.len(s)
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

		t[#t + 1] = string.sub(b64chars, (a >> 2) + 1, (a >> 2) + 1)
		t[#t + 1] = string.sub(b64chars, n2 + 1, n2 + 1)

		if 1 == remain then
			t[#t + 1] = "="
			t[#t + 1] = "="
		elseif 2 == remain then
			t[#t + 1] = string.sub(b64chars, n3 + 1, n3 + 1)
			t[#t + 1] = "="
		else
			t[#t + 1] = string.sub(b64chars, n3 + 1, n3 + 1)
			t[#t + 1] = string.sub(b64chars, (c & 63) + 1, (c & 63) + 1)
		end

		i = i + 3
	end

	return table.concat(t)
end


-- @brief Base64 解码; 非法字符返回 nil
local b64dec = function (s)
	if "string" ~= type(s) then
		return nil
	end

	local map = {}
	for i = 1, #b64chars do
		map[string.sub(b64chars, i, i)] = i - 1
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


local function rotl(x, n)
	local v = x & 0xffffffff
	return ((v << n) | (v >> (32 - n))) & 0xffffffff
end


-- @brief SHA-1, 返回 20 字节二进制
local sha1_bin = function (msg)
	local h0 = 0x67452301
	local h1 = 0xEFCDAB89
	local h2 = 0x98BADCFE
	local h3 = 0x10325476
	local h4 = 0xC3D2E1F0

	local bytes = { string.byte(msg, 1, string.len(msg)) }
	local bit_len = string.len(msg) * 8

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

	local off = 1
	while off <= #bytes do
		local w = {}
		for i = 0, 15 do
			local j = off + i * 4
			w[i] = ((bytes[j] << 24) | (bytes[j + 1] << 16) | (bytes[j + 2] << 8) | bytes[j + 3]) & 0xffffffff
		end

		for i = 16, 79 do
			w[i] = rotl(w[i - 3] ~ w[i - 8] ~ w[i - 14] ~ w[i - 16], 1)
		end

		local a, b, c, d, e = h0, h1, h2, h3, h4
		for i = 0, 79 do
			local f, k
			if 19 >= i then
				f = (b & c) | ((~b) & d)
				k = 0x5A827999
			elseif 39 >= i then
				f = b ~ c ~ d
				k = 0x6ED9EBA1
			elseif 59 >= i then
				f = (b & c) | (b & d) | (c & d)
				k = 0x8F1BBCDC
			else
				f = b ~ c ~ d
				k = 0xCA62C1D6
			end

			local temp = (rotl(a, 5) + (f & 0xffffffff) + e + k + w[i]) & 0xffffffff
			e = d
			d = c
			c = rotl(b, 30)
			b = a
			a = temp
		end

		h0 = (h0 + a) & 0xffffffff
		h1 = (h1 + b) & 0xffffffff
		h2 = (h2 + c) & 0xffffffff
		h3 = (h3 + d) & 0xffffffff
		h4 = (h4 + e) & 0xffffffff
		off = off + 64
	end

	local out = {}
	for _, h in ipairs({ h0, h1, h2, h3, h4 }) do
		out[#out + 1] = string.char((h >> 24) & 0xff, (h >> 16) & 0xff, (h >> 8) & 0xff, h & 0xff)
	end

	return table.concat(out)
end


--------------------------------------------------------------------------------------------
-- MD5 / HTTP Digest

local MD5_S = {
	7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
	5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
	4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
	6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21,
}

local MD5_K = {
	0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
	0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
	0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
	0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
	0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
	0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
	0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
	0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
}


local function lrotl(x, n)
	local v = x & 0xffffffff
	return ((v << n) | (v >> (32 - n))) & 0xffffffff
end


-- @brief MD5; 返回 16 字节二进制
local function md5_bin(msg)
	local bytes = { string.byte(msg, 1, #msg) }
	local bit_len = #msg * 8

	bytes[#bytes + 1] = 0x80
	while 56 ~= (#bytes % 64) do
		bytes[#bytes + 1] = 0
	end

	for i = 0, 3 do
		bytes[#bytes + 1] = (bit_len >> (8 * i)) & 0xff
	end
	for i = 1, 4 do
		bytes[#bytes + 1] = 0
	end

	local a0 = 0x67452301
	local b0 = 0xefcdab89
	local c0 = 0x98badcfe
	local d0 = 0x10325476

	local off = 1
	while off <= #bytes do
		local m = {}
		for i = 0, 15 do
			local j = off + i * 4
			m[i] = (bytes[j] | (bytes[j + 1] << 8) | (bytes[j + 2] << 16) | (bytes[j + 3] << 24)) & 0xffffffff
		end

		local a, b, c, d = a0, b0, c0, d0
		for i = 0, 63 do
			local f
			local g
			if 15 >= i then
				f = (b & c) | ((~b) & d)
				g = i
			elseif 31 >= i then
				f = (d & b) | ((~d) & c)
				g = (5 * i + 1) % 16
			elseif 47 >= i then
				f = b ~ c ~ d
				g = (3 * i + 5) % 16
			else
				f = c ~ (b | (~d))
				g = (7 * i) % 16
			end

			local tmp = d
			d = c
			c = b
			local sum = (a + f + MD5_K[i + 1] + m[g]) & 0xffffffff
			b = (b + lrotl(sum, MD5_S[i + 1])) & 0xffffffff
			a = tmp
		end

		a0 = (a0 + a) & 0xffffffff
		b0 = (b0 + b) & 0xffffffff
		c0 = (c0 + c) & 0xffffffff
		d0 = (d0 + d) & 0xffffffff

		off = off + 64
	end

	local out = {}
	for _, x in ipairs({ a0, b0, c0, d0 }) do
		out[#out + 1] = string.char(x & 0xff, (x >> 8) & 0xff, (x >> 16) & 0xff, (x >> 24) & 0xff)
	end

	return table.concat(out)
end


-- @brief MD5 十六进制小写
soap_util.md5_hex = function (s)
	return (string.gsub(md5_bin(tostring(s or "")), ".", function (c)
		return string.format("%02x", string.byte(c))
	end))
end


-- @brief 解析 WWW-Authenticate: Digest 质询
-- @param [in]  head[string]			HTTP 头文本
-- @return [table|nil]				键小写的质询表; 无 Digest 为 nil
soap_util.parse_digest_challenge = function (head)
	if "string" ~= type(head) then
		return nil
	end

	local line = string.match(head, "WWW%-[Aa]uthenticate:%s*[Dd]igest%s+([^\r\n]+)")
	if nil == line then
		line = string.match(head, "www%-authenticate:%s*[Dd]igest%s+([^\r\n]+)")
	end

	if nil == line then
		return nil
	end

	local out = {}
	for k, v in string.gmatch(line, "([%w%-_]+)%s*=%s*\"([^\"]*)\"") do
		out[string.lower(k)] = v
	end

	for k in string.gmatch(line, "([%w%-_]+)%s*=%s*[^\"%s,]") do
		out[string.lower(k)] = true
	end

	return out
end


-- @brief 组装 HTTP Digest Authorization 头值
-- @param [in]  method[string]			"POST" 等
-- @param [in]  uri[string]			请求目标 (path + query)
-- @param [in]  user[string]
-- @param [in]  password[string]
-- @param [in]  challenge[table]		parse_digest_challenge 结果
-- @return [string|nil]				头值; 质询非法为 nil
soap_util.digest_authorization = function (method, uri, user, password, challenge)
	if "table" ~= type(challenge) or "string" ~= type(challenge.realm) or "string" ~= type(challenge.nonce) then
		return nil
	end

	local realm = challenge.realm
	local nonce = challenge.nonce
	local qop = challenge.qop
	local ha1 = soap_util.md5_hex(user .. ":" .. realm .. ":" .. password)
	local ha2 = soap_util.md5_hex(method .. ":" .. uri)

	local parts = {
		'username="' .. user .. '"',
		'realm="' .. realm .. '"',
		'nonce="' .. nonce .. '"',
		'uri="' .. uri .. '"',
	}

	if "string" == type(qop) and nil ~= string.find(qop, "auth", 1, true) then
		local nc = "00000001"
		local cnonce = soap_util.md5_hex(tostring(os.time()) .. ":" .. tostring(krand.rand(0x7fffffff)))
		local response = soap_util.md5_hex(ha1 .. ":" .. nonce .. ":" .. nc .. ":" .. cnonce .. ":auth:" .. ha2)
		parts[#parts + 1] = 'response="' .. response .. '"'
		parts[#parts + 1] = "qop=auth"
		parts[#parts + 1] = "nc=" .. nc
		parts[#parts + 1] = 'cnonce="' .. cnonce .. '"'
	else
		parts[#parts + 1] = 'response="' .. soap_util.md5_hex(ha1 .. ":" .. nonce .. ":" .. ha2) .. '"'
	end

	if "string" == type(challenge.opaque) then
		parts[#parts + 1] = 'opaque="' .. challenge.opaque .. '"'
	end

	parts[#parts + 1] = "algorithm=MD5"

	return "Digest " .. table.concat(parts, ", ")
end


--------------------------------------------------------------------------------------------
-- XML helpers


soap_util.escape = function (s)
	if "string" ~= type(s) then
		return ""
	end

	s = string.gsub(s, "&", "&amp;")
	s = string.gsub(s, "<", "&lt;")
	s = string.gsub(s, ">", "&gt;")
	s = string.gsub(s, "\"", "&quot;")
	return s
end


soap_util.find_tag_text = function (xml, tag)
	if "string" ~= type(xml) or "string" ~= type(tag) then
		return nil
	end

	local pat = "<[%w%-]*:?" .. tag .. "[^>]*>([^<]*)</[%w%-]*:?" .. tag .. ">"
	return string.match(xml, pat)
end


soap_util.find_all_tag_text = function (xml, tag)
	local list = {}
	if "string" ~= type(xml) or "string" ~= type(tag) then
		return list
	end

	local pat = "<[%w%-]*:?" .. tag .. "[^>]*>([^<]*)</[%w%-]*:?" .. tag .. ">"
	for v in string.gmatch(xml, pat) do
		list[#list + 1] = v
	end

	return list
end


--------------------------------------------------------------------------------------------
-- Envelope / Security


-- @brief 组装 UsernameToken Security 头片段
-- @param [in]      user[string]
-- @param [in]      password[string]
-- @param [in]      mode[string|nil]	`"digest"` (默认) 或 `"text"` (PasswordText)
-- @return [string]	Header 内 Security 块; 无凭证时 ""
soap_util.build_security_header = function (user, password, mode)
	if "string" ~= type(user) or "" == user then
		return ""
	end

	if "string" ~= type(password) then
		password = ""
	end

	local nonce_raw = krand.rand_string(16)
	local created = os.date("!%Y-%m-%dT%H:%M:%SZ")
	local nonce_b64 = b64enc(nonce_raw)

	local ptype = "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-username-token-profile-1.0#PasswordDigest"
	local pval = b64enc(sha1_bin(nonce_raw .. created .. password))
	if "text" == mode then
		ptype = "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-username-token-profile-1.0#PasswordText"
		pval = soap_util.escape(password)
	end

	return table.concat({
		'<wsse:Security s:mustUnderstand="1"',
		' xmlns:wsse="http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd"',
		' xmlns:wsu="http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd">',
		"<wsse:UsernameToken>",
		"<wsse:Username>", soap_util.escape(user), "</wsse:Username>",
		'<wsse:Password Type="', ptype, '">',
		pval, "</wsse:Password>",
		'<wsse:Nonce EncodingType="http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-soap-message-security-1.0#Base64Binary">',
		nonce_b64, "</wsse:Nonce>",
		"<wsu:Created>", created, "</wsu:Created>",
		"</wsse:UsernameToken>",
		"</wsse:Security>",
	})
end


-- @brief 解析报文中的 UsernameToken
-- @param [in]  xml[string]
-- @return [table|nil]	`{ user, password, mode="digest"|"text", nonce, created }`; 无凭证为 nil
soap_util.parse_security = function (xml)
	if "string" ~= type(xml) then
		return nil
	end

	local user = soap_util.find_tag_text(xml, "Username")
	if nil == user or "" == user then
		return nil
	end

	local attrs, password = string.match(xml,
		"<[%w%-]*:?Password([^>]*)>([^<]*)</[%w%-]*:?Password>")

	local mode = "text"
	if "string" == type(attrs) and nil ~= string.find(attrs, "PasswordDigest", 1, true) then
		mode = "digest"
	end

	return {
		user = user,
		password = password or "",
		mode = mode,
		nonce = soap_util.find_tag_text(xml, "Nonce") or "",
		created = soap_util.find_tag_text(xml, "Created") or "",
	}
end


-- @brief 校验 UsernameToken
-- @param [in]  xml[string]				请求报文
-- @param [in]  auth[table|nil]			`{ user, password }`; user 为空表示不鉴权
-- @return ok[boolean]
-- @return [string]						失败原因
soap_util.verify_security = function (xml, auth)
	if "table" ~= type(auth) or "string" ~= type(auth.user) or "" == auth.user then
		return true
	end

	local sec = soap_util.parse_security(xml)
	if nil == sec then
		return false, "no security header"
	end

	if sec.user ~= auth.user then
		return false, "unknown user"
	end

	local pass = tostring(auth.password or "")

	if "digest" == sec.mode then
		local nonce = b64dec(sec.nonce)
		if nil == nonce then
			return false, "bad nonce"
		end

		local expect = b64enc(sha1_bin(nonce .. sec.created .. pass))
		if expect == sec.password then
			return true
		end

		return false, "bad digest"
	end

	if pass == sec.password then
		return true
	end

	return false, "bad password"
end


-- @brief 组装 SOAP 信封
-- @param [in]      body_inner[string]	Body 内层 XML (含操作元素)
-- @param [in]      opts[table]			[可选] user / password / mode / extra_ns
-- @return [string]
soap_util.build_envelope = function (body_inner, opts)
	opts = opts or {}
	local security = soap_util.build_security_header(opts.user, opts.password, opts.mode)
	local extra_ns = opts.extra_ns or ""

	return table.concat({
		'<?xml version="1.0" encoding="UTF-8"?>',
		'<s:Envelope xmlns:s="http://www.w3.org/2003/05/soap-envelope"',
		' xmlns:tds="http://www.onvif.org/ver10/device/wsdl"',
		' xmlns:trt="http://www.onvif.org/ver10/media/wsdl"',
		' xmlns:tt="http://www.onvif.org/ver10/schema"',
		extra_ns, ">",
		"  <s:Header>",
		security,
		"  </s:Header>",
		"  <s:Body>",
		body_inner or "",
		"  </s:Body>",
		"</s:Envelope>",
	})
end


-- @brief 是否 SOAP Fault (按 Fault 元素判定, 非任意子串)
soap_util.is_fault = function (xml)
	if "string" ~= type(xml) then
		return false
	end

	return nil ~= string.find(xml, "<[%w%-]*:?Fault[%s>]")
end


-- @brief 结构化解析 SOAP Fault
-- @param [in]      xml[string]			响应 XML
-- @return [table|nil]	`{ code, subcode, reason, detail }`; 非 Fault 为 nil
soap_util.fault_of = function (xml)
	if not soap_util.is_fault(xml) then
		return nil
	end

	local code = soap_util.find_tag_text(xml, "Value") or ""
	local subcode = ""
	local sub_block = soap_util.find_tag_text(xml, "Subcode")
	if "string" == type(sub_block) and "" ~= sub_block then
		subcode = soap_util.find_tag_text(sub_block, "Value") or ""
	end

	local reason = soap_util.find_tag_text(xml, "Text") or soap_util.find_tag_text(xml, "Reason") or ""
	local detail = soap_util.find_tag_text(xml, "Detail") or ""

	return {
		code = code,
		subcode = subcode,
		reason = reason,
		detail = detail,
	}
end


return soap_util
