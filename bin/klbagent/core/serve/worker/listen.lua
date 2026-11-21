--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   listen.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  多口监听
-- @stability stable
-- @impl evolving
-- @attention
--   \n @stability stable: AI 禁止擅自添加/删除/变更本文件对外接口 (含导出名, 参数, 返回与已文档含义); 须用户点名
--   \n @impl evolving: 内部可重构, 只要不破坏本文件导出签名与可观察语义
-- @note   kthread 入口: kthread.start("klbagent.core.serve.worker.listen", false, -1); 加载本文件即执行
--   \n 文件尾 klbenv.co_main_loop; first_run 内 bind/入队
-- @history 修改历史
--  \n [2026-11] 创建文件; P2 候选口 ports[]; RPC 单队; 入口禁空转等退出, 收尾改 kexit; bind 后写 serve.port
--]]
local ksys = require("ksys")
local klblisten = require("klbcore.klblisten")
local klbenv = require("klbcore.klbenv")
local worker = require("klbagent.core.serve.worker")
local port = require("klbagent.core.serve.port")
local tls = require("klbagent.core.serve.security.tls")


-- 默认监听口: serve 选口池首选 (12817); 显式 opts.port / opts.ports 优先
local PORT = port.DEFAULT_PORT

-- 默认绑定地址: nil → INADDR_ANY (0.0.0.0)
local HOST = nil

-- 默认某口最大连接数 (与 C klisten / listener 缺省对齐)
local MAX_CONNECT = 20

-- 默认协议面: 4 族明文 + TLS 同口白名单
--   \n HTTP: http/https + ws/wss; SMP: smp/smps; MNP: mnp/mnps; RPC: smprpc/mnprpc 及 TLS 变体
--   \n kmuxlist 同族 TLS/明文同队; 实际 TLS 投递面由 tls_face 单路决定
local SCHEMES = {
	"http", "https", "ws", "wss",
	"smp", "smps",
	"mnp", "mnps",
	"smprpc", "smprpcs",
	"mnprpc", "mnprpcs",
}

-- 每条 kmuxlist 队列积压上限: 满时销毁队头未消费 sock (防消费端缺席时 fd 堆积)
local MAX_QUEUE = 256


-- @brief 入口取参: 未给的取默认
-- @param [in] opts[table|nil]			port / ports / host / schemes / max_connect / cert / key / tls / tls_info / tls_face
-- @return o[table]					{ port, ports, host, schemes, max_connect, cert, key, tls, tls_info, tls_face }
local function _resolve_opts(opts)
	local o = {
		port = PORT,
		host = HOST,
		schemes = SCHEMES,
	}

	if "table" ~= type(opts) then
		return o
	end

	if "number" == type(opts.port) then
		o.port = opts.port
	end

	if "table" == type(opts.ports) and #opts.ports > 0 then
		o.ports = opts.ports
	end

	if "string" == type(opts.host) then
		o.host = opts.host
	end

	if "table" == type(opts.schemes) then
		o.schemes = opts.schemes
	end

	if "number" == type(opts.max_connect) then
		o.max_connect = opts.max_connect
	end

	if "string" == type(opts.cert) then
		o.cert = opts.cert
	end

	if "string" == type(opts.key) then
		o.key = opts.key
	end

	if "boolean" == type(opts.tls) then
		o.tls = opts.tls
	end

	if "table" == type(opts.tls_info) then
		o.tls_info = opts.tls_info
	end

	if "string" == type(opts.tls_face) then
		o.tls_face = opts.tls_face
	end

	return o
end


-- @brief 组装候选监听口
-- @param [in] o[table]
-- @return ports[table]					number 数组
local function _candidates(o)
	if "table" == type(o.ports) and #o.ports > 0 then
		return o.ports
	end

	return { o.port or PORT }
end


-- @brief 取 TLS 面模式: 入参 tls_face 优先, 否则 env ALUA_SERVE_TLS_AS_HTTP / pref serve.json (与 mux 同源)
-- @param [in] o[table]					_resolve_opts 结果
-- @return face[string]					"http"|"smp"|"mnp"
local function _tls_face(o)
	local face = o.tls_face
	if "string" == type(face) then
		local low = string.lower(face)
		if "https" == low then
			return "http"
		end

		if "smps" == low then
			return "smp"
		end

		if "mnps" == low then
			return "mnp"
		end

		if "http" == low or "smp" == low or "mnp" == low then
			return low
		end
	end

	return tls.tls_over_default()
end


-- @brief 取 TLS 入参: 入参 tls_info 优先 (同 mux); 其次显式 cert/key; 再按点名 / env 解析
-- @note 无调用方 tls_info 且未点名 TLS 时保持明文 (worker 不主动自签证书)
--   \n 证书不在 listen 层装载 (klisten cfg 无 cert/key): 仅据此判定 TLS 面与日志; 建会话时由消费端自备
-- @param [in] o[table]					_resolve_opts 结果
-- @return info[table]					{ tls, cert, key }
-- @return err[string|nil]
local function _resolve_tls(o)
	if "table" == type(o.tls_info) then
		if true == o.tls_info.tls then
			return {
				tls = true,
				cert = o.tls_info.cert,
				key = o.tls_info.key,
			}, nil
		end

		return { tls = false }, nil
	end

	if false == o.tls then
		return { tls = false }, nil
	end

	local has_cert = ("string" == type(o.cert) and "" ~= o.cert)
	local has_key = ("string" == type(o.key) and "" ~= o.key)
	if has_cert ~= has_key then
		return nil, "need both cert and key"
	end

	if has_cert then
		return {
			tls = true,
			cert = o.cert,
			key = o.key,
		}, nil
	end

	local env_tls = os.getenv("ALUA_SERVE_TLS")
	local want = (true == o.tls) or ("string" == type(env_tls) and "" ~= env_tls)
	if not want then
		return { tls = false }, nil
	end

	local info, err = tls.resolve_tls()
	if info == nil then
		return nil, tostring(err or "tls resolve fail")
	end

	return info, nil
end


-- @brief 入口: 按默认口/协议面 bind 一组监听; 结果写入 kmuxlist
-- @param [in] opts[table|nil]			可选: port / host / schemes / max_connect / cert / key / tls / tls_info / tls_face
-- @return code[number]				0 成功; 1 失败
local function _entry(opts)
	local o = _resolve_opts(opts)

	local tls_info, err_tls = _resolve_tls(o)
	if nil == tls_info then
		io.stderr:write("alua serve listen: tls fail: " .. tostring(err_tls) .. "\n")
		return 1
	end

	-- TLS 投递面单路 (http|smp|mnp 互斥); 白名单已含 4 族明文+TLS, 显式 schemes 由调用方自负
	local over = _tls_face(o)
	local schemes = o.schemes

	-- 本层只做 probe + TLS 面投递; 证书在消费端 new_serve 装载 (klisten cfg 无 cert/key)
	--   \n kmuxlist: 强制 raw; 组 open 后自起 accept worker; 同族 TLS/明文同队
	local cfg = {
		tls_as_http = ("http" == over),
		tls_as_mnp = ("mnp" == over),
		max_connect = o.max_connect or MAX_CONNECT,
		kmuxlist = {
			max = MAX_QUEUE,
			http = worker.LIST_HTTP,
			smp = worker.LIST_SMP,
			mnp = worker.LIST_MNP,
			smp_rpc = worker.LIST_RPC,
			mnp_rpc = worker.LIST_RPC,
		},
	}

	local candidates = _candidates(o)
	local grp, code, err = nil, nil, nil
	local try_port = nil

	for i = 1, #candidates do
		try_port = candidates[i]
		grp, code, err = klblisten.listen_group({
			{
				transport = "tcp",
				port = try_port,
				host = o.host,
				schemes = schemes,
			},
		}, cfg)
		if nil ~= grp then
			break
		end

		io.stderr:write("alua serve listen: bind skip port=" .. tostring(try_port)
			.. " code=" .. tostring(code) .. " err=" .. tostring(err) .. "\n")
		grp = nil
	end

	if nil == grp then
		io.stderr:write("alua serve listen: bind fail last_port=" .. tostring(try_port)
			.. " code=" .. tostring(code) .. " err=" .. tostring(err) .. "\n")
		return 1
	end

	local ip, port_real = "0.0.0.0", try_port
	do
		local slot0 = grp:info().slots[1]
		if "table" == type(slot0) and "table" == type(slot0.local_addr) then
			local la = slot0.local_addr
			if "string" == type(la.ip) and "" ~= la.ip then
				ip = la.ip
			end

			if "number" == type(la.port) then
				port_real = la.port
			end
		end
	end

	local tls_text = "plain"
	if true == tls_info.tls then
		tls_text = "tls face=" .. over .. " cert=" .. tostring(tls_info.cert)
	end

	io.stderr:write("alua serve listen: listen " .. table.concat(schemes, "+")
		.. "://" .. tostring(ip) .. ":" .. tostring(port_real) .. " (" .. tls_text .. ")\n")
	io.stderr:write("alua serve listen: queues "
		.. worker.LIST_HTTP .. " "
		.. worker.LIST_SMP .. " "
		.. worker.LIST_MNP .. " "
		.. worker.LIST_RPC .. "\n")

	-- 落盘实际监听口 (供客户端 / health 读取)
	if type(port_real) == "number" then
		port.write_listen_port(port_real)
	end

	return 0
end

local function _first_run()
	-- 线程附加参数 (kthread.start 第 4 个参数起): 形如 { port, host, schemes, cert, key, tls, tls_info, tls_face }
	local arg_opts = ksys.get_args()
	if "table" ~= type(arg_opts) then
		arg_opts = nil
	end

	local ok_run, err_run = pcall(_entry, arg_opts)
	if not ok_run then
		io.stderr:write("alua.serve listen: " .. tostring(err_run) .. "\n")
	end
end

---------------------------------------------------------------------------------------------
--- 线程入口执行 (kthread 加载本文件即在此 env 内运行)

klbenv.set_first_run(_first_run)
klbenv.co_main_loop()

-- end
