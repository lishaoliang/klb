--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   serve/lpcer.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klblpc 服务循环
-- @note   方法名是 co_recv 载荷首参; REQUEST 的返回值经 response 发回; src 非空视为 call
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local kco = require("kco")
local klpc = require("klpc")
local cfger = require("klbcore.klblpc.cfger")
local router = require("klbcore.klblpc.router")
local errcode = require("klbcore.klblpc.errcode")


--------------------------------------------------------------------------------------------
-- 前置定义


-- @brief 保住 nil 洞与个数
local function capture(...)
	return {
		n = select("#", ...),
		...,
	}
end


-- @brief src 非空则按 REQUEST(call); 否则按 POST
local function kind_of_src(src)
	if "string" == type(src) and "" ~= src then
		return "call"
	end

	return "post"
end


-- @brief 一条模块上循环收消息并分发
local function co_serve_module(app)
	local mo = app._mo
	while true do
		if not app._serving or not mo then
			return
		end

		local cap = capture(mo:co_recv())
		-- 关闭或异常时 co_recv 无返回值; 勿空转
		if 0 == cap.n then
			return
		end

		local src = cap[1]
		local kind = kind_of_src(src)
		local name = cap[2]

		if "string" ~= type(name) or "" == name then
			if "call" == kind then
				mo:response(src, false, "unknown")
			end
		else
			local fn = app._router:find(kind, name)
			if not fn and "post" == kind then
				fn = app._router:find("notify", name)
				if fn then
					kind = "notify"
				end
			end

			if not fn then
				if "call" == kind then
					mo:response(src, false, "unknown")
				end
			else
				local args = {}
				for i = 3, cap.n do
					args[i - 2] = cap[i]
				end

				local req = {
					name = name,			-- [必须] string    方法名
					args = args,			-- [必须] array     方法名之后的参数; 无则为 {}
					src = src or "",		-- [必须] string    来源 LPC 名; post 时常为空
					kind = kind,			-- [必须] string    call / post / notify
				}

				local pok, nret, rets = pcall(function ()
					local got = capture(fn(req))
					return got.n, got
				end)

				if "call" == kind then
					if not pok then
						mo:response(src, false, "error")
					else
						mo:response(src, table.unpack(rets, 1, nret))
					end
				end
			end
		end
	end
end


local function fork_serve(app)
	kco.fork(function ()
		co_serve_module(app)
	end)
end


--------------------------------------------------------------------------------------------
-- 服务对象

local L = {}


-- @brief 配置模块名; 服务中返回 false
-- @param [in]      cfg[table]			[可选] `{ name }`
-- @return ok[boolean]
-- @return [number|nil] code			失败错误码
function L:setup(cfg)
	if self._serving then
		return false, errcode.E_BUSY
	end

	local parsed, ok = cfger.parse_setup(cfg)
	if not ok or not parsed then
		return false, errcode.E_ARG
	end

	if "" ~= parsed.name then
		self._name = parsed.name
		self._name_ok = true
	end

	return true
end


-- @brief 注册模块并启动 co_recv 循环
-- @return ok[boolean]					未配置 name 或 new_module 失败为 false
-- @return [number|nil] code			失败错误码
-- @note 幂等; 已启动则直接 true
function L:serve()
	if not self._name_ok or "" == self._name then
		return false, errcode.E_STATE
	end

	if self._serving then
		return true
	end

	local mo, code = klpc.new_module(self._name)
	if not mo then
		self._last_code = code
		return false, code
	end

	self._mo = mo
	self._last_code = 0
	self._serving = true
	fork_serve(self)
	return true
end


-- @brief 关闭模块; 保留方法表与 name
-- @return ok[boolean]					true
function L:stop()
	local mo = self._mo
	self._serving = false
	self._mo = nil

	if mo and mo.close then
		mo:close()
	end

	return true
end


-- @brief 注册 REQUEST 方法
-- @param [in]      name[string]		方法名
-- @param [in]      fn[function]		`fn(req)`, 返回值作为 response 载荷
-- @return ok[boolean]
function L:call(name, fn)
	return self._router:add("call", name, fn)
end


-- @brief 注册 POST 方法; 无回应
-- @param [in]      name[string]		方法名
-- @param [in]      fn[function]		`fn(req)`
-- @return ok[boolean]
function L:post(name, fn)
	return self._router:add("post", name, fn)
end


-- @brief 注册 NOTIFY 方法; 与 post 同收包路径, 无回应
-- @param [in]      name[string]		方法名
-- @param [in]      fn[function]		`fn(req)`
-- @return ok[boolean]
function L:notify(name, fn)
	return self._router:add("notify", name, fn)
end


--------------------------------------------------------------------------------------------
-- 工厂

local lpcer = {}


-- @brief 新建一个 LPC 服务
-- @param [in]      cfg[table]			[可选] `{ name }`
-- @return [table]						服务对象
lpcer.new = function (cfg)
	cfg = cfg or {}
	local parsed, ok = cfger.parse_setup(cfg)
	local name = ""
	local name_ok = false
	if ok and parsed and "" ~= parsed.name then
		name = parsed.name
		name_ok = true
	elseif nil ~= cfg.name then
		name_ok = false
	end

	local obj = {
		_router = router.new(),
		_name = name,
		_name_ok = name_ok,
		_mo = nil,
		_serving = false,
	}

	setmetatable(obj, {
		__index = L,
	})

	return obj
end


return lpcer
