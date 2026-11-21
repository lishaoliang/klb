--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  客户端身份 SSID 包入口
-- @stability stable
-- @impl evolving
-- @attention
--   \n @stability stable: AI 禁止擅自添加/删除/变更本文件对外接口 (含导出名, 参数, 返回与已文档含义); 须用户点名
--   \n @impl evolving: 内部可重构, 只要不破坏本文件导出签名与可观察语义
-- @note   require("klbagent.core.ssid"); new_serve → 仅 serve worker/owner; new_client → CLI/stdio/tools/web
--   \n CLI/stdio/MCP/tools 跨进程须 new_client(xpc) → 本机 serve owner; 禁 new_serve
--   \n 同进程消费线程 (serve.security.auth) 用 lpc; 原始 token 仅 serve 维护
-- @history 修改历史
--  \n [2026-11] 创建文件
--  \n [2026-11] 注明 xpc 含 co_begin_ctx; MCP 须 new_client(xpc)
--  \n [2026-11] xpc 空 path 时由 xpccall 按 serve.port 拼 /tmp/alua.ssid.xpc.unix.pxxxx
--]]
local ssider = require("klbagent.core.ssid.serve")
local lpcclient = require("klbagent.core.ssid.client.lpcclient")
local xpcclient = require("klbagent.core.ssid.client.xpcclient")


local M = {}


--------------------------------------------------------------------------------------------
-- 服务端


-- @brief 取 ssid owner 服务面 (本地实现 + 可 ensure LPC/XPC)
-- @param [in] opts[table|nil]		预留; 当前忽略
-- @return serve[table]				ssid.serve 业务表
-- @stability stable
-- @note 仅 core/serve/worker.ssid (owner 线程) 使用; CLI/stdio/tools/web 禁调
--   \n 对齐 klblpc.new; 业务为 co_whoami / co_ensure … + ensure_owner
M.new_serve = function (opts)
	opts = opts
	return ssider
end


--------------------------------------------------------------------------------------------
-- 客户端


-- @brief 新建/取 ssid 客户端面 (LPC / XPC 函数面不同)
-- @param [in] opts[table|nil]		transport?, path?
-- @return client[table]			lpcclient 或 xpcclient 业务表
-- @stability stable
-- @note transport="lpc"(缺省) 同进程: co_whoami / co_get_token_plain / co_begin_ctx …
--   \n transport="xpc" 跨进程: co_whoami / co_ensure / co_begin_ctx … (无明文; begin_ctx 本机组装)
--   \n 可 set_peer; 业务方法一律 co_*; 须在 kco 协程内; 对齐 klblpc.new_client
--   \n CLI/stdio/MCP/tools 跨进程须 new_client({transport="xpc"}); 禁 new_serve
M.new_client = function (opts)
	if type(opts) ~= "table" then
		opts = {}
	end

	local transport = opts.transport
	if transport == "xpc" then
		-- 面选型 "xpc" 勿写入 kxpc peer.transport; 仅透传 path / 真后端名
		-- 空 path 留给 xpccall.co_call 按 serve.port 现场拼; 禁预锁默认口
		local peer = {
			port = opts.port,
			ports = opts.ports,
		}
		if type(opts.path) == "string" and opts.path ~= "" then
			peer.path = opts.path
		end

		local backend = opts.xpc_transport
		if type(backend) ~= "string" or backend == "" then
			backend = opts.backend
		end

		if type(backend) == "string" and backend ~= ""
			and backend ~= "xpc" and backend ~= "lpc" then
			peer.transport = backend
		end

		xpcclient.set_peer(peer)
		return xpcclient
	end

	return lpcclient
end


return M

-- end
