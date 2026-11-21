--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   util.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  ssid 客户端共用辅助 (opts / timeout / 临时 token 形态)
-- @stability internal
-- @impl evolving
-- @note   require("klbagent.core.ssid.client.util")
-- @history 修改历史
--  \n [2026-11] 自 lpcclient / xpcclient 迁出
--]]
local const = require("klbagent.core.ssid.base.const")


local M = {}


-- @brief 临时会话 token 形态 (t. 前缀)
-- @param [in] tok[string|nil]
-- @return yes[boolean]
M.is_temp_token = function (tok)
	local pfx = const.TEMP_TOKEN_PREFIX or "t."
	return type(tok) == "string" and tok ~= "" and string.sub(tok, 1, #pfx) == pfx
end


-- @brief C1 证明票形态 (p. 前缀)
-- @param [in] tok[string|nil]
-- @return yes[boolean]
M.is_proof_token = function (tok)
	local pfx = const.PROOF_TOKEN_PREFIX or "p."
	return type(tok) == "string" and tok ~= "" and string.sub(tok, 1, #pfx) == pfx
end


-- @brief 非 raw 形态 (会话票或证明票; 可经客户端通道)
-- @param [in] tok[string|nil]
-- @return yes[boolean]
M.is_non_raw_token = function (tok)
	return M.is_temp_token(tok) or M.is_proof_token(tok)
end


-- @brief 规范化 alua_ssid (16 位 hex, 小写)
-- @param [in] ssid[string|nil]
-- @return sid[string|nil]
M.norm_ssid = function (ssid)
	if type(ssid) ~= "string" then
		return nil
	end

	local low = string.lower(ssid)
	if #low ~= (const.SSID_LEN or 16) or string.find(low, "^[0-9a-f]+$") == nil then
		return nil
	end

	return low
end


-- @brief pref 目录 (缺省尾斜杠已剥除)
-- @return dir[string|nil]
local function _pref_dir()
	local ok_s, store = pcall(require, "klbagent.core.ssid.base.store")
	if not (ok_s and type(store) == "table" and type(store.dir) == "function") then
		return nil
	end

	local dir = store.dir()
	if type(dir) ~= "string" or dir == "" then
		return nil
	end

	return (string.gsub(dir, "[/\\]+$", ""))
end


-- @brief 对端 raw (peers.<id>.env); 仅本模块内取用, 禁外泄
-- @param [in] peer_id[string|nil]
-- @return raw[string|nil]
M.peer_raw = function (peer_id)
	if type(peer_id) ~= "string" or peer_id == "" then
		return nil
	end

	local dir = _pref_dir()
	if dir == nil then
		return nil
	end

	local f = io.open(dir .. "/peers." .. peer_id .. ".env", "rb")
	if f == nil then
		return nil
	end

	local text = f:read("*a")
	f:close()
	if type(text) ~= "string" or text == "" then
		return nil
	end

	return string.match(text, "ALUA_SERVE_TOKEN%s*=%s*(%S+)")
end


-- @brief 目标 raw: 显式 > peers.<id>.env > 本机 serve.env/环境变量
-- @param [in] opts[table|nil]		raw?/token?, peer?
-- @return raw[string|nil]
M.target_raw = function (opts)
	opts = M.norm_opts(opts)

	local raw = opts.raw or opts.token
	if type(raw) ~= "string" or raw == "" or M.is_non_raw_token(raw) then
		raw = nil
	end

	if raw == nil then
		raw = M.peer_raw(opts.peer)
	end

	if raw == nil then
		local ok_p, proof = pcall(require, "klbagent.core.ssid.base.proof")
		if ok_p and type(proof) == "table" and type(proof.raw_expect) == "function" then
			raw = proof.raw_expect()
		end
	end

	return raw
end


-- @brief 目标证明票 (raw 只在客户端内取用, 结果 p. 票可随请求外发)
-- @param [in] opts[table|nil]		alua_ssid, raw?/token?, peer?
-- @return proof[string|nil]
-- @return err[string|nil]
M.proof_token = function (opts)
	opts = M.norm_opts(opts)

	local sid = M.norm_ssid(opts.alua_ssid)
	if sid == nil then
		return nil, "bad alua_ssid"
	end

	local raw = M.target_raw(opts)
	if type(raw) ~= "string" or raw == "" then
		return nil, "need raw (serve.env / peers.<id>.env)"
	end

	return M.make_proof(sid, raw)
end


-- @brief 本机 raw + ssid → C1 证明票 (禁把 raw 打到 RPC)
-- @param [in] ssid[string]
-- @param [in] raw[string]
-- @return proof[string|nil]
-- @return err[string|nil]
M.make_proof = function (ssid, raw)
	local proof = require("klbagent.core.ssid.base.proof")
	local sid = nil
	if type(ssid) == "string" then
		local low = string.lower(ssid)
		if #low == (const.SSID_LEN or 16) and string.find(low, "^[0-9a-f]+$") ~= nil then
			sid = low
		end
	end

	if sid == nil then
		return nil, "bad alua_ssid"
	end

	return proof.make_proof(sid, raw)
end


-- @brief opts 规范化
-- @param [in] opts[table|nil]
-- @return opts[table]
M.norm_opts = function (opts)
	if type(opts) ~= "table" then
		return {}
	end

	return opts
end


-- @brief timeout_ms 规范化 (缺省 15000)
-- @param [in] timeout_ms[number|nil]
-- @return ms[number]
M.norm_timeout_ms = function (timeout_ms)
	if type(timeout_ms) ~= "number" or timeout_ms <= 0 then
		return 15000
	end

	return timeout_ms
end


-- @brief 浅拷贝 opts 表
-- @param [in] opts[table|nil]
-- @return copy[table]
M.copy_opts = function (opts)
	opts = M.norm_opts(opts)
	local out = {}
	for k, v in pairs(opts) do
		out[k] = v
	end

	return out
end


-- @brief 剥除 get_token 回包中的非临时 token 明文
-- @param [in] out[table|nil]
-- @return out[table|nil]
M.strip_non_temp_token = function (out)
	if type(out) ~= "table" then
		return out
	end

	if type(out.token) == "string" and out.token ~= "" and not M.is_temp_token(out.token) then
		out.token = nil
	end

	return out
end


return M

-- end
