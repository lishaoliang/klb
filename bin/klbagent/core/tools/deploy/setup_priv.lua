--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   setup_priv.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  deploy.setup_priv
-- @note   目标机一次配置限定 NOPASSWD (方案 1)
-- @history 修改历史
--  \n [2026-10] 从 init.lua 拆出
--]]


local hostinfo = require("klbagent.core.hostinfo")
local util = require("klbagent.core.tools.deploy.util")


local M = {}


local SUDOERS_DROPIN = "/etc/sudoers.d/alua-deploy"


-- 限定 NOPASSWD 只含 rsync/ldconfig/mkdir; 勿用 sudo -n true (会误报失败)
local function _probe_sudo_n()
	local rsync = util.resolve_bin("rsync")
	if rsync == "" then
		rsync = "/usr/bin/rsync"
	end

	local ok = util.run("sudo -n " .. util.sh_quote(rsync) .. " --version")
	return ok == true
end


local function _safe_unix_user(name)
	if type(name) ~= "string" or name == "" then
		return false
	end

	if #name > 32 then
		return false
	end

	-- 拒绝路径/空白/注入字符; 允许常见 Linux 用户名
	if string.match(name, "^[a-z_][a-z0-9_-]*$") == nil then
		return false
	end

	return true
end


local function _elev_prefix(sudo_pass)
	if sudo_pass ~= "" then
		return "printf '%s\\n' " .. util.sh_quote(sudo_pass) .. " | sudo -S "
	end

	return "sudo -n "
end


-- 方案 1: 目标机一次写入限定 NOPASSWD; 后续 deploy 用 sudo -n
local function _setup_priv(args)
	args = args or {}
	local hi = hostinfo.probe()
	local kind = hi.host_kind
	if kind ~= "linux" and kind ~= "wsl" then
		error("deploy.setup_priv only on linux/wsl (host_kind=" .. tostring(kind) .. ")")
	end

	local check_only = args.check_only == true
	local sudo_n_before = _probe_sudo_n()

	if check_only then
		local drop_exists = util.run("test -f " .. util.sh_quote(SUDOERS_DROPIN))
		return {
			ok = true,
			check_only = true,
			sudo_n = sudo_n_before,
			dropin = SUDOERS_DROPIN,
			dropin_present = drop_exists == true,
			host_kind = kind,
			user = "",
			commands = {},
			sudo_n_before = sudo_n_before,
			note = "check only; pass confirm=true to install limited NOPASSWD",
		}
	end

	if args.confirm ~= true then
		error("need confirm=true to write " .. SUDOERS_DROPIN .. " (or check_only=true)")
	end

	local user = args.user
	if type(user) ~= "string" or user == "" then
		local ok_u, out_u = util.run("id -un")
		if not ok_u then
			error("cannot detect user")
		end

		user = util.trim(out_u)
	end

	if not _safe_unix_user(user) then
		error("bad user name")
	end

	local rsync = util.resolve_bin("rsync")
	local ldconfig = util.resolve_bin("ldconfig")
	local mkdir = util.resolve_bin("mkdir")
	if rsync == "" or ldconfig == "" or mkdir == "" then
		error("need rsync+ldconfig+mkdir (PATH or absolute fallback) (rsync="
			.. rsync .. " ldconfig=" .. ldconfig .. " mkdir=" .. mkdir .. ")")
	end

	-- 不写 Defaults:* !requiretty: sudo-rs(Ubuntu 新默认) 不支持该设置, visudo 会拒绝
	local body = table.concat({
		"# Managed by alua deploy.setup_priv; limited NOPASSWD for deploy.*",
		"# Do not use NOPASSWD: ALL",
		string.format(
			"%s ALL=(root) NOPASSWD: %s, %s, %s",
			user, rsync, ldconfig, mkdir),
		"",
	}, "\n")

	local tmp = string.format("/tmp/alua_sudoers_%d", math.floor(os.time()))
	local ok_w, err_w = util.write_file(tmp, body)
	if not ok_w then
		error("write temp sudoers fail: " .. tostring(err_w))
	end

	-- 安装前语法检查 (visudo -cf 可读用户文件); PATH 缺 /usr/sbin 时回退绝对路径
	local visudo = util.resolve_bin("visudo")
	if visudo == "" then
		visudo = "visudo"
	end

	local ok_cf, out_cf = util.run(util.sh_quote(visudo) .. " -cf " .. util.sh_quote(tmp))
	if not ok_cf then
		util.run("rm -f " .. util.sh_quote(tmp))
		error("visudo -cf fail: " .. tostring(out_cf))
	end

	local sudo_pass = util.sudo_pass_from(args)
	local elev = _elev_prefix(sudo_pass)
	-- 已是 root 则直写
	local uid_ok, uid_out = util.run("id -u")
	local is_root = uid_ok and util.trim(uid_out) == "0"
	if is_root then
		elev = ""
	end

	local ok_inst, out_inst = util.run(string.format(
		"%scp %s %s && %schmod 0440 %s",
		elev, util.sh_quote(tmp), util.sh_quote(SUDOERS_DROPIN),
		elev, util.sh_quote(SUDOERS_DROPIN)))
	util.run("rm -f " .. util.sh_quote(tmp))
	if not ok_inst then
		error("install sudoers fail (need sudo_pass or existing sudo): " .. tostring(out_inst))
	end

	local ok_cf2, out_cf2 = util.run(elev .. util.sh_quote(visudo) .. " -cf " .. util.sh_quote(SUDOERS_DROPIN))
	if not ok_cf2 then
		error("installed but visudo -cf fail: " .. tostring(out_cf2)
			.. "; remove " .. SUDOERS_DROPIN .. " manually if needed")
	end

	local sudo_n_after = _probe_sudo_n()
	if not sudo_n_after then
		error("sudoers installed but sudo -n still fail; check Defaults/path")
	end

	return {
		ok = true,
		check_only = false,
		user = user,
		dropin = SUDOERS_DROPIN,
		dropin_present = true,
		commands = {
			rsync = rsync,
			ldconfig = ldconfig,
			mkdir = mkdir,
		},
		sudo_n_before = sudo_n_before,
		sudo_n = true,
		host_kind = kind,
		note = "limited NOPASSWD ready; subsequent deploy.* can use sudo -n",
	}
end


-- @brief 注册 deploy.setup_priv
-- @return ok[boolean]
-- @return err[string|nil]
M.register = function (catalog, dispatch)
	local ok, err = catalog.register({
		name = "deploy.setup_priv",
		category = "unsafe",
		title = "one-shot limited NOPASSWD for deploy",
		summary = "Scheme 1: once on target, write /etc/sudoers.d/alua-deploy (rsync/ldconfig/mkdir only). Then deploy.* uses sudo -n. Needs confirm=true + sudo once. Linux/wsl only.",
		min_profile = "unsafe",
		args_schema = {
			confirm = "boolean optional; must be true to write sudoers (with check_only may omit)",
			check_only = "boolean optional probe sudo -n / drop-in without writing",
			user = "string optional unix user (default id -un)",
			sudo_pass = "string optional once for installing drop-in; else ALUA_DEPLOY_SUDO or existing sudo -n",
		},
		examples = {
			{
				args = {
					check_only = true,
				},
			},
			{
				args = {
					confirm = true,
				},
			},
		},
		docs = "tools/remote.md",
		side_effects = { "unsafe" },
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("deploy.setup_priv", _setup_priv)
	if not ok then
		return false, err
	end

	return true
end


return M

-- end
