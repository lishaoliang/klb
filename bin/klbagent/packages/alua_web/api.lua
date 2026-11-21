--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   api.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua_web /api/v1/* 接口 (会话/状态/配置/服务/日志)
-- @note   统一响应 { ok, code, message, name, data }; 除 web-health 外须会话
--   \n 写接口 (PUT/POST) 另须 X-CSRF-Token; web 服务自身不可被本页启停
--   \n 只读配置隐藏 token/pass 等密值; 证书/私钥为文件路径, 保留可回写
-- @history 修改历史
--  \n [2026-10] 创建文件 (P0 只读 + P1 写/启停/日志)
--]]
local cjson = require("cjson")
local codes = require("klbagent.code")
local version = require("klbagent.version")
local config = require("klbagent.core.config")
local logmod = require("klbagent.core.log")
local status = require("klbagent.core.cli.status")
local setup_common = require("klbagent.core.cli.setup.common")
local session = require("klbagent.packages.alua_web.session")


local M = {}


-- 可读/可写配置模块白名单
local MODULES = {
	runtime = true,
	log = true,
	proxy = true,
	serve = true,
	web = true,
	peers = true,
}

-- 可经本页启停的常驻服务 (web 自身拒绝)
local SERVICES = {
	serve = true,
	proxy = true,
}

-- 只读回显时打码的键名 (小写精确匹配)
local SECRET_KEYS = {
	token = true,
	pass = true,
	password = true,
	secret = true,
	psk = true,
	credential = true,
}


-- @brief 写审计日志 (web 模块; 失败不阻断请求)
-- @param [in] level[string]
-- @param [in] msg[string]
-- @param [in] fields[table]
-- @return 无
local function _log(level, msg, fields)
	pcall(logmod.write, level, msg, fields)
end


-- @brief 统一 JSON 响应
-- @param [in] res[table]
-- @param [in] http_code[number]
-- @param [in] code[number]
-- @param [in] message[string]
-- @param [in] data[table]
-- @return 无
local function _send(res, http_code, code, message, data)
	res:co_json({
		ok = code == codes.OK,
		code = code,
		message = message or "",
		name = codes.str(code) or "",
		data = type(data) == "table" and data or {},
	}, http_code)
end


-- @brief 密值打码 (保序: 数组按下标, 对象按键)
-- @param [in] v[any]
-- @param [in] depth[number]
-- @return out[any]
local function _redact(v, depth)
	if type(v) ~= "table" then
		return v
	end

	if 8 < depth then
		return {}
	end

	local out = {}
	local n = #v
	if 0 < n then
		for i = 1, n do
			out[i] = _redact(v[i], depth + 1)
		end
	end

	for k, val in pairs(v) do
		local is_index = type(k) == "number" and 1 <= k and k <= n
		if not is_index then
			if type(k) == "string" and SECRET_KEYS[string.lower(k)] then
				out[k] = "***"
			else
				out[k] = _redact(val, depth + 1)
			end
		end
	end

	return out
end


-- @brief 会话守卫
-- @param [in] req[table]
-- @param [in] res[table]
-- @return row[table|nil]
local function _guard(req, res)
	local row = session.load(req)
	if type(row) ~= "table" then
		_send(res, 401, codes.alua_web.DENIED, "login required", {})
		return nil
	end

	return row
end


-- @brief 写守卫: 会话 + CSRF
-- @param [in] req[table]
-- @param [in] res[table]
-- @return row[table|nil]
local function _guard_write(req, res)
	local row = _guard(req, res)
	if row == nil then
		return nil
	end

	if not session.csrf_ok(req, row) then
		_send(res, 403, codes.alua_web.DENIED, "csrf token missing or bad", {})
		return nil
	end

	return row
end


-- @brief GET /api/v1/web-health (公开)
-- @param [in] req[table]
-- @param [in] res[table]
-- @param [in] ctx[table]
-- @return 无
local function _web_health(req, res, ctx)
	local uptime_s = 0
	if type(ctx) == "table" and type(ctx.started_ms) == "number" then
		local now = os.time() * 1000
		uptime_s = math.max(0, math.floor((now - ctx.started_ms) / 1000))
	end

	_send(res, 200, codes.OK, "ok", {
		app = "alua-web",
		version = version.string,
		uptime_s = uptime_s,
	})
end


-- @brief POST /api/v1/session: token 登录
-- @param [in] req[table]
-- @param [in] res[table]
-- @param [in] ctx[table]
-- @return 无
local function _session_post(req, res, ctx)
	local body = req.json
	local token = nil
	if type(body) == "table" and type(body.token) == "string" then
		token = body.token
	elseif type(req.headers) == "table" then
		token = req.headers["x-alua-token"]
	end

	local info, err = session.login(req, res, token)
	if type(info) ~= "table" then
		_send(res, 401, codes.alua_web.DENIED, tostring(err or "bad token"), {})
		return
	end

	_log("info", "web login ok", {
		module = "web",
		peer = tostring(req.peer or ""),
	})

	_send(res, 200, codes.OK, "ok", {
		csrf = info.csrf,
		profile = info.profile,
		kind = info.kind,
	})
end


-- @brief DELETE /api/v1/session: 注销
-- @param [in] req[table]
-- @param [in] res[table]
-- @param [in] ctx[table]
-- @return 无
local function _session_delete(req, res, ctx)
	session.drop(req)
	session.clear_cookie(res)
	_send(res, 200, codes.OK, "ok", {})
end


-- @brief GET /api/v1/status: 聚合状态
-- @param [in] req[table]
-- @param [in] res[table]
-- @param [in] ctx[table]
-- @return 无
local function _status_get(req, res, ctx)
	if _guard(req, res) == nil then
		return
	end

	local ok, report = pcall(status.collect)
	if not ok or type(report) ~= "table" then
		_send(res, 500, codes.alua_web.TOOL_FAILED, "status collect fail", {})
		return
	end

	_send(res, 200, codes.OK, "ok", report)
end


-- @brief GET /api/v1/config: 全部配置 (密值打码)
-- @param [in] req[table]
-- @param [in] res[table]
-- @param [in] ctx[table]
-- @return 无
local function _config_get(req, res, ctx)
	if _guard(req, res) == nil then
		return
	end

	_send(res, 200, codes.OK, "ok", _redact(config.load(), 0))
end


-- @brief 取路径模块名并校验白名单
-- @param [in] req[table]
-- @return name[string]
local function _module_of(req)
	local params = req.params
	local name = ""
	if type(params) == "table" then
		name = string.lower(tostring(params.module or ""))
	end

	return name
end


-- @brief GET /api/v1/config/{module}
-- @param [in] req[table]
-- @param [in] res[table]
-- @param [in] ctx[table]
-- @return 无
local function _config_module_get(req, res, ctx)
	if _guard(req, res) == nil then
		return
	end

	local name = _module_of(req)
	if not MODULES[name] then
		_send(res, 404, codes.alua_web.TOOL_NOT_FOUND, "unknown config module: " .. name, {})
		return
	end

	_send(res, 200, codes.OK, "ok", _redact(config[name].load(), 0))
end


-- @brief PUT /api/v1/config/{module}
-- @param [in] req[table]
-- @param [in] res[table]
-- @param [in] ctx[table]
-- @return 无
local function _config_module_put(req, res, ctx)
	if _guard_write(req, res) == nil then
		return
	end

	local name = _module_of(req)
	if not MODULES[name] then
		_send(res, 404, codes.alua_web.TOOL_NOT_FOUND, "unknown config module: " .. name, {})
		return
	end

	if type(req.json) ~= "table" or next(req.json) == nil then
		_send(res, 400, codes.alua_web.BAD_REQUEST, "need JSON object body", {})
		return
	end

	local ok = config[name].save(req.json)
	if not ok then
		_send(res, 500, codes.alua_web.TOOL_FAILED, "save " .. name .. ".json fail", {})
		return
	end

	_log("info", "web config save", { module = "web", target = name })
	_send(res, 200, codes.OK, "ok", _redact(config[name].load(), 0))
end


-- @brief POST /api/v1/services/{name}: 启停 serve/proxy
-- @param [in] req[table]
-- @param [in] res[table]
-- @param [in] ctx[table]
-- @return 无
local function _services_post(req, res, ctx)
	if _guard_write(req, res) == nil then
		return
	end

	local name = ""
	if type(req.params) == "table" then
		name = string.lower(tostring(req.params.name or ""))
	end

	if not SERVICES[name] then
		local msg = "unknown service: " .. name
		if name == "web" then
			msg = "web service cannot be controlled from itself"
		end

		_send(res, 400, codes.alua_web.BAD_REQUEST, msg, {})
		return
	end

	local action = ""
	if type(req.json) == "table" and type(req.json.action) == "string" then
		action = string.lower(req.json.action)
	end

	if action == "stop" or action == "disable" or action == "no" then
		action = "no"
	elseif action == "start" or action == "enable" or action == "yes" then
		action = "yes"
	else
		_send(res, 400, codes.alua_web.BAD_REQUEST, "action must be start|stop", {})
		return
	end

	local bin = setup_common.alua_bin()
	local cmd = setup_common.sh_quote(bin) .. " setup " .. name .. " " .. action
	local ok, out = setup_common.run(cmd)
	if not ok then
		_send(res, 500, codes.alua_web.TOOL_FAILED, "setup " .. name .. " " .. action .. " fail", {
			output = tostring(out or ""),
		})
		return
	end

	_log("info", "web service control", {
		module = "web",
		service = name,
		action = action,
	})

	_send(res, 200, codes.OK, "ok", {
		service = name,
		action = action,
		output = tostring(out or ""),
	})
end


-- @brief GET /api/v1/logs?module=&level=&lines=
-- @param [in] req[table]
-- @param [in] res[table]
-- @param [in] ctx[table]
-- @return 无
local function _logs_get(req, res, ctx)
	if _guard(req, res) == nil then
		return
	end

	local q = req.query
	if type(q) ~= "table" then
		q = {}
	end

	local opts = {
		lines = tonumber(q.lines) or 50,
		module = type(q.module) == "string" and q.module or nil,
		level = type(q.level) == "string" and q.level or nil,
	}

	local result = logmod.tail(opts)

	-- pref 未落盘 alua.log 时回退 journald (web 单元 stderr 即 JSONL, 与文件源同形)
	if result.exists ~= true and type(logmod.tail_journal) == "function" then
		local journal = logmod.tail_journal(opts, setup_common.WEB_UNIT)
		if journal.exists == true then
			result = journal
		end
	end

	_send(res, 200, codes.OK, "ok", result)
end


-- @brief 注册全部 /api/v1/* 路由 (须在 static 之前)
-- @param [in] app[table]			klbweb 站点
-- @param [in] ctx[table]			{ started_ms }
-- @return ok[boolean]
M.register = function (app, ctx)
	if type(app) ~= "table" then
		return false
	end

	ctx = type(ctx) == "table" and ctx or {}
	ctx.started_ms = tonumber(ctx.started_ms) or os.time() * 1000

	app:get("/api/v1/web-health", function (req, res)
		_web_health(req, res, ctx)
	end)

	app:post("/api/v1/session", function (req, res)
		_session_post(req, res, ctx)
	end)

	app:delete("/api/v1/session", function (req, res)
		_session_delete(req, res, ctx)
	end)

	app:get("/api/v1/status", function (req, res)
		_status_get(req, res, ctx)
	end)

	app:get("/api/v1/config", function (req, res)
		_config_get(req, res, ctx)
	end)

	app:get("/api/v1/config/:module", function (req, res)
		_config_module_get(req, res, ctx)
	end)

	app:put("/api/v1/config/:module", function (req, res)
		_config_module_put(req, res, ctx)
	end)

	app:post("/api/v1/services/:name", function (req, res)
		_services_post(req, res, ctx)
	end)

	app:get("/api/v1/logs", function (req, res)
		_logs_get(req, res, ctx)
	end)

	return true
end


return M

-- end
