--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   peers.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua 对端登记配置模块 (peers.json)
-- @note   id 对齐 host_kind: linux/wsl/windows; 禁 win; mesh 调用见 Todo.2026-14 / D33
-- @history 修改历史
--  \n [2026-10] 创建文件 (M1 pref JSON); set_live / load live 优先; approved_at / upsert / counts / norm_id 导出
--]]
local store = require("klbagent.core.config.store")


local M = {}


M.FILE = "peers.json"


local function _norm_id(v)
	if type(v) ~= "string" or v == "" then
		return nil
	end

	local id = string.lower(v)
	-- 人对话简写 win → 完整 windows (禁入库 win)
	if id == "win" then
		id = "windows"
	end

	if id == "linux" or id == "wsl" or id == "windows" then
		return id
	end

	-- 自定义 id: 字母数字下划线短横
	if string.match(id, "^[%w][%w%-_]*$") and #id <= 64 then
		return id
	end

	return nil
end


-- @brief 规范化 peer id (win→windows)
-- @param [in] v[string]
-- @return id[string|nil]
M.norm_id = function (v)
	return _norm_id(v)
end


local function _norm_state(v, def)
	if v == "pending" or v == "trusted" or v == "revoked" then
		return v
	end

	return def
end


local function _norm_scheme(v, def)
	if type(v) ~= "string" or v == "" then
		return def
	end

	local low = string.lower(v)
	if low == "mnps" or low == "mnp" or low == "smps" or low == "smp"
		or low == "https" or low == "http" or low == "wss" or low == "ws"
		or low == "auto" then
		return low
	end

	return def
end


local function _norm_port(v, def)
	local n = tonumber(v)
	if n == nil then
		return def
	end

	n = math.floor(n)
	if n < 1 or n > 65535 then
		return def
	end

	return n
end


local function _norm_peer(row)
	if type(row) ~= "table" then
		return nil
	end

	local id = _norm_id(row.id)
	if id == nil then
		return nil
	end

	local host = ""
	if type(row.host) == "string" then
		host = row.host
	end

	local out = {
		id = id,
		host = host,
		port = _norm_port(row.port, 12817),
		scheme = _norm_scheme(row.scheme, "mnps"),
		cert_sha256 = "",
		token_ref = "file:serve.env",
		profile_hint = "",
		state = _norm_state(row.state, "pending"),
		approved_at = "",
		note = "",
	}

	if type(row.cert_sha256) == "string" then
		out.cert_sha256 = row.cert_sha256
	end

	if type(row.token_ref) == "string" and row.token_ref ~= "" then
		out.token_ref = row.token_ref
	end

	if type(row.profile_hint) == "string" then
		out.profile_hint = row.profile_hint
	end

	if type(row.approved_at) == "string" then
		out.approved_at = row.approved_at
	end

	if type(row.note) == "string" then
		out.note = row.note
	end

	return out
end


-- @brief 缺省
-- @return cfg[table]
M.defaults = function ()
	return {
		peers = {},
	}
end


-- @brief 规范: peers 数组; 非法项丢弃; id 去重保留后者
-- @param [in] cfg[table]
-- @return cfg[table]
M.normalize = function (cfg)
	local def = M.defaults()
	if type(cfg) ~= "table" then
		return def
	end

	local src = cfg.peers
	if type(src) ~= "table" then
		return def
	end

	local by_id = {}
	local order = {}

	-- 支持数组或 id->row 表
	local is_array = (#src > 0)
	if is_array then
		for i = 1, #src do
			local p = _norm_peer(src[i])
			if p ~= nil then
				if by_id[p.id] == nil then
					order[#order + 1] = p.id
				end
				by_id[p.id] = p
			end
		end
	else
		for k, row in pairs(src) do
			local copy = row
			if type(copy) == "table" and (copy.id == nil or copy.id == "") then
				copy = {}
				for kk, vv in pairs(row) do
					copy[kk] = vv
				end
				copy.id = k
			end
			local p = _norm_peer(copy)
			if p ~= nil then
				if by_id[p.id] == nil then
					order[#order + 1] = p.id
				end
				by_id[p.id] = p
			end
		end
	end

	local peers = {}
	for i = 1, #order do
		peers[#peers + 1] = by_id[order[i]]
	end

	return {
		peers = peers,
	}
end


-- @brief 按 id 查找 (load 后)
-- @param [in] cfg[table]
-- @param [in] id[string]
-- @return peer[table|nil]
M.find = function (cfg, id)
	id = _norm_id(id)
	if id == nil or type(cfg) ~= "table" or type(cfg.peers) ~= "table" then
		return nil
	end

	for i = 1, #cfg.peers do
		local p = cfg.peers[i]
		if type(p) == "table" and p.id == id then
			return p
		end
	end

	return nil
end


-- @brief 插入或按 id 覆盖一行; 返回规范化后的 cfg 与 peer
-- @param [in] cfg[table|nil]
-- @param [in] row[table]
-- @return cfg[table]
-- @return peer[table]
M.upsert = function (cfg, row)
	local base = M.normalize(cfg)
	local peer = _norm_peer(row)
	if peer == nil then
		error("invalid peer row")
	end

	local found = false
	for i = 1, #base.peers do
		if base.peers[i].id == peer.id then
			base.peers[i] = peer
			found = true
			break
		end
	end

	if not found then
		base.peers[#base.peers + 1] = peer
	end

	return base, peer
end


-- @brief 统计 state
-- @param [in] cfg[table|nil]
-- @return counts[table]  { total, trusted, pending, revoked }
M.counts = function (cfg)
	local n = M.normalize(cfg)
	local out = {
		total = 0,
		trusted = 0,
		pending = 0,
		revoked = 0,
	}

	for i = 1, #n.peers do
		local p = n.peers[i]
		out.total = out.total + 1
		local st = p.state or "pending"
		if st == "trusted" then
			out.trusted = out.trusted + 1
		elseif st == "revoked" then
			out.revoked = out.revoked + 1
		else
			out.pending = out.pending + 1
		end
	end

	return out
end


-- @brief 配置文件路径
-- @return path[string]
M.path = function ()
	return store.path(M.FILE)
end


local _live = nil


-- @brief 设置进程内 peers 配置 (不写盘)
-- @param [in] cfg[table]
-- @return cfg[table]
M.set_live = function (cfg)
	_live = M.normalize(cfg)
	return _live
end


M.clear_live = function ()
	_live = nil
end


M.has_live = function ()
	return _live ~= nil
end


-- @brief 读取 peers.json (live 优先)
-- @return cfg[table]
M.load = function ()
	if _live ~= nil then
		return _live
	end

	return store.load(M)
end


-- @brief 写入 peers.json
-- @param [in] cfg[table]
-- @return ok[boolean]
M.save = function (cfg)
	return store.save(M, cfg)
end


return M
