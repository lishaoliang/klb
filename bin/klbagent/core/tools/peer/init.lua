--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  peer.* 工具: list / ping / approve / revoke (mesh P0+P1)
-- @note   peer id 对齐 host_kind; 禁 win
-- @history 修改历史
--  \n [2026-10] 创建文件 (M3 / Todo.2026-14 / D33)
--]]


local peers_cfg = require("klbagent.core.config.peers")
local util = require("klbagent.core.tools.remote.util")
local transport = require("klbagent.core.tools.remote.transport")
local profiles = require("klbagent.core.profiles")


local M = {}


local function _iso_now()
	return os.date("!%Y-%m-%dT%H:%M:%SZ")
end


local function _id_of(args)
	local id = nil
	if type(args) == "table" then
		id = args.id or args.peer
	end

	id = peers_cfg.norm_id(id)
	if id == nil then
		error("need id (linux|wsl|windows or custom)")
	end

	return id
end


local function _list(_args)
	local cfg = peers_cfg.load()
	local counts = peers_cfg.counts(cfg)
	local rows = {}
	for i = 1, #cfg.peers do
		local p = cfg.peers[i]
		rows[#rows + 1] = {
			id = p.id,
			host = p.host,
			port = p.port,
			scheme = p.scheme,
			state = p.state,
			cert_sha256 = p.cert_sha256,
			token_set = (type(p.token_ref) == "string" and p.token_ref ~= ""),
			profile_hint = p.profile_hint,
			approved_at = p.approved_at,
			note = p.note,
		}
	end

	return {
		peers = rows,
		counts = counts,
	}
end


local function _ping(args)
	local id = _id_of(args)
	local expanded = util.expand_peer_args({
		peer = id,
		timeout_sec = args and args.timeout_sec,
		scheme = args and args.scheme,
	}, { allow_pending = true })

	local host, port, token, timeout_sec, _chunk, scheme, pin, ca_path =
		util.resolve_peer(expanded, { allow_pending = true })

	local obj = transport.call_remote(
		host, port, "runtime.ping", {}, token, timeout_sec, nil, scheme, pin, ca_path, nil)

	return {
		peer = id,
		host = host,
		port = port,
		scheme = scheme,
		state = expanded._peer_row and expanded._peer_row.state,
		result = obj,
	}
end


-- @brief 建立/提升 peer 信任须 device+ (本机 CLI / raw / local 放行)
-- @return ok[boolean]
-- @return err[string|nil]
local function _may_approve()
	local ok_a, auth = pcall(require, "klbagent.core.auth")
	if not ok_a or type(auth) ~= "table" then
		return false, "auth unavailable"
	end

	local ra = nil
	if type(auth.request_auth) == "function" then
		ra = auth.request_auth()
	end

	-- 无请求上下文 (本机 CLI) / 原始 token / stdio-lpc 本地桥: 本地信任
	if type(ra) ~= "table" or ra.kind == "raw" or ra.kind == "local" then
		return true
	end

	local caller = "readonly"
	if type(auth.effective_profile) == "function" then
		caller = auth.effective_profile()
	end

	if profiles.rank(caller) < profiles.rank("device") then
		return false, "peer.approve needs device+ (temp token cannot create trust)"
	end

	return true
end


local function _approve(args)
	local id = _id_of(args)

	local ok_m, err_m = _may_approve()
	if not ok_m then
		error(tostring(err_m or "peer.approve denied"))
	end

	local cfg = peers_cfg.load()
	local row = peers_cfg.find(cfg, id)
	if row == nil then
		error("peer not found: " .. id .. " (register via config.set module=peers first)")
	end

	if row.state == "revoked" and args.force ~= true then
		error("peer revoked; pass force=true to re-approve")
	end

	local expanded = util.expand_peer_args({
		peer = id,
		timeout_sec = args and args.timeout_sec,
		scheme = args and args.scheme,
		token = args and args.token,
	}, { allow_pending = true })

	local host = expanded.host
	local port = expanded.port
	if type(port) ~= "number" or port < 1 or port > 65535 then
		error("alua.core.serve.UNAVAILABLE: need port")
	end

	local token = expanded.token
	if type(token) ~= "string" or token == "" then
		token = util.resolve_token_ref(row.token_ref, id)
	end

	if type(token) ~= "string" or token == "" then
		error("need token (token_ref / ALUA_SERVE_TOKEN)")
	end

	local scheme = util.normalize_scheme(expanded.scheme or row.scheme)
	local timeout_sec = args and args.timeout_sec
	if type(timeout_sec) ~= "number" or timeout_sec <= 0 then
		timeout_sec = 30
	end

	-- 批准仪式: 先取指纹 (TLS 面); 再 hostinfo
	local fp = transport.peer_cert_sha256(host, port)
	local pin = util.norm_fp(fp)
	if pin == nil and util.scheme_is_tls(scheme) then
		-- TLS 面强烈建议钉扎; 明文 scheme 可无
		if args.allow_no_pin ~= true then
			error("approve: cannot read peer cert fingerprint (openssl); pass allow_no_pin=true to skip")
		end
	end

	local hostinfo = transport.call_remote(
		host, port, "runtime.hostinfo", {}, token, timeout_sec, nil, scheme, pin, nil, nil)

	local hi = hostinfo
	if type(hostinfo) == "table" and type(hostinfo.result) == "table" then
		hi = hostinfo.result
	end

	local now = _iso_now()
	local note = row.note
	if type(args) == "table" and type(args.note) == "string" then
		note = args.note
	end

	local new_row = {
		id = id,
		host = host,
		port = port,
		scheme = scheme,
		cert_sha256 = pin or row.cert_sha256 or "",
		token_ref = row.token_ref,
		profile_hint = row.profile_hint,
		state = "trusted",
		approved_at = now,
		note = note,
	}

	if type(hi) == "table" then
		if type(hi.host_kind) == "string" and hi.host_kind ~= "" then
			new_row.profile_hint = hi.host_kind
		end
	end

	local new_cfg, saved = peers_cfg.upsert(cfg, new_row)
	local ok_save = peers_cfg.save(new_cfg)
	if not ok_save then
		error("save peers.json fail")
	end

	peers_cfg.clear_live()

	return {
		peer = saved.id,
		state = saved.state,
		approved_at = saved.approved_at,
		cert_sha256 = saved.cert_sha256,
		host = saved.host,
		port = saved.port,
		scheme = saved.scheme,
		hostinfo = hi,
		saved = true,
	}
end


local function _revoke(args)
	local id = _id_of(args)
	local cfg = peers_cfg.load()
	local row = peers_cfg.find(cfg, id)
	if row == nil then
		error("peer not found: " .. id)
	end

	local new_row = {
		id = row.id,
		host = row.host,
		port = row.port,
		scheme = row.scheme,
		cert_sha256 = row.cert_sha256,
		token_ref = row.token_ref,
		profile_hint = row.profile_hint,
		state = "revoked",
		approved_at = row.approved_at,
		note = row.note,
	}

	if type(args) == "table" and type(args.note) == "string" then
		new_row.note = args.note
	end

	local new_cfg, saved = peers_cfg.upsert(cfg, new_row)
	local ok_save = peers_cfg.save(new_cfg)
	if not ok_save then
		error("save peers.json fail")
	end

	peers_cfg.clear_live()

	return {
		peer = saved.id,
		state = saved.state,
		saved = true,
	}
end


M.register = function (catalog, dispatch_reg)
	local ok, err

	ok, err = catalog.register({
		name = "peer.list",
		category = "net",
		title = "list peers.json",
		summary = "List peers and counts; token_set bool only (no token_ref).",
		min_profile = "readonly",
		args_schema = {},
		examples = { { args = {} } },
		docs = "tools/peers.md",
		side_effects = {},
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch_reg.register("peer.list", _list)
	if not ok then
		return false, err
	end

	ok, err = catalog.register({
		name = "peer.ping",
		category = "net",
		title = "ping a named peer",
		summary = "Resolve peer= from peers.json (pending ok) and call runtime.ping.",
		min_profile = "net",
		args_schema = {
			id = "string optional peer id (linux|wsl|windows; win→windows; or peer=)",
			peer = "string optional alias of id",
			timeout_sec = "number optional",
			scheme = "string optional override",
		},
		examples = {
			{ args = { id = "windows" } },
		},
		docs = "tools/peers.md",
		side_effects = { "net" },
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch_reg.register("peer.ping", _ping)
	if not ok then
		return false, err
	end

	ok, err = catalog.register({
		name = "peer.approve",
		category = "net",
		title = "approve peer (trust ceremony)",
		summary = "Fetch cert fingerprint + runtime.hostinfo; set state=trusted and save peers.json. Needs device+ for temp tokens.",
		min_profile = "net",
		args_schema = {
			id = "string optional peer id (or peer=)",
			peer = "string optional alias of id",
			force = "boolean optional re-approve revoked",
			allow_no_pin = "boolean optional skip fingerprint fail",
			note = "string optional",
			timeout_sec = "number optional",
			scheme = "string optional",
			token = "string optional override",
		},
		examples = {
			{ args = { id = "windows" } },
		},
		docs = "tools/peers.md",
		side_effects = { "net", "write_fs" },
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch_reg.register("peer.approve", _approve)
	if not ok then
		return false, err
	end

	ok, err = catalog.register({
		name = "peer.revoke",
		category = "net",
		title = "revoke peer trust",
		summary = "Set peer state=revoked; remote.* peer= will refuse.",
		min_profile = "net",
		args_schema = {
			id = "string optional peer id (or peer=)",
			peer = "string optional alias of id",
			note = "string optional",
		},
		examples = {
			{ args = { id = "windows" } },
		},
		docs = "tools/peers.md",
		side_effects = { "write_fs" },
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch_reg.register("peer.revoke", _revoke)
	if not ok then
		return false, err
	end

	return true
end


return M

-- end
