--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   util.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  deploy.* 公共辅助
-- @note   stage/rsync/版本钉扎/热更等共享逻辑
-- @history 修改历史
--  \n [2026-10] 从 init.lua 拆出
--]]


local version = require("klbagent.version")
local hostinfo = require("klbagent.core.hostinfo")
local log = require("klbagent.core.log")
local serve = require("klbagent.core.serve")



local M = {}


M.VERSION_REL = "share/klua/klbagent/VERSION"


function M.sh_quote(s)
	return "'" .. tostring(s):gsub("'", "'\\''") .. "'"
end


function M.run(cmd)
	local f = io.popen(cmd .. " 2>&1", "r")
	if f == nil then
		return false, "", -1
	end

	local out = f:read("*a") or ""
	local ok, _, code = f:close()
	if ok then
		return true, out, 0
	end

	return false, out, tonumber(code) or 1
end


function M.trim(s)
	return (tostring(s or ""):gsub("^%s+", ""):gsub("%s+$", ""))
end


-- PATH 缺 /usr/sbin 时 (常见于精简 serve 环境) 回退绝对路径
local BIN_FALLBACKS = {
	rsync = { "/usr/bin/rsync", "/bin/rsync" },
	ldconfig = { "/usr/sbin/ldconfig", "/sbin/ldconfig" },
	mkdir = { "/usr/bin/mkdir", "/bin/mkdir" },
	visudo = { "/usr/sbin/visudo", "/usr/bin/visudo" },
}


-- @brief 解析可执行路径: command -v 优先, 否则已知绝对路径
-- @param [in] name[string]		如 rsync / ldconfig / mkdir
-- @return path[string]			空串表示未找到
function M.resolve_bin(name)
	if type(name) ~= "string" or name == "" then
		return ""
	end

	local ok, out = M.run("command -v " .. name)
	if ok then
		local p = M.trim(out)
		p = string.match(p, "^[^\r\n]+") or p
		if p ~= "" then
			return p
		end
	end

	local fb = BIN_FALLBACKS[name]
	if type(fb) == "table" then
		for i = 1, #fb do
			local path = fb[i]
			if M.run("test -x " .. M.sh_quote(path)) then
				return path
			end
		end
	end

	return ""
end


function M.read_text(path)
	local f = io.open(path, "rb")
	if f == nil then
		return ""
	end

	local data = f:read("*a") or ""
	f:close()
	return M.trim(data)
end


-- 解析 stage 根: 含 bin/alua, 或 usr/local/bin/alua
function M.resolve_stage_src(root)
	local src = root .. "/usr/local"
	local st = io.open(src .. "/bin/alua", "r")
	if st ~= nil then
		st:close()
		return src
	end

	st = io.open(root .. "/bin/alua", "r")
	if st ~= nil then
		st:close()
		return root
	end

	return nil
end


function M.version_parts(s)
	local parts = {}
	for n in string.gmatch(tostring(s or ""), "%d+") do
		parts[#parts + 1] = tonumber(n) or 0
	end

	return parts
end


function M.version_ge(a, b)
	local pa = M.version_parts(a)
	local pb = M.version_parts(b)
	local n = #pa
	if #pb > n then
		n = #pb
	end

	for i = 1, n do
		local x = pa[i] or 0
		local y = pb[i] or 0
		if x > y then
			return true
		end

		if x < y then
			return false
		end
	end

	return true
end


function M.stage_version(src)
	return M.read_text(src .. "/" .. M.VERSION_REL)
end


function M.installed_version()
	local file_ver = M.read_text("/usr/local/" .. M.VERSION_REL)
	local ok_v, out_v = M.run("/usr/local/bin/alua -v")
	local ver_out = ""
	if ok_v then
		ver_out = M.trim(out_v)
	end

	return file_ver, ver_out, ok_v
end


-- @return ok[boolean]
-- @return err[string]
function M.check_version_pin(file_ver, ver_out, expect_version, min_version)
	if type(expect_version) == "string" and expect_version ~= "" then
		local hit = (file_ver == expect_version)
			or (ver_out ~= "" and string.find(ver_out, expect_version, 1, true) ~= nil)
		if not hit then
			return false, "expect_version fail want=" .. expect_version
				.. " file=" .. file_ver .. " alua_v=" .. ver_out
		end
	end

	if type(min_version) == "string" and min_version ~= "" then
		local cur = file_ver
		if cur == "" then
			cur = ver_out
		end

		if cur == "" or not M.version_ge(cur, min_version) then
			return false, "min_version fail want>=" .. min_version
				.. " have=" .. cur
		end
	end

	return true, ""
end


function M.list_top_names(src)
	local ok, out = M.run("ls -1 " .. M.sh_quote(src))
	if not ok then
		return {}
	end

	local names = {}
	for line in string.gmatch(out, "[^\r\n]+") do
		if line ~= "" and line ~= "." and line ~= ".." then
			names[#names + 1] = line
		end
	end

	return names
end


-- 备份 dest 中与 stage 重叠的顶层目录/文件, 供失败回滚
function M.backup_overlap(src, sudo_prefix)
	local rb = string.format("/tmp/alua_rollback_%d", math.floor(os.time()))
	local ok_mk, out_mk = M.run("mkdir -p " .. M.sh_quote(rb))
	if not ok_mk then
		error("rollback mkdir fail: " .. out_mk)
	end

	local names = M.list_top_names(src)
	for i = 1, #names do
		local name = names[i]
		local dest_path = "/usr/local/" .. name
		local st = io.open(dest_path, "r")
		if st ~= nil then
			st:close()
			local ok_b, out_b = M.run(string.format(
				"%srsync -a %s %s/",
				sudo_prefix, M.sh_quote(dest_path), M.sh_quote(rb)))
			if not ok_b then
				M.run("rm -rf " .. M.sh_quote(rb))
				error("rollback backup fail: " .. out_b)
			end
		else
			-- 目录也可能无法用 io.open; 再试 test -e
			local ok_e = M.run("test -e " .. M.sh_quote(dest_path))
			if ok_e then
				local ok_b, out_b = M.run(string.format(
					"%srsync -a %s %s/",
					sudo_prefix, M.sh_quote(dest_path), M.sh_quote(rb)))
				if not ok_b then
					M.run("rm -rf " .. M.sh_quote(rb))
					error("rollback backup fail: " .. out_b)
				end
			end
		end
	end

	return rb
end


function M.do_rollback(rb, sudo_prefix)
	if type(rb) ~= "string" or rb == "" then
		return false, "no rollback dir"
	end

	local ok_r, out_r = M.run(string.format(
		"%srsync -a %s/ %s/", sudo_prefix, M.sh_quote(rb), M.sh_quote("/usr/local")))
	local ldconfig = M.resolve_bin("ldconfig")
	if ldconfig == "" then
		ldconfig = "ldconfig"
	end

	M.run(sudo_prefix .. M.sh_quote(ldconfig))
	if not ok_r then
		return false, out_r
	end

	return true, ""
end


-- 从已解压/已定位的 stage src 装入 /usr/local
function M.rsync_install(src, sudo_pass, restart, expect_version, min_version)
	local dest = "/usr/local"
	local sudo_prefix = "sudo -n "
	if sudo_pass ~= "" then
		sudo_prefix = "printf '%s\\n' " .. M.sh_quote(sudo_pass) .. " | sudo -S "
	end

	local stage_ver = M.stage_version(src)
	local ok_pin, err_pin = M.check_version_pin(stage_ver, "", expect_version, min_version)
	if not ok_pin then
		error("stage " .. err_pin)
	end

	local rb = M.backup_overlap(src, sudo_prefix)
	local rolled_back = false

	-- 仅合并装入; 不在 alua 内对目标做 --delete / 自行清残留 (运维或 install.mk 负责)
	local ok_rs, out_rs = M.run(string.format(
		"%srsync -a %s/ %s/", sudo_prefix, M.sh_quote(src), M.sh_quote(dest)))
	if not ok_rs then
		M.do_rollback(rb, sudo_prefix)
		M.run("rm -rf " .. M.sh_quote(rb))
		log.write("error", "deploy rsync fail", {
			module = "deploy",
			err = tostring(out_rs),
		})
		error("rsync install fail: " .. out_rs)
	end

	local ldconfig = M.resolve_bin("ldconfig")
	if ldconfig == "" then
		ldconfig = "ldconfig"
	end

	M.run(sudo_prefix .. M.sh_quote(ldconfig))

	local file_ver, ver_out, ok_v = M.installed_version()
	if not ok_v then
		local ok_rb, err_rb = M.do_rollback(rb, sudo_prefix)
		rolled_back = ok_rb
		M.run("rm -rf " .. M.sh_quote(rb))
		log.write("error", "deploy post-install verify fail", {
			module = "deploy",
			err = tostring(ver_out),
			rollback = rolled_back,
		})
		error("post-install alua -v fail; rollback="
			.. tostring(rolled_back) .. " " .. tostring(err_rb or ver_out))
	end

	ok_pin, err_pin = M.check_version_pin(file_ver, ver_out, expect_version, min_version)
	if not ok_pin then
		local ok_rb, err_rb = M.do_rollback(rb, sudo_prefix)
		rolled_back = ok_rb
		M.run("rm -rf " .. M.sh_quote(rb))
		log.write("error", "deploy version pin fail", {
			module = "deploy",
			err = tostring(err_pin),
			rollback = rolled_back,
		})
		error(err_pin .. "; rollback=" .. tostring(rolled_back)
			.. " " .. tostring(err_rb or ""))
	end

	-- 校验通过才允许 restart
	local restart_done = false
	local restart_mode = ""
	if restart then
		local ok_r, mode = serve.schedule_restart({
			sudo_pass = sudo_pass,
			delay_sec = 2,
		})
		restart_done = ok_r == true
		restart_mode = mode or "none"
		if not restart_done then
			restart_mode = "none"
		end
	end

	M.run("rm -rf " .. M.sh_quote(rb))

	return {
		ok = true,
		installed = true,
		dest = dest,
		restart = restart_done,
		restart_requested = restart,
		restart_mode = restart_mode,
		version = ver_out,
		file_version = file_ver,
		stage_version = stage_ver,
		agent_version = version.string,
		rolled_back = false,
	}
end


function M.check_caps()
	local hi = hostinfo.probe()
	if type(hi.caps) ~= "table" or hi.caps.deploy_pull ~= true then
		error("deploy unsupported on host_kind="
			.. tostring(hi.host_kind)
			.. " (need caps.deploy_pull; linux/wsl + rsync + curl|wget)")
	end
end


function M.sudo_pass_from(args)
	local sudo_pass = args.sudo_pass
	if type(sudo_pass) ~= "string" or sudo_pass == "" then
		sudo_pass = os.getenv("ALUA_DEPLOY_SUDO") or ""
	end

	return sudo_pass
end


function M.restart_flag(args)
	if args.restart == false then
		return false
	end

	return true
end


function M.pin_from(args)
	local expect_version = ""
	if type(args.expect_version) == "string" then
		expect_version = args.expect_version
	end

	local min_version = ""
	if type(args.min_version) == "string" then
		min_version = args.min_version
	end

	return expect_version, min_version
end


function M.write_file(path, text)
	local f, err = io.open(path, "wb")
	if f == nil then
		return false, err
	end

	f:write(text)
	f:close()
	return true
end


local function _home_dir()
	local home = os.getenv("HOME")
	if type(home) == "string" and home ~= "" then
		return home
	end

	return nil
end


-- @brief 默认源码树根: ALUA_SOURCE_TREE, 否则 $HOME/source_tree (须存在)
-- @return path[string]
function M.default_source_tree()
	local od = os.getenv("ALUA_SOURCE_TREE")
	if type(od) == "string" and od ~= "" then
		return od
	end

	local home = _home_dir()
	if home ~= nil then
		local cand = home .. "/source_tree"
		local st = io.open(cand .. "/Makefile", "r")
		if st ~= nil then
			st:close()
			return cand
		end
	end

	error("need source tree path (args.source_tree or ALUA_SOURCE_TREE)")
end


-- @brief 默认 stage 根: ALUA_STAGE_DIR, 否则 $HOME/.tmp/…, 再否则 /tmp/…
-- @return path[string]
function M.default_stage_dir()
	local st = os.getenv("ALUA_STAGE_DIR")
	if type(st) == "string" and st ~= "" then
		return st
	end

	local home = _home_dir()
	if home ~= nil then
		return home .. "/.tmp/alua_wsl_stage"
	end

	return "/tmp/alua_wsl_stage"
end


function M.resolve_klbagent_src(path)
	local candidates = {
		path,
		path .. "/klb/bin/klbagent",
		path .. "/share/klua/klbagent",
		path .. "/usr/local/share/klua/klbagent",
	}

	for i = 1, #candidates do
		local p = candidates[i]
		local ver = M.read_text(p .. "/VERSION")
		local boot = io.open(p .. "/alua_boot.lua", "r")
		if ver ~= "" and boot ~= nil then
			boot:close()
			return p
		end

		if boot ~= nil then
			boot:close()
		end
	end

	return nil
end


function M.hot_reload_klbagent()
	local catalog = require("klbagent.core.catalog")
	local dispatch = require("klbagent.core.dispatch")

	-- 磁盘已 rsync 新脚本; 先换上新 runtime.reload, 再 scope=klbagent (sticky)
	package.loaded["klbagent.core.tools.runtime"] = nil
	local runtime = require("klbagent.core.tools.runtime")
	local ok_r, err_r = runtime.register(catalog, dispatch)
	if not ok_r then
		return false, { error = tostring(err_r or "runtime rebind fail") }
	end

	local ok, ret = dispatch.call({
		name = "runtime.reload",
		args = {
			scope = "klbagent",
		},
	})
	if not ok then
		return false, ret
	end

	return true, ret
end


return M

-- end
