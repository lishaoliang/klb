--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbxpc init.lua
-- @note   本机跨进程 XPC; 底层 kxpc / klb_xpc; **非** 本进程 LPC(klblpc); **非** 网络 RPC(klbrpc)
--   \n 单路: new/setup/serve; 多传输组: new_serve_group/serve_group (serve/grouper.lua)
-- @history 修改历史
--		[2026-11] 创建文件
--		[2026-11] M3: serve/client 主路径
--]]
local cfger = require("klbcore.klbxpc.cfger")
local xpcer = require("klbcore.klbxpc.serve.xpcer")
local grouper = require("klbcore.klbxpc.serve.grouper")
local xpcclienter = require("klbcore.klbxpc.client.xpcclienter")
local xpcclient_co = require("klbcore.klbxpc.client.xpcclient")


local klbxpc = {}


-- 默认传输名 (POSIX unix / Windows msgq); 使用方取默认值, 勿各自硬编码
klbxpc.DEFAULT_TRANSPORT = cfger.DEFAULT_TRANSPORT
klbxpc.ALL_TRANSPORTS = cfger.ALL_TRANSPORTS
klbxpc.MODE_RECV = cfger.MODE_RECV
klbxpc.MODE_ROUTE = cfger.MODE_ROUTE
-- 名/路径缓冲上限 (同 C KLB_XPC_NAME_BUF / PATH_MAX); 非业务短名策略
klbxpc.NAME_BUF = cfger.NAME_BUF
klbxpc.PATH_MAX = cfger.PATH_MAX
klbxpc.NAME_MAX = cfger.NAME_MAX


--------------------------------------------------------------------------------------------
-- 服务端 (单路)

klbxpc.new = function (cfg)
	return xpcer.new(cfg)
end


local builtin = nil


local function ensure_builtin()
	if not builtin then
		builtin = xpcer.new()
	end

	return builtin
end


klbxpc.setup = function (cfg)
	return ensure_builtin():setup(cfg)
end


klbxpc.serve = function ()
	return ensure_builtin():serve()
end


klbxpc.stop = function ()
	if builtin then
		builtin:stop()
	end

	return true
end


klbxpc.call = function (name, fn)
	return ensure_builtin():call(name, fn)
end


klbxpc.post = function (name, fn)
	return ensure_builtin():post(name, fn)
end


klbxpc.notify = function (name, fn)
	return ensure_builtin():notify(name, fn)
end


--------------------------------------------------------------------------------------------
-- 服务端 (多传输组; 与单路 builtin 独立)

-- @brief 新建未 open 的 serve 组
-- @param [in] cfg[table|nil]
-- @return [table]  组对象 (:open/:serve/:co_recv/:response/:stop/:info ...)
klbxpc.new_serve_group = function (cfg)
	return grouper.new(cfg)
end


-- @brief 按规格 open+serve, 返回已启动的组
-- @param [in] cfg[table]  name; transports?|"all"; mode?=recv|route; paths?
-- @return [table]|nil, code[number|nil]
-- @note mode=recv: 使用者 co_recv, 首返回值=transport; REQUEST 用 :response(tr, src, corr, ...)
--   \n mode=route: 须先 g=new_serve_group; g:call/...; g:open; g:serve (本函数不注册方法)
klbxpc.serve_group = function (cfg)
	local g = grouper.new()
	local ok, code = g:open(cfg)
	if not ok then
		return nil, code
	end

	ok, code = g:serve()
	if not ok then
		g:stop()
		return nil, code
	end

	return g
end


--------------------------------------------------------------------------------------------
-- 客户端

klbxpc.new_client = function (peer_name, opts)
	return xpcclienter.new_client(peer_name, opts)
end


klbxpc.post_to = function (mo_name, ...)
	return xpcclient_co.post(mo_name, ...)
end


klbxpc.post_to_ex = function (mo_name, opts, ...)
	return xpcclient_co.post_ex(mo_name, opts, ...)
end


klbxpc.co_call = function (mo_name, ...)
	return xpcclient_co.co_call(mo_name, ...)
end


klbxpc.co_call_timed = function (mo_name, ms, ...)
	return xpcclient_co.co_call_timed(ms, mo_name, ...)
end


klbxpc.co_call_ex = function (mo_name, opts, ms, ...)
	return xpcclient_co.co_call_ex(mo_name, opts, ms, ...)
end


klbxpc.set_timeout = function (ms)
	xpcclient_co.set_timeout(ms)
end


return klbxpc
