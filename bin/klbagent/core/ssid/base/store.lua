--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   store.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  SSID 上下文仓 (内存 + pref ssid.clients.json)
-- @stability internal
-- @impl evolving
-- @note   存 token_ref / prefs / fingerprint_summary; 禁日志打明文 token
--   \n M2: pref 跨启动保留; client_id := alua_ssid
--   \n require("klbagent.core.ssid.base.store"); 宜仅 serve 侧写
-- @history 修改历史
--  \n [2026-11] 创建文件 (桩); M4 内存+pref
--  \n [2026-11] 迁入 base/
--]]


local cjson = require("cjson")


local M = {}


local g_by_ssid = {}
local g_loaded = false
-- 最近一次读/写时的文件签名 (size:mtime); 供跨进程/env 变更检测
local g_sig = ""


local function _cfg_store()
	local ok, store = pcall(require, "klbagent.core.config.store")
	if ok and type(store) == "table" then
		return store
	end

	return nil
end


local function _clients_path()
	local store = _cfg_store()
	if store == nil or type(store.path) ~= "function" then
		return ""
	end

	return store.path("ssid.clients.json")
end


local function _now_ms()
	local ok_t, ktime = pcall(require, "ktime")
	if ok_t and type(ktime) == "table" and type(ktime.tick_count) == "function" then
		-- wall-ish: prefer os.time*1000 for expire align with auth
	end

	return math.floor(os.time() * 1000)
end


local function _is_windows()
	return package.config:sub(1, 1) == "\\"
end


-- @brief 私有权限 (POSIX chmod 600; Windows / 无 sh 时跳过)
-- @param [in] path[string]
-- @return ok[boolean]
M.set_private = function (path)
	if type(path) ~= "string" or path == "" then
		return false
	end

	if _is_windows() then
		return false
	end

	local esc = string.gsub(path, "([\"\\$`])", "\\%1")
	pcall(os.execute, "chmod 600 \"" .. esc .. "\" 2>/dev/null")
	return true
end


-- @brief 文件签名 (size:modification); 无 lfs 时返回 ""
-- @param [in] path[string]
-- @return sig[string]
local function _stat_sig(path)
	local ok_l, lfs = pcall(require, "lfs")
	if not ok_l or type(lfs) ~= "table" or type(lfs.attributes) ~= "function" then
		return ""
	end

	local ok_a, at = pcall(lfs.attributes, path)
	if not ok_a or type(at) ~= "table" then
		return ""
	end

	return string.format("%d:%d", tonumber(at.size) or 0, tonumber(at.modification) or 0)
end


local function _copy_ctx(ctx)
	if type(ctx) ~= "table" then
		return nil
	end

	local out = {}
	for k, v in pairs(ctx) do
		out[k] = v
	end

	return out
end


local function _persist()
	local path = _clients_path()
	if path == "" then
		return false, "pref path unavailable"
	end

	local store = _cfg_store()
	local dir = store.dir()
	if type(dir) == "string" and dir ~= "" and type(store.ensure_dir) == "function" then
		store.ensure_dir(dir)
	end

	local obj = {
		version = 2,
		clients = {},
	}
	for sid, ctx in pairs(g_by_ssid) do
		-- 不落明文 token; tokens[] 仅 meta / token_ref
		obj.clients[sid] = {
			alua_ssid = ctx.alua_ssid,
			via = ctx.via,
			caller_kind = ctx.caller_kind,
			summary = ctx.summary,
			token_ref = ctx.token_ref,
			expire_ms = ctx.expire_ms,
			profile = ctx.profile,
			tokens = ctx.tokens,
			created_ms = ctx.created_ms,
			last_seen_ms = ctx.last_seen_ms,
			prefs = ctx.prefs,
		}
	end

	local ok_e, text = pcall(cjson.encode, obj)
	if not ok_e or type(text) ~= "string" then
		return false, "encode fail"
	end

	-- 原子写: 先写 .tmp 再 rename; 失败退化删后改名 (Windows rename 不覆盖)
	local tmp = path .. ".tmp"
	local f, err = io.open(tmp, "wb")
	if f == nil then
		return false, tostring(err)
	end

	f:write(text)
	f:write("\n")
	f:close()

	local ok_r = os.rename(tmp, path)
	if not ok_r then
		pcall(os.remove, path)
		ok_r = os.rename(tmp, path)
	end

	if not ok_r then
		pcall(os.remove, tmp)
		return false, "rename fail"
	end

	M.set_private(path)
	g_sig = _stat_sig(path)
	return true
end


local function _load()
	if g_loaded then
		return
	end

	g_loaded = true
	local path = _clients_path()
	if path == "" then
		return
	end

	g_sig = _stat_sig(path)
	local f = io.open(path, "rb")
	if f == nil then
		return
	end

	local text = f:read("*a") or ""
	f:close()
	if text == "" then
		return
	end

	local ok_d, obj = pcall(cjson.decode, text)
	if not ok_d or type(obj) ~= "table" or type(obj.clients) ~= "table" then
		return
	end

	for sid, ctx in pairs(obj.clients) do
		if type(sid) == "string" and type(ctx) == "table" then
			g_by_ssid[sid] = {
				alua_ssid = ctx.alua_ssid or sid,
				via = ctx.via,
				caller_kind = ctx.caller_kind,
				summary = ctx.summary,
				token_ref = ctx.token_ref,
				expire_ms = ctx.expire_ms,
				profile = ctx.profile,
				tokens = ctx.tokens,
				created_ms = ctx.created_ms,
				last_seen_ms = ctx.last_seen_ms,
				prefs = ctx.prefs,
			}
		end
	end
end


-- @brief 磁盘被其它进程 / env 改写时重载 (load-merge-write, 免覆盖对方条目)
local function _maybe_reload()
	if not g_loaded then
		_load()
		return
	end

	local path = _clients_path()
	if path == "" then
		return
	end

	local sig = _stat_sig(path)
	if sig == "" or sig == g_sig then
		return
	end

	g_by_ssid = {}
	g_loaded = false
	_load()
end


-- @brief 读取 SSID 上下文副本
-- @param [in] alua_ssid[string]
-- @return ctx[table|nil]
-- @return err[string|nil]
M.get = function (alua_ssid)
	_maybe_reload()
	if type(alua_ssid) ~= "string" or alua_ssid == "" then
		return nil, "need alua_ssid"
	end

	local ctx = g_by_ssid[alua_ssid]
	if ctx == nil then
		return nil, "ssid not found"
	end

	return _copy_ctx(ctx)
end


-- @brief 写入/合并 SSID 上下文
-- @param [in] alua_ssid[string]
-- @param [in] patch[table]
-- @return ok[boolean]
-- @return err[string|nil]
M.put = function (alua_ssid, patch)
	_maybe_reload()
	if type(alua_ssid) ~= "string" or alua_ssid == "" then
		return false, "need alua_ssid"
	end

	if type(patch) ~= "table" then
		return false, "need patch"
	end

	local now = _now_ms()
	local ctx = g_by_ssid[alua_ssid]
	if ctx == nil then
		ctx = {
			alua_ssid = alua_ssid,
			created_ms = now,
		}
		g_by_ssid[alua_ssid] = ctx
	end

	for k, v in pairs(patch) do
		if type(k) == "string" and k ~= "token" then
			ctx[k] = v
		end
	end

	ctx.alua_ssid = alua_ssid
	ctx.last_seen_ms = now
	local ok_p, err_p = _persist()
	if not ok_p then
		return false, err_p
	end

	return true
end


-- @brief 删除 SSID 上下文
-- @param [in] alua_ssid[string]
-- @return ok[boolean]
-- @return err[string|nil]
M.remove = function (alua_ssid)
	_maybe_reload()
	if type(alua_ssid) ~= "string" or alua_ssid == "" then
		return false, "need alua_ssid"
	end

	if g_by_ssid[alua_ssid] == nil then
		return false, "ssid not found"
	end

	g_by_ssid[alua_ssid] = nil
	local ok_p, err_p = _persist()
	if not ok_p then
		return false, err_p
	end

	return true
end


-- @brief 列出已知 SSID 摘要 (禁真值)
-- @return list[table|nil]
-- @return err[string|nil]
M.list = function ()
	_maybe_reload()
	local list = {}
	for sid, ctx in pairs(g_by_ssid) do
		local host_tool = nil
		if type(ctx.summary) == "table" then
			host_tool = ctx.summary.host_tool
		end

		list[#list + 1] = {
			alua_ssid = sid,
			via = ctx.via,
			caller_kind = ctx.caller_kind,
			host_tool = host_tool,
			expire_ms = ctx.expire_ms,
			profile = ctx.profile,
			has_token_ref = type(ctx.token_ref) == "string" and ctx.token_ref ~= "",
			last_seen_ms = ctx.last_seen_ms,
		}
	end

	table.sort(list, function (a, b)
		return tostring(a.alua_ssid) < tostring(b.alua_ssid)
	end)

	return list
end


-- @brief 已知数量
-- @return n[number]
M.count = function ()
	_maybe_reload()
	local n = 0
	for _ in pairs(g_by_ssid) do
		n = n + 1
	end

	return n
end


-- @brief 清空内存表 (测试; 不删 pref 文件除非 persist 空表)
M.clear = function ()
	g_by_ssid = {}
	g_loaded = true
	_persist()
end


-- @brief 按 last_seen_ms 过期清理旧 SSID (不碰 machine_key)
-- @param [in] opts[table|nil]		max_age_ms?(默认 7d), dry_run?
-- @return result[table]			{ removed, kept, dry_run, max_age_ms, ids }
M.gc = function (opts)
	_maybe_reload()
	if type(opts) ~= "table" then
		opts = {}
	end

	local max_age_ms = opts.max_age_ms
	if type(max_age_ms) ~= "number" or max_age_ms <= 0 then
		max_age_ms = 7 * 24 * 3600 * 1000
	end

	local dry_run = opts.dry_run == true
	local now = _now_ms()
	local removed = {}
	local kept = 0

	for sid, ctx in pairs(g_by_ssid) do
		local seen = ctx.last_seen_ms
		if type(seen) ~= "number" then
			seen = ctx.created_ms
		end

		if type(seen) ~= "number" then
			seen = 0
		end

		if (now - seen) > max_age_ms then
			removed[#removed + 1] = sid
		else
			kept = kept + 1
		end
	end

	table.sort(removed)

	if not dry_run then
		for i = 1, #removed do
			g_by_ssid[removed[i]] = nil
		end

		if #removed > 0 then
			_persist()
		end
	end

	return {
		removed = #removed,
		kept = kept,
		dry_run = dry_run,
		max_age_ms = max_age_ms,
		ids = removed,
	}
end


return M
