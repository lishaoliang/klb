--[[
-- Copyright(c) 2022, LGPL All Rights Reserved
-- @file   krpc.lua
-- @brief  C krpc, Remote Procedure Call Protocol
--   \n require("klpc")
--   \n C导出文件: ./klb/src_c/klua/klua_net/klua_krpc.c
--   \n 网络远程RPC
--   \n 定义模块(服务端), 访问模块(客户端)
-- @version 0.1
--]]

local krpc = {}


-- @brief 新建一个导出模块(newmetatable)
-- @param [in]	port[number(int)]	socket端口
-- @return module对象
-- @note 
krpc.new_module = function (port)
	local mo = {}
	
	-- @brief 关闭
	-- @return 无
	-- @note 显示关闭, 可提前释放非Lua相关的资源(内存, 文件句柄等)
	--		不显示关闭, 则需要等待gc才释放
	mo:close = function ()
		return
	end

	-- @brief 获取当前状态
	-- @return 	b[boolean]					是否正常
	--			status[number(int)]			状态码; 0.正常; 非0.错误码
	mo:status = function ()
		return true, 0
	end
	
	-- @brief 设置路径对应的协议
	-- @param [in]	path[string]			路径; eg. '/test'
	-- @param [in]	protocol[string]		协议; eg. 'http-lua','http-json','ws-lua','ws-json'
	-- @return 无
	mo:route = function (path, protocol)
		return
	end
	
	-- @brief 读取(RPC)消息
	-- @return 	protocol[number(int)]		RPC协议编号
	--			id[number(int)]				socket编号
	--			sequence[number(int)]		序列号: 小于1000.为post消息; 大于等于1000.为call消息
	--			...[任意]					请求参数列表
	-- @note 仅在协程中使用
	mo:co_recv = function ()
		local protocol = 10		-- RPC协议编号
		local id = 1000			-- socket编号
		local sequence = 1000	-- 序列号
		-- ...	请求的参数列表
		return protocol, id, sequence, ...
	end

	-- @brief 回复请求
	-- @param [in]	protocol[number(int)]	RPC协议编号
	-- @param [in]	id[number(int)]			socket编号
	-- @param [in]	sequence[number(int)]	序列号: 大于等于1000.为call消息
	-- @param [in]	...[任意]				回复的数据
	-- @return 无
	-- @note 在mo:co_recv处理后,回应请求
	mo:response = function (protocol, id, sequence, ...)
		return
	end

	-- @brief 通知单个客户端: 服务方主动发起
	-- @param [in]	protocol[number(int)]	RPC协议编号
	-- @param [in]	id[number(int)]			socket编号
	-- @param [in]	...[任意]				通知参数列表
	-- @return b[boolean]					通知是否送达网络; false.网络已断开(id失效)
	-- @note 业务层可以做成"订阅"机制, 或直接"推送"机制
	mo:notify = function (protocol, id, ...)
		return true
	end
	
	-- @brief 通知所有客户端: 服务方主动发起
	-- @param [in]	...[任意]				通知参数列表
	-- @return 无
	mo:notify_all = function (...)
		return
	end
	
	return mo
end


-- @brief 新建一个rpc对象
-- @param [in]	ip[string]			目标模块IP
-- @param [in]	port[number(int)]	端口
-- @return rpc对象
krpc.new = function (ip, port)
	local rpc = {}

	-- @brief 关闭
	-- @return 无
	-- @note 显示关闭, 可提前释放非Lua相关的资源(内存, 文件句柄等)
	--		不显示关闭, 则需要等待gc才释放
	rpc:close = function ()
		return
	end

	-- @brief 获取当前状态
	-- @return 	b[boolean]					是否正常
	--			status[number(int)]			状态码; 0.正常; 非0.错误码
	rpc:status = function ()
		return true, 0
	end

	-- @brief post消息
	-- @param [in]	...[任意]		请求参数列表
	-- @return 无
	rpc:post = function (...)
		return
	end

	-- @brief 向某个模块发起请求,并等待返回数据
	-- @param [in]	...[任意]		请求参数列表
	-- @return [...] 对方回复的数据列表
	-- @note 仅在协程中使用
	rpc:co_call = function (...)
		return ...
	end
	
	-- @brief 读取通知消息
	-- @return [...] 对方回复的数据列表
	-- @note 仅在协程中使用
	rpc:co_recv_notify = function ()
		return ...
	end
	
	return rpc
end

return krpc
