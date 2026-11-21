--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   view.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  ssid serve 视图/规范化辅助 (info / summary / renew 判定)
-- @stability internal
-- @impl evolving
-- @note   require("klbagent.core.ssid.serve.view")
-- @history 修改历史
--  \n [2026-11] 自 serve/init 迁出
--]]
local const = require("klbagent.core.ssid.base.const")
local fingerprint = require("klbagent.core.ssid.base.fingerprint")
local token_env = require("klbagent.core.ssid.serve.token_env")


local M = {}


-- @brief 当前毫秒墙钟
-- @return ms[number]
M.now_ms = function ()
	return math.floor(os.time() * 1000)
end


-- @brief 规范化 16 位 SSID (字符集 [0-9a-f]); 非法返回 nil
-- @param [in] s[string]
-- @return sid[string|nil]
M.normalize_ssid = function (s)
	if type(s) ~= "string" then
		return nil
	end

	local low = string.lower(s)
	if #low ~= const.SSID_LEN or string.find(low, "^[0-9a-f]+$") == nil then
		return nil
	end

	return low
end


-- @brief 指纹摘要表
-- @param [in] fp[table|nil]
-- @param [in] kind[string|nil]
-- @return summary[table]
M.summary_from_fp = function (fp, kind)
	local mcp_name = nil
	if type(fp) == "table" and type(fp.mcp) == "table" then
		mcp_name = fp.mcp.client_name
	end

	local host_tool = ""
	if type(fingerprint.resolve_host_tool) == "function" then
		host_tool = fingerprint.resolve_host_tool(fp) or ""
	end

	return {
		via = fp and fp.via or nil,
		caller_kind = kind,
		host_tool = host_tool,
		parent_comm = fp and fp.parent_comm or nil,
		host_kind = fp and fp.host_kind or nil,
		arch = fp and fp.arch or nil,
		virt = fp and fp.virt or nil,
		tty = fp and fp.tty or nil,
		ppid_hash = fp and fp.ppid_hash or nil,
		mcp_client = mcp_name,
		cwd_hash = fp and fp.cwd_hash or nil,
	}
end


-- @brief store 上下文 → 对外 info
-- @param [in] ctx[table]
-- @return info[table]
M.info_from_ctx = function (ctx)
	local host_tool = nil
	if type(ctx.summary) == "table" then
		host_tool = ctx.summary.host_tool
	end

	if (host_tool == nil or host_tool == "") and type(ctx.material) == "table" then
		host_tool = ctx.material.host_tool
	end

	local tokens_mod = require("klbagent.core.ssid.serve.tokens")
	local token_list = tokens_mod.list_summary(ctx)

	return {
		alua_ssid = ctx.alua_ssid,
		via = ctx.via,
		caller_kind = ctx.caller_kind,
		host_tool = host_tool,
		summary = ctx.summary,
		token_ref = ctx.token_ref,
		expire_ms = ctx.expire_ms,
		profile = ctx.profile,
		tokens = token_list,
		token_count = #token_list,
		client_id = ctx.alua_ssid,
		created_ms = ctx.created_ms,
		last_seen_ms = ctx.last_seen_ms,
	}
end


-- @brief 是否需要续期/重签临时 token (可针对槽)
-- @param [in] ctx[table|nil]
-- @param [in] slot_row[table|nil]	指定槽行; 缺省用顶层/default
-- @return need[boolean]
M.need_renew = function (ctx, slot_row)
	local row = slot_row
	if type(row) ~= "table" then
		if type(ctx) ~= "table" then
			return true
		end

		row = ctx
	end

	if type(row.token_ref) ~= "string" or row.token_ref == "" then
		return true
	end

	-- 临时票仅内存: 重启后 ref 仍在 pref 但票已丢, 须重签
	local tok = token_env.read(row.token_ref)
	if type(tok) ~= "string" or tok == "" then
		return true
	end

	if type(row.expire_ms) ~= "number" or row.expire_ms <= 0 then
		-- embedded ttl 0: 内存有票即有效
		return false
	end

	return row.expire_ms <= (M.now_ms() + const.RENEW_MARGIN_MS)
end


return M

-- end
