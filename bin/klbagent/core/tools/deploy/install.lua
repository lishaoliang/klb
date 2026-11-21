--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   install.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  deploy.pull_install / self_install / source_install / scripts_install
-- @note   对端安装四类工具
-- @history 修改历史
--  \n [2026-10] 从 init.lua 拆出
--]]


local version = require("klbagent.version")
local hostinfo = require("klbagent.core.hostinfo")
local util = require("klbagent.core.tools.deploy.util")
local stage = require("klbagent.core.tools.deploy.stage")
local serve = require("klbagent.core.serve")


local M = {}


local function _check_source_caps()
	util.check_caps()
	local hi = hostinfo.probe()
	if type(hi.caps) ~= "table" or hi.caps.deploy_source ~= true then
		error("deploy.source_install unsupported (need caps.deploy_source; make + deploy_pull)")
	end
end


-- 层 D: 本机源码树 stage 后装入 /usr/local (无第二角色预置 tgz)
local function _source_install(args)
	_check_source_caps()

	local source_tree = args.source_tree
	if type(source_tree) ~= "string" or source_tree == "" then
		source_tree = util.default_source_tree()
	end

	if string.sub(source_tree, 1, 1) ~= "/" then
		error("source_tree must be absolute")
	end

	local stage_dir = args.stage_dir
	if type(stage_dir) ~= "string" or stage_dir == "" then
		stage_dir = util.default_stage_dir()
	end

	if string.sub(stage_dir, 1, 1) ~= "/" then
		error("stage_dir must be absolute")
	end

	local rebuild = true
	if args.rebuild == false then
		rebuild = false
	end

	if rebuild then
		local ok_b, out_b = stage.build_stage(source_tree, stage_dir)
		if not ok_b then
			error("stage build fail: " .. tostring(out_b))
		end
	end

	local prefix = stage_dir .. "/usr/local"
	local chk = io.open(prefix .. "/bin/alua", "r")
	if chk == nil then
		error("missing stage " .. prefix .. "/bin/alua (rebuild?)")
	end

	chk:close()

	local expect_version, min_version = util.pin_from(args)
	local ret = util.rsync_install(
		prefix,
		util.sudo_pass_from(args),
		util.restart_flag(args),
		expect_version,
		min_version)
	ret.source = "source_tree"
	ret.source_tree = source_tree
	ret.stage = stage_dir
	ret.rebuild = rebuild
	return ret
end


-- 层 E (脚本侧): 仅同步 klbagent Lua 树, 进程内 runtime.reload; 不热换 .so/bin
local function _scripts_install(args)
	util.check_caps()

	local path = args.path
	if type(path) ~= "string" or path == "" then
		path = util.default_source_tree()
	end

	if string.sub(path, 1, 1) ~= "/" then
		error("path must be absolute")
	end

	local src = util.resolve_klbagent_src(path)
	if src == nil then
		error("klbagent scripts not found under path (need VERSION + alua_boot.lua)")
	end

	local expect_version, min_version = util.pin_from(args)
	local stage_ver = util.read_text(src .. "/VERSION")
	local ok_pin, err_pin = util.check_version_pin(stage_ver, "", expect_version, min_version)
	if not ok_pin then
		error("scripts " .. err_pin)
	end

	local sudo_pass = util.sudo_pass_from(args)
	local sudo_prefix = "sudo -n "
	if sudo_pass ~= "" then
		sudo_prefix = "printf '%s\\n' " .. util.sh_quote(sudo_pass) .. " | sudo -S "
	end

	local dest = "/usr/local/share/klua/klbagent"
	local rb = string.format("/tmp/alua_scripts_rb_%d", math.floor(os.time()))
	util.run("mkdir -p " .. util.sh_quote(rb))
	local had_dest = util.run("test -d " .. util.sh_quote(dest))
	if had_dest then
		local ok_b, out_b = util.run(string.format(
			"%srsync -a %s/ %s/",
			sudo_prefix, util.sh_quote(dest), util.sh_quote(rb)))
		if not ok_b then
			util.run("rm -rf " .. util.sh_quote(rb))
			error("scripts backup fail: " .. tostring(out_b))
		end
	end

	local ok_rs, out_rs = util.run(string.format(
		"%smkdir -p %s && %srsync -a --delete %s/ %s/",
		sudo_prefix, util.sh_quote(dest),
		sudo_prefix, util.sh_quote(src), util.sh_quote(dest)))
	if not ok_rs then
		if had_dest then
			util.run(string.format(
				"%srsync -a --delete %s/ %s/",
				sudo_prefix, util.sh_quote(rb), util.sh_quote(dest)))
		end

		util.run("rm -rf " .. util.sh_quote(rb))
		error("scripts rsync fail: " .. tostring(out_rs))
	end

	local file_ver = util.read_text(dest .. "/VERSION")
	ok_pin, err_pin = util.check_version_pin(file_ver, "", expect_version, min_version)
	if not ok_pin then
		if had_dest then
			util.run(string.format(
				"%srsync -a --delete %s/ %s/",
				sudo_prefix, util.sh_quote(rb), util.sh_quote(dest)))
		end

		util.run("rm -rf " .. util.sh_quote(rb))
		error(err_pin .. "; scripts rolled back")
	end

	util.run("rm -rf " .. util.sh_quote(rb))

	-- 默认热更 Lua; restart=true 才再 schedule 进程重启 (一般不需要)
	local reload_ok = false
	local reload_ret = {}
	local do_reload = true
	if args.reload == false then
		do_reload = false
	end

	if do_reload then
		local ok_h, ret_h = util.hot_reload_klbagent()
		reload_ok = ok_h == true
		if type(ret_h) == "table" then
			reload_ret = ret_h
		else
			reload_ret = {
				ok = false,
				error = tostring(ret_h or "reload fail"),
			}
		end
	end

	local restart_done = false
	local restart_mode = "reload"
	if not reload_ok then
		restart_mode = "none"
	end

	if args.restart == true then
		local ok_r, mode = serve.schedule_restart({
			sudo_pass = sudo_pass,
			delay_sec = 2,
		})
		restart_done = ok_r == true
		restart_mode = mode or "none"
	end

	return {
		ok = true,
		installed = true,
		dest = dest,
		path = path,
		scripts_src = src,
		source = "scripts",
		file_version = file_ver,
		stage_version = stage_ver,
		agent_version = version.string,
		reload = reload_ok,
		reload_result = reload_ret,
		restart = restart_done,
		restart_requested = args.restart == true,
		restart_mode = restart_mode,
		rolled_back = false,
		note = "Lua only; .so/bin/alua still need source_install|self_install + process restart",
	}
end


local function _pull_install(args)
	local url = args.url
	if type(url) ~= "string" or url == "" then
		error("need url")
	end

	util.check_caps()

	if not string.match(url, "^https?://") then
		error("url must be http:// or https://")
	end

	local restart = util.restart_flag(args)
	local sudo_pass = util.sudo_pass_from(args)
	local expect_version, min_version = util.pin_from(args)
	local work = string.format("/tmp/alua_deploy_%d", math.floor(os.time()))
	local tgz = work .. ".tgz"

	local ok_mk, out_mk = util.run("mkdir -p " .. util.sh_quote(work))
	if not ok_mk then
		error("mkdir fail: " .. out_mk)
	end

	local dl = string.format(
		"curl -fsSL --connect-timeout 10 --max-time 300 -o %s %s || wget -q -O %s %s",
		util.sh_quote(tgz), util.sh_quote(url), util.sh_quote(tgz), util.sh_quote(url))
	local ok_dl, out_dl = util.run(dl)
	if not ok_dl then
		util.run("rm -rf " .. util.sh_quote(work) .. " " .. util.sh_quote(tgz))
		error("download fail: " .. out_dl)
	end

	local ok_tar, out_tar = util.run(string.format(
		"tar -xzf %s -C %s", util.sh_quote(tgz), util.sh_quote(work)))
	if not ok_tar then
		util.run("rm -rf " .. util.sh_quote(work) .. " " .. util.sh_quote(tgz))
		error("tar fail: " .. out_tar)
	end

	local src = util.resolve_stage_src(work)
	if src == nil then
		util.run("rm -rf " .. util.sh_quote(work) .. " " .. util.sh_quote(tgz))
		error("stage missing bin/alua")
	end

	local ret = util.rsync_install(src, sudo_pass, restart, expect_version, min_version)
	util.run("rm -rf " .. util.sh_quote(work) .. " " .. util.sh_quote(tgz))
	ret.url = url
	ret.source = "url"
	return ret
end


-- 本机 path: .tgz 或已解压 stage 目录 (含 bin/alua 或 usr/local/bin/alua)
local function _self_install(args)
	local path = args.path
	if type(path) ~= "string" or path == "" then
		error("need path")
	end

	if string.find(path, "[\r\n\0]") ~= nil then
		error("bad path")
	end

	if string.sub(path, 1, 1) ~= "/" then
		error("path must be absolute")
	end

	util.check_caps()

	local restart = util.restart_flag(args)
	local sudo_pass = util.sudo_pass_from(args)
	local expect_version, min_version = util.pin_from(args)
	local work = nil
	local src = nil
	local cleanup = nil

	local is_tgz = string.match(path, "%.tgz$") or string.match(path, "%.tar%.gz$")
	if is_tgz then
		local st = io.open(path, "rb")
		if st == nil then
			error("tgz not found: " .. path)
		end

		st:close()
		work = string.format("/tmp/alua_self_%d", math.floor(os.time()))
		local ok_mk, out_mk = util.run("mkdir -p " .. util.sh_quote(work))
		if not ok_mk then
			error("mkdir fail: " .. out_mk)
		end

		local ok_tar, out_tar = util.run(string.format(
			"tar -xzf %s -C %s", util.sh_quote(path), util.sh_quote(work)))
		if not ok_tar then
			util.run("rm -rf " .. util.sh_quote(work))
			error("tar fail: " .. out_tar)
		end

		src = util.resolve_stage_src(work)
		cleanup = work
	else
		src = util.resolve_stage_src(path)
	end

	if src == nil then
		if cleanup ~= nil then
			util.run("rm -rf " .. util.sh_quote(cleanup))
		end

		error("stage missing bin/alua under path")
	end

	local ret = util.rsync_install(src, sudo_pass, restart, expect_version, min_version)
	if cleanup ~= nil then
		util.run("rm -rf " .. util.sh_quote(cleanup))
	end

	ret.path = path
	ret.source = "path"
	return ret
end


-- @brief 注册 deploy 安装工具
-- @return ok[boolean]
-- @return err[string|nil]
M.register = function (catalog, dispatch)
	local ok, err = catalog.register({
		name = "deploy.pull_install",
		category = "unsafe",
		title = "pull stage tarball and install to /usr/local",
		summary = "Download http(s) tar.gz of usr/local layout; sudo rsync to /usr/local; verify version; optional restart (systemctl or exec). Needs unsafe.",
		min_profile = "unsafe",
		args_schema = {
			url = "string http(s) URL to .tgz",
			restart = "boolean optional default true (schedule serve restart after verify)",
			sudo_pass = "string optional; else env ALUA_DEPLOY_SUDO",
			expect_version = "string optional exact pin (VERSION or substring of alua -v)",
			min_version = "string optional numeric floor on VERSION",
		},
		examples = {
			{
				args = {
					url = "http://192.0.2.10:18080/alua_usr_local.tgz",
					restart = true,
				},
			},
		},
		docs = "tools/remote.md",
		side_effects = { "unsafe" },
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("deploy.pull_install", _pull_install)
	if not ok then
		return false, err
	end

	ok, err = catalog.register({
		name = "deploy.self_install",
		category = "unsafe",
		title = "install local stage path into /usr/local",
		summary = "Install from absolute path (.tgz/.tar.gz or extracted stage with bin/alua). Verify + rollback on fail. Needs unsafe + caps.deploy_pull.",
		min_profile = "unsafe",
		args_schema = {
			path = "string absolute path to .tgz or stage dir",
			restart = "boolean optional default true (schedule serve restart after verify)",
			sudo_pass = "string optional; else env ALUA_DEPLOY_SUDO",
			expect_version = "string optional exact pin (VERSION or substring of alua -v)",
			min_version = "string optional numeric floor on VERSION",
		},
		examples = {
			{
				args = {
					path = "/tmp/alua_usr_local.tgz",
					restart = true,
				},
			},
			{
				args = {
					path = "/tmp/alua_wsl_stage/usr/local",
					restart = false,
					expect_version = "0.1.72.202610",
				},
			},
		},
		docs = "tools/remote.md",
		side_effects = { "unsafe" },
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("deploy.self_install", _self_install)
	if not ok then
		return false, err
	end

	ok, err = catalog.register({
		name = "deploy.source_install",
		category = "unsafe",
		title = "build local source stage and install",
		summary = "Layer D: make stage from source tree on this host, then rsync /usr/local + verify/restart. Needs unsafe + caps.deploy_source.",
		min_profile = "unsafe",
		args_schema = {
			source_tree = "string optional absolute source tree root (else ALUA_SOURCE_TREE)",
			stage_dir = "string optional stage root (else ALUA_STAGE_DIR)",
			rebuild = "boolean optional default true",
			restart = "boolean optional default true",
			sudo_pass = "string optional; else env ALUA_DEPLOY_SUDO",
			expect_version = "string optional exact pin",
			min_version = "string optional numeric floor",
		},
		examples = {
			{
				args = {
					rebuild = true,
					restart = true,
				},
			},
		},
		docs = "tools/remote.md",
		side_effects = { "unsafe" },
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("deploy.source_install", _source_install)
	if not ok then
		return false, err
	end

	ok, err = catalog.register({
		name = "deploy.scripts_install",
		category = "unsafe",
		title = "install klbagent Lua scripts and hot-reload",
		summary = "Layer E scripts: rsync share/klua/klbagent only, then runtime.reload scope=klbagent. Does not hot-swap .so/bin. Needs unsafe.",
		min_profile = "unsafe",
		args_schema = {
			path = "string optional absolute klbagent dir or source tree root",
			reload = "boolean optional default true (runtime.reload)",
			restart = "boolean optional default false (process restart; rarely needed)",
			sudo_pass = "string optional; else env ALUA_DEPLOY_SUDO",
			expect_version = "string optional exact pin",
			min_version = "string optional numeric floor",
		},
		examples = {
			{
				args = {
					reload = true,
				},
			},
		},
		docs = "tools/remote.md",
		side_effects = { "unsafe" },
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("deploy.scripts_install", _scripts_install)
	if not ok then
		return false, err
	end

	return true
end


return M

-- end
