--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @brief  klblisten: 单端口 / 一组多口共享 listen + probe 分发 (对外入口)
-- @stability stable
-- @impl mature
-- @attention
--   \n @stability stable: AI 禁止擅自添加/删除/变更本文件对外接口 (含导出名, 参数, 返回与已文档含义); 须用户点名
--   \n @impl mature: 实现倾向冻结; 重构须同等谨慎, 不得破坏本文件导出签名与可观察语义
-- @note   require("klbcore.klblisten"); 契约见本文件导出; 实现见 listen/, cfger/
-- @history 修改历史
--  \n 2026 创建文件
--]]


local knetcode = require("knetcode")
local net = require("klbcore.klbcode.netcode")
local listener = require("klbcore.klblisten.listen.listener")
local grouper = require("klbcore.klblisten.listen.grouper")


local klblisten = {}


-- @brief 失败返回: nil, code, errstr (脚本自定义或 knetcode.strerror)
-- @param [in]      code[number|nil]	klb_netcode_e; 缺省为 INVAL
-- @param [in]      errstr[string|nil]	自定义描述; 缺省用 knetcode.strerror(code)
-- @return nil, code[number], errstr[string]
local function Fail(code, errstr)
	local c = code
	if nil == c then
		c = net.INVAL
	end

	if nil ~= errstr then
		return nil, c, errstr
	end

	return nil, c, knetcode.strerror(c)
end


--------------------------------------------------------------------------------------------
-- 单端口


-- @brief 新建共享监听模块 (单口)
-- @param [in]      cfg[table]			[可选] schemes/tls/plain/cert/key + C mux + http/rtsp/smp/mnp; 见 :open
-- @return [table]	监听模块
-- @stability stable
-- @note 对象方法: `:open` / `:co_accept` / `:close` / `:port` / `:host` / `:local_addr` / `:native_fd` / `:rebind(host)` / `:is_open` / `:schemes` / `:set_schemes` / `:mux_opts` / `:info` / `:last_code` / `:last_error`
--   \n open opts 另含 `family`(ipv4/ipv6/dual) / `reuseport` / `nodelay` / `keepalive`
klblisten.new_listen = function (cfg)
	return listener.new(cfg)
end


-- @brief 监听单端口并返回已 open 的模块
-- @param [in]      port_or_spec[number|table]	端口, 或与 listen_group 单条同形的规格表
-- @param [in]      cfg[table]			[可选] tls/cert/key 等; 规格表内 schemes 优先并入
-- @return [table]|nil	监听模块; 失败为 nil
-- @return code[number|nil]	失败时 klb_netcode_e (可脚本自定义, eg. net.INVAL)
-- @return errstr[string|nil]	失败时描述串; C 码用 knetcode.strerror; 脚本可自定义
-- @stability stable
-- @note 同口 schemes 协议面任意组合 (明文 + TLS 面均可并存)
--   \n eg. 80: http + mnp + mnps:
--   \n   klblisten.listen(80, { schemes = { "http", "mnp", "mnps" }, cert = "...", key = "..." })
--   \n   klblisten.listen({ port = 80, schemes = { "http", "mnp", "mnps" } }, { cert = "...", key = "..." })
--   \n `:co_accept` → face, session, msg, tls, addr (face 为 http/https/ws/wss/mnp/mnps/...; addr 为对端 `{ ip, port }`)
--   \n raw = true: 白名单命中即交裸 sock (lightuserdata), 不建会话也不校验证书
--   \n kmuxlist: 结果写入 kmuxlist (强制 raw); 他线程 kmuxlist.pop 消费; 勿再 :co_accept
--   \n   单队: "/path" 或 { path="/path", max=N }
--   \n   按协议分队: { http=, smp=, mnp=, rtsp=, smp_rpc=, mnp_rpc=, path=回落, max=N }
--   \n   分队值: "/path" 或 { path="/path", max=N }; TLS/明文同队; RPC 独立; :info().kmuxlist 可查
--   \n schemes 仅 ws/wss: 自动 HTTP Upgrade→klbws 会话; 与 http 并存时交 httpserver + from_http
--   \n 多 TLS 面同口 (eg. https + mnps): C mux 仅投递优先序一路; 未投递面见 `:info().tls_face_dropped`
--   \n 查询: `:info` / `:port` / `:schemes` / `:mux_opts` / `:is_open`
--   \n 失败时本调用返回 nil, code, errstr; 对象可用 :last_code / :last_error
klblisten.listen = function (port_or_spec, cfg)
	local port = port_or_spec
	local open_cfg = cfg

	if "table" == type(port_or_spec) then
		local spec = port_or_spec
		port = spec.port
		if "number" ~= type(port) then
			return Fail(net.INVAL, "Invalid argument")
		end

		open_cfg = {}
		if "table" == type(cfg) then
			for k, v in pairs(cfg) do
				open_cfg[k] = v
			end
		end

		if nil == open_cfg.schemes and nil ~= spec.schemes then
			open_cfg.schemes = spec.schemes
		end

		if nil == open_cfg.scheme and nil ~= spec.scheme then
			open_cfg.scheme = spec.scheme
		end

		if nil == open_cfg.host and nil ~= spec.host then
			open_cfg.host = spec.host
		end
	elseif "number" ~= type(port) then
		return Fail(net.INVAL, "Invalid argument")
	end

	local l = listener.new(open_cfg)
	local ok, code = l:open(port, open_cfg)
	if not ok then
		return Fail(code, l:last_error())
	end

	return l
end


--------------------------------------------------------------------------------------------
-- 一组多口 (每口可配 schemes 白名单)


-- @brief 新建一组监听 (未 open)
-- @param [in]      cfg[table]			[可选] 交给各口的 tls/cert 等; 同 new_listen
-- @return [table]	组对象
-- @stability stable
-- @note 对象方法: `:open` / `:co_accept` / `:close` / `:slots` / `:ports` / `:schemes(port)` / `:set_schemes(port, schemes)` / `:rebind(port, host)` / `:add_listen` / `:remove_listen` / `:info([port])` / `:port_info(port)` / `:is_open` / `:last_code` / `:last_error`
klblisten.new_listen_group = function (cfg)
	return grouper.new(cfg)
end


-- @brief 按一组规格 bind 多口, 返回已 open 的组
-- @param [in]      listen[table]		一条或数组; schemes 为协议面 (可含 TLS 后缀)
-- @param [in]      cfg[table]			[可选] 同 new_listen_group; 含 TLS 面时须 cert/key
-- @return [table]|nil	组对象; 失败为 nil
-- @return code[number|nil]	失败时 klb_netcode_e (可脚本自定义)
-- @return errstr[string|nil]	失败时描述串; 同 listen
-- @stability stable
-- @note 同口 schemes 协议面任意组合 (明文 + TLS 面均可并存; 不互斥)
--   \n eg. 80: http + mnp + mnps:
--   \n   klblisten.listen_group({
--   \n     { port = 80, schemes = { "http", "mnp", "mnps" } },
--   \n   }, { cert = "...", key = "..." })
--   \n `:co_accept` → face, session, msg, slot, tls, addr (addr 为对端 `{ ip, port }`)
--   \n raw = true (规格或组 cfg): 白名单命中即交裸 sock (lightuserdata), 本层不建会话;
--   \n   供跨线程移交; 不校验证书
--   \n kmuxlist (组 cfg): 同 listen; open 后 accept 按 face 写入对应 kmuxlist (强制 raw);
--   \n   他线程 require("kmuxlist").pop(path) 取 sock; :info().kmuxlist 可查 path/分队表
--   \n 多 TLS 面同口 (eg. https + mnps): C mux 仅投递优先序一路; 未投递面见 `:info(port).tls_face_dropped`
--   \n 查询: `:schemes(port)` 仅协议面; `:info(port)` / `:port_info(port)` 该口全信息; `:info()` 整组全信息
--   \n eg. grp:schemes(80) → { "http", "mnp", "mnps" }, false
--   \n eg. grp:info(80).schemes / grp:info().slots
--   \n 失败时本调用返回 nil, code, errstr; 组对象可用 :last_code / :last_error
klblisten.listen_group = function (listen, cfg)
	local g = grouper.new(cfg)
	local ok, code = g:open(listen)
	if not ok then
		return Fail(code, g:last_error())
	end

	return g
end


return klblisten
