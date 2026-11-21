--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   ctx.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  ssid 执行上下文门面 (begin_ctx / bind_request)
-- @stability internal
-- @impl evolving
-- @note   cli → ssid.begin_ctx → 执行层; 内部调 auth 仅经 ssid
--   \n 归属 base; 业务 API 由调用方传入 serve/client 表
-- @history 修改历史
--  \n [2026-11] 创建文件 (M1)
--  \n [2026-11] 迁入 base; begin_ctx 可注入 serve/client API
--]]

local auth = require("klbagent.core.auth")


local M = {}


-- @brief 组装并可选绑定请求上下文
-- @param [in] opts[table|nil]		via?, profile?, skip_token?, no_bind?
-- @param [in] ssid_api[table|nil]	serve / lpc / xpc 业务表; 缺省 require ssid.serve
-- @return ctx[table|nil]			{ alua_ssid, token_ref, profile, caps?, … }
-- @return err[string|nil]
M.begin_ctx = function (opts, ssid_api)
	if type(opts) ~= "table" then
		opts = {}
	end

	local ssid = ssid_api
	if type(ssid) ~= "table" then
		-- 特例: 可选注入回落 + 避免 base↔serve 循环依赖; require 须见顶部
		ssid = require("klbagent.core.ssid.serve")
	end

	-- 注入面可为 shim(whoami) 或 serve/client 的 co_*; 统一成可调表
	local whoami_fn = ssid.whoami or ssid.co_whoami
	local register_fn = ssid.register or ssid.co_register
	local get_token_fn = ssid.get_token or ssid.co_get_token

	local via = opts.via
	if type(via) ~= "string" or via == "" then
		via = "call"
	end

	local reg_opts = {
		via = via,
		mcp_init = opts.mcp_init,
		caller_kind = opts.caller_kind,
		alua_ssid = opts.alua_ssid,
		profile = opts.profile,
		env_extra = opts.env_extra,
		http_headers = opts.http_headers,
	}

	local who, err_w = nil, "ssid whoami missing"
	if type(whoami_fn) == "function" then
		who, err_w = whoami_fn(reg_opts)
	end

	if who == nil then
		-- whoami 依赖 resolve/register
		local info, err_r = nil, "ssid register missing"
		if type(register_fn) == "function" then
			info, err_r = register_fn(reg_opts)
		end

		if info == nil then
			return nil, err_r or err_w or "ssid register fail"
		end

		who = {
			alua_ssid = info.alua_ssid,
			client_id = info.client_id,
			via = info.via,
			caller_kind = info.caller_kind,
			host_tool = info.host_tool,
			summary = info.summary,
		}
	end

	local host_tool = who.host_tool
	if (host_tool == nil or host_tool == "") and type(who.summary) == "table" then
		host_tool = who.summary.host_tool
	end

	local ctx = {
		alua_ssid = who.alua_ssid,
		client_id = who.client_id or who.alua_ssid,
		via = who.via or via,
		caller_kind = who.caller_kind,
		host_tool = host_tool,
		summary = who.summary,
		module = ssid.MODULE_NAME,
	}

	if opts.skip_token ~= true then
		local tok_opts = {
			alua_ssid = ctx.alua_ssid,
			profile = opts.profile,
			via = via,
			trust = opts.trust,
			reveal = opts.reveal,
		}
		if type(get_token_fn) == "function" then
			local got, err_t = get_token_fn(tok_opts)
			if got == nil then
				-- 无 raw 时仍返回身份上下文 (执行层可只读本地)
				ctx.token_err = err_t
			else
				ctx.token_ref = got.token_ref
				ctx.expire_ms = got.expire_ms
				ctx.profile = got.profile
				if type(got.token) == "string" then
					ctx.token = got.token
				end
			end
		else
			ctx.token_err = "ssid get_token missing"
		end
	end

	if type(ctx.profile) ~= "string" or ctx.profile == "" then
		ctx.profile = opts.profile or "net"
	end

	if opts.no_bind ~= true then
		local ok_b, err_b = M.bind_request(ctx)
		if not ok_b then
			ctx.bind_err = err_b
		else
			ctx.bound = true
		end
	end

	return ctx
end


-- @brief 将上下文绑到 auth.request_auth (经 ssid 封装, 供执行层档位)
-- @note 已有 raw/temp (token) 绑定时不覆盖, 以免本机 local 绑定抹掉档位裁剪;
--   \n 须强制覆盖时置 ctx.force_bind = true
-- @param [in] ctx[table]
-- @return ok[boolean]
-- @return err[string|nil]
M.bind_request = function (ctx)
	if type(ctx) ~= "table" then
		return false, "need ctx"
	end

	-- 仅 ssid 可触达 auth
	if type(auth.set_request_auth) ~= "function" then
		return false, "auth.set_request_auth missing"
	end

	-- 保留外层 token 绑定 (serve 入口按 token 定档; mux 路径先 bind 再跑 begin_ctx)
	if ctx.force_bind ~= true and type(auth.request_auth) == "function" then
		local prev = auth.request_auth()
		if type(prev) == "table" and (prev.kind == "raw" or prev.kind == "temp") then
			return true
		end
	end

	local profile = ctx.profile
	if type(profile) ~= "string" or profile == "" then
		profile = "net"
	end

	auth.set_request_auth({
		ok = true,
		kind = "local",
		tls = false,
		client_id = ctx.alua_ssid or ctx.client_id,
		profile = profile,
		caps = ctx.caps,
		alua_ssid = ctx.alua_ssid,
		token_ref = ctx.token_ref,
	})

	return true
end


-- @brief 清除请求绑定
-- @return ok[boolean]
M.clear_request = function ()
	if type(auth.clear_request_auth) == "function" then
		auth.clear_request_auth()
	end

	return true
end


-- @brief 读当前请求绑定 (仅 ssid 触达 auth; 供 lpc 嵌套恢复)
-- @return info[table|nil]
M.get_request = function ()
	if type(auth.request_auth) ~= "function" then
		return nil
	end

	return auth.request_auth()
end


-- @brief 恢复或清除请求绑定 (嵌套 LPC 调用后还原外层)
-- @param [in] info[table|nil]		先前 get_request 快照; nil 则 clear
-- @return ok[boolean]
M.restore_request = function (info)
	if type(info) == "table" then
		if type(auth.set_request_auth) == "function" then
			auth.set_request_auth(info)
			return true
		end

		return false
	end

	return M.clear_request()
end


return M
