--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klblpc init.lua
-- @note   本进程跨线程 LPC; 服务在 serve/; 客户端在 client/; 底层 klpc; **非** 网络 RPC
--   \n 返回码: 直接用 klbcore.klbcode.base / net; 不本地再映射 E_*
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local lpcer = require("klbcore.klblpc.serve.lpcer")
local lpcclienter = require("klbcore.klblpc.client.lpcclienter")
local lpcclient_co = require("klbcore.klblpc.client.lpcclient")


local klblpc = {}


--------------------------------------------------------------------------------------------
-- 服务端

-- @brief 新建一个独立 LPC 服务
-- @param [in]      cfg[table]			[可选] `{ name }`
-- @return [table]						服务对象
klblpc.new = function (cfg)
	return lpcer.new(cfg)
end


--------------------------------------------------------------------------------------------
-- 内置服务 (一份; 与 new 互不影响)

local builtin = nil


local function ensure_builtin()
	if not builtin then
		builtin = lpcer.new()
	end

	return builtin
end


-- @brief 配置内置服务模块名
-- @param [in]      cfg[table]			[可选] `{ name }`
-- @return ok[boolean]					服务中未 stop, 或 name 非法时为 false
klblpc.setup = function (cfg)
	return ensure_builtin():setup(cfg)
end


-- @brief 注册模块并启动 serve 循环
-- @return ok[boolean]					未配置 name 为 false
-- @note 幂等.
klblpc.serve = function ()
	return ensure_builtin():serve()
end


-- @brief 关闭内置服务模块; 保留方法表
-- @return ok[boolean]					true
klblpc.stop = function ()
	if builtin then
		builtin:stop()
	end

	return true
end


--------------------------------------------------------------------------------------------
-- 方法注册 (转到内置服务)

-- @brief 注册 REQUEST 方法
-- @param [in]      name[string]		方法名
-- @param [in]      fn[function]		`fn(req)`, 返回值作为 response 载荷
-- @return ok[boolean]
klblpc.call = function (name, fn)
	return ensure_builtin():call(name, fn)
end


-- @brief 注册 POST 方法; 无回应
-- @param [in]      name[string]		方法名
-- @param [in]      fn[function]		`fn(req)`
-- @return ok[boolean]
klblpc.post = function (name, fn)
	return ensure_builtin():post(name, fn)
end


-- @brief 注册 NOTIFY 方法; 与 post 同收包路径, 无回应
-- @param [in]      name[string]		方法名
-- @param [in]      fn[function]		`fn(req)`
-- @return ok[boolean]
klblpc.notify = function (name, fn)
	return ensure_builtin():notify(name, fn)
end


--------------------------------------------------------------------------------------------
-- 客户端 LPC (长期)

-- @brief 新建可长期复用的 LPC 客户端
-- @return [table]						klpc 客户端; 失败为 nil
-- @note 对象方法: `:co_call(mo_name, ...)` / `:post(mo_name, ...)` / `:close`
klblpc.new_client = function ()
	return lpcclienter.new_client()
end


--------------------------------------------------------------------------------------------
-- 客户端 LPC (一次性)

-- @brief 向模块 post; 不等待
-- @param [in]      mo_name[string]		目标模块名
-- @param [in]      ...					载荷; 首参建议为方法名
-- @return ok[boolean]
klblpc.post_to = function (mo_name, ...)
	return lpcclient_co.post(mo_name, ...)
end


-- @brief 向模块 REQUEST 并等待回应; 须在 kco 协程内
-- @param [in]      mo_name[string]		目标模块名
-- @param [in]      ...					载荷; 首参建议为方法名
-- @return [...]						对方 response 的数据; 失败 `nil, code[, err]`
klblpc.co_call = function (mo_name, ...)
	return lpcclient_co.co_call(mo_name, ...)
end


-- @brief 带 deadline 的 REQUEST (显式毫秒)
-- @param [in]      mo_name[string]		目标模块名
-- @param [in]      ms[number]			毫秒; nil 或 <=0 不限
-- @param [in]      ...					载荷
-- @return [...]						同 `co_call`
klblpc.co_call_timed = function (mo_name, ms, ...)
	return lpcclient_co.co_call_timed(mo_name, ms, ...)
end


-- @brief 设置默认单次调用 deadline
-- @param [in]      ms[number]			毫秒; nil 或 <=0 不限
klblpc.set_timeout = function (ms)
	lpcclient_co.set_timeout(ms)
end


-- @brief 注册对端失联回调; 投递失败时触发
-- @param [in]      fn[function]		`fn(mo_name)`
-- @return ok[boolean]
klblpc.on_peer_gone = function (fn)
	return lpcclient_co.on_peer_gone(fn)
end


return klblpc
