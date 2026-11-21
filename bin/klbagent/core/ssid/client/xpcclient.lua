--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   client/xpcclient.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  ssid XPC 客户端导出面 (跨进程 → owner alua.ssid)
-- @stability stable
-- @impl evolving
-- @attention
--   \n @stability stable: AI 禁止擅自添加/删除/变更本文件对外接口 (含导出名, 参数, 返回与已文档含义); 须用户点名
--   \n @impl evolving: 内部可重构 (xpccall/util), 不得破坏本文件导出签名与可观察语义
-- @note   对齐 klbxpc.client.xpcclient; require(…) 或 ssid.new_client({transport="xpc"})
--   \n 本文件仅导出; 须在 kco 协程内; 禁明文与原始 token
--   \n co_begin_ctx / clear_request 为本机组装+绑定 (RPC 仍走 whoami/register/get_token; XPC 方法表无 begin_ctx)
-- @history 修改历史
--  \n [2026-11] 自 client/xpc.lua 改名; 显式 co_*
--  \n [2026-11] 增 co_begin_ctx / bind_request / clear_request (供 MCP 等跨进程面)
--]]
local const = require("klbagent.core.ssid.base.const")
local ctx_mod = require("klbagent.core.ssid.base.ctx")
local util = require("klbagent.core.ssid.client.util")
local xpccall = require("klbagent.core.ssid.client.xpccall")


local M = {}


M.MODULE_NAME = const.MODULE_NAME
M.SSID_LEN = const.SSID_LEN
M.DIGEST_LEN = const.DIGEST_LEN
M.SALT_LEN = const.SALT_LEN
M.RENEW_MARGIN_MS = const.RENEW_MARGIN_MS
M.TEMP_TOKEN_PREFIX = const.TEMP_TOKEN_PREFIX
M.PROOF_TOKEN_PREFIX = const.PROOF_TOKEN_PREFIX
M.DEFAULT_TOKEN_SLOT = const.DEFAULT_TOKEN_SLOT
M.transport = "xpc"


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

-- @brief 设置对端传输
-- @param [in] opts[table|nil]		transport?, path?
M.set_peer = function (opts)
	return xpccall.set_peer(opts)
end


-- @brief 当前对端配置
-- @return peer[table]
M.get_peer = function ()
	return xpccall.get_peer()
end



-- @brief 登记 SSID
-- @param [in] opts[table|nil]
-- @param [in] timeout_ms[number|nil]
-- @return info[table|nil]
-- @return err[string|nil]
M.co_register = function (opts, timeout_ms)
	return xpccall.co_call("register", opts, timeout_ms)
end


-- @brief 解析 SSID
-- @param [in] opts[table|nil]
-- @param [in] timeout_ms[number|nil]
-- @return info[table|nil]
-- @return err[string|nil]
M.co_resolve = function (opts, timeout_ms)
	return xpccall.co_call("resolve", opts, timeout_ms)
end


-- @brief 当前身份
-- @param [in] opts[table|nil]
-- @param [in] timeout_ms[number|nil]
-- @return info[table|nil]
-- @return err[string|nil]
M.co_whoami = function (opts, timeout_ms)
	return xpccall.co_call("whoami", opts, timeout_ms)
end


-- @brief 保证有效临时 token
-- @param [in] opts[table|nil]
-- @param [in] timeout_ms[number|nil]
-- @return info[table|nil]
-- @return err[string|nil]
M.co_ensure = function (opts, timeout_ms)
	return xpccall.co_call("ensure", opts, timeout_ms)
end


-- @brief 取凭证元数据 (禁 reveal / 禁 raw)
-- @param [in] opts[table|nil]
-- @param [in] timeout_ms[number|nil]
-- @return info[table|nil]
-- @return err[string|nil]
M.co_get_token = function (opts, timeout_ms)
	local call_opts = util.copy_opts(opts)
	call_opts.reveal = nil
	local out, err = xpccall.co_call("get_token", call_opts, timeout_ms)
	if out == nil then
		return nil, err
	end

	out.token = nil
	return out
end


-- @brief 强制续期
-- @param [in] opts[table|nil]
-- @param [in] timeout_ms[number|nil]
-- @return info[table|nil]
-- @return err[string|nil]
M.co_renew = function (opts, timeout_ms)
	return xpccall.co_call("renew", opts, timeout_ms)
end


-- @brief 销毁 SSID 上下文
-- @param [in] opts[table|nil]
-- @param [in] timeout_ms[number|nil]
-- @return ok[boolean]
-- @return err[string|nil]
M.co_destroy = function (opts, timeout_ms)
	local r, err = xpccall.co_call("destroy", opts, timeout_ms)
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
	return xpccall.co_call("status", opts, timeout_ms)
end


-- @brief 过期清理 SSID pref (经 owner; 默认 dry_run 由调用方决定)
-- @param [in] opts[table|nil]		dry_run?, max_age_ms?
-- @param [in] timeout_ms[number|nil]
-- @return info[table|nil]
-- @return err[string|nil]
M.co_gc = function (opts, timeout_ms)
	return xpccall.co_call("gc", opts, timeout_ms)
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
	local r, err = xpccall.co_call("verify_token", {
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
			err = "ssid xpc fail"
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
	return xpccall.co_call("list_tokens", opts, timeout_ms)
end


-- @brief 组装执行上下文 (经 XPC whoami/register/get_token; 本机 bind)
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


-- @brief 绑定请求鉴权 (不 yield; 本机)
-- @param [in] ctx[table]
-- @return ok[boolean]
-- @return err[string|nil]
M.bind_request = function (ctx)
	return ctx_mod.bind_request(ctx)
end


-- @brief 清除请求绑定 (本机)
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
