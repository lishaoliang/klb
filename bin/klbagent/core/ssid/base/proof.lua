--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   proof.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  ssid 证明票派生: raw → mid(缓存) → proof(ssid+mid); 供 serve/client 共用
-- @stability internal
-- @impl evolving
-- @note   require("klbagent.core.ssid.base.proof")
--   \n mid 仅服务端内存缓存 (约 6h); 校验用 mid 快路径, 不每次完整重算
--   \n 客户端持本地 raw 自算 mid 再 make_proof; 禁经 LPC/XPC 传 raw
--   \n SHA256 优先 C kcrypto; 回退 klbtunnel.crypto
-- @history 修改历史
--  \n [2026-11] 创建文件 (原 c1.lua); 证同派生
--]]
local const = require("klbagent.core.ssid.base.const")


local M = {}


-- mid 缓存 TTL (与 auth 临时票窗对齐)
M.MID_TTL_MS = const.MID_TTL_MS or (6 * 60 * 60 * 1000)

local g_mid = nil
local g_mid_expire_ms = 0
local g_mid_from = nil


local function _now_ms()
	return math.floor(os.time() * 1000)
end


local function _norm_sha256(dig)
	if type(dig) == "string" and #dig == 32 then
		return dig
	end

	-- 若实现返回 hex
	if type(dig) == "string" and #dig == 64 and string.find(dig, "^[0-9a-fA-F]+$") then
		local parts = {}
		for i = 1, 64, 2 do
			parts[#parts + 1] = string.char(tonumber(string.sub(dig, i, i + 1), 16))
		end

		return table.concat(parts)
	end

	return nil
end


local function _sha256_bin(msg)
	local ok_k, kcrypto = pcall(require, "kcrypto")
	if ok_k and type(kcrypto) == "table" and type(kcrypto.sha256) == "function" then
		local dig = _norm_sha256(kcrypto.sha256(msg or ""))
		if dig ~= nil then
			return dig
		end
	end

	local ok, crypto = pcall(require, "klbcore.klbtunnel.crypto")
	if ok and type(crypto) == "table" and type(crypto.sha256) == "function" then
		return _norm_sha256(crypto.sha256(msg or ""))
	end

	return nil
end


local function _hex(bin, nbytes)
	if type(bin) ~= "string" then
		return nil
	end

	local n = nbytes or #bin
	if n > #bin then
		n = #bin
	end

	local parts = {}
	for i = 1, n do
		parts[#parts + 1] = string.format("%02x", string.byte(bin, i))
	end

	return table.concat(parts)
end


local function _token_eq(a, b)
	if type(a) ~= "string" or type(b) ~= "string" then
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

	return diff == 0
end


-- @brief 本机原始 token (只读; 不经客户端通道)
-- @return raw[string|nil]
M.raw_expect = function ()
	local ok, tokmod = pcall(require, "klbagent.core.serve.security.token")
	if ok and type(tokmod) == "table" and type(tokmod.token_expect) == "function" then
		local raw = tokmod.token_expect()
		if type(raw) == "string" and raw ~= "" then
			return raw
		end
	end

	local env = os.getenv("ALUA_SERVE_TOKEN")
	if type(env) == "string" and env ~= "" then
		env = string.gsub(env, "[\r\n]", "")
		env = string.match(env, "^%s*(.-)%s*$") or env
		if env ~= "" then
			return env
		end
	end

	return nil
end


-- @brief raw → mid (完整算法段; 结果可缓存)
-- @param [in] raw[string]
-- @return mid[string|nil]		32B 二进制摘要
-- @return err[string|nil]
M.compute_mid = function (raw)
	if type(raw) ~= "string" or raw == "" then
		return nil, "need raw"
	end

	local dig = _sha256_bin("alua.ssid.proof.mid|v1|" .. raw)
	if dig == nil then
		return nil, "sha256 unavailable"
	end

	return dig
end


-- @brief 服务端确保 mid 缓存 (过期重算)
-- @param [in] force[boolean|nil]
-- @return mid[string|nil]
-- @return err[string|nil]
M.ensure_mid = function (force)
	local now = _now_ms()
	if force ~= true and type(g_mid) == "string" and g_mid ~= "" and g_mid_expire_ms > now then
		return g_mid
	end

	local raw = M.raw_expect()
	if raw == nil then
		return nil, "raw unavailable"
	end

	local mid, err = M.compute_mid(raw)
	if mid == nil then
		return nil, err
	end

	g_mid = mid
	g_mid_expire_ms = now + M.MID_TTL_MS
	g_mid_from = "raw"
	return g_mid
end


-- @brief mid 缓存状态 (禁真值)
-- @return info[table]
M.mid_status = function ()
	local now = _now_ms()
	return {
		cached = type(g_mid) == "string" and g_mid ~= "",
		expire_ms = g_mid_expire_ms,
		ttl_left_ms = (g_mid_expire_ms > now) and (g_mid_expire_ms - now) or 0,
		from = g_mid_from,
	}
end


-- @brief mid + ssid → 证明票 (非 raw; 前缀 p.)
-- @param [in] ssid[string]		已规范化 alua_ssid
-- @param [in] mid[string]		二进制 mid
-- @return proof[string|nil]
-- @return err[string|nil]
M.proof_from_mid = function (ssid, mid)
	if type(ssid) ~= "string" or ssid == "" then
		return nil, "need ssid"
	end

	if type(mid) ~= "string" or mid == "" then
		return nil, "need mid"
	end

	local dig = _sha256_bin("alua.ssid.proof.tok|v1|" .. mid .. "|" .. ssid)
	if dig == nil then
		return nil, "sha256 unavailable"
	end

	local hex = _hex(dig, 16)
	if hex == nil then
		return nil, "hex fail"
	end

	local pfx = const.PROOF_TOKEN_PREFIX or "p."
	return pfx .. hex
end


-- @brief 客户端/本机: 用本地 raw + ssid 派生证明票
-- @param [in] ssid[string]
-- @param [in] raw[string]
-- @return proof[string|nil]
-- @return err[string|nil]
M.make_proof = function (ssid, raw)
	local mid, err = M.compute_mid(raw)
	if mid == nil then
		return nil, err
	end

	return M.proof_from_mid(ssid, mid)
end


-- @brief 服务端快路径校验证明票 (用缓存 mid)
-- @param [in] ssid[string]
-- @param [in] proof[string]
-- @return ok[boolean]
-- @return err[string|nil]
M.verify_proof = function (ssid, proof)
	local pfx = const.PROOF_TOKEN_PREFIX or "p."
	if type(proof) ~= "string" or string.sub(proof, 1, #pfx) ~= pfx then
		return false, "not proof token"
	end

	local mid, err = M.ensure_mid(false)
	if mid == nil then
		return false, err or "mid unavailable"
	end

	local expect, err_p = M.proof_from_mid(ssid, mid)
	if expect == nil then
		return false, err_p
	end

	if not _token_eq(proof, expect) then
		return false, "proof mismatch"
	end

	return true
end


-- @brief 是否证明票形态
-- @param [in] tok[string|nil]
-- @return yes[boolean]
M.is_proof_token = function (tok)
	local pfx = const.PROOF_TOKEN_PREFIX or "p."
	return type(tok) == "string" and tok ~= "" and string.sub(tok, 1, #pfx) == pfx
end


return M

-- end
