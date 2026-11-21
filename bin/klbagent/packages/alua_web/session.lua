--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   session.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua_web 会话 (token 登录 → HttpOnly Cookie; 内存 sid/csrf)
-- @note   只在 web 进程内有效; 重启即失效; CSRF 供写接口校验
--   \n token 校验复用 core.auth.verify; 明文 HTTP 不强制 raw token TLS (由调用方决定)
-- @history 修改历史
--  \n [2026-10] 创建文件 (P0)
--]]
-- @brief ssid 客户端面 (禁 new_serve; XPC → 本机 serve owner)
local function _ssid()
	local pkg = require("klbagent.core.ssid")
	return pkg.new_client({ transport = "xpc" })
end


local M = {}


-- 会话 Cookie 名 (HttpOnly)
M.COOKIE = "alua_web_sid"

-- 会话有效期 2h (每次访问滑动续期)
M.TTL_MS = 2 * 60 * 60 * 1000


-- sid → { csrf, expire_ms, profile, kind }
local _rows = {}


-- @brief 当前毫秒 (秒级时钟够用; 非单调)
-- @return ms[number]
local function _now_ms()
	return os.time() * 1000
end


-- @brief 生成 n 字节十六进制串 (优先 /dev/urandom)
-- @param [in] n[number]
-- @return hex[string]
local function _rand_hex(n)
	local f = io.open("/dev/urandom", "rb")
	if f ~= nil then
		local bytes = f:read(n)
		f:close()
		if type(bytes) == "string" and #bytes == n then
			local out = {}
			for i = 1, #bytes do
				out[#out + 1] = string.format("%02x", string.byte(bytes, i))
			end

			return table.concat(out)
		end
	end

	-- 回退: 无 urandom 时用 math.random
	math.randomseed(os.time())
	local out = {}
	for i = 1, n do
		out[#out + 1] = string.format("%02x", math.random(0, 255))
	end

	return table.concat(out)
end


-- @brief 清理过期会话行
local function _sweep()
	local now = _now_ms()
	for sid, row in pairs(_rows) do
		if type(row) ~= "table" or (tonumber(row.expire_ms) or 0) <= now then
			_rows[sid] = nil
		end
	end
end


-- @brief token 登录: 校验 + 建会话 + 下发 HttpOnly Cookie
-- @param [in] req[table]
-- @param [in] res[table]
-- @param [in] token[string]
-- @return info[table|nil]			{ sid, csrf, profile, kind }
-- @return err[string|nil]
M.login = function (req, res, token)
	if type(token) ~= "string" or token == "" then
		return nil, "need token"
	end

	-- 明文 HTTP 不传 tls 提示 (raw token 在线面才强制 TLS; 由部署决定)
	local opts = { slide = false }
	local tls = false
	if type(req) == "table" and req.tls == true then
		tls = true
		opts.tls = true
	end

	local info, err = _ssid().co_verify_token(nil, token, opts)
	if type(info) ~= "table" or info.ok ~= true then
		return nil, tostring(err or "bad token")
	end

	_sweep()

	local sid = _rand_hex(24)
	local csrf = _rand_hex(16)
	_rows[sid] = {
		csrf = csrf,
		expire_ms = _now_ms() + M.TTL_MS,
		profile = tostring(info.profile or ""),
		kind = tostring(info.kind or ""),
	}

	if type(res) == "table" and type(res.set_cookie) == "function" then
		res:set_cookie(M.COOKIE, sid, {
			path = "/",
			max_age = math.floor(M.TTL_MS / 1000),
			http_only = true,
			secure = tls,
			same_site = "Lax",
		})
	end

	return {
		sid = sid,
		csrf = csrf,
		profile = tostring(info.profile or ""),
		kind = tostring(info.kind or ""),
	}, nil
end


-- @brief 按请求 Cookie 取会话行并滑动续期
-- @param [in] req[table]
-- @return row[table|nil]			{ csrf, expire_ms, profile, kind }
-- @return sid[string]
M.load = function (req)
	if type(req) ~= "table" or type(req.cookies) ~= "table" then
		return nil, ""
	end

	local sid = req.cookies[M.COOKIE]
	if type(sid) ~= "string" or sid == "" then
		return nil, ""
	end

	local row = _rows[sid]
	if type(row) ~= "table" then
		return nil, ""
	end

	local now = _now_ms()
	if (tonumber(row.expire_ms) or 0) <= now then
		_rows[sid] = nil
		return nil, ""
	end

	row.expire_ms = now + M.TTL_MS
	return row, sid
end


-- @brief 注销: 移除会话行
-- @param [in] req[table]
-- @return removed[boolean]
M.drop = function (req)
	if type(req) ~= "table" or type(req.cookies) ~= "table" then
		return false
	end

	local sid = req.cookies[M.COOKIE]
	if type(sid) ~= "string" or sid == "" then
		return false
	end

	local hit = _rows[sid] ~= nil
	_rows[sid] = nil
	return hit
end


-- @brief 清客户端 Cookie (Max-Age=0)
-- @param [in] res[table]
-- @return 无
M.clear_cookie = function (res)
	if type(res) ~= "table" or type(res.set_cookie) ~= "function" then
		return
	end

	res:set_cookie(M.COOKIE, "", {
		path = "/",
		max_age = 0,
		http_only = true,
		same_site = "Lax",
	})
end


-- @brief 写接口 CSRF 校验 (头 X-CSRF-Token)
-- @param [in] req[table]
-- @param [in] row[table]
-- @return ok[boolean]
M.csrf_ok = function (req, row)
	if type(req) ~= "table" or type(row) ~= "table" then
		return false
	end

	local headers = req.headers
	if type(headers) ~= "table" then
		return false
	end

	local got = headers["x-csrf-token"]
	if type(got) ~= "string" or got == "" then
		return false
	end

	local want = row.csrf
	return type(want) == "string" and want ~= "" and got == want
end


return M

-- end
