--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb 站点配置
-- @note   默认值见 defaults; new 得到生效表; apply 按 patch 改已生效表
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local defaults = require("klbcore.klbweb.cfger.defaults")


local cfger = {}

cfger.defaults = defaults


local PASS = {
	"max_body_paths",
	"cors",
	"https",
	"log",
	"session",
	"session_secret",
	"request_id",
	"csrf",
	"healthz",
	"spa",
	"pages",
	"on_error",
	"upload_dir",
	"max_file",
	"conn_gate",
}


-- @brief 把 patch 合并进已生效的站点配置 (原地)
-- @param [in,out]  site[table]			已生效配置 (app._cfg)
-- @param [in]      patch[table]		[可选] 要覆盖的字段; 省略的键不变
-- @return site[table]					同一张表; site 非法时原样返回
-- @note 影响后续请求读 _cfg 的项 (server / max_body / keep_alive 等).
--       cors / https / log / session / request_id / csrf / healthz 在首次 listen/serve 时按汇总配置安装; apply 改字段不重装.
cfger.apply = function (site, patch)
	if "table" ~= type(site) then
		return site, false, "site"
	end

	patch = patch or {}

	-- 已知字段类型不符时失败 (不再静默忽略)
	if nil ~= patch.server and ("string" ~= type(patch.server) or "" == patch.server) then
		return site, false, "server"
	end

	if nil ~= patch.keep_alive and "boolean" ~= type(patch.keep_alive) then
		return site, false, "keep_alive"
	end

	if nil ~= patch.max_body and "number" ~= type(patch.max_body) then
		return site, false, "max_body"
	end

	if nil ~= patch.max_keep_alive and "number" ~= type(patch.max_keep_alive) then
		return site, false, "max_keep_alive"
	end

	if nil ~= patch.read_timeout_ms and "number" ~= type(patch.read_timeout_ms) then
		return site, false, "read_timeout_ms"
	end

	if nil ~= patch.write_timeout_ms and "number" ~= type(patch.write_timeout_ms) then
		return site, false, "write_timeout_ms"
	end

	if nil ~= patch.idle_ms and "number" ~= type(patch.idle_ms) then
		return site, false, "idle_ms"
	end

	if "string" == type(patch.server) and "" ~= patch.server then
		site.server = patch.server
	end

	if false == patch.keep_alive then
		site.keep_alive = false
	elseif true == patch.keep_alive then
		site.keep_alive = true
	end

	if "number" == type(patch.max_body) then
		site.max_body = patch.max_body
	end

	if "number" == type(patch.max_keep_alive) then
		site.max_keep_alive = patch.max_keep_alive
	end

	if "number" == type(patch.read_timeout_ms) then
		site.read_timeout_ms = patch.read_timeout_ms
	end

	if "number" == type(patch.write_timeout_ms) then
		site.write_timeout_ms = patch.write_timeout_ms
	end

	if "number" == type(patch.idle_ms) then
		site.idle_ms = patch.idle_ms
	end

	for i = 1, #PASS do
		local k = PASS[i]
		if nil ~= patch[k] then
			site[k] = patch[k]
		end
	end

	return site, true
end


-- @brief 按用户 cfg 得到生效的站点配置 (新表, 含默认)
-- @param [in]      cfg[table]			[可选] 用户站点配置
-- @return site[table]					生效配置, 供 weber 写入 _cfg
cfger.new = function (cfg)
	-- cfg = {
	--   server[string]			[可选] Server 头, 默认 `klbweb`
	--   keep_alive[boolean]	[可选] 是否允许 keep-alive, 默认 `true`
	--   max_body[number]		[可选] 请求体上限字节, 默认 256KiB; 0 不限制
	--   max_body_paths[table]	[可选] 前缀 -> 更大上限, 供升级包等
	--   max_keep_alive[number]	[可选] 每连接最大请求数, 默认 100
	--   read_timeout_ms[number]	[可选] 读超时毫秒, 默认 0=关; 透传 khttp:set_timeout
	--   write_timeout_ms[number]	[可选] 写超时毫秒, 默认 0=关
	--   idle_ms[number]		[可选] 空闲超时毫秒, 默认 0=关
	--   cors[boolean|table]	[可选] 默认关; `true` 或表见 middleware.cors
--   https[boolean|table]	[可选] 默认关; `true` 或表则 HTTP 跳 HTTPS; 表可含 port, 见 middleware.https
	--   log[boolean|function]	[可选] 默认开; `false` 关闭; 函数为写日志
	--   session[boolean|table]	[可选] 默认关; 内存 Cookie 会话
	--   request_id[boolean|table]	[可选] 默认开; `false` 关; 表见 middleware.request_id
	--   csrf[boolean|table]	[可选] 默认关; 宜配合 session; 表见 middleware.csrf
	--   healthz[boolean]		[可选] 默认关; 为 true 时注册 GET /healthz
	--   spa[boolean|string]	[可选] 静态 404 且 Accept html 时回 index
	--   pages[table]			[可选] 按状态码覆盖内置 HTML; 如 `[404]` / `[500]`
	--   on_error[function]		[可选] `fn(req, res, code)`
	--   upload_dir[string]		[可选] multipart 落盘目录
	--   listing[boolean]		[可选] 静态目录列表默认, 默认关; new/setup 时给 staticer
	--   max_file[number]		[可选] 上传单文件与动态 gzip 上限, 默认 8MiB; 静态 C send_file 不 413
	--   conn_gate[boolean|table]	[可选] 连接闸门; 默认开 (同 IP 10s 内 600 次); `false` 关
	--     { window_ms, max, ban_ms }	窗口 / 上限 / 短 ban 毫秒
	--   gzip[boolean]			[可选] 静态预压 .gz, 默认开; new/setup 时给 staticer
	--   gzip_dynamic[boolean]	[可选] 动态 gzip, 默认关; new/setup 时给 staticer
	--   listen[number|table]	[可选] 监听; 未设且未 :listen 时 serve 默认 8000 HTTP
	--     8000
	--     { port = 8000, tls, plain, cert, key }
	--     { port = 8000, tls = true, plain = true, cert, key }		同端口 HTTP+HTTPS, 一个监听
	--     { port = 0, host, family, max_connect, reuseport, nodelay, keepalive }	临时端口 + bind 可配
	--     { { port = 8000 }, { port = 8000, tls = true, cert, key } }	同端口两项会合并
	--     { { port = 8000 }, { port = 8443, tls = true, cert, key } }	不同端口各一个监听
	-- }
	local site = defaults.site()
	return cfger.apply(site, cfg)
end


-- @brief 把 bind/socket 可选字段并入规格表
local function copy_bind_fields(dst, src)
	dst.host = src.host
	dst.family = src.family
	dst.max_connect = src.max_connect
	dst.reuseport = src.reuseport
	dst.nodelay = src.nodelay
	dst.keepalive = src.keepalive
	return dst
end


-- @brief 同端口多项合并为一个监听规格
-- @param [in]      specs[table]		`{ { port, tls, plain, cert, key, ... }, ... }`
-- @return specs[table]					按端口合并后的数组
-- @note HTTP 与 HTTPS 写在同一 port 时合并为 `tls=true, plain=true`
--   \n `port=0` 不合并, 各项各自 bind
local function merge_listen_specs(specs)
	local by_port = {}
	local order = {}
	local list = {}

	for i = 1, #specs do
		local spec = specs[i]
		local port = spec.port

		if 0 == port then
			local one = copy_bind_fields({
				port = 0,
				tls = spec.tls,
				plain = spec.plain,
				cert = spec.cert,
				key = spec.key,
			}, spec)
			list[#list + 1] = one
		else
			local old = by_port[port]
			if not old then
				by_port[port] = copy_bind_fields({
					port = port,
					tls = spec.tls,
					plain = spec.plain,
					cert = spec.cert,
					key = spec.key,
				}, spec)
				order[#order + 1] = port
			else
				local new_tls = true == spec.tls
				local old_tls = true == old.tls
				if new_tls and not old_tls then
					old.tls = true
					old.plain = true
					old.cert = spec.cert or old.cert
					old.key = spec.key or old.key
				elseif old_tls and not new_tls then
					old.plain = true
				else
					old.cert = old.cert or spec.cert
					old.key = old.key or spec.key
				end

				if true == spec.plain then
					old.plain = true
				end

				copy_bind_fields(old, spec)
			end
		end
	end

	for i = 1, #order do
		list[#list + 1] = by_port[order[i]]
	end

	return list
end


-- @brief 解析 setup/new 的 listen 字段为规格数组
-- @param [in]      listen[number|table]	[可选] 见 cfger.new 内联注释
-- @return specs[table]					`{ { port, tls, plain, cert, key }, ... }`; 未设为 nil
-- @return ok[boolean]					true 合法或未设; 有 listen 但非法为 false
-- @note 同一 port 的 HTTP 与 HTTPS 合并为一个规格
cfger.parse_listen = function (listen)
	if nil == listen then
		return nil, true
	end

	local function one(item)
		if "number" == type(item) then
			if item < 0 or 65535 < item then
				return nil
			end

			return {
				port = item,
			}
		end

		if "table" ~= type(item) then
			return nil
		end

		local port = item.port
		if "number" ~= type(port) or port < 0 or 65535 < port then
			return nil
		end

		return copy_bind_fields({
			port = port,
			tls = item.tls,
			plain = item.plain,
			cert = item.cert,
			key = item.key,
		}, item)
	end

	if "number" == type(listen) then
		local spec = one(listen)
		if not spec then
			return nil, false
		end

		return { spec }, true
	end

	if "table" ~= type(listen) then
		return nil, false
	end

	if nil ~= listen[1] then
		local list = {}
		for i = 1, #listen do
			local spec = one(listen[i])
			if not spec then
				return nil, false
			end

			list[#list + 1] = spec
		end

		return merge_listen_specs(list), true
	end

	if nil ~= listen.port then
		local spec = one(listen)
		if not spec then
			return nil, false
		end

		return { spec }, true
	end

	if nil == next(listen) then
		return {}, true
	end

	return nil, false
end


-- @brief 从 listen cfg 抽出传给 klbhttp.listen 的字段
-- @param [in]      cfg[table]			[可选] 监听配置
-- @return lcfg[table]					tls/plain/cert/key + host/family/max_connect/opts; cfg 空为 nil
cfger.listen_only = function (cfg)
	if not cfg then
		return nil
	end

	return copy_bind_fields({
		tls = cfg.tls,
		plain = cfg.plain,
		cert = cfg.cert,
		key = cfg.key,
	}, cfg)
end


-- @brief 从站点 cfg 抽出 staticer.new 的默认项
-- @param [in]      cfg[table]			[可选] 用户站点配置
-- @return opts[table]					listing / index / max_file / gzip / gzip_dynamic
cfger.static_defaults = function (cfg)
	cfg = cfg or {}
	return {
		listing = cfg.listing,
		index = cfg.index,
		max_file = cfg.max_file,
		gzip = cfg.gzip,
		gzip_dynamic = cfg.gzip_dynamic,
	}
end


return cfger
