--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   client/lpcclient.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  ssid LPC 客户端导出面 (同进程 → owner alua.ssid)
-- @stability stable
-- @impl evolving
-- @attention
--   \n @stability stable: AI 禁止擅自添加/删除/变更本文件对外接口 (含导出名, 参数, 返回与已文档含义); 须用户点名
--   \n @impl evolving: 内部可重构 (lpccall/util), 不得破坏本文件导出签名与可观察语义
-- @note   对齐 klblpc.client.lpcclient; require(…) 或 ssid.new_client()
--   \n 本文件仅导出; 须在 kco 协程内; 禁读写原始 token
-- @history 修改历史
--  \n [2026-11] 自 client/lpc.lua 改名; 显式 co_*
--]]
local const = require("klbagent.core.ssid.base.const")
local ctx_mod = require("klbagent.core.ssid.base.ctx")
local util = require("klbagent.core.ssid.client.util")
local lpccall = require("klbagent.core.ssid.client.lpccall")


local M = {}


M.MODULE_NAME = const.MODULE_NAME
M.SSID_LEN = const.SSID_LEN
M.DIGEST_LEN = const.DIGEST_LEN
M.SALT_LEN = const.SALT_LEN
M.RENEW_MARGIN_MS = const.RENEW_MARGIN_MS
M.TEMP_TOKEN_PREFIX = const.TEMP_TOKEN_PREFIX
M.PROOF_TOKEN_PREFIX = const.PROOF_TOKEN_PREFIX
M.DEFAULT_TOKEN_SLOT = const.DEFAULT_TOKEN_SLOT
M.transport = "lpc"


-- @brief 本机 raw + ssid → C1 证明票 (不 RPC; 禁传 raw)
-- @param [in] ssid[string]
-- @param [in] raw[string]
-- @return proof[string|nil]
-- @return err[string|nil]
M.make_proof = function (ssid, raw)
	return util.make_proof(ssid, raw)
end


-- @brief 目标证明票 (raw 由客户端内取用; 不 RPC)
-- @param [in] opts[table|nil]		alua_ssid, raw?/token?, peer?
-- @return proof[string|nil]
-- @return err[string|nil]
M.proof_token = function (opts)
	return util.proof_token(opts)
end


-- @brief 登记 SSID
-- @param [in] opts[table|nil]
-- @param [in] timeout_ms[number|nil]
-- @return info[table|nil]
-- @return err[string|nil]
M.co_register = function (opts, timeout_ms)
	return lpccall.co_call("register", opts, timeout_ms)
end


-- @brief 解析 SSID
-- @param [in] opts[table|nil]
-- @param [in] timeout_ms[number|nil]
-- @return info[table|nil]
-- @return err[string|nil]
M.co_resolve = function (opts, timeout_ms)
	return lpccall.co_call("resolve", opts, timeout_ms)
end


-- @brief 当前身份
-- @param [in] opts[table|nil]
-- @param [in] timeout_ms[number|nil]
-- @return info[table|nil]
-- @return err[string|nil]
M.co_whoami = function (opts, timeout_ms)
	return lpccall.co_call("whoami", opts, timeout_ms)
end


-- @brief 保证有效临时 token
-- @param [in] opts[table|nil]
-- @param [in] timeout_ms[number|nil]
-- @return info[table|nil]
-- @return err[string|nil]
M.co_ensure = function (opts, timeout_ms)
	return lpccall.co_call("ensure", opts, timeout_ms)
end


-- @brief 取凭证; 默认可带 reveal 取临时 token 明文 (禁 raw)
-- @param [in] opts[table|nil]		alua_ssid?, profile?, reveal?
-- @param [in] timeout_ms[number|nil]
-- @return info[table|nil]
-- @return err[string|nil]
M.co_get_token = function (opts, timeout_ms)
	local call_opts = util.copy_opts(opts)
	local out, err = lpccall.co_call("get_token", call_opts, timeout_ms)
	if out == nil then
		return nil, err
	end

	return util.strip_non_temp_token(out)
end


-- @brief 取临时 token 明文
-- @param [in] opts[table|nil]
-- @param [in] timeout_ms[number|nil]
-- @return info[table|nil]
-- @return err[string|nil]
M.co_get_token_plain = function (opts, timeout_ms)
	local call_opts = util.copy_opts(opts)
	call_opts.reveal = true
	local out, err = M.co_get_token(call_opts, timeout_ms)
	if out == nil then
		return nil, err
	end

	if type(out.token) ~= "string" or out.token == "" then
		return nil, "raw token forbidden on ssid client"
	end

	return out
end


-- @brief 强制续期
-- @param [in] opts[table|nil]
-- @param [in] timeout_ms[number|nil]
-- @return info[table|nil]
-- @return err[string|nil]
M.co_renew = function (opts, timeout_ms)
	return lpccall.co_call("renew", opts, timeout_ms)
end


-- @brief 销毁 SSID 上下文
-- @param [in] opts[table|nil]
-- @param [in] timeout_ms[number|nil]
-- @return ok[boolean]
-- @return err[string|nil]
M.co_destroy = function (opts, timeout_ms)
	local r, err = lpccall.co_call("destroy", opts, timeout_ms)
	if r == nil then
		return false, err
	end

	return true
end


-- @brief 状态摘要
-- @param [in] opts[table|nil]
-- @param [in] timeout_ms[number|nil]
-- @return info[table|nil]
-- @return err[string|nil]
M.co_status = function (opts, timeout_ms)
	return lpccall.co_call("status", opts, timeout_ms)
end


-- @brief 过期清理 SSID pref (经 owner; 默认 dry_run 由调用方决定)
-- @param [in] opts[table|nil]		dry_run?, max_age_ms?
-- @param [in] timeout_ms[number|nil]
-- @return info[table|nil]
-- @return err[string|nil]
M.co_gc = function (opts, timeout_ms)
	return lpccall.co_call("gc", opts, timeout_ms)
end


-- @brief 校验 ssid + token (会话票或 C1 证明票; 禁 raw)
-- @param [in] ssid[string|nil]		alua_ssid; 证明票必填
-- @param [in] tok[string]
-- @param [in] opts[table|nil]		tls?, slide?, issue?, slot?, profile?
-- @param [in] timeout_ms[number|nil]
-- @return info[table|nil]
-- @return err[string|nil]
-- @return code[number|nil]
M.co_verify_token = function (ssid, tok, opts, timeout_ms)
	local ok_a, am = pcall(require, "klbagent.core.auth")
	local bad_code = nil
	local unavail_code = nil
	if ok_a and type(am) == "table" then
		bad_code = am.AUTH_ERR_BAD_TOKEN
		unavail_code = am.AUTH_ERR_SSID_UNAVAIL
	end

	if not util.is_non_raw_token(tok) then
		return nil, "raw token forbidden on ssid client", bad_code
	end

	opts = util.norm_opts(opts)
	local r, err = lpccall.co_call("verify_token", {
		alua_ssid = ssid,
		token = tok,
		tls = opts.tls,
		slide = opts.slide,
		issue = opts.issue,
		slot = opts.slot,
		profile = opts.profile,
		caps = opts.caps,
		force = opts.force,
	}, timeout_ms)
	if r == nil then
		if type(err) ~= "string" or err == "" then
			err = "ssid lpc fail"
		end

		return nil, err, unavail_code
	end

	return r.info, r.err, r.code
end


-- @brief 列出 SSID 下 token 槽与权限摘要
-- @param [in] opts[table|nil]
-- @param [in] timeout_ms[number|nil]
-- @return info[table|nil]
-- @return err[string|nil]
M.co_list_tokens = function (opts, timeout_ms)
	return lpccall.co_call("list_tokens", opts, timeout_ms)
end


-- @brief 组装执行上下文 (经 co_whoami / co_get_token)
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


-- @brief 绑定请求鉴权 (不 yield)
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


return M

-- end
