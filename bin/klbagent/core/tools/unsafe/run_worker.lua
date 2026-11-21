--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   run_worker.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  unsafe.run_file 的 kthread 入口 (独立 klua_env)
-- @note   由 kthread.start("klbagent.core.tools.unsafe.run_worker", ...) 加载;
--         参数 job={path,argv,result_path}; 写 JSON 结果后 ksys.exit
--   \n 整段脚本在 kco.fork + klbenv.co_main_loop 下跑 (与全包入口一致; LPC 用 co_sleep)
-- @history 修改历史
--  \n [2026-10] 创建文件 (非安全脚本隔离 P1); 跑脚本前 os.exit/ksys.exit/kenv.exit 改退本线程; 区分: os.exit 拦截带码; ksys/kenv.exit=授权结束 worker(成功); 注入 alua_lpc.call → 主侧 alua.tools (readonly_only)
--]]


local ksys = require("ksys")
local lfs = require("lfs")
local cjson = require("cjson")


-- @brief worker 内同步 LPC 调主侧只读工具 (须在 co_main_loop / kco 内)
-- @param [in] name[string]
-- @param [in] args[table|nil]
-- @param [in] timeout_ms[number|nil]
-- @return ok[boolean]
-- @return result_or_err[any]
local function _alua_lpc_call(name, args, timeout_ms)
	if type(name) ~= "string" or name == "" then
		return false, "need name"
	end

	local kco = require("kco")
	local klblpc = require("klbcore.klblpc")
	local co_util = require("klbagent.core.co_util")

	if not co_util.in_kco() then
		return false, "alua_lpc: must in kco under co_main_loop"
	end

	if type(timeout_ms) ~= "number" or timeout_ms <= 0 then
		timeout_ms = 15000
	end

	local state = { done = false, a1 = nil, a2 = nil }
	kco.fork(function ()
		local a1, a2 = klblpc.co_call(
			"alua.tools",
			"tool",
			name,
			args or {},
			{ readonly_only = true })
		state.a1 = a1
		state.a2 = a2
		state.done = true
	end)

	local ok_w, err_w = pcall(co_util.await_done, state, timeout_ms / 1000, "lpc")
	if not ok_w then
		return false, tostring(err_w or "lpc timeout")
	end

	if state.a1 == true then
		return true, state.a2
	end

	return false, state.a2 or state.a1
end


local function _write_result(result_path, obj)
	if type(result_path) ~= "string" or result_path == "" then
		return
	end

	local ok_j, text = pcall(cjson.encode, obj)
	if not ok_j then
		text = cjson.encode({
			ok = false,
			err = "result encode fail: " .. tostring(text),
		})
	end

	local tmp = result_path .. ".tmp"
	local f = io.open(tmp, "wb")
	if f == nil then
		return
	end

	f:write(text)
	f:close()
	os.rename(tmp, result_path)
end


local function _normalize_returns(packed)
	local returns = {}
	for i = 2, packed.n do
		local v = packed[i]
		local t = type(v)
		if t == "nil" or t == "boolean" or t == "number" or t == "string" then
			returns[#returns + 1] = v
		elseif t == "table" then
			local ok_e = pcall(cjson.encode, v)
			if ok_e then
				returns[#returns + 1] = v
			else
				returns[#returns + 1] = {
					__alua_type = "table",
					__alua_repr = tostring(v),
				}
			end
		else
			returns[#returns + 1] = {
				__alua_type = t,
				__alua_repr = tostring(v),
			}
		end
	end

	return returns
end


-- @brief 跑用户脚本前安装退出钩子
--   os.exit     -> 永不真杀进程; 结果带 exit_code (授权由主侧 CLI 决定是否进程退出)
--   ksys/kenv.exit -> 授权结束本 worker env (成功路径)
-- @return install[function]  restore[function]  state[table]
local function _make_exit_hooks(result_path, path)
	local state = {
		active = false,
		written = false,
		via = nil,
		code = 0,
	}

	local real_os_exit = os.exit
	local real_ksys_exit = ksys.exit
	local kenv = nil
	local real_kenv_exit = nil
	local ok_e, mod = pcall(require, "kenv")
	if ok_e and type(mod) == "table" and type(mod.exit) == "function" then
		kenv = mod
		real_kenv_exit = mod.exit
	end

	local function _abort_chunk(via)
		if type(real_ksys_exit) == "function" then
			real_ksys_exit()
		end

		error("alua:" .. via)
	end

	local function _finish_os_exit(code)
		if not state.active then
			if type(real_os_exit) == "function" then
				return real_os_exit(code)
			end

			return
		end

		local n = tonumber(code)
		if n == nil then
			n = 0
		else
			n = math.floor(n)
		end

		state.via = "os.exit"
		state.code = n
		if not state.written then
			-- ok=true: 隔离层成功拦截; 进程是否退出由主侧按权限决定
			_write_result(result_path, {
				ok = true,
				path = path,
				returns = {},
				exit_intercepted = true,
				exit_via = "os.exit",
				exit_code = n,
			})
			state.written = true
		end

		_abort_chunk("os.exit")
	end

	local function _finish_env_exit(via)
		if not state.active then
			if via == "ksys.exit" and type(real_ksys_exit) == "function" then
				return real_ksys_exit()
			end

			if via == "kenv.exit" and type(real_kenv_exit) == "function" then
				return real_kenv_exit()
			end

			return
		end

		-- 用户脚本调用 ksys/kenv.exit = 授权结束本 worker (成功)
		state.via = via
		state.code = 0
		if not state.written then
			_write_result(result_path, {
				ok = true,
				path = path,
				returns = {},
				exit_intercepted = true,
				exit_via = via,
				exit_code = 0,
			})
			state.written = true
		end

		_abort_chunk(via)
	end

	local function install()
		state.active = true
		os.exit = function (code)
			_finish_os_exit(code)
		end

		ksys.exit = function ()
			_finish_env_exit("ksys.exit")
		end

		if kenv ~= nil then
			kenv.exit = function ()
				_finish_env_exit("kenv.exit")
			end
		end
	end

	local function restore()
		state.active = false
		os.exit = real_os_exit
		ksys.exit = real_ksys_exit
		if kenv ~= nil and real_kenv_exit ~= nil then
			kenv.exit = real_kenv_exit
		end
	end

	return install, restore, state
end


local function _run()
	local job = ksys.get_args()
	if type(job) ~= "table" then
		_write_result(nil, { ok = false, err = "bad job" })
		ksys.exit()
		return
	end

	local result_path = job.result_path
	local path = job.path
	if type(path) ~= "string" or path == "" then
		_write_result(result_path, { ok = false, err = "need path" })
		ksys.exit()
		return
	end

	local mode = lfs.attributes(path, "mode")
	if mode ~= "file" then
		_write_result(result_path, { ok = false, err = "not a file" })
		ksys.exit()
		return
	end

	local chunk, err = loadfile(path)
	if chunk == nil then
		_write_result(result_path, { ok = false, err = tostring(err) })
		ksys.exit()
		return
	end

	local argv = {}
	if type(job.argv) == "table" then
		for i = 1, #job.argv do
			local v = job.argv[i]
			if v ~= nil then
				argv[#argv + 1] = tostring(v)
			end
		end
	end

	local new_arg = { [0] = path }
	for i = 1, #argv do
		new_arg[i] = argv[i]
	end

	local install_hooks, restore_hooks, exit_state = _make_exit_hooks(result_path, path)

	-- L2: 脚本可 require("alua_lpc").call("runtime.ping", {})
	package.loaded["alua_lpc"] = nil
	package.preload["alua_lpc"] = function ()
		return {
			call = _alua_lpc_call,
			module = "alua.tools",
		}
	end

	-- worker 独立 env: 整段脚本走 klbenv.co_main_loop (与全包入口一致)
	local kco = require("kco")
	local kenv = require("kenv")
	local klbenv = require("klbcore.klbenv")
	local box = { packed = nil }

	_G.arg = new_arg
	_G.alua_lpc = { call = _alua_lpc_call, module = "alua.tools" }

	local h = kco.fork(function ()
		install_hooks()
		box.packed = table.pack(pcall(chunk))
		restore_hooks()
	end)

	klbenv.set_loop_run(function ()
		if kco.is_done(h) then
			kenv.exit()
		end
	end)
	klbenv.co_main_loop()

	_G.arg = nil
	_G.alua_lpc = nil
	package.preload["alua_lpc"] = nil
	package.loaded["alua_lpc"] = nil

	if exit_state.written then
		ksys.exit()
		return
	end

	local packed = box.packed
	if type(packed) ~= "table" then
		_write_result(result_path, {
			ok = false,
			err = "run_worker: no result",
			path = path,
		})
		ksys.exit()
		return
	end

	if not packed[1] then
		_write_result(result_path, {
			ok = false,
			err = tostring(packed[2]),
			path = path,
		})
		ksys.exit()
		return
	end

	_write_result(result_path, {
		ok = true,
		path = path,
		returns = _normalize_returns(packed),
	})
	ksys.exit()
end


local ok_run, err_run = pcall(_run)
if not ok_run then
	local job = nil
	pcall(function ()
		job = ksys.get_args()
	end)
	local result_path = nil
	if type(job) == "table" then
		result_path = job.result_path
	end

	_write_result(result_path, {
		ok = false,
		err = tostring(err_run),
	})
	pcall(ksys.exit)
end

-- end
