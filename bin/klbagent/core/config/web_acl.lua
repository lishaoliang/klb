--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   web_acl.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua web 配置页黑白名单 (web.json acl) 规则规范与对端 IP 判定
-- @note   规则项为 IPv4 / IPv6 单 IP 或 CIDR; 判定次序 deny 优先
--   \n 只认 socket 对端地址, 不信任 X-Forwarded-For
-- @history 修改历史
--  \n [2026-10] 创建文件 (P5 黑白名单)
--]]


local M = {}


M.DEFAULT = "allow"


-- 已编译规则缓存 (规则文本 → 解析结果)
local _cache = {}


-- @brief IPv4 点分 → 32 位数值
-- @param [in] text[string]
-- @return num[number|nil]			非法为 nil
local function _v4_num(text)
	local a, b, c, d = string.match(text, "^(%d+)%.(%d+)%.(%d+)%.(%d+)$")
	if a == nil then
		return nil
	end

	a, b, c, d = tonumber(a), tonumber(b), tonumber(c), tonumber(d)
	if 255 < a or 255 < b or 255 < c or 255 < d then
		return nil
	end

	return a * 16777216 + b * 65536 + c * 256 + d
end


-- @brief 冒号组 → 数值数组 ("::" 空段跳过)
-- @param [in] part[string]
-- @return list[table|nil]			非法为 nil
local function _v6_groups(part)
	local list = {}
	if part == nil or part == "" then
		return list
	end

	for g in string.gmatch(part, "([^:]+)") do
		local n = tonumber(g, 16)
		if n == nil or 65535 < n then
			return nil
		end

		list[#list + 1] = n
	end

	return list
end


-- @brief IPv6 文本 → 16 字节串 (支持 :: 压缩与末尾内嵌 IPv4)
-- @param [in] text[string]
-- @return bytes[string|nil]			16 字节; 非法为 nil
-- @return mapped4[number|nil]		v4 映射地址的 v4 数值 (非映射为 nil)
local function _v6_bytes(text)
	if string.find(text, ":", 1, true) == nil then
		return nil
	end

	-- 去 zone id (fe80::1%eth0)
	local s = string.match(text, "^([^%%]*)")
	if s == nil or s == "" then
		return nil
	end

	-- 末尾内嵌 IPv4 (::ffff:1.2.3.4)
	local head, v4 = string.match(s, "^(.*:)(%d+%.%d+%.%d+%.%d+)$")
	if head ~= nil then
		local n = _v4_num(v4)
		if n == nil then
			return nil
		end

		s = head .. string.format("%x:%x", math.floor(n / 65536), n % 65536)
	end

	local dbl = string.find(s, "::", 1, true)
	local left, right
	if dbl ~= nil then
		left = string.sub(s, 1, dbl - 1)
		right = string.sub(s, dbl + 2)
	else
		left = s
	end

	local l = _v6_groups(left)
	local r = _v6_groups(right)
	if l == nil or r == nil then
		return nil
	end

	local total = #l + #r
	if dbl ~= nil then
		if 7 < total then
			return nil
		end
	elseif total ~= 8 then
		return nil
	end

	local nums = {}
	for i = 1, #l do
		nums[#nums + 1] = l[i]
	end

	if dbl ~= nil then
		for _ = 1, 8 - total do
			nums[#nums + 1] = 0
		end
	end

	for i = 1, #r do
		nums[#nums + 1] = r[i]
	end

	local bytes = {}
	for i = 1, 8 do
		local v = nums[i]
		bytes[#bytes + 1] = string.char(math.floor(v / 256), v % 256)
	end

	local out = table.concat(bytes)
	local mapped = nil
	if string.sub(out, 1, 12) == string.rep("\0", 10) .. "\255\255" then
		local hi, lo = string.byte(out, 13), string.byte(out, 14)
		local c, d = string.byte(out, 15), string.byte(out, 16)
		mapped = hi * 16777216 + lo * 65536 + c * 256 + d
	end

	return out, mapped
end


-- @brief IP 文本规范化解析
-- @param [in] text[string]
-- @return kind[string]				"v4" / "v6"; 非法为 ""
-- @return val[number|string]		v4 为 32 位数值; v6 为 16 字节串
-- @return mapped4[number|nil]		v4 映射地址的 v4 数值
local function _ip_of(text)
	if type(text) ~= "string" then
		return "", nil, nil
	end

	local s = string.match(text, "^%s*(.-)%s*$")
	if s == nil or s == "" then
		return "", nil, nil
	end

	local v4 = _v4_num(s)
	if v4 ~= nil then
		return "v4", v4, nil
	end

	local bytes, mapped = _v6_bytes(s)
	if bytes ~= nil then
		return "v6", bytes, mapped
	end

	return "", nil, nil
end


-- @brief 规则项规范 ("ip" / "ip/prefix") 并编译
-- @param [in] text[string]
-- @return rule[table|nil]			{ text, kind, val, bits }; 非法为 nil
local function _rule_of(text)
	if type(text) ~= "string" then
		return nil
	end

	local s = string.match(text, "^%s*(.-)%s*$")
	if s == nil or s == "" then
		return nil
	end

	local addr, bits_s = string.match(s, "^([^/]+)/(%d+)$")
	if addr == nil then
		addr = s
	end

	local kind, val = _ip_of(addr)
	if kind == "" then
		return nil
	end

	local max = (kind == "v4") and 32 or 128
	local bits = max
	if bits_s ~= nil then
		bits = tonumber(bits_s)
		if bits == nil or bits > max then
			return nil
		end
	end

	-- 规范文本: 满前缀省略 "/max" (a.b.c.d 与 a.b.c.d/32 视为同一规则)
	local text = addr
	if bits < max then
		text = addr .. "/" .. bits
	end

	return {
		text = text,		-- 规范规则文本 (审计回写/去重)
		kind = kind,		-- 地址族
		val = val,			-- 网段基址
		bits = bits,		-- 前缀长度
	}
end


-- @brief 取已编译规则 (带缓存)
-- @param [in] text[string]
-- @return rule[table|nil]
local function _compile(text)
	local r = _cache[text]
	if r ~= nil then
		return r
	end

	r = _rule_of(text)
	if r ~= nil then
		_cache[text] = r
	end

	return r
end


-- @brief IPv4 前缀比较
-- @param [in] a[number]
-- @param [in] b[number]
-- @param [in] bits[number]
-- @return same[boolean]
local function _mask_eq_v4(a, b, bits)
	if 32 <= bits then
		return a == b
	end

	local unit = 2 ^ (32 - bits)
	return math.floor(a / unit) == math.floor(b / unit)
end


-- @brief IPv6 前缀比较
-- @param [in] a[string]			16 字节
-- @param [in] b[string]			16 字节
-- @param [in] bits[number]
-- @return same[boolean]
local function _mask_eq_v6(a, b, bits)
	local full = math.floor(bits / 8)
	if 0 < full and string.sub(a, 1, full) ~= string.sub(b, 1, full) then
		return false
	end

	local rem = bits % 8
	if 0 == rem then
		return true
	end

	local unit = 2 ^ (8 - rem)
	return math.floor(string.byte(a, full + 1) / unit) == math.floor(string.byte(b, full + 1) / unit)
end


-- @brief 单条规则是否命中
-- @param [in] rule[table]			已编译规则
-- @param [in] kind[string]
-- @param [in] val[number|string]
-- @param [in] mapped4[number|nil]
-- @return hit[boolean]
local function _hit(rule, kind, val, mapped4)
	if rule.kind == kind then
		if "v4" == kind then
			return _mask_eq_v4(rule.val, val, rule.bits)
		end

		return _mask_eq_v6(rule.val, val, rule.bits)
	end

	-- v4 规则可匹配 ::ffff:a.b.c.d 映射地址
	if "v4" == rule.kind and nil ~= mapped4 then
		return _mask_eq_v4(rule.val, mapped4, rule.bits)
	end

	return false
end


-- @brief 缺省 ACL (全放行)
-- @return acl[table]
M.defaults = function ()
	return {
		allow = {},			-- 白名单; 空=不启用白名单
		deny = {},			-- 黑名单; 优先
		default = M.DEFAULT,-- 白名单为空且未命中黑名单时的默认判定: allow / deny
	}
end


-- @brief 规范 ACL: 丢弃非法项并按原文去重; 未知键不带出
-- @param [in] src[table|nil]
-- @return acl[table]
M.normalize = function (src)
	local out = M.defaults()
	if type(src) ~= "table" then
		return out
	end

	for _, key in ipairs({ "allow", "deny" }) do
		local list = src[key]
		if type(list) == "table" then
			local seen = {}
			for i = 1, #list do
				local rule = _compile(list[i])
				if rule ~= nil and not seen[rule.text] then
					seen[rule.text] = true
					out[key][#out[key] + 1] = rule.text
				end
			end
		end
	end

	local def = src.default
	if type(def) == "string" then
		def = string.lower(def)
		if "deny" == def or "allow" == def then
			out.default = def
		end
	end

	return out
end


-- @brief 规则项是否合法 (CLI 校验用)
-- @param [in] text[string]
-- @return ok[boolean]
M.rule_ok = function (text)
	return _compile(text) ~= nil
end


-- @brief 规则项是否启用 (非空表)
-- @param [in] acl[table]
-- @return on[boolean]
M.enabled = function (acl)
	if type(acl) ~= "table" then
		return false
	end

	if 0 < #(acl.deny or {}) or 0 < #(acl.allow or {}) then
		return true
	end

	return "deny" == acl.default
end


-- @brief 对端串取 IP ("a.b.c.d:port" / "[::1]:port" / 纯 IP)
-- @param [in] peer[string]
-- @return ip[string]			取不到为空串
-- @note 先按整体 IP 判定 (裸 IPv6 无端口), 不合法再拆 ":port"
M.peer_ip = function (peer)
	if type(peer) ~= "string" then
		return ""
	end

	local s = string.match(peer, "^%s*(.-)%s*$")
	if s == nil or s == "" then
		return ""
	end

	-- [IPv6]:port
	local bracket = string.match(s, "^%[(.-)%]")
	if bracket ~= nil then
		return bracket
	end

	local kind = _ip_of(s)
	if "" ~= kind then
		return s
	end

	-- "ip:port"
	local ip = string.match(s, "^(.-):%d+$")
	if ip ~= nil and ip ~= "" and "" ~= (_ip_of(ip)) then
		return ip
	end

	return s
end


-- @brief 判定对端 IP 是否放行 (deny 优先)
-- @param [in] acl[table]
-- @param [in] ip[string]			对端 IP (可含端口)
-- @return allow[boolean]
-- @return reason[string]			判定依据 (审计): deny:<规则> / allow:<规则> / allow-miss / default
M.check = function (acl, ip)
	if type(acl) ~= "table" then
		return true, "default"
	end

	local kind, val, mapped4 = _ip_of(M.peer_ip(ip))
	if "" == kind then
		-- 取不到对端地址时不拦 (同 klbweb 连接闸门)
		return true, "default"
	end

	local deny = acl.deny or {}
	for i = 1, #deny do
		local rule = _compile(deny[i])
		if rule ~= nil and _hit(rule, kind, val, mapped4) then
			return false, "deny:" .. rule.text
		end
	end

	local allow = acl.allow or {}
	if 0 < #allow then
		for i = 1, #allow do
			local rule = _compile(allow[i])
			if rule ~= nil and _hit(rule, kind, val, mapped4) then
				return true, "allow:" .. rule.text
			end
		end

		return false, "allow-miss"
	end

	if "deny" == acl.default then
		return false, "default:deny"
	end

	return true, "default"
end


return M

-- end
