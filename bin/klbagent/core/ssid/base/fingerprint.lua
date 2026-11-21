--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   fingerprint.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  SSID 指纹采集 (MCP / CLI / 宿主 env)
-- @stability internal
-- @impl evolving
-- @note   仅作 material / 标签; 不当鉴权; 路径只存 hash
--   \n 对齐 packages/mcp/DESIGN.md 第 3 节
--   \n require("klbagent.core.ssid.base.fingerprint")
-- @history 修改历史
--  \n [2026-11] 创建文件 (桩); M3 采集/分类/material
--  \n [2026-11] M5 宿主指纹深化: hostinfo/CI/tty + HTTP 头摘要
--  \n [2026-11] 迁入 base/
--]]


local M = {}


local function _trim(s)
	if type(s) ~= "string" then
		return ""
	end

	return (s:gsub("^%s+", ""):gsub("%s+$", ""))
end


-- @brief FNV-1a 32bit → 8 hex (脱敏)
-- @param [in] s[string]
-- @return hex[string]
local function _fnv1a_hex(s)
	if type(s) ~= "string" then
		s = ""
	end

	local h = 2166136261
	for i = 1, #s do
		h = (h ~ string.byte(s, i)) & 0xffffffff
		h = (h * 16777619) & 0xffffffff
	end

	return string.format("%08x", h)
end


-- @brief FNV-1a 64bit → 16 hex (新装环境摘要; 比 32bit 单轮更难碰撞)
-- @param [in] s[string]
-- @return hex[string]
local function _fnv1a64_hex(s)
	if type(s) ~= "string" then
		s = ""
	end

	local h = 0xcbf29ce484222325
	for i = 1, #s do
		h = h ~ string.byte(s, i)
		h = h * 0x100000001b3
	end

	return string.format("%016x", h)
end


local function _env_flag(name)
	local v = os.getenv(name)
	return type(v) == "string" and v ~= ""
end


local function _is_windows()
	if package.config:sub(1, 1) == "\\" then
		return true
	end

	local os_env = os.getenv("OS") or ""
	return string.find(string.lower(os_env), "windows", 1, true) ~= nil
end


local function _cwd()
	local ok_l, lfs = pcall(require, "lfs")
	if ok_l and type(lfs) == "table" and type(lfs.currentdir) == "function" then
		local d = lfs.currentdir()
		if type(d) == "string" then
			return d
		end
	end

	return ""
end


local function _ppid_name()
	if _is_windows() then
		return "", ""
	end

	local f = io.open("/proc/self/stat", "r")
	if f == nil then
		return "", ""
	end

	local line = f:read("*l") or ""
	f:close()
	-- pid (comm) state ppid ...
	local ppid = string.match(line, "^%d+%s+%([^%)]*%)%s+%S+%s+(%d+)")
	if type(ppid) ~= "string" or ppid == "" then
		return "", ""
	end

	local cf = io.open("/proc/" .. ppid .. "/comm", "r")
	if cf == nil then
		return ppid, ""
	end

	local comm = _trim(cf:read("*l") or "")
	cf:close()
	return ppid, comm
end


local function _tty_hint()
	if _is_windows() then
		return false
	end

	local ok, f = pcall(io.open, "/dev/tty", "r")
	if ok and f ~= nil then
		f:close()
		return true
	end

	return false
end


-- @brief 采集调用方指纹
-- @param [in] opts[table|nil]		via?, mcp_init?, env_extra?, caller_kind?
-- @return fp[table|nil]
-- @return err[string|nil]
M.collect = function (opts)
	if type(opts) ~= "table" then
		opts = {}
	end

	local via = opts.via
	if type(via) ~= "string" or via == "" then
		via = "call"
	end

	local host_kind = "unknown"
	local arch = ""
	local virt = ""
	local ok_h, hostinfo = pcall(require, "klbagent.core.hostinfo")
	if ok_h and type(hostinfo) == "table" and type(hostinfo.probe) == "function" then
		local hi = hostinfo.probe()
		if type(hi) == "table" then
			if type(hi.host_kind) == "string" then
				host_kind = hi.host_kind
			end

			if type(hi.arch) == "string" then
				arch = hi.arch
			end

			if type(hi.virt) == "string" then
				virt = hi.virt
			end
		end
	end

	local ppid, pcomm = _ppid_name()
	local cwd = _cwd()
	local env = {
		cursor = _env_flag("CURSOR_TRACE_ID") or _env_flag("CURSOR_AGENT")
			or _env_flag("CURSOR_SESSION_ID"),
		codebuddy = _env_flag("CODEBUDDY_API_KEY") or _env_flag("CODEBUDDY_HOME"),
		trae = _env_flag("TRAE_HOME") or _env_flag("TRAE_AGENT"),
		vscode = _env_flag("VSCODE_PID") or _env_flag("VSCODE_IPC_HOOK"),
		claude = _env_flag("CLAUDE_CODE") or _env_flag("CLAUDECODE"),
		ci = _env_flag("CI") or _env_flag("GITHUB_ACTIONS") or _env_flag("GITLAB_CI")
			or _env_flag("JENKINS_HOME") or _env_flag("BUILD_ID") or _env_flag("TEAMCITY_VERSION")
			or _env_flag("CIRCLECI") or _env_flag("TRAVIS"),
		ssh = _env_flag("SSH_CLIENT") or _env_flag("SSH_CONNECTION"),
		term = _env_flag("TERM"),
		term_program = os.getenv("TERM_PROGRAM") or "",
		alua_caller = os.getenv("ALUA_CALLER_KIND") or "",
		alua_name = os.getenv("ALUA_CALLER_NAME") or "",
		alua_ssid = os.getenv("ALUA_SSID") or "",
	}

	if type(opts.env_extra) == "table" then
		for k, v in pairs(opts.env_extra) do
			if type(k) == "string" then
				env[k] = v
			end
		end
	end

	local mcp = nil
	if type(opts.mcp_init) == "table" then
		local ci = opts.mcp_init.clientInfo or opts.mcp_init.client_info
		mcp = {
			protocol_version = opts.mcp_init.protocolVersion or opts.mcp_init.protocol_version,
			client_name = type(ci) == "table" and ci.name or nil,
			client_version = type(ci) == "table" and ci.version or nil,
			client_title = type(ci) == "table" and ci.title or nil,
		}
		if type(opts.mcp_init.capabilities) == "table" then
			local keys = {}
			for k, _ in pairs(opts.mcp_init.capabilities) do
				if type(k) == "string" then
					keys[#keys + 1] = k
				end
			end

			table.sort(keys)
			mcp.cap_keys = keys
		end
	end

	-- HTTP 传输面摘要 (仅 hash / 短标; 不当鉴权; M5/M6)
	local http = nil
	if type(opts.http_headers) == "table" then
		local h = opts.http_headers
		local ua = type(h.user_agent) == "string" and h.user_agent or ""
		local accept = type(h.accept) == "string" and h.accept or ""
		local proto = type(h.protocol_version) == "string" and h.protocol_version or ""
		local sid = type(h.session_id) == "string" and h.session_id or ""
		http = {
			ua_hash = (ua ~= "") and _fnv1a_hex(ua) or "",
			accept_hash = (accept ~= "") and _fnv1a_hex(accept) or "",
			protocol_version = proto,
			session_id_hash = (sid ~= "") and _fnv1a_hex(sid) or "",
		}
	end

	local fp = {
		via = via,
		host_kind = host_kind,
		arch = arch,
		virt = virt,
		ppid_hash = (ppid ~= "") and _fnv1a_hex(ppid) or "",
		parent_comm = pcomm,
		cwd_hash = (cwd ~= "") and _fnv1a_hex(cwd) or "",
		tty = _tty_hint(),
		env = env,
		mcp = mcp,
		http = http,
		uid_hash = _fnv1a_hex(tostring(os.getenv("USER") or os.getenv("USERNAME") or "")),
	}

	if type(opts.caller_kind) == "string" and opts.caller_kind ~= "" then
		fp.caller_kind_hint = opts.caller_kind
	end

	return fp
end


-- 已知 MCP AI 宿主标准名 (mcp-xx); 未适配 / 未知 = mcp-unknown
-- 配置 JSON 键仍用 "alua"; 本表只作身份标签 (ALUA_CALLER_NAME / whoami.host_tool)
local HOST_TOOL_KNOWN = {
	["mcp-cursor"] = true,
	["mcp-codebuddy"] = true,
	["mcp-trae"] = true,
	["mcp-vscode"] = true,
	["mcp-copilot"] = true,
	["mcp-claude"] = true,
	["mcp-claude-desktop"] = true,
	["mcp-claude-code"] = true,
	["mcp-windsurf"] = true,
	["mcp-unknown"] = true,
}


-- @brief 从 clientInfo.name / 父进程名粗匹配 mcp-xx
-- @param [in] name[string]
-- @return host_tool[string|nil]
local function _match_mcp_name(name)
	if type(name) ~= "string" or name == "" then
		return nil
	end

	local n = string.lower(name)
	if string.find(n, "cursor", 1, true) then
		return "mcp-cursor"
	end

	if string.find(n, "codebuddy", 1, true) then
		return "mcp-codebuddy"
	end

	if string.find(n, "trae", 1, true) then
		return "mcp-trae"
	end

	if string.find(n, "windsurf", 1, true) or string.find(n, "codeium", 1, true) then
		return "mcp-windsurf"
	end

	if string.find(n, "copilot", 1, true) then
		return "mcp-copilot"
	end

	if string.find(n, "claude", 1, true) then
		if string.find(n, "desktop", 1, true) then
			return "mcp-claude-desktop"
		end

		if string.find(n, "code", 1, true) then
			return "mcp-claude-code"
		end

		return "mcp-claude"
	end

	if string.find(n, "vscode", 1, true) or string.find(n, "visual studio code", 1, true)
		or n == "code" then
		return "mcp-vscode"
	end

	return nil
end


-- @brief 解析 AI/MCP 标准名 host_tool (mcp-xx); 非 MCP 面返回 ""
-- @param [in] fp[table|nil]
-- @return host_tool[string]
M.resolve_host_tool = function (fp)
	if type(fp) ~= "table" then
		return ""
	end

	local env = fp.env
	if type(env) ~= "table" then
		env = {}
	end

	-- 显式覆盖 (仍只作标签)
	local override = env.alua_name
	if type(override) == "string" and override ~= "" then
		override = string.lower(_trim(override))
		if HOST_TOOL_KNOWN[override] or string.sub(override, 1, 4) == "mcp-" then
			return override
		end
	end

	if env.cursor == true then
		return "mcp-cursor"
	end

	if env.codebuddy == true then
		return "mcp-codebuddy"
	end

	if env.trae == true then
		return "mcp-trae"
	end

	if env.claude == true then
		return "mcp-claude"
	end

	if env.vscode == true then
		local from_mcp = nil
		if type(fp.mcp) == "table" then
			from_mcp = _match_mcp_name(fp.mcp.client_name)
		end

		if from_mcp == "mcp-copilot" then
			return "mcp-copilot"
		end

		return "mcp-vscode"
	end

	local via = fp.via
	if via == "mcp" then
		local client = ""
		if type(fp.mcp) == "table" and type(fp.mcp.client_name) == "string" then
			client = fp.mcp.client_name
		end

		local hit = _match_mcp_name(client)
		if hit ~= nil then
			return hit
		end

		hit = _match_mcp_name(fp.parent_comm)
		if hit ~= nil then
			return hit
		end

		return "mcp-unknown"
	end

	return ""
end


-- @brief 粗判 caller_kind
-- @param [in] fp[table|nil]
-- @return kind[string]
M.classify = function (fp)
	if type(fp) ~= "table" then
		return "unknown"
	end

	-- 忽略过时 hint "mcp"(非 DESIGN 枚举); 其余显式 hint 仍优先
	if type(fp.caller_kind_hint) == "string" and fp.caller_kind_hint ~= ""
		and fp.caller_kind_hint ~= "mcp" then
		return fp.caller_kind_hint
	end

	local env = fp.env
	if type(env) ~= "table" then
		env = {}
	end

	if type(env.alua_caller) == "string" and env.alua_caller ~= "" then
		local k = string.lower(env.alua_caller)
		if k ~= "mcp" then
			return k
		end
	end

	if env.ci == true then
		return "ci"
	end

	local host_tool = M.resolve_host_tool(fp)
	if host_tool ~= "" and host_tool ~= "mcp-unknown" then
		return "ai_host"
	end

	if env.cursor or env.codebuddy or env.trae or env.vscode or env.claude then
		return "ai_host"
	end

	local via = fp.via
	if via == "mcp" then
		if host_tool == "mcp-unknown" then
			-- 未适配 AI 或自研 client: 有 AI 信号则 ai_host, 否则 mcp_client
			local name = ""
			if type(fp.mcp) == "table" and type(fp.mcp.client_name) == "string" then
				name = fp.mcp.client_name
			end

			if name ~= "" then
				return "ai_host"
			end

			return "mcp_client"
		end

		return "mcp_client"
	end

	if via == "serve" or via == "http" or via == "https" or via == "remote" then
		return "remote_peer"
	end

	if fp.tty == true and not env.ci then
		local p = string.lower(tostring(fp.parent_comm or ""))
		if p == "bash" or p == "zsh" or p == "sh" or p == "fish"
			or p == "cmd.exe" or p == "powershell.exe" or p == "pwsh.exe" then
			return "human_cli"
		end

		return "human_cli"
	end

	if fp.tty ~= true then
		return "script"
	end

	return "unknown"
end


-- @brief 规范化 material (稳定字符串字段; 无秘密明文)
-- @param [in] fp[table|nil]
-- @return material[table|nil]
-- @return err[string|nil]
M.material = function (fp)
	if type(fp) ~= "table" then
		return nil, "need fingerprint"
	end

	local kind = M.classify(fp)
	local host_tool = M.resolve_host_tool(fp)
	local client_name = ""
	if type(fp.mcp) == "table" and type(fp.mcp.client_name) == "string" then
		client_name = fp.mcp.client_name
	elseif type(fp.parent_comm) == "string" then
		client_name = fp.parent_comm
	end

	local env = fp.env or {}
	local env_bits = {}
	local keys = {
		"cursor", "codebuddy", "trae", "vscode", "claude", "ci", "ssh", "term",
	}
	for i = 1, #keys do
		local k = keys[i]
		if env[k] == true then
			env_bits[#env_bits + 1] = k
		end
	end

	if type(env.term_program) == "string" and env.term_program ~= "" then
		env_bits[#env_bits + 1] = "term_program"
	end

	table.sort(env_bits)

	local mat = {
		via = tostring(fp.via or "call"),
		caller_kind = kind,
		host_tool = host_tool,
		client_name = client_name,
		host_kind = tostring(fp.host_kind or ""),
		arch = tostring(fp.arch or ""),
		virt = tostring(fp.virt or ""),
		cwd_hash = tostring(fp.cwd_hash or ""),
		uid_hash = tostring(fp.uid_hash or ""),
		ppid_hash = tostring(fp.ppid_hash or ""),
		parent_comm = tostring(fp.parent_comm or ""),
		tty = fp.tty == true and "1" or "0",
		env_bits = table.concat(env_bits, ","),
		protocol_version = "",
		http_ua_hash = "",
		http_accept_hash = "",
		http_session_hash = "",
	}

	if type(fp.mcp) == "table" and fp.mcp.protocol_version ~= nil then
		mat.protocol_version = tostring(fp.mcp.protocol_version)
	end

	if type(fp.http) == "table" then
		mat.http_ua_hash = tostring(fp.http.ua_hash or "")
		mat.http_accept_hash = tostring(fp.http.accept_hash or "")
		mat.http_session_hash = tostring(fp.http.session_id_hash or "")
		if mat.protocol_version == "" and type(fp.http.protocol_version) == "string" then
			mat.protocol_version = fp.http.protocol_version
		end
	end

	return mat
end


-- @brief material → 稳定拼接串 (供推 SSID)
-- @param [in] mat[table]
-- @return s[string]
M.material_string = function (mat)
	if type(mat) ~= "table" then
		return ""
	end

	return table.concat({
		mat.via or "",
		mat.caller_kind or "",
		mat.host_tool or "",
		mat.client_name or "",
		mat.host_kind or "",
		mat.arch or "",
		mat.virt or "",
		mat.cwd_hash or "",
		mat.uid_hash or "",
		mat.ppid_hash or "",
		mat.parent_comm or "",
		mat.tty or "",
		mat.env_bits or "",
		mat.protocol_version or "",
		mat.http_ua_hash or "",
		mat.http_accept_hash or "",
		mat.http_session_hash or "",
	}, "|")
end


-- 标准名表只读导出 (文档/测试对照)
M.HOST_TOOL_KNOWN = HOST_TOOL_KNOWN


-- 导出哈希供 store/init 复用摘要
M.fnv1a_hex = _fnv1a_hex
M.fnv1a64_hex = _fnv1a64_hex


return M
