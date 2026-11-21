--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   deploy.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  remote.deploy
-- @history 修改历史
--  \n [2026-10] 从 init.lua 拆出; scheme 文档/契约扩 ws|wss (push/peer_tool 同族); scheme 扩 smp|mnp|smps|mnps
--]]


local deploy_util = require("klbagent.core.tools.deploy.util")
local transport = require("klbagent.core.tools.remote.transport")
local transfer = require("klbagent.core.tools.remote.transfer")
local util = require("klbagent.core.tools.remote.util")


local M = {}


local function _is_loopback(host)
	return host == "127.0.0.1" or host == "localhost" or host == "::1"
end


-- transport: same_fs | push | auto(默认); auto 对 loopback 先试 same_fs, 失败回退 push
local function _want_same_fs(args, host)
	local t = args.transport
	if t == "same_fs" then
		return true
	end

	if t == "push" then
		return false
	end

	return _is_loopback(host)
end


local function _deploy(args)
	args = util.expand_peer_args(args)

	local host = args.host
	if type(host) ~= "string" or host == "" then
		error("need host or peer")
	end

	local port = args.port
	if type(port) ~= "number" or port < 1 or 65535 < port then
		error("alua.core.serve.UNAVAILABLE: need port")
	end

	local source_tree = args.source_tree
	if type(source_tree) ~= "string" or source_tree == "" then
		source_tree = deploy_util.default_source_tree()
	end

	local stage = args.stage_dir
	if type(stage) ~= "string" or stage == "" then
		stage = deploy_util.default_stage_dir()
	end

	local rebuild = true
	if args.rebuild == false then
		rebuild = false
	end

	local restart = true
	if args.restart == false then
		restart = false
	end

	local token = util.resolve_call_token(args)
	if type(token) ~= "string" or token == "" then
		error("need token (ssid ensure failed; set serve.env / allow_raw_token)")
	end

	local scheme = util.normalize_scheme(args.scheme)
	local pin = util.resolve_pin(args)
	local ca_path = util.resolve_ca(args)

	local sudo_pass = args.sudo_pass
	if type(sudo_pass) ~= "string" then
		sudo_pass = os.getenv("ALUA_DEPLOY_SUDO") or ""
	end

	local timeout_sec = args.timeout_sec
	if type(timeout_sec) ~= "number" or timeout_sec <= 0 then
		timeout_sec = 180
	end

	if rebuild then
		local ok_b, out_b = transport.build_stage(source_tree, stage)
		if not ok_b then
			error("stage build fail: " .. tostring(out_b))
		end
	end

	local prefix = stage .. "/usr/local"
	local chk = io.open(prefix .. "/bin/alua", "r")
	if chk == nil then
		error("missing stage " .. prefix .. "/bin/alua (rebuild?)")
	end

	chk:close()

	local stamp = math.floor(os.time())
	local tgz_dir = "/tmp/alua_deploy_pack"
	local tgz_name = string.format("alua_usr_local_%d.tgz", stamp)
	local tgz_path = tgz_dir .. "/" .. tgz_name
	util.run("mkdir -p " .. util.sh_quote(tgz_dir))
	local ok_t, out_t = util.run(string.format(
		"tar -C %s -czf %s usr/local",
		util.sh_quote(stage), util.sh_quote(tgz_path)))
	if not ok_t then
		error("tar fail: " .. tostring(out_t))
	end

	local try_same_fs = _want_same_fs(args, host)
	local remote_tgz = tgz_path
	local pushed = false
	local used_scheme = scheme
	local transport_used = try_same_fs and "same_fs" or "push"
	local fallback_from_same_fs = false

	local function _do_push()
		remote_tgz = string.format("/tmp/%s", tgz_name)
		local push_ret = transfer.push({
			host = host,
			port = port,
			local_path = tgz_path,
			remote_path = remote_tgz,
			token = token,
			scheme = scheme,
			cert_sha256 = args.cert_sha256,
			ca_path = ca_path,
			timeout_sec = timeout_sec,
			chunk_bytes = args.chunk_bytes,
		})
		pushed = true
		transport_used = "push"
		if type(push_ret.scheme) == "string" and push_ret.scheme ~= "" then
			used_scheme = push_ret.scheme
			scheme = push_ret.scheme
		end
	end

	if not try_same_fs then
		_do_push()
	end

	local install_args = {
		path = remote_tgz,
		restart = restart,
	}
	if sudo_pass ~= "" then
		install_args.sudo_pass = sudo_pass
	end

	if type(args.expect_version) == "string" and args.expect_version ~= "" then
		install_args.expect_version = args.expect_version
	end

	if type(args.min_version) == "string" and args.min_version ~= "" then
		install_args.min_version = args.min_version
	end

	local alua_ssid = args._resolved_ssid or util.current_alua_ssid(args)

	local function _self_install()
		return transport.peer_tool(
			host, port, token, timeout_sec, "deploy.self_install", install_args,
			65536, scheme, pin, ca_path, alua_ssid)
	end

	local ok_call, result_or_err, peer_scheme = pcall(_self_install)

	-- auto: loopback same_fs 失败 (如 SSH 隧道非同文件系统) 回退 push
	if (not ok_call) and try_same_fs and args.transport ~= "same_fs" then
		fallback_from_same_fs = true
		_do_push()
		install_args.path = remote_tgz
		ok_call, result_or_err, peer_scheme = pcall(_self_install)
	end

	if pushed then
		pcall(function ()
			transport.peer_tool(
				host, port, token, 30, "fs.remove", { path = remote_tgz },
				65536, scheme, pin, ca_path, alua_ssid)
		end)
	end

	util.run("rm -f " .. util.sh_quote(tgz_path))

	if not ok_call then
		error("remote deploy fail: " .. tostring(result_or_err))
	end

	local obj = result_or_err
	if type(obj) ~= "table" then
		error("remote deploy bad result")
	end

	if type(peer_scheme) == "string" and peer_scheme ~= "" then
		used_scheme = peer_scheme
	end

	obj.host = host
	obj.port = port
	obj.scheme = used_scheme
	obj.stage = stage
	obj.remote_path = remote_tgz
	obj.transport = transport_used
	if fallback_from_same_fs then
		obj.transport_fallback = "same_fs->push"
	end

	return obj
end


M.register = function (catalog, dispatch_reg)
	local ok, err = catalog.register({
		name = "remote.deploy",
		category = "net",
		title = "deploy local stage to remote alua serve",
		summary = "Build usr/local stage, tar; same_fs or remote.push then peer deploy.self_install. transport=auto|same_fs|push. Needs profile net; peer unsafe.",
		min_profile = "net",
		args_schema = {
			host = "string optional remote host (or SSH-forward localhost; or use peer=)",
			peer = "string optional trusted peer id",
			port = "number optional peer serve port",
			source_tree = "string optional source tree root (else ALUA_SOURCE_TREE or $HOME/source_tree)",
			stage_dir = "string optional stage root (else ALUA_STAGE_DIR or $HOME/.tmp/alua_wsl_stage)",
			rebuild = "boolean optional default true",
			restart = "boolean optional default true",
			transport = "string optional auto|same_fs|push (default auto; auto falls back push if same_fs fails)",
			sudo_pass = "string optional peer sudo; else ALUA_DEPLOY_SUDO",
			expect_version = "string optional peer deploy version pin",
			min_version = "string optional peer deploy min version",
			token = "string optional debug only (normally omit; ssid ensure)",
			alua_ssid = "string optional bind ssid (default current); unique auth entry",
			allow_raw_token = "boolean optional allow raw serve.env on TLS only (default false)",
			scheme = "string optional auto|https|http|ws|wss|smp|mnp|smps|mnps|smprpc|mnprpc (raw token needs explicit TLS scheme: https|wss|smps|mnps)",
			cert_sha256 = "string optional peer cert SHA256 pin (else ALUA_SERVE_CERT_SHA256)",
			ca_path = "string optional CA PEM (else ALUA_SERVE_CA)",
			timeout_sec = "number optional (default 180)",
			chunk_bytes = "number optional push chunk size",
		},
		examples = {
			{
				args = {
					host = "127.0.0.1",
					port = 22817,
					rebuild = true,
					restart = true,
				},
			},
			{
				args = {
					host = "127.0.0.1",
					port = 22817,
					transport = "push",
					scheme = "wss",
				},
			},
		},
		docs = "tools/remote.md",
		side_effects = { "net" },
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch_reg.register("remote.deploy", _deploy)
	if not ok then
		return false, err
	end

	return true
end


return M

-- end
