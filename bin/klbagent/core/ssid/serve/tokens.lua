--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   tokens.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  ssid → 多 token 槽辅助 (槽名 / 权限 profile / token_ref)
-- @stability internal
-- @impl evolving
-- @note   require("klbagent.core.ssid.serve.tokens"); 仅 serve 使用
--   \n 授权档位仍用 auth profile 码; 本模块只维护 ssid 下多槽映射
-- @history 修改历史
--  \n [2026-11] 创建文件; ssid 多 token
--]]
local const = require("klbagent.core.ssid.base.const")


local M = {}


-- @brief 规范化槽名
-- @param [in] slot[string|nil]
-- @return name[string]
M.norm_slot = function (slot)
	if type(slot) ~= "string" or slot == "" then
		return const.DEFAULT_TOKEN_SLOT or "default"
	end

	local s = string.lower(slot)
	s = string.gsub(s, "[^%w%.%-_]", "-")
	s = string.gsub(s, "%-+", "-")
	if s == "" then
		return const.DEFAULT_TOKEN_SLOT or "default"
	end

	return s
end


-- @brief auth.issue 用的 client_id (一槽一 client_id)
-- @param [in] ssid[string]
-- @param [in] slot[string]
-- @return client_id[string]
M.auth_client_id = function (ssid, slot)
	slot = M.norm_slot(slot)
	local def = const.DEFAULT_TOKEN_SLOT or "default"
	if slot == def then
		return ssid
	end

	return ssid .. "." .. slot
end


-- @brief 从 ctx 取 tokens 表; 兼容旧单 token_ref
-- @param [in] ctx[table|nil]
-- @return tokens[table]
M.tokens_of = function (ctx)
	if type(ctx) ~= "table" then
		return {}
	end

	local tokens = ctx.tokens
	if type(tokens) ~= "table" then
		tokens = {}
	end

	local def = const.DEFAULT_TOKEN_SLOT or "default"
	if tokens[def] == nil and type(ctx.token_ref) == "string" and ctx.token_ref ~= "" then
		tokens[def] = {
			slot = def,
			token_ref = ctx.token_ref,
			expire_ms = ctx.expire_ms,
			profile = ctx.profile or "net",
			auth_client_id = ctx.alua_ssid or nil,
		}
	end

	return tokens
end


-- @brief 取单槽
-- @param [in] ctx[table|nil]
-- @param [in] slot[string|nil]
-- @return row[table|nil]
M.get_slot = function (ctx, slot)
	slot = M.norm_slot(slot)
	local tokens = M.tokens_of(ctx)
	local row = tokens[slot]
	if type(row) == "table" then
		return row
	end

	return nil
end


-- @brief 合并写入槽; 同步 default 到顶层 token_ref 兼容字段
-- @param [in] ssid[string]
-- @param [in] ctx[table|nil]		现有 ctx
-- @param [in] slot[string]
-- @param [in] row[table]			token_ref, expire_ms, profile, caps?
-- @return patch[table]				供 store.put
M.patch_slot = function (ssid, ctx, slot, row)
	slot = M.norm_slot(slot)
	local tokens = M.tokens_of(ctx)
	local entry = {
		slot = slot,
		token_ref = row.token_ref,
		expire_ms = row.expire_ms,
		profile = row.profile or "net",
		caps = row.caps,
		auth_client_id = row.auth_client_id or M.auth_client_id(ssid, slot),
		updated_ms = math.floor(os.time() * 1000),
	}
	tokens[slot] = entry

	local patch = {
		tokens = tokens,
	}

	local def = const.DEFAULT_TOKEN_SLOT or "default"
	if slot == def then
		patch.token_ref = entry.token_ref
		patch.expire_ms = entry.expire_ms
		patch.profile = entry.profile
	end

	return patch
end


-- @brief 槽摘要列表 (禁明文)
-- @param [in] ctx[table|nil]
-- @return list[table]
M.list_summary = function (ctx)
	local tokens = M.tokens_of(ctx)
	local list = {}
	for name, row in pairs(tokens) do
		if type(name) == "string" and type(row) == "table" then
			list[#list + 1] = {
				slot = name,
				profile = row.profile,
				expire_ms = row.expire_ms,
				has_token_ref = type(row.token_ref) == "string" and row.token_ref ~= "",
				auth_client_id = row.auth_client_id,
			}
		end
	end

	table.sort(list, function (a, b)
		return tostring(a.slot) < tostring(b.slot)
	end)

	return list
end


-- @brief 按明文 token 反查槽 (读 token_env)
-- @param [in] ctx[table|nil]
-- @param [in] tok[string]
-- @param [in] read_fn[function]	token_ref → token
-- @return row[table|nil]
-- @return slot[string|nil]
M.find_by_token = function (ctx, tok, read_fn)
	if type(tok) ~= "string" or tok == "" or type(read_fn) ~= "function" then
		return nil, nil
	end

	local tokens = M.tokens_of(ctx)
	for name, row in pairs(tokens) do
		if type(row) == "table" and type(row.token_ref) == "string" then
			local got = read_fn(row.token_ref)
			if type(got) == "string" and got == tok then
				return row, name
			end
		end
	end

	return nil, nil
end


return M

-- end
