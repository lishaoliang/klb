--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   pump.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  双向字节泵送 (正式路径: co_* duplex; 禁默认 kproxy.relay)
-- @note   proxy 包内中继泵 (原 klbcore.kproxy_relay); entry.lua / init.lua 唯一入口
-- @history 修改历史
--  \n [2026-10] 创建文件 (桩); 收口为 kproxy.relay 门面; entry 唯一入口; N3-2 userdata/table 混合时 Lua poll 回退; 默认 co_relay_duplex + co_*; opts.blocking 才走旧 relay; 仅协程泵公开名 co_relay_* (无旧名别名); co_relay_duplex 接受 userdata 端 (无 Lua send/recv 亦可); touch_idle / last_recv_ms: 吸入 C 心跳活动 (Lua 不发 Ping); 自 proxy.pump 迁入 klbcore; 仅 proxy 使用, 自 klbcore 迁回 packages/proxy; co_relay_duplex 半关: a2b EOF 不关两端 + drain_ms (Bug.2026-8); 收尾加 flush_ms 写缓存窗口, 关端延后至刷出之后 (Bug.2026-8)
--]]


local kproxy_io = require("klbcore.klbproxy.io")


local M = {}


local function _tick_now()
	local ok_e, kenv = pcall(require, "kenv")
	if ok_e and type(kenv) == "table" and type(kenv.tick_count) == "function" then
		return kenv.tick_count()
	end

	return nil
end


-- @brief 可选标记端点活动 (遗留); 心跳活动优先吸 C last_recv_ms
-- @param [in]  x[table]	stream / wrap 端
M.touch_idle = function (x)
	if type(x) ~= "table" then
		return
	end

	local now = _tick_now()
	if now == nil then
		return
	end

	x._idle_touch_ms = now
end


-- @brief 从 table 取空闲活动时刻 (含嵌套 tcp / C last_recv)
local function _pick_idle_ms(obj, best)
	if type(obj) ~= "table" then
		return best
	end

	local t = obj._idle_touch_ms
	if type(t) == "number" and (best == nil or t > best) then
		best = t
	end

	if type(obj.idle_touch_ms) == "function" then
		local ok, v = pcall(obj.idle_touch_ms, obj)
		if ok and type(v) == "number" and (best == nil or v > best) then
			best = v
		end
	end

	local ud = obj._serve or obj._mnp or obj._klb or obj._ws
	if ud ~= nil and type(ud.last_recv_ms) == "function" then
		local ok, v = pcall(function ()
			return ud:last_recv_ms()
		end)
		if ok and type(v) == "number" and v > 0 and (best == nil or v > best) then
			best = v
		end
	end

	return best
end


-- @brief 端点空闲活动时刻: Lua touch / idle_touch_ms / C last_recv_ms (PING/PONG)
local function _endpoint_idle_ms(x)
	if type(x) ~= "table" then
		return nil
	end

	local best = _pick_idle_ms(x, nil)
	-- ws/smp 纯 Lua 帧路径: touch 落在内层 stream (x.tcp)
	best = _pick_idle_ms(x.tcp, best)
	return best
end


-- @brief 把端点空闲活动吸入 last_progress (含收 PING/PONG)
local function _absorb_idle_touch(state, x)
	if type(state) ~= "table" then
		return
	end

	local t = _endpoint_idle_ms(x)
	if type(t) ~= "number" then
		return
	end

	if state.last_progress == nil or t > state.last_progress then
		state.last_progress = t
	end
end


local function _backend_name()
	if kproxy_io.co_available() then
		return "co_duplex"
	end

	local ok_kp, kproxy = pcall(require, "kproxy")
	if ok_kp and type(kproxy) == "table" and type(kproxy.relay) == "function" then
		return "kproxy_legacy"
	end

	return nil
end


local function _is_userdata_tcp(x)
	return type(x) == "userdata"
end


-- @brief 当前泵送后端名
-- @return [string|nil]	`co_duplex` / `kproxy_legacy` / nil
M.backend = function ()
	return _backend_name()
end


-- @brief 是否可用
-- @return [boolean]
M.available = function ()
	return true
end


local function _fatal_err(err)
	if err == nil or err == "timeout" then
		return false
	end
	return true
end


-- @brief Lua 轮询双向拷贝 (一端可为密封 wrap table)
-- @param [in]  a[any]
-- @param [in]  b[any]
-- @param [in]  opts[table|nil]	`{ timeout_ms[number] }`
-- @return [boolean] ok
-- @return [string|nil] err
M.relay_poll = function (a, b, opts)
	if a == nil or b == nil then
		return false, "need both ends"
	end

	if type(a.send) ~= "function" or type(a.recv) ~= "function" then
		return false, "bad a"
	end

	if type(b.send) ~= "function" or type(b.recv) ~= "function" then
		return false, "bad b"
	end

	local poll_ms = 40
	if type(opts) == "table" and opts.timeout_ms ~= nil then
		poll_ms = tonumber(opts.timeout_ms) or poll_ms
	end

	local idle = 0
	while true do
		local progressed = false

		local ca, ea = kproxy_io.recv(a, 4096, poll_ms)
		if type(ca) == "string" and #ca > 0 then
			local ok_s = kproxy_io.send(b, ca)
			if not ok_s then
				return false, "b send fail"
			end
			progressed = true
			idle = 0
		elseif _fatal_err(ea) then
			break
		end

		local cb, eb = kproxy_io.recv(b, 4096, poll_ms)
		if type(cb) == "string" and #cb > 0 then
			local ok_s = kproxy_io.send(a, cb)
			if not ok_s then
				return false, "a send fail"
			end
			progressed = true
			idle = 0
		elseif _fatal_err(eb) then
			break
		end

		if not progressed then
			idle = idle + 1
			if idle > 2500 then
				break
			end
		end
	end

	return true
end


-- @brief 协作式中继 (遗留; 正式路径改用 co_relay_duplex)
-- @note 须在 kco 协程内; 非阻塞 recv + co_sleep yield; 仅无 socket co_* 时回退
-- @param [in]  a[any]
-- @param [in]  b[any]
-- @param [in]  opts[table|nil]	`{ idle_ms[number] }` 空闲超时, 默认 100000
-- @return [boolean] ok
-- @return [string|nil] err
M.co_relay_coop = function (a, b, opts)
	if a == nil or b == nil then
		return false, "need both ends"
	end

	if type(a.send) ~= "function" or type(a.recv) ~= "function" then
		return false, "bad a"
	end

	if type(b.send) ~= "function" or type(b.recv) ~= "function" then
		return false, "bad b"
	end

	local ok_c, kco = pcall(require, "kco")
	if not ok_c or type(kco) ~= "table" or type(kco.co_sleep) ~= "function" then
		return false, "kco.co_sleep unavailable"
	end

	-- 有 socket co_* 时改走 duplex (避免 co_sleep 忙等)
	if kproxy_io.co_available()
		and (_is_userdata_tcp(a) or _is_userdata_tcp(b) or M.is_klb_stream(a) or M.is_klb_stream(b)) then
		return M.co_relay_duplex(a, b, opts)
	end

	local idle_ms = 100000
	if type(opts) == "table" and opts.idle_ms ~= nil then
		idle_ms = tonumber(opts.idle_ms) or idle_ms
	end

	local state = {
		last_progress = _tick_now(),
	}
	local busy_rounds = 0
	local idle_rounds = 0
	local idle_rounds_max = math.floor(idle_ms / 10)
	if idle_rounds_max < 1 then
		idle_rounds_max = 1
	end

	while true do
		local progressed = false

		local ca, ea = kproxy_io.recv(a, 4096, 0)
		if type(ca) == "string" and #ca > 0 then
			local ok_s = kproxy_io.send(b, ca)
			if not ok_s then
				return false, "b send fail"
			end
			progressed = true
		elseif _fatal_err(ea) then
			break
		end

		local cb, eb = kproxy_io.recv(b, 4096, 0)
		if type(cb) == "string" and #cb > 0 then
			local ok_s = kproxy_io.send(a, cb)
			if not ok_s then
				return false, "a send fail"
			end
			progressed = true
		elseif _fatal_err(eb) then
			break
		end

		_absorb_idle_touch(state, a)
		_absorb_idle_touch(state, b)

		if progressed then
			state.last_progress = _tick_now() or state.last_progress
			idle_rounds = 0
			busy_rounds = busy_rounds + 1
			if busy_rounds >= 8 then
				busy_rounds = 0
				kco.co_sleep(1)
			end
		else
			busy_rounds = 0
			idle_rounds = idle_rounds + 1
			local now = _tick_now()
			if now ~= nil and state.last_progress ~= nil then
				if (now - state.last_progress) > idle_ms then
					break
				end
			elseif idle_rounds > idle_rounds_max then
				break
			end

			kco.co_sleep(10)
		end
	end

	return true
end


-- @brief 是否 klb mnp/smp 密封流 (recv 走 co_recv, 会挂起协程)
-- @param [in]  x[any]
-- @return [boolean]
M.is_klb_stream = function (x)
	return type(x) == "table" and (x._mnp ~= nil or x._serve ~= nil)
end


local function _try_close_end(x)
	if x == nil then
		return
	end

	if type(x.close) == "function" then
		pcall(function ()
			x:close()
		end)
	end
end


local function _recv_one(src, max_n)
	max_n = tonumber(max_n) or 4096
	-- userdata / 带 co_recv 的流: 挂起等待 (正式异步路径)
	if type(src) == "userdata" or type(src.co_recv) == "function" then
		return kproxy_io.recv(src, max_n)
	end

	if M.is_klb_stream(src) then
		return src:recv(max_n)
	end

	-- 其它 table (WS 等): 短超时避免饿死对向
	return kproxy_io.recv(src, max_n, 40)
end


local function _send_one(dst, data)
	return kproxy_io.send(dst, data)
end


-- @brief 双协程单向泵 (正式主路径; userdata/mnps 均走 co_*)
-- @note 须在 kco 协程内; 一端 co_recv 挂起时另一端仍可读上游回程
-- @param [in]  a[any]
-- @param [in]  b[any]
-- @param [in]  opts[table|nil]	`{ idle_ms[number] }` 默认 100000
-- @return [boolean] ok
-- @return [string|nil] err
local function _end_ok(x)
	if x == nil then
		return false
	end

	-- kproxy.tcp 等 userdata: 走 kproxy_io co_send/co_recv, 无须 Lua 域 send/recv
	if type(x) == "userdata" then
		return true
	end

	if type(x.send) == "function" and type(x.recv) == "function" then
		return true
	end

	return false
end


M.co_relay_duplex = function (a, b, opts)
	if a == nil or b == nil then
		return false, "need both ends"
	end

	if not _end_ok(a) then
		return false, "bad a"
	end

	if not _end_ok(b) then
		return false, "bad b"
	end

	local ok_c, kco = pcall(require, "kco")
	if not ok_c or type(kco) ~= "table"
		or type(kco.fork) ~= "function"
		or type(kco.co_sleep) ~= "function" then
		return false, "kco.fork unavailable"
	end

	local idle_ms = 100000
	if type(opts) == "table" and opts.idle_ms ~= nil then
		idle_ms = tonumber(opts.idle_ms) or idle_ms
	end

	-- a→b 先 EOF 后的收尾等待 (0=禁用); 供明文 forward 等半关场景
	local drain_ms = 0
	if type(opts) == "table" and opts.drain_ms ~= nil then
		drain_ms = tonumber(opts.drain_ms) or 0
	end

	-- 关端前的写缓存刷新窗口 (ms); 关端即断链, 未刷末段会丢 (Bug.2026-8)
	local flush_ms = 200
	if type(opts) == "table" and opts.flush_ms ~= nil then
		flush_ms = tonumber(opts.flush_ms) or flush_ms
	end

	local state = {
		closed = false,
		err = nil,
		alive = 2,
		last_progress = _tick_now(),
		a2b_eof = false,
	}

	-- @brief 置收尾标记; 关端统一延迟到 flush 窗口之后
	local function stop(err)
		if state.closed then
			return
		end

		state.closed = true
		if err ~= nil and state.err == nil then
			state.err = err
		end
	end


	local function close_ends()
		_try_close_end(a)
		_try_close_end(b)
	end

	local function one_way(src, dst, tag, half)
		kco.fork(function ()
			while not state.closed do
				local chunk, err = _recv_one(src, 4096)
				if type(chunk) == "string" and #chunk > 0 then
					local ok_s = _send_one(dst, chunk)
					if not ok_s then
						stop(tag .. " send fail")
						break
					end

					state.last_progress = _tick_now() or state.last_progress
				elseif _fatal_err(err) then
					if half then
						-- 半关: a→b 先 EOF 仅停本方向, 勿掐断 b→a 回程 (Bug.2026-8)
						state.a2b_eof = true
					else
						stop(nil)
					end
					break
				else
					-- timeout / 空: 吸入 PING/PONG touch 后短 sleep
					_absorb_idle_touch(state, src)
					kco.co_sleep(5)
				end
			end

			state.alive = state.alive - 1
			if state.alive <= 0 then
				state.closed = true
			end
		end)
	end

	one_way(a, b, "a2b", true)
	one_way(b, a, "b2a", false)

	local idle_rounds = 0
	local idle_rounds_max = math.floor(idle_ms / 10)
	if idle_rounds_max < 1 then
		idle_rounds_max = 1
	end

	while not state.closed do
		_absorb_idle_touch(state, a)
		_absorb_idle_touch(state, b)

		local now = _tick_now()
		if now ~= nil and state.last_progress ~= nil then
			if (now - state.last_progress) > idle_ms then
				stop("idle timeout")
				break
			end
		else
			idle_rounds = idle_rounds + 1
			if idle_rounds > idle_rounds_max then
				stop("idle timeout")
				break
			end
		end

		-- 半关后回程已无进展: 收尾 (不等待满 idle)
		if now ~= nil and state.a2b_eof and drain_ms > 0
			and state.last_progress ~= nil
			and (now - state.last_progress) > drain_ms then
			stop(nil)
			break
		end

		kco.co_sleep(10)
	end

	local flush_rounds = math.floor(flush_ms / 10)
	if flush_rounds < 1 then
		flush_rounds = 1
	end

	-- 先让 netmulti 把写缓存刷出, 再关端 (立即关会丢末段/触发 RST; Bug.2026-8)
	for _ = 1, flush_rounds do
		kco.co_sleep(10)
	end

	close_ends()

	for _ = 1, 200 do
		if state.alive <= 0 then
			break
		end

		kco.co_sleep(10)
	end

	if state.err ~= nil then
		return false, state.err
	end

	return true
end


-- @brief 在 a/b 两端双向拷贝直至关闭
-- @param [in]  a[any]			一端 (kproxy.tcp 或 send/recv 表)
-- @param [in]  b[any]			另一端
-- @param [in]  opts[table|nil]	`{ timeout_ms, idle_ms, blocking=true }`
-- @return [boolean] ok
-- @return [string|nil] err
-- @note 默认走 duplex/poll; 仅 opts.blocking 才调遗留 kproxy.relay
M.relay = function (a, b, opts)
	if a == nil or b == nil then
		return false, "need both ends"
	end

	if opts ~= nil and type(opts) ~= "table" then
		return false, "bad opts"
	end

	-- M1.a: 默认禁用阻塞 C relay; 显式 blocking 才走旧路径
	if type(opts) == "table" and opts.blocking == true
		and _is_userdata_tcp(a) and _is_userdata_tcp(b) then
		local ok_kp, kproxy = pcall(require, "kproxy")
		if not ok_kp or type(kproxy) ~= "table" or type(kproxy.relay) ~= "function" then
			return false, "kproxy.relay unavailable"
		end

		return kproxy.relay(a, b, opts)
	end

	local ok_c, kco = pcall(require, "kco")
	if ok_c and type(kco) == "table" and type(kco.fork) == "function" then
		return M.co_relay_duplex(a, b, opts)
	end

	return M.relay_poll(a, b, opts)
end


return M

-- end
