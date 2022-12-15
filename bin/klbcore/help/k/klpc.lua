--[[
-- Copyright(c) 2022, LGPL All Rights Reserved
-- @file   klpc.lua
-- @brief  C klpc, Local Procedure Call Protocol
--   \n require("klpc")
--   \n C导出文件: ./klb/src_c/klua/klua_base/klua_klpc.c
--   \n 本地夸线程通信
--   \n 定义模块, 访问模块
-- @version 0.1
--]]

local klpc = {}


-- @brief 新建一个导出模块(newmetatable)
-- @param [in]	name[string]		模块名称(不可重复)
-- @return module对象
-- @note 
klpc.new_module = function (name)
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
	
	-- @brief 读取(LPC)消息
	-- @return [string]name		请求(call/post)来源名称
	--			[任意]...		消息列表
	-- @note 仅在协程中使用
	--		eg. local src, msg = mo:co_recv()
	--			...
	--			mo:response(src, '123', true)
	mo:co_recv = function ()
		local name = 'efse2'
		return name, ...
	end

	-- @brief 回复数据
	-- @param [in]	name[string]	目标名称
	-- @param [in]	...[任意]		消息
	-- @return 无
	-- @note 由 co_recv 函数获取后, 回应 response
	mo:response = function (name, ...)
		return
	end
	
	-- @brief 通知消息
	-- @param [in]	name[string]	目标名称
	-- @param [in]	...[任意]		消息
	-- @return 无
	-- @note 
	mo:notify = function (name, ...)
		return
	end
	
	return mo
end


-- @brief 新建一个lpc对象
-- @return lpc对象
klpc.new = function ()
	local lpc = {}

	-- @brief 关闭
	-- @return 无
	-- @note 显示关闭, 可提前释放非Lua相关的资源(内存, 文件句柄等)
	--		不显示关闭, 则需要等待gc才释放
	lpc:close = function ()
		return
	end

	-- @brief 获取当前状态
	-- @return 	b[boolean]					是否正常
	--			status[number(int)]			状态码; 0.正常; 非0.错误码
	lpc:status = function ()
		return true, 0
	end

	-- @brief 向某个模块post消息(不等待返回)
	-- @param [in]	mo_name[string]	目标模块名称
	-- @return [boolean] 是否成功
	lpc:post = function (mo_name, ...)
		return true
	end	

	-- @brief 向某个模块发送消息,并等待返回数据
	-- @param [in]	mo_name[string]	目标模块名称
	-- @param [in]	...[任意类型]	请求数据
	-- @return [...] 对方回复的数据
	-- @note 仅在协程中使用
	lpc:co_call = function (mo_name, ...)
		return ...
	end

	-- @brief 接收 'notify' 通知消息
	-- @return [...] 通知消息
	-- @note 需要初始化时开启通知接收, 仅在协程中使用
	lpc:co_recv_notify = function ()
		return ...
	end
	
	return lpc
end


-- @brief 向某个模块post消息(不等待返回)
-- @param [in]	mo_name[string]	目标模块名称
-- @return [boolean] 是否成功
klpc.post = function (mo_name, ...)
	return true
end


return klpc
