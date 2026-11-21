--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   run_file.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  unsafe.run_file (kthread 独立 klua_env)
-- @history 修改历史
--  \n [2026-10] 从 init.lua 拆出; 非安全脚本隔离 P1: kthread 独立 env
--]]


local lfs = require("lfs")
local cjson = require("cjson")
local util = require("klbagent.core.tools.unsafe.util")


local M = {}


-- 同时进行的 run_file 上限 (一用一毁)
local MAX_INFLIGHT = 2
local g_inflight = 0


local function _tmp_result_path()
	local dir = os.getenv("TMPDIR") or os.getenv("TEMP") or "/tmp"
	local uniq = tostring({}):match("(%x+)$") or "0"
	return string.format("%s/alua_run_file_%s_%s.json", dir, tostring(os.time()), uniq)
end


local function _read_result(path)
	local f = io.open(path, "rb")
	if f == nil then
		return nil
	end

	local text = f:read("*a") or ""
	f:close()
	if text == "" then
		return nil
	end

	local ok, obj = pcall(cjson.decode, text)
	if not ok or type(obj) ~= "table" then
		return nil
	end

	return obj
end


local function _cleanup_result(path)
	if type(path) ~= "string" or path == "" then
		return
	end

	pcall(os.remove, path)
	pcall(os.remove, path .. ".tmp")
end


local function _run_file(args)
	local path = args.path
	if type(path) ~= "string" or path == "" then
		error("need path")
	end

	local mode = lfs.attributes(path, "mode")
	if mode ~= "file" then
		error("not a file")
	end

	if g_inflight >= MAX_INFLIGHT then
		error("run_file busy (max concurrent " .. tostring(MAX_INFLIGHT) .. ")")
	end

	local ok_k, kthread = pcall(require, "kthread")
	if not ok_k or type(kthread) ~= "table" or type(kthread.start) ~= "function" then
		error("kthread unavailable")
	end

	local argv = {}
	if type(args.argv) == "table" then
		for i = 1, #args.argv do
			local v = args.argv[i]
			if v ~= nil then
				argv[#argv + 1] = tostring(v)
			end
		end
	end

	local timeout_sec = util.clamp_timeout(args.timeout_sec)
	local result_path = _tmp_result_path()
	local job = {
		path = path,
		argv = argv,
		result_path = result_path,
	}

	g_inflight = g_inflight + 1
	local thread_name = nil
	local timed_out = false
	local result_obj = nil

	-- L2: 主侧需泵送 LPC, 供 worker 经 alua.tools 调只读工具
	local ok_lpc, lpc_bridge = pcall(require, "klbagent.core.lpc_bridge")
	if ok_lpc and type(lpc_bridge) == "table" and type(lpc_bridge.ensure) == "function" then
		pcall(lpc_bridge.ensure)
	end

	local ok_co, co_util = pcall(require, "klbagent.core.co_util")
	local in_kco = ok_co and type(co_util) == "table" and co_util.in_kco() == true
	local ok_kco, kco = pcall(require, "kco")
	if not in_kco or not ok_kco or type(kco) ~= "table"
		or type(kco.co_sleep) ~= "function" then
		error("unsafe.run_file: must in kco under co_main_loop")
	end

	local ok_body, err_body = pcall(function ()
		-- wait=false: 主侧可超时轮询; worker 在 entry 内跑完并写结果
		thread_name = kthread.start(
			"klbagent.core.tools.unsafe.run_worker",
			false,
			-1,
			job)

		local deadline = os.time() + timeout_sec
		while true do
			result_obj = _read_result(result_path)
			if result_obj ~= nil then
				break
			end

			if os.time() >= deadline then
				timed_out = true
				break
			end

			-- 宿主 co_main_loop 泵送; 只 co_sleep 让出
			kco.co_sleep(5)
		end
	end)

	if type(thread_name) == "string" and thread_name ~= "" then
		pcall(kthread.stop, thread_name)
	end

	g_inflight = g_inflight - 1
	if g_inflight < 0 then
		g_inflight = 0
	end

	if not ok_body then
		_cleanup_result(result_path)
		error(tostring(err_body))
	end

	if result_obj == nil then
		result_obj = _read_result(result_path)
	end

	_cleanup_result(result_path)

	if timed_out and result_obj == nil then
		error("run_file timeout (" .. tostring(timeout_sec) .. "s)")
	end

	if type(result_obj) ~= "table" then
		error("run_file no result")
	end

	if result_obj.ok ~= true then
		error(tostring(result_obj.err or "run_file failed"))
	end

	local returns = result_obj.returns
	if type(returns) ~= "table" then
		returns = {}
	end

	local out = {
		path = result_obj.path or path,
		returns = returns,
		isolated = true,
		thread = thread_name,
		timed_out = timed_out,
		timeout_sec = timeout_sec,
	}

	-- 退出钩子结果: 带 exit_code/exit_via; 进程是否真退由 CLI 权限决定
	if result_obj.exit_intercepted == true then
		out.exit_intercepted = true
		out.exit_via = result_obj.exit_via
		local n = tonumber(result_obj.exit_code)
		if n == nil then
			n = 0
		else
			n = math.floor(n)
		end

		out.exit_code = n
	end

	return out
end


M.register = function (catalog, dispatch)
	local ok, err = catalog.register({
		name = "unsafe.run_file",
		category = "unsafe",
		title = "run a lua file",
		summary = "Run .lua in kthread env. os.exit intercepted (exit_code in result); alua run may process-exit. Needs unsafe.",
		min_profile = "unsafe",
		args_schema = {
			path = "string path to .lua file",
			argv = "array optional string args -> arg[1..]",
			timeout_sec = "number optional wait timeout (default 60, max 300)",
		},
		examples = {
			{ args = { path = "/tmp/task.lua", argv = { "a", "b" } } },
		},
		docs = "tools/unsafe.md",
		side_effects = { "unsafe" },
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("unsafe.run_file", _run_file)
	if not ok then
		return false, err
	end

	return true
end


return M

-- end
