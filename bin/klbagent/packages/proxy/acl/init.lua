--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   acl.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  proxy 来源 ACL / 鉴权失败限速 / 静默拒绝
-- @note   防被扫(降暴露面); 非反审查; 公网仍靠 OS 防火墙
--]]


local M = {}


local function _now_ms()
	local ok_e, kenv = pcall(require, "kenv")
	if ok_e and type(kenv) == "table" and type(kenv.tick_count) == "function" then
		return kenv.tick_count()
	end
	return math.floor(os.clock() * 1000)
end


local function _ipv4_to_u32(ip)
	local a, b, c, d = string.match(tostring(ip or ""), "^(%d+)%.(%d+)%.(%d+)%.(%d+)$")
	if a == nil then
		return nil
	end
	local na, nb, nc, nd = tonumber(a), tonumber(b), tonumber(c), tonumber(d)
	if na == nil or nb == nil or nc == nil or nd == nil then
		return nil
	end
	if na > 255 or nb > 255 or nc > 255 or nd > 255 then
		return nil
	end
	return na * 16777216 + nb * 65536 + nc * 256 + nd
end


-- @brief 单条 allow 规则是否命中 (精确 IP 或 IPv4 CIDR)
-- @param [in]  ip[string]
-- @param [in]  rule[string]
-- @return [boolean]
M.ip_match_rule = function (ip, rule)
	if type(ip) ~= "string" or ip == "" or type(rule) ~= "string" or rule == "" then
		return false
	end

	rule = string.lower(string.gsub(rule, "%s+", ""))
	ip = string.lower(ip)

	if rule == "*" or rule == "any" or rule == "0.0.0.0/0" then
		return true
	end

	local base, bits = string.match(rule, "^([^/]+)/(%d+)$")
	if base ~= nil then
		local nbits = tonumber(bits)
		local ipn = _ipv4_to_u32(ip)
		local basen = _ipv4_to_u32(base)
		if ipn == nil or basen == nil or nbits == nil or nbits < 0 or nbits > 32 then
			return false
		end
		if nbits == 0 then
			return true
		end
		local shift = 32 - nbits
		local div = 2 ^ shift
		return math.floor(ipn / div) == math.floor(basen / div)
	end

	return ip == rule
end


-- @brief allow_from 列表是否允许 (空/nil = 全允许)
-- @param [in]  ip[string|nil]
-- @param [in]  list[table|nil]
-- @return [boolean]
M.allow_from_ok = function (ip, list)
	if list == nil then
		return true
	end
	if type(list) ~= "table" then
		return true
	end
	if #list == 0 and next(list) == nil then
		return true
	end

	-- array form
	local n = #list
	if n > 0 then
		for i = 1, n do
			if M.ip_match_rule(ip, list[i]) then
				return true
			end
		end
		return false
	end

	-- map form { ["192.0.2.0/24"]=true }
	for rule, on in pairs(list) do
		if on and M.ip_match_rule(ip, tostring(rule)) then
			return true
		end
	end
	return false
end


-- @brief 比对入口 auth (userpass 多用户 / bearer)
-- @param [in]  auth[table|nil]
-- @param [in]  user[string|nil]
-- @param [in]  pass[string|nil]
-- @param [in]  token[string|nil]	Bearer token
-- @return [boolean] ok
-- @return [string|nil] err
M.check_credentials = function (auth, user, pass, token)
	if type(auth) ~= "table" then
		return true
	end

	local mode = string.lower(tostring(auth.mode or "none"))
	if mode == "none" or mode == "off" or mode == "disabled" or mode == "" then
		return true
	end

	if mode == "bearer" or mode == "token" then
		local expect = tostring(auth.token or "")
		if expect == "" then
			return false, "auth.token required"
		end
		if type(token) == "string" and token == expect then
			return true
		end
		return false, "auth rejected"
	end

	if mode == "basic" or mode == "user" or mode == "password" then
		mode = "userpass"
	end

	if mode ~= "userpass" then
		return false, "unsupported auth"
	end

	user = tostring(user or "")
	pass = tostring(pass or "")

	if type(auth.users) == "table" then
		local users = auth.users
		if #users > 0 then
			for i = 1, #users do
				local u = users[i]
				if type(u) == "table"
					and tostring(u.user or "") == user
					and tostring(u.pass or "") == pass then
					return true
				end
			end
		else
			local p = users[user]
			if p ~= nil and tostring(p) == pass then
				return true
			end
		end
		return false, "auth rejected"
	end

	if user == tostring(auth.user or "") and pass == tostring(auth.pass or "") then
		return true
	end

	return false, "auth rejected"
end


-- @brief 创建限速/封禁守卫
-- @param [in]  cfg[table|nil]	读 cfg.security
-- @return [table] guard
M.new_guard = function (cfg)
	local sec = {}
	if type(cfg) == "table" and type(cfg.security) == "table" then
		sec = cfg.security
	end

	local af = sec.auth_fail
	if type(af) ~= "table" then
		af = {}
	end

	local max_fail = tonumber(af.max) or 8
	local window_ms = tonumber(af.window_ms) or 60000
	local ban_ms = tonumber(af.ban_ms) or 300000
	local quiet = sec.quiet_reject ~= false

	local state = {
		-- ip -> { fails=[{t}], ban_until=ms }
	}

	local guard = {}

	guard.quiet = quiet

	guard.allow_from = nil
	if type(sec.allow_from) == "table" then
		guard.allow_from = sec.allow_from
	elseif type(cfg) == "table" and type(cfg.allow_from) == "table" then
		guard.allow_from = cfg.allow_from
	end

	guard.check_peer = function (ip)
		if type(ip) ~= "string" or ip == "" then
			-- 无 peer 时不拦 (兼容旧绑定)
			return true, nil
		end

		if not M.allow_from_ok(ip, guard.allow_from) then
			return false, "allow_from deny"
		end

		local st = state[ip]
		if st ~= nil and st.ban_until ~= nil then
			local now = _now_ms()
			if now < st.ban_until then
				return false, "temp ban"
			end
			st.ban_until = nil
			st.fails = {}
		end

		return true, nil
	end

	guard.note_auth_fail = function (ip)
		if type(ip) ~= "string" or ip == "" then
			return
		end
		local now = _now_ms()
		local st = state[ip]
		if st == nil then
			st = { fails = {}, ban_until = nil }
			state[ip] = st
		end

		local kept = {}
		for i = 1, #(st.fails or {}) do
			local t = st.fails[i]
			if now - t <= window_ms then
				kept[#kept + 1] = t
			end
		end
		kept[#kept + 1] = now
		st.fails = kept

		if #kept >= max_fail then
			st.ban_until = now + ban_ms
			st.fails = {}
		end
	end

	guard.note_auth_ok = function (ip)
		if type(ip) ~= "string" or ip == "" then
			return
		end
		local st = state[ip]
		if st ~= nil then
			st.fails = {}
			st.ban_until = nil
		end
	end

	guard.is_banned = function (ip)
		local ok = guard.check_peer(ip)
		return not ok
	end

	return guard
end


-- @brief 从 tcp userdata 取 peer ip
-- @param [in]  tcp[any]
-- @return [string|nil]
M.peer_ip = function (tcp)
	if tcp == nil or type(tcp.peer_addr) ~= "function" then
		return nil
	end
	local ok, ip = pcall(function ()
		local a, _p = tcp:peer_addr()
		return a
	end)
	if ok and type(ip) == "string" and ip ~= "" then
		return ip
	end
	return nil
end


return M

-- end
