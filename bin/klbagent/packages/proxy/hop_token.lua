--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   hop_token.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  proxy hop 鉴权材料: alua_ssid + C1 证明票 (p.)
-- @stability internal
-- @impl evolving
-- @note   对齐 ssid 2026-11 契约: 客户端持本地 raw 自算证明票; 禁 new_serve / 禁读 temp.*.env
--   \n 远端 serve 入口验票: serve.security.auth.verify_token → ssid.co_verify_token (proof 快路径)
--   \n 上送: SMP/MNP TEXT body 与 WS 头须同时带 alua_ssid (证明票必填)
--   \n 显式 hop.token 仅排障; hop.token_ref=file 不再取票, 仅兼容解析为 ssid
--   \n 非 hex 逻辑名 → machine.make_ssid("hop|"..名); auth_err_is_transient 供冷建重试
-- @history 修改历史
--  \n [2026-10] 创建文件; temp token 缓存 / ensure_hop / ensure_hops
--  \n [2026-11] 改 ssid 证明票: 删 new_serve / 读 token 文件 / peer_tool 续期; 增 hop_ssid
--]]
local proof = require("klbagent.core.ssid.base.proof")
local machine = require("klbagent.core.ssid.serve.machine")
local ctx_mod = require("klbagent.core.ssid.base.ctx")
local ssid = require("klbagent.core.ssid")


local M = {}


-- ssid 形态: 16 位小写 hex (digest12 + salt4)
local SSID_LEN = 16

-- 进程内证明票缓存: sid -> p.<hex>
local g_tokens = {}

-- 本机兜底 ssid 缓存
local g_machine_sid = nil


local function _safe_client_id(client_id)
	local s = string.gsub(tostring(client_id), "[^%w%.%-_]", "-")
	s = string.gsub(s, "%-+", "-")
	s = string.gsub(s, "^%-", "")
	s = string.gsub(s, "%-$", "")
	if s == "" then
		return nil
	end

	return s
end


local function _is_ssid(sid)
	if type(sid) ~= "string" or #sid ~= SSID_LEN then
		return false
	end

	return string.find(sid, "^[0-9a-f]+$") ~= nil
end


-- @brief 从 hop.alua_ssid / hop.client_id / 历史 token_ref=temp 解析 ssid
-- @param [in] hop[table]
-- @return client_id[string|nil]
M.client_id_of_hop = function (hop)
	if type(hop) ~= "table" then
		return nil
	end

	if type(hop.alua_ssid) == "string" and hop.alua_ssid ~= "" then
		return _safe_client_id(hop.alua_ssid)
	end

	if type(hop.client_id) == "string" and hop.client_id ~= "" then
		return _safe_client_id(hop.client_id)
	end

	-- 兼容旧配置: mem:<id> / file:temp.<id>.env → ssid; 不再作对外用法
	local ref = hop.token_ref
	if type(ref) ~= "string" or ref == "" then
		return nil
	end

	local id = string.match(ref, "^mem:(.+)$")
	if id == nil then
		id = string.match(ref, "^file:temp%.(.+)%.env$")
	end

	if id == nil then
		id = string.match(ref, "^temp%.(.+)%.env$")
	end

	if id == nil then
		return nil
	end

	return _safe_client_id(id)
end


-- @brief 本机 ssid 兜底 (machine_key + 材质 → 16 位 ssid)
-- @return sid[string]
local function _machine_sid()
	if type(g_machine_sid) == "string" and g_machine_sid ~= "" then
		return g_machine_sid
	end

	local ok_h, hostinfo = pcall(require, "klbagent.core.hostinfo")
	local mat = "proxy"
	if ok_h and type(hostinfo) == "table" and type(hostinfo.probe) == "function" then
		local hi = hostinfo.probe()
		if type(hi) == "table" and type(hi.host_kind) == "string" then
			mat = "proxy|" .. hi.host_kind
		end
	end

	g_machine_sid = machine.make_ssid(mat)
	return g_machine_sid
end


-- @brief 逻辑名 → 稳定 16-hex (本机 machine_key 派生; 跨机同 raw 时证明票仍可对)
-- @param [in] label[string]
-- @return sid[string]
local function _label_ssid(label)
	return machine.make_ssid("hop|" .. tostring(label))
end


-- @brief hop 的 alua_ssid (证明票必填)
-- @param [in] hop[table]
-- @return sid[string]
-- @note 优先级: 16-hex ssid > 逻辑名(如 cursor-ai-public→稳定 hex) > 请求上下文 > 本机 machine
M.hop_ssid = function (hop)
	local sid = M.client_id_of_hop(hop)
	if _is_ssid(sid) then
		return sid
	end

	-- 非 hex 逻辑名: 映射稳定 16-hex; 禁静默吞成裸 machine_sid (否则同机多 hop 名冲突/难对账)
	if type(sid) == "string" and sid ~= "" then
		return _label_ssid(sid)
	end

	local req = ctx_mod.get_request()
	if type(req) == "table" and _is_ssid(req.alua_ssid) then
		return req.alua_ssid
	end

	return _machine_sid()
end


-- @brief hop 鉴权票: 显式 hop.token(排障) > ssid 证明票 (进程内缓存)
-- @param [in] hop[table]
-- @return token[string|nil]
-- @return err[string|nil]
M.hop_token = function (hop)
	if type(hop) ~= "table" then
		return nil, "bad hop"
	end

	if type(hop.token) == "string" and hop.token ~= "" then
		return hop.token, nil
	end

	local sid = M.hop_ssid(hop)
	local cached = g_tokens[sid]
	if type(cached) == "string" and cached ~= "" then
		return cached, nil
	end

	local raw = proof.raw_expect()
	if type(raw) ~= "string" or raw == "" then
		return nil, "need raw token (ALUA_SERVE_TOKEN / serve.env)"
	end

	local client = ssid.new_client({ transport = "xpc" })
	local tok, err = client.make_proof(sid, raw)
	if type(tok) ~= "string" or tok == "" then
		return nil, tostring(err or "make_proof fail")
	end

	g_tokens[sid] = tok
	return tok, nil
end


-- @brief 激活 hop 鉴权: 绑定 alua_ssid 并派生证明票
-- @param [in] hop[table]
-- @param [in] opts[table|nil]		force=true 丢弃缓存重算
-- @return ok[boolean]
-- @return info[table|string]
M.ensure_hop = function (hop, opts)
	if type(hop) ~= "table" then
		return false, "bad hop"
	end

	local sid = M.hop_ssid(hop)
	if type(sid) ~= "string" or sid == "" then
		return false, "need alua_ssid"
	end

	-- 绑定 hop 侧 ssid 字段; 清除对外 token_ref=file 依赖
	hop.alua_ssid = sid
	hop.client_id = sid
	hop.token_ref = nil

	if type(opts) == "table" and opts.force == true then
		g_tokens[sid] = nil
	end

	local tok, err = M.hop_token(hop)
	if type(tok) ~= "string" or tok == "" then
		return false, tostring(err or "hop token fail")
	end

	return true, {
		ok = true,
		skipped = false,
		action = "proof",
		client_id = sid,
		alua_ssid = sid,
		kind = "proof",
		host = hop.host,
		port = tonumber(hop.port),
	}
end


-- @brief 对 hops[] 批量 ensure
-- @param [in] hops[table]
-- @param [in] opts[table|nil]
-- @return all_ok[boolean]
-- @return reports[table]
M.ensure_hops = function (hops, opts)
	local reports = {}
	if type(hops) ~= "table" then
		return true, reports
	end

	local all_ok = true
	for i = 1, #hops do
		local hop = hops[i]
		local ok, info = M.ensure_hop(hop, opts)
		reports[#reports + 1] = {
			index = i,
			ok = ok,
			info = info,
		}
		if not ok then
			all_ok = false
		end
	end

	return all_ok, reports
end


-- @brief 鉴权失败是否像瞬时断链 (宜冷建重试; 不必先 force ensure)
-- @param [in] err[string|nil]
-- @return yes[boolean]
M.auth_err_is_transient = function (err)
	local s = tostring(err or "")
	if s == "" then
		return false
	end

	if string.find(s, "auth early", 1, true) then
		return true
	end

	if string.find(s, "auth timeout", 1, true) then
		return true
	end

	if string.find(s, "auth send fail", 1, true) then
		return true
	end

	if string.find(s, "mnp connect fail", 1, true) then
		return true
	end

	if string.find(s, "smp connect fail", 1, true) then
		return true
	end

	return false
end


-- @brief 鉴权失败文案是否值得 ensure 后重试
-- @param [in] err[string|nil]
-- @return yes[boolean]
M.auth_err_needs_ensure = function (err)
	local s = tostring(err or "")
	if s == "" then
		return false
	end

	if string.find(s, "raw token requires tls", 1, true) then
		return true
	end

	if string.find(s, "raw token only for auth", 1, true) then
		return true
	end

	if string.find(s, "token expired", 1, true) then
		return true
	end

	if string.find(s, "bad token", 1, true) then
		return true
	end

	if string.find(s, "auth deny", 1, true) then
		return true
	end

	if string.find(s, "proof mismatch", 1, true) then
		return true
	end

	if string.find(s, "need alua_ssid", 1, true) then
		return true
	end

	-- 瞬时断链也可能夹带证明票过期; ensure 作第二道
	if M.auth_err_is_transient(s) then
		return true
	end

	return false
end


return M

-- end
