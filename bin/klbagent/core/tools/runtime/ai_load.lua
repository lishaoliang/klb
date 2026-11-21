--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   ai_load.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  runtime.ai_bootstrap; AI L0 资料自检 / 宿主反射对账
-- @note   docs 真源; 软链由有能力的宿主 AI 执行; 过期须点名提示用户
--]]


local version = require("klbagent.version")
local ai_load_gate = require("klbagent.core.ai_load_gate")
local lfs = require("lfs")


local M = {}


local PROTOCOL_VERSION = "1"


local REQUIRED = {
	"bootstrap.md",
	"rules/index.md",
	"rules/core.md",
	"rules/host.md",
}


local function _pkg_root_from(source_path)
	if type(source_path) ~= "string" or source_path:sub(1, 1) ~= "@" then
		return nil
	end

	local path = source_path:sub(2)
	local root = path:match("(.+)[/\\]core[/\\]tools[/\\]runtime[/\\]ai_load%.lua$")
	return root
end


local function _docs_root()
	local info = debug.getinfo(1, "S")
	local root = nil
	if type(info) == "table" then
		root = _pkg_root_from(info.source)
	end

	if root == nil or root == "" then
		return nil
	end

	local last = root:sub(-1)
	if last == "/" or last == "\\" then
		return root .. "docs"
	end

	return root .. "/docs"
end


local function _join(base, rel)
	local last = base:sub(-1)
	if last == "/" or last == "\\" then
		return base .. rel
	end

	return base .. "/" .. rel
end


local function _read_file(full)
	local f = io.open(full, "rb")
	if f == nil then
		return nil
	end

	local text = f:read("*a")
	f:close()
	if type(text) ~= "string" then
		return ""
	end

	return text
end


local function _file_meta(root, rel)
	local full = _join(root, rel)
	local attr = lfs.attributes(full)
	if type(attr) ~= "table" or attr.mode ~= "file" then
		return nil
	end

	return {
		path = rel,
		size = attr.size or 0,
		mtime = attr.modification or 0,
	}
end


local function _list_md(dir, prefix, out)
	for name in lfs.dir(dir) do
		if name ~= "." and name ~= ".." then
			local full = _join(dir, name)
			local rel = prefix == "" and name or (prefix .. "/" .. name)
			local mode = lfs.attributes(full, "mode")
			if mode == "file" and string.match(name, "%.md$") then
				local attr = lfs.attributes(full)
				out[#out + 1] = {
					path = rel,
					size = (type(attr) == "table" and attr.size) or 0,
					mtime = (type(attr) == "table" and attr.modification) or 0,
				}
			elseif mode == "directory" then
				_list_md(full, rel, out)
			end
		end
	end
end


local function _fingerprint(files)
	local n = #files
	local sum = 0
	for i = 1, n do
		local e = files[i]
		sum = sum + (e.size or 0) + (e.mtime or 0)
	end

	return string.format("n%d_s%d", n, sum)
end


-- @brief 构建 docs manifest (供 bootstrap / selfcheck)
-- @return manifest[table]
M.build_manifest = function ()
	local root = _docs_root()
	local files = {}
	local missing = {}
	local docs_root = ""

	if type(root) == "string" and lfs.attributes(root, "mode") == "directory" then
		docs_root = root
		_list_md(root, "", files)
		table.sort(files, function (a, b)
			return a.path < b.path
		end)
	else
		missing[#missing + 1] = "docs/"
	end

	for i = 1, #REQUIRED do
		local rel = REQUIRED[i]
		if docs_root == "" or _file_meta(docs_root, rel) == nil then
			missing[#missing + 1] = rel
		end
	end

	local fp = _fingerprint(files)
	return {
		protocol_version = PROTOCOL_VERSION,
		docs_revision = version.string,
		fingerprint = fp,
		docs_root = docs_root,
		required = REQUIRED,
		missing = missing,
		file_count = #files,
		files = files,
		l0_path = "bootstrap.md",
		l1_paths = {
			"rules/index.md",
			"rules/core.md",
			"rules/host.md",
		},
		l2_hint = "skills/*/SKILL.md",
		l3_hint = "tools/*.md",
	}
end


local function _user_prompt(status, manifest)
	if status == "missing" then
		return "alua AI docs incomplete or not installed (missing L0/L1). Deploy/update klbagent so share/.../klbagent/docs includes bootstrap.md and rules/; then retry runtime.ai_bootstrap / runtime.selfcheck."
	end

	if status == "stale" then
		return "Host rules/skills pointer or symlink is stale vs published alua docs (docs_revision/fingerprint mismatch). Update alua install and recreate soft-link/pointer to klbagent/docs (or docs/bootstrap.md); then re-run runtime.selfcheck with host_docs_revision."
	end

	if status == "unchecked" then
		return nil
	end

	return nil
end


-- @brief 对账: 包内 docs + 可选宿主上报 revision
-- @param [in] args[table|nil]  host_docs_revision / host_fingerprint / host_capable
-- @return info[table]
M.evaluate = function (args)
	args = args or {}
	local manifest = M.build_manifest()
	local status = "ok"
	local host_status = "unchecked"
	local warn = {}

	if type(manifest.missing) == "table" and #manifest.missing > 0 then
		status = "missing"
		warn[#warn + 1] = "ai_docs_missing"
	end

	local host_rev = args.host_docs_revision
	local host_fp = args.host_fingerprint
	if type(host_rev) == "string" and host_rev ~= "" then
		if host_rev ~= manifest.docs_revision then
			status = "stale"
			host_status = "stale"
			warn[#warn + 1] = "host_docs_revision_mismatch"
		else
			host_status = "ok"
		end
	end

	if type(host_fp) == "string" and host_fp ~= "" then
		if host_fp ~= manifest.fingerprint then
			status = "stale"
			host_status = "stale"
			warn[#warn + 1] = "host_fingerprint_mismatch"
		elseif host_status == "unchecked" then
			host_status = "ok"
		end
	end

	local host_capable = args.host_capable
	if host_capable == true then
		warn[#warn + 1] = "host_capable_should_symlink_or_pointer"
	end

	local ok_cj, cjson = pcall(require, "cjson")
	if ok_cj and type(cjson) == "table" and cjson.empty_array ~= nil then
		if #warn == 0 then
			warn = cjson.empty_array
		end

		local miss = manifest.missing
		if type(miss) ~= "table" or #miss == 0 then
			manifest.missing = cjson.empty_array
		end
	end

	local prompt = _user_prompt(status, manifest)
	return {
		ok = (status == "ok" or status == "unchecked"),
		status = status,
		host_status = host_status,
		protocol_version = PROTOCOL_VERSION,
		docs_revision = manifest.docs_revision,
		fingerprint = manifest.fingerprint,
		manifest = manifest,
		host_reflection = {
			if_capable = "soft_link_or_pointer",
			target = "klbagent/docs (prefer docs root or bootstrap.md)",
			no_full_copy = true,
			must_reconcile = true,
			must_prompt_user_on_stale_or_missing = true,
		},
		user_prompt = prompt,
		ai_must = {
			"Call runtime.ai_bootstrap or docs.read bootstrap.md (L0) before business tools",
			"If host can persist rules/skills: soft-link/pointer to published docs; do not duplicate L3 bodies",
			"Reconcile host_docs_revision/fingerprint with selfcheck.ai_load; on stale|missing MUST prompt user with user_prompt",
		},
		warn = warn,
	}
end


local function _bootstrap(args)
	local info = M.evaluate(args)
	local root = info.manifest.docs_root
	local text = ""
	if type(root) == "string" and root ~= "" then
		local raw = _read_file(_join(root, "bootstrap.md"))
		if type(raw) == "string" then
			text = raw
		end
	end

	local gate = ai_load_gate.status()
	info.bootstrap_text = text
	info.ack_required = gate.enabled == true and gate.acked ~= true
	info.bootstrap_tool = "runtime.ai_bootstrap"
	info.ack_tool = "runtime.ai_load_ack"
	info.gate = gate
	return info
end


local function _ai_load_ack(args)
	args = args or {}
	local pv = args.protocol_version
	if type(pv) == "string" and pv ~= "" and pv ~= PROTOCOL_VERSION then
		error("protocol_version mismatch want " .. PROTOCOL_VERSION)
	end

	local info = ai_load_gate.ack({
		session_id = args.session_id,
	})
	info.protocol_version = PROTOCOL_VERSION
	info.docs_revision = version.string
	return info
end


M.register = function (catalog, dispatch)
	local ok, err = catalog.register({
		name = "runtime.ai_bootstrap",
		category = "runtime",
		title = "AI L0 bootstrap and docs reconcile",
		summary = "Return L0 bootstrap.md text, docs manifest, host-reflection rules, and reconcile status for AI self-check.",
		args_schema = {
			host_docs_revision = "string optional pinned revision from host rules",
			host_fingerprint = "string optional fingerprint from host",
			host_capable = "boolean optional true if host can soft-link rules/skills",
		},
		examples = {
			{ args = {} },
			{ args = { host_docs_revision = "0.1.99.202610", host_capable = true } },
		},
		docs = "bootstrap.md",
		side_effects = {},
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("runtime.ai_bootstrap", _bootstrap)
	if not ok then
		return false, err
	end

	ok, err = catalog.register({
		name = "runtime.ai_load_ack",
		category = "runtime",
		title = "ack AI L0 load gate",
		summary = "Acknowledge L0 bootstrap for serve/stdio hard gate; unlock business tools until process exits.",
		args_schema = {
			session_id = "string optional also set session attr ai_load_ack",
			protocol_version = "string optional must match if set",
		},
		examples = {
			{ args = {} },
			{ args = { session_id = "s…" } },
		},
		docs = "bootstrap.md",
		side_effects = {},
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("runtime.ai_load_ack", _ai_load_ack)
	if not ok then
		return false, err
	end

	return true
end


return M

-- end
