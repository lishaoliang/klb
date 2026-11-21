--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  ssid 服务端导出面 (owner 本地实现 + LPC/XPC)
-- @stability stable
-- @impl evolving
-- @attention
--   \n @stability stable: AI 禁止擅自添加/删除/变更本文件对外接口 (含导出名, 参数, 返回与已文档含义); 须用户点名
--   \n @impl evolving: 内部可重构 (trust/machine/…), 不得破坏本文件导出签名与可观察语义
-- @note   require("klbagent.core.ssid.serve") 或经 ssid.new_serve(); 仅 owner 线程/inline
--   \n 业务 co_*; 原始 token 仅本面维护; 实现见同目录子模块
-- @history 修改历史
--  \n [2026-11] 创建文件; 自 core.ssid 门面拆出服务端面
--  \n [2026-11] C1 证明票 + ssid 多 token 槽 + ssid/token 鉴权
--]]
local const = require("klbagent.core.ssid.base.const")
local fingerprint = require("klbagent.core.ssid.base.fingerprint")
local store = require("klbagent.core.ssid.base.store")
local proof = require("klbagent.core.ssid.base.proof")
local ctx_mod = require("klbagent.core.ssid.base.ctx")
local service = require("klbagent.core.ssid.serve.service")
local trust = require("klbagent.core.ssid.serve.trust")
local machine = require("klbagent.core.ssid.serve.machine")
local token_env = require("klbagent.core.ssid.serve.token_env")
local tokens = require("klbagent.core.ssid.serve.tokens")
local view = require("klbagent.core.ssid.serve.view")
local auth = require("klbagent.core.auth")


local M = {}


M.MODULE_NAME = const.MODULE_NAME
M.SSID_LEN = const.SSID_LEN
M.DIGEST_LEN = const.DIGEST_LEN
M.SALT_LEN = const.SALT_LEN
M.RENEW_MARGIN_MS = const.RENEW_MARGIN_MS
M.TEMP_TOKEN_PREFIX = const.TEMP_TOKEN_PREFIX
M.PROOF_TOKEN_PREFIX = const.PROOF_TOKEN_PREFIX
M.DEFAULT_TOKEN_SLOT = const.DEFAULT_TOKEN_SLOT
M.MID_TTL_MS = const.MID_TTL_MS


-- @brief 标记当前协程受信 (仅供 LPC/XPC 服务派发)
-- @param [in] on[boolean]
M.set_trust = function (on)
	return trust.set_trust(on)
end


-- @brief 当前协程是否受信
-- @return trusted[boolean]
M.is_trusted = function ()
	return trust.is_trusted()
end


-- @brief 采集指纹并登记/复用 SSID
-- @param [in] opts[table|nil]		via?, caller_kind?, attrs?, mcp_init?, alua_ssid?
-- @return info[table|nil]
-- @return err[string|nil]
M.co_register = function (opts)
	if type(opts) ~= "table" then
		opts = {}
	end

	-- 显式覆盖 (调试); 非 16 位 [0-9a-f] 直接拒绝
	if opts.alua_ssid ~= nil then
		local sid = view.normalize_ssid(opts.alua_ssid)
		if sid == nil then
			return nil, "bad alua_ssid"
		end

		local ctx = store.get(sid)
		if ctx == nil then
			store.put(sid, {
				via = opts.via or "call",
				caller_kind = opts.caller_kind or "unknown",
				summary = { override = true },
			})
			ctx = store.get(sid)
		end

		return view.info_from_ctx(ctx)
	end

	-- 环境变量覆盖: 仅接受 16 位 [0-9a-f]; 非法则忽略, 回落指纹
	local env_ssid = os.getenv("ALUA_SSID")
	if type(env_ssid) == "string" and #env_ssid == M.SSID_LEN then
		local sid = view.normalize_ssid(env_ssid)
		if sid ~= nil then
			opts.alua_ssid = sid
			return M.co_register(opts)
		end
	end

	local fp, err_f = fingerprint.collect(opts)
	if fp == nil then
		return nil, err_f or "fingerprint collect fail"
	end

	local kind = fingerprint.classify(fp)
	local mat, err_m = fingerprint.material(fp)
	if mat == nil then
		return nil, err_m or "material fail"
	end

	local mat_str = fingerprint.material_string(mat)
	local sid = machine.make_ssid(mat_str)
	local summary = view.summary_from_fp(fp, kind)

	local ok_p, err_p = store.put(sid, {
		via = mat.via,
		caller_kind = kind,
		summary = summary,
		material = mat,
	})
	if not ok_p then
		return nil, err_p
	end

	local ctx = store.get(sid)
	return view.info_from_ctx(ctx)
end


-- @brief 解析当前或指定指纹对应的 alua_ssid
-- @param [in] opts[table|nil]
-- @return info[table|nil]
-- @return err[string|nil]
M.co_resolve = function (opts)
	if type(opts) ~= "table" then
		opts = {}
	end

	if type(opts.alua_ssid) == "string" and opts.alua_ssid ~= "" then
		local sid = view.normalize_ssid(opts.alua_ssid)
		if sid == nil then
			return nil, "bad alua_ssid"
		end

		local ctx, err = store.get(sid)
		if ctx == nil then
			return nil, err or "ssid not found"
		end

		return view.info_from_ctx(ctx)
	end

	return M.co_register(opts)
end


-- @brief whoami: SSID + 指纹摘要, 无秘密
-- @param [in] opts[table|nil]
-- @return info[table|nil]
-- @return err[string|nil]
M.co_whoami = function (opts)
	local info, err = M.co_resolve(opts)
	if info == nil then
		return nil, err
	end

	return {
		alua_ssid = info.alua_ssid,
		client_id = info.client_id,
		via = info.via,
		caller_kind = info.caller_kind,
		host_tool = info.host_tool,
		summary = info.summary,
		profile = info.profile,
		has_token_ref = type(info.token_ref) == "string" and info.token_ref ~= "",
		expire_ms = info.expire_ms,
		module = M.MODULE_NAME,
	}
end


-- @brief 保证该 SSID 某槽有有效临时会话 token (auth.issue; 档位=profile)
-- @param [in] opts[table|nil]		alua_ssid?, slot?, profile?, force?, caps?
-- @return info[table|nil]
-- @return err[string|nil]
-- @note ssid → 多 token: 每槽独立 auth client_id / token_ref / profile
M.co_ensure = function (opts)
	if type(opts) ~= "table" then
		opts = {}
	end

	local info, err = M.co_resolve(opts)
	if info == nil then
		return nil, err
	end

	local sid = info.alua_ssid
	local slot = tokens.norm_slot(opts.slot or opts.token_id)
	local ctx = store.get(sid)
	local slot_row = tokens.get_slot(ctx, slot)
	local force = opts.force == true
	if not force and not view.need_renew(ctx, slot_row) then
		return {
			alua_ssid = sid,
			client_id = sid,
			slot = slot,
			auth_client_id = slot_row.auth_client_id or tokens.auth_client_id(sid, slot),
			token_ref = slot_row.token_ref,
			expire_ms = slot_row.expire_ms,
			profile = slot_row.profile or opts.profile or "net",
			caps = slot_row.caps,
			renewed = false,
		}
	end

	local profile = opts.profile
	if type(profile) ~= "string" or profile == "" then
		if type(slot_row) == "table" and type(slot_row.profile) == "string" and slot_row.profile ~= "" then
			profile = slot_row.profile
		else
			profile = "net"
		end
	end

	local auth_cid = tokens.auth_client_id(sid, slot)
	local issue_args = {
		client_id = auth_cid,
		profile = profile,
		-- 内部签发裁决: 授权在 ssid 面; service 派发期剥除外部同名参数
		issued_by = auth.ISSUED_BY_SSID,
	}
	if type(opts.caps) == "table" then
		issue_args.caps = opts.caps
	end

	local issued, code = auth.issue(issue_args)
	if issued == nil then
		local msg = "auth.issue fail"
		if type(auth.auth_err_name) == "function" then
			msg = auth.auth_err_name(code)
		end

		return nil, msg
	end

	-- 内存键区分槽, 避免多槽互相覆盖 (重启丢失须重新 ensure)
	local save_id = auth_cid
	local token_ref, err_s = token_env.save(save_id, issued.token)
	if token_ref == nil then
		return nil, err_s
	end

	local patch = tokens.patch_slot(sid, ctx, slot, {
		token_ref = token_ref,
		expire_ms = issued.expire_ms,
		profile = issued.profile or profile,
		caps = issued.caps or opts.caps,
		auth_client_id = auth_cid,
	})
	store.put(sid, patch)

	return {
		alua_ssid = sid,
		client_id = sid,
		slot = slot,
		auth_client_id = auth_cid,
		token_ref = token_ref,
		expire_ms = issued.expire_ms,
		profile = issued.profile or profile,
		caps = issued.caps or opts.caps,
		renewed = true,
	}
end


-- @brief 取可用凭证; 默认仅 token_ref(mem:); 明文仅 LPC 受信协程可读
-- @param [in] opts[table|nil]		reveal? (XPC / trust=xpc 强制忽略)
-- @return info[table|nil]
-- @return err[string|nil]
-- @note 临时票在 token_env 内存; 无盘文件
M.co_get_token = function (opts)
	if type(opts) ~= "table" then
		opts = {}
	end

	-- 跨进程通道禁明文
	if opts.trust == "xpc" then
		opts.reveal = false
	end

	local ensured, err = M.co_ensure(opts)
	if ensured == nil then
		return nil, err
	end

	local out = {
		alua_ssid = ensured.alua_ssid,
		client_id = ensured.client_id,
		slot = ensured.slot,
		auth_client_id = ensured.auth_client_id,
		token_ref = ensured.token_ref,
		expire_ms = ensured.expire_ms,
		profile = ensured.profile,
		caps = ensured.caps,
	}

	if opts.reveal == true and opts.trust ~= "xpc" and M.is_trusted() then
		local tok = token_env.read(ensured.token_ref)
		local pfx = const.TEMP_TOKEN_PREFIX or "t."
		-- 仅回传临时 token; 原始 token 不经 get_token 出口
		if type(tok) == "string" and string.sub(tok, 1, #pfx) == pfx then
			out.token = tok
		end
	end

	return out
end


-- @brief 取临时 token 明文 (仅 serve 本地; 供 issue / remote 中继回填)
-- @note 不经 LPC/XPC RPC; 仅 Lua 内调 owner 面; 禁回传原始 token
-- @param [in] opts[table|nil]
-- @return info[table|nil]
-- @return err[string|nil]
M.co_get_token_plain = function (opts)
	if type(opts) ~= "table" then
		opts = {}
	end

	opts.reveal = true
	-- 本地门面: 勿带 xpc trust, 以免被剥 reveal
	if opts.trust == "xpc" then
		opts.trust = nil
	end

	local out, err = trust.with_trust(function ()
		return M.co_get_token(opts)
	end)

	if out == nil then
		return nil, err
	end

	if type(out.token) ~= "string" or out.token == "" then
		local tok = token_env.read(out.token_ref)
		local pfx = const.TEMP_TOKEN_PREFIX or "t."
		if type(tok) == "string" and string.sub(tok, 1, #pfx) == pfx then
			out.token = tok
		else
			out.token = ""
		end
	end

	if out.token == "" then
		return nil, "raw token forbidden (temp only)"
	end

	return out
end


-- @brief 强制续期/重签
-- @param [in] opts[table|nil]
-- @return info[table|nil]
-- @return err[string|nil]
M.co_renew = function (opts)
	if type(opts) ~= "table" then
		opts = {}
	end

	opts.force = true
	return M.co_ensure(opts)
end


-- @brief 销毁 SSID 上下文并吊销绑定 token
-- @param [in] opts[table|nil]		alua_ssid (必填, 或可 resolve)
-- @return ok[boolean]
-- @return err[string|nil]
M.co_destroy = function (opts)
	if type(opts) ~= "table" then
		opts = {}
	end

	local sid = opts.alua_ssid
	if type(sid) ~= "string" or sid == "" then
		local info = M.co_resolve(opts)
		if info == nil or type(info.alua_ssid) ~= "string" then
			return false, "need alua_ssid"
		end

		sid = info.alua_ssid
	else
		sid = string.lower(sid)
	end

	local ctx = store.get(sid)
	local slot_rows = tokens.tokens_of(ctx)
	for _, row in pairs(slot_rows) do
		if type(row) == "table" then
			if type(row.auth_client_id) == "string" and row.auth_client_id ~= "" then
				pcall(auth.revoke, {
					client_id = row.auth_client_id,
					issued_by = auth.ISSUED_BY_SSID,
				})
			end

			token_env.unlink(row.token_ref)
		end
	end

	pcall(function ()
		auth.revoke({
			client_id = sid,
			issued_by = auth.ISSUED_BY_SSID,
		})
	end)

	if type(ctx) == "table" then
		token_env.unlink(ctx.token_ref)
	end

	local ok, err = store.remove(sid)
	if not ok then
		-- 已吊销文件; 找不到上下文仍算成功
		if err == "ssid not found" then
			return true
		end

		return false, err
	end

	return true
end


-- @brief 状态摘要 (禁真值)
-- @param [in] opts[table|nil]		detail=true 才附 clients (全表易超 XPC ~8KB 单帧)
-- @return info[table|nil]
-- @return err[string|nil]
M.co_status = function (opts)
	if type(opts) ~= "table" then
		opts = {}
	end

	local out = {
		module = M.MODULE_NAME,
		count = store.count(),
		lpc_ready = service.lpc_ready(),
		xpc_ready = service.xpc_ready(),
		ssid_len = M.SSID_LEN,
	}

	-- 默认不带 clients: XPC 帧上限约 8KB; CLI/status 只需 count/ready
	if opts.detail == true then
		out.clients = store.list() or {}
	end

	return out
end


-- @brief 过期清理旧 SSID pref 条目 (不删 machine_key; 默认 dry_run 外须显式)
-- @note 真清时先吊销 auth 内临时 token 并清内存票, 再删 pref 条目
--   \n 可选 purge_temp_files=true 删除 pref 下历史残留 temp.*.env (不再使用)
-- @param [in] opts[table|nil]		max_age_ms?, dry_run?, purge_temp_files?
-- @return result[table|nil]
-- @return err[string|nil]
M.gc = function (opts)
	if type(opts) ~= "table" then
		opts = {}
	end

	local dry_run = opts.dry_run == true
	local scan = store.gc({
		max_age_ms = opts.max_age_ms,
		dry_run = true,
	})

	local purged_files = 0
	if opts.purge_temp_files == true then
		local ok_cs, cfg_store = pcall(require, "klbagent.core.config.store")
		if ok_cs and type(cfg_store) == "table" and type(cfg_store.dir) == "function" then
			local dir = cfg_store.dir()
			if type(dir) == "string" and dir ~= "" then
				dir = string.gsub(dir, "[/\\]+$", "")
				local pipe = io.popen(string.format("ls -1 %q/temp.*.env 2>/dev/null", dir))
				if pipe ~= nil then
					for line in pipe:lines() do
						local base = string.match(line, "([^/\\]+)$") or line
						if string.match(base, "^temp%..+%.env$") then
							purged_files = purged_files + 1
							if not dry_run then
								pcall(os.remove, dir .. "/" .. base)
							end
						end
					end

					pipe:close()
				end
			end
		end
	end

	if dry_run or scan.removed == 0 then
		scan.dry_run = dry_run
		scan.purged_temp_files = purged_files
		return scan
	end

	-- 先吊销绑定 token / 清内存票, 避免 pref 项没了 token 仍有效
	for i = 1, #scan.ids do
		local sid = scan.ids[i]
		local ctx = store.get(sid)
		if type(ctx) == "table" then
			token_env.unlink(ctx.token_ref)
		end

		pcall(auth.revoke, {
			client_id = sid,
			issued_by = auth.ISSUED_BY_SSID,
		})
	end

	local removed = store.gc({
		max_age_ms = opts.max_age_ms,
		dry_run = false,
	})
	if type(removed) ~= "table" then
		return nil, "ssid store gc fail"
	end

	removed.dry_run = false
	removed.purged_temp_files = purged_files
	return removed
end


-- @brief 本地用 raw 派生 C1 证明票 (禁 RPC 传 raw; 仅 owner/本机)
-- @param [in] ssid[string]
-- @param [in] raw[string|nil]		缺省读本机 raw
-- @return proof[string|nil]
-- @return err[string|nil]
M.make_proof = function (ssid, raw)
	local sid = view.normalize_ssid(ssid)
	if sid == nil then
		return nil, "bad alua_ssid"
	end

	if type(raw) ~= "string" or raw == "" then
		raw = proof.raw_expect()
	end

	if type(raw) ~= "string" or raw == "" then
		return nil, "need raw"
	end

	return proof.make_proof(sid, raw)
end


-- @brief 列出 SSID 下 token 槽与权限摘要
-- @param [in] opts[table|nil]		alua_ssid?
-- @return info[table|nil]
-- @return err[string|nil]
M.co_list_tokens = function (opts)
	if type(opts) ~= "table" then
		opts = {}
	end

	local info, err = M.co_resolve(opts)
	if info == nil then
		return nil, err
	end

	local ctx = store.get(info.alua_ssid)
	return {
		alua_ssid = info.alua_ssid,
		tokens = tokens.list_summary(ctx),
		mid = proof.mid_status(),
	}
end


-- @brief 校验 ssid + token (C1 证明票 / 会话票; 授权码仍用 auth)
-- @param [in] ssid[string|nil]		alua_ssid; C1/多槽鉴权建议必填
-- @param [in] tok[string]
-- @param [in] opts[table|nil]		tls?, slide?, trust?, issue?, slot?, profile?
-- @return info[table|nil]
-- @return err[string|nil]
-- @return code[number|nil]
-- @note 证明票通过且 opts.issue=true 时可签发新会话票; LPC/XPC 禁 raw
M.co_verify_token = function (ssid, tok, opts)
	if type(opts) ~= "table" then
		opts = {}
	end

	if type(tok) ~= "string" or tok == "" then
		return nil, auth.auth_err_name(auth.AUTH_ERR_NEED_TOKEN), auth.AUTH_ERR_NEED_TOKEN
	end

	local sid = nil
	if type(ssid) == "string" and ssid ~= "" then
		sid = view.normalize_ssid(ssid)
		if sid == nil then
			return nil, "bad alua_ssid", auth.AUTH_ERR_BAD_TOKEN
		end
	end

	local trust_ch = opts.trust
	local temp_pfx = const.TEMP_TOKEN_PREFIX or "t."
	local proof_pfx = const.PROOF_TOKEN_PREFIX or "p."
	if trust_ch == "lpc" or trust_ch == "xpc" then
		local is_temp = string.sub(tok, 1, #temp_pfx) == temp_pfx
		local is_proof = string.sub(tok, 1, #proof_pfx) == proof_pfx
		if not is_temp and not is_proof then
			return nil, "raw token forbidden on ssid client", auth.AUTH_ERR_BAD_TOKEN
		end
	end

	-- 1) C1 证明票: ssid + proof → 证同 raw (快路径用缓存 mid)
	if proof.is_proof_token(tok) then
		if sid == nil then
			return nil, "need alua_ssid", auth.AUTH_ERR_BAD_TOKEN
		end

		local ok_p, err_p = proof.verify_proof(sid, tok)
		if not ok_p then
			return nil, err_p or "proof fail", auth.AUTH_ERR_BAD_TOKEN
		end

		local out = {
			ok = true,
			kind = "proof",
			alua_ssid = sid,
			client_id = sid,
			profile = opts.profile or "net",
			caps = opts.caps,
		}

		-- 通过后可授权新会话 token (ssid 多槽)
		if opts.issue == true then
			local issued, err_i = M.co_ensure({
				alua_ssid = sid,
				slot = opts.slot,
				profile = opts.profile,
				caps = opts.caps,
				force = opts.force == true,
			})
			if issued == nil then
				return nil, err_i or "issue after proof fail", auth.AUTH_ERR_BAD_TOKEN
			end

			out.issued = true
			out.slot = issued.slot
			out.token_ref = issued.token_ref
			out.expire_ms = issued.expire_ms
			out.profile = issued.profile
			out.caps = issued.caps
			out.auth_client_id = issued.auth_client_id
		end

		return out, nil, auth.AUTH_OK
	end

	-- 2) 会话票: auth.verify + ssid/槽归属
	local info, code = auth.verify(tok, {
		tls = opts.tls,
		slide = opts.slide,
	})
	if info == nil then
		return nil, auth.auth_err_name(code), code
	end

	if sid ~= nil and info.kind == "temp" then
		local ctx = store.get(sid)
		local row, slot_name = tokens.find_by_token(ctx, tok, token_env.read)
		local auth_cid = info.client_id
		local expect_default = tokens.auth_client_id(sid, const.DEFAULT_TOKEN_SLOT)
		local ok_bind = false
		if row ~= nil then
			ok_bind = true
			info.slot = slot_name
			info.profile = row.profile or info.profile
			info.caps = row.caps or info.caps
		elseif type(auth_cid) == "string" then
			if auth_cid == sid or auth_cid == expect_default then
				ok_bind = true
				info.slot = const.DEFAULT_TOKEN_SLOT
			elseif string.sub(auth_cid, 1, #sid + 1) == (sid .. ".") then
				ok_bind = true
				info.slot = string.sub(auth_cid, #sid + 2)
			end
		end

		if not ok_bind then
			return nil, "ssid mismatch", auth.AUTH_ERR_BAD_TOKEN
		end

		info.alua_ssid = sid
	end

	return info, nil, code
end


-- @brief CLI 一站式上下文
-- @param [in] opts[table|nil]
-- @return ctx[table|nil]
-- @return err[string|nil]
M.co_begin_ctx = function (opts)
	local shim = {
		MODULE_NAME = M.MODULE_NAME,
		whoami = M.co_whoami,
		register = M.co_register,
		get_token = M.co_get_token,
	}
	return ctx_mod.begin_ctx(opts, shim)
end


-- @brief 绑定请求鉴权
-- @param [in] ctx[table]
-- @return ok[boolean]
-- @return err[string|nil]
M.bind_request = function (ctx)
	return ctx_mod.bind_request(ctx)
end


-- @brief 清除请求绑定
-- @return ok[boolean]
M.clear_request = function ()
	return ctx_mod.clear_request()
end


-- @brief 读当前请求绑定
-- @return info[table|nil]
M.get_request = function ()
	return ctx_mod.get_request()
end


-- @brief 恢复请求绑定
-- @param [in] info[table|nil]
-- @return ok[boolean]
M.restore_request = function (info)
	return ctx_mod.restore_request(info)
end


-- @brief 启动本进程 LPC 服务 alua.ssid
-- @return ok[boolean]
-- @return err[string|nil]
M.ensure_lpc = function ()
	return service.ensure_lpc(M.MODULE_NAME)
end


-- @brief 启动本机 XPC 服务 alua.ssid (宜 serve/unit owner)
-- @param [in] opts[table|nil]		transport?, path?
-- @return ok[boolean]
-- @return err[string|nil]
M.ensure_xpc = function (opts)
	return service.ensure_xpc(M.MODULE_NAME, opts)
end


-- @brief 本 env 作 ssid owner (inline); kthread owner 由 serve.worker 起, 禁止本模块反向启动
-- @param [in] opts[table|nil]		透传 ensure_xpc
-- @return mode[string]		恒 "inline"
-- @return err[string|nil]
M.ensure_owner = function (opts)
	if type(opts) ~= "table" then
		opts = {}
	end

	local ok_l, err_l = M.ensure_lpc()
	local ok_x, err_x = M.ensure_xpc(opts)
	if not ok_l then
		return "inline", err_l
	end

	if not ok_x then
		return "inline", err_x
	end

	return "inline"
end


return M

-- end
