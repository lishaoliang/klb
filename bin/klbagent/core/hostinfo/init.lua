--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  宿主形态与能力宣告 (host_kind / virt / caps)
-- @note   契约层不变; 供 selfcheck/health/deploy 决策; 可用 ALUA_HOST_KIND 覆盖
-- @history 修改历史
--  \n [2026-09] 创建文件
--]]


local M = {}


local _cache = nil


local function _trim(s)
	if type(s) ~= "string" then
		return ""
	end

	return (s:gsub("^%s+", ""):gsub("%s+$", ""))
end


-- Win 下勿用 2>/dev/null: cmd 会打印「系统找不到指定的路径」
local function _is_windows()
	if package.config:sub(1, 1) == "\\" then
		return true
	end

	local os_env = os.getenv("OS") or ""
	if string.find(string.lower(os_env), "windows", 1, true) ~= nil then
		return true
	end

	return false
end


local function _read_cmd(cmd)
	if _is_windows() then
		return ""
	end

	local f = io.popen(cmd .. " 2>/dev/null", "r")
	if f == nil then
		return ""
	end

	local out = f:read("*a") or ""
	f:close()
	return _trim(out)
end


local function _has_cmd(name)
	if _is_windows() then
		-- where.exe; 2>NUL 为 cmd 合法空设备
		local f = io.popen("where " .. tostring(name) .. " 2>NUL", "r")
		if f == nil then
			return false
		end

		local out = _trim(f:read("*l") or "")
		f:close()
		return out ~= ""
	end

	local out = _read_cmd("command -v " .. name)
	return out ~= ""
end


local function _read_file(path, maxn)
	local f = io.open(path, "r")
	if f == nil then
		return ""
	end

	local n = maxn or 4096
	local data = f:read(n) or ""
	f:close()
	return data
end


local function _file_exists(path)
	local f = io.open(path, "r")
	if f == nil then
		return false
	end

	f:close()
	return true
end


local function _win_arch()
	local a = os.getenv("PROCESSOR_ARCHITECTURE") or ""
	local low = string.lower(a)
	if low == "amd64" or low == "x86_64" then
		return "x86_64"
	end

	if low == "arm64" or low == "aarch64" then
		return "aarch64"
	end

	if low == "x86" then
		return "x86"
	end

	if a ~= "" then
		return a
	end

	return "unknown"
end


local function _is_wsl(osrelease, version_txt)
	if type(os.getenv("WSL_DISTRO_NAME")) == "string" and os.getenv("WSL_DISTRO_NAME") ~= "" then
		return true
	end

	if type(os.getenv("WSL_INTEROP")) == "string" and os.getenv("WSL_INTEROP") ~= "" then
		return true
	end

	local blob = string.lower((osrelease or "") .. "\n" .. (version_txt or ""))
	if string.find(blob, "microsoft", 1, true) ~= nil then
		return true
	end

	if string.find(blob, "wsl", 1, true) ~= nil then
		return true
	end

	return false
end


local function _detect_virt(host_kind, osrelease)
	if host_kind == "wsl" then
		local low = string.lower(osrelease or "")
		if string.find(low, "wsl2", 1, true) ~= nil then
			return "wsl2"
		end

		if string.find(low, "microsoft", 1, true) ~= nil then
			return "wsl2"
		end

		return "wsl2"
	end

	if _file_exists("/.dockerenv") or _file_exists("/run/.containerenv") then
		return "container"
	end

	local sdv = _read_cmd("systemd-detect-virt")
	if sdv ~= "" and sdv ~= "none" then
		local low = string.lower(sdv)
		if low == "qemu" or low == "kvm" or low == "xen" then
			return "qemu"
		end

		if low == "docker" or low == "podman" or low == "lxc" or low == "container" then
			return "container"
		end

		return "vm"
	end

	local cpu = string.lower(_read_file("/proc/cpuinfo", 8192))
	if string.find(cpu, "qemu", 1, true) ~= nil then
		return "qemu"
	end

	return "none"
end


local function _looks_embedded(arch, has_systemctl)
	local ov = os.getenv("ALUA_HOST_KIND")
	if type(ov) == "string" and string.lower(ov) == "embedded" then
		return true
	end

	local os_rel = _read_file("/etc/os-release", 8192)
	local low = string.lower(os_rel)
	if string.find(low, "id=buildroot", 1, true) ~= nil
		or string.find(low, "id=openwrt", 1, true) ~= nil
		or string.find(low, "id=yocto", 1, true) ~= nil then
		return true
	end

	local arm = string.find(arch, "arm", 1, true) == 1 or arch == "aarch64"
	if arm and _file_exists("/bin/busybox") and (not has_systemctl) then
		return true
	end

	return false
end


local function _build_caps(host_kind, has_systemctl, has_rsync, has_curl, has_wget, has_make)
	local linux_family = (host_kind == "linux" or host_kind == "wsl")
	local deploy_pull = linux_family and has_rsync and (has_curl or has_wget)
	-- source_install: 本机源码树 + make stage (层 D)
	local deploy_source = deploy_pull and has_make == true
	-- exec: 工具是否具备; 嵌入式仍可由 profile 关掉, 勿与门禁混用
	return {
		http_listen = true,
		deploy_pull = deploy_pull,
		deploy_source = deploy_source,
		systemctl = has_systemctl,
		exec = true,
		recommend_readonly = (host_kind == "embedded"),
	}
end


local function _network_hint(host_kind, virt)
	if host_kind == "wsl" then
		return "nat_tunnel"
	end

	if virt == "container" or virt == "vm" then
		return "nat_or_bridged"
	end

	if host_kind == "embedded" then
		return "lan_or_serial"
	end

	return "lan_direct"
end


-- @brief 探测宿主形态与能力 (进程内缓存)
-- @param [in] opts[table|nil]		可选 { refresh=true }
-- @return info[table]
M.probe = function (opts)
	if _cache ~= nil and not (type(opts) == "table" and opts.refresh == true) then
		return _cache
	end

	local override = os.getenv("ALUA_HOST_KIND")
	local win = _is_windows()
	local sysname = ""
	local arch = ""
	local osrelease = ""
	local version_txt = ""
	local has_systemctl = false
	local has_rsync = false
	local has_curl = false
	local has_wget = false
	local has_make = false

	-- 须先判 Win: 避免 uname/command -v + 2>/dev/null 刷屏
	if win then
		sysname = "Windows"
		arch = _win_arch()
		has_curl = _has_cmd("curl")
		has_make = _has_cmd("make")
	else
		sysname = _read_cmd("uname -s")
		arch = _read_cmd("uname -m")
		osrelease = _read_file("/proc/sys/kernel/osrelease", 512)
		version_txt = _read_file("/proc/version", 512)
		has_systemctl = _has_cmd("systemctl")
		has_rsync = _has_cmd("rsync")
		has_curl = _has_cmd("curl")
		has_wget = _has_cmd("wget")
		has_make = _has_cmd("make")
	end

	local host_kind = "linux"
	if type(override) == "string" and override ~= "" then
		host_kind = string.lower(override)
	elseif win then
		host_kind = "windows"
	elseif sysname == "Darwin" then
		host_kind = "macos"
	elseif sysname == "Linux" or sysname == "" then
		if _is_wsl(osrelease, version_txt) then
			host_kind = "wsl"
		elseif _looks_embedded(arch, has_systemctl) then
			host_kind = "embedded"
		else
			host_kind = "linux"
		end
	elseif sysname ~= "" then
		host_kind = "linux"
	end

	local virt = "none"
	if not win then
		virt = _detect_virt(host_kind, osrelease)
	end

	local caps = _build_caps(host_kind, has_systemctl, has_rsync, has_curl, has_wget, has_make)
	if host_kind == "embedded" or host_kind == "macos" or host_kind == "windows" then
		caps.deploy_pull = false
		caps.deploy_source = false
	end

	-- core.serve 包外契约已收口; 不再探 token/TLS 面
	local transport = { "http" }
	local token_set = false

	local info = {
		host_kind = host_kind,
		virt = virt,
		arch = arch ~= "" and arch or "unknown",
		sysname = sysname ~= "" and sysname or "unknown",
		network = _network_hint(host_kind, virt),
		transport = transport,
		caps = caps,
		token_set = token_set,
		override = (type(override) == "string" and override ~= "") and true or false,
	}

	_cache = info
	return info
end


-- @brief 清除探测缓存 (测试用)
M.reset_cache = function ()
	_cache = nil
end


return M

-- end
