--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   kmuxlist.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  C kmuxlist: 专用 klb_netmux_socket_t* 跨线程 FIFO
--   \n require("kmuxlist")
--   \n C导出文件: ./klb/src_c/klua/klua_multithread/klua_kmuxlist.c
--   \n 内置 wrap/unwrap; push/pop 直接传 netmux sock lightuserdata
--   \n 适用于监听线程 → 业务线程 移交已探测连接
-- @stability experimental
-- @impl evolving
-- @history 修改历史
--  \n [2026-11] 创建文件
--]]

local kmuxlist = {}


-- @brief 放入已探测 sock 到尾部 (内置 wrap)
-- @param [in] path[string]				路径: eg. '/aaa/listen-http'
-- @param [in] sock[lightuserdata]		klb_netmux_socket_t*; 成功后所有权迁走
-- @return 无
-- @note 先进先出; 若该 path 已配 max 且已满, 先销毁队头未消费项
kmuxlist.push = function (path, sock)
	return
end


-- @brief 取出头部 sock (内置 unwrap)
-- @param [in] path[string]				路径
-- @return [lightuserdata|nil]			klb_netmux_socket_t*
-- @note 先进先出
kmuxlist.pop = function (path)
	return
end


-- @brief 创建 kmuxlist 对象 (按 path 共享同一队列)
-- @param [in] path[string]				路径
-- @param [in] opts[table|nil]			[可选] { max=N } 或 { max_size=N }; 0/缺省=不限
-- @return kmuxlist对象
-- @note 满时 push 自动销毁队头未消费 sock (关 fd)
--   \n eg. kmuxlist.new("/serve/listen-http", { max = 256 })
--   \n klblisten cfg.kmuxlist 可按协议分 path (http/smp/mnp/rtsp/smp_rpc/mnp_rpc)
kmuxlist.new = function (path, opts)
	local o

	-- @brief 放入 sock
	-- @param [in] sock[lightuserdata]	klb_netmux_socket_t*
	o.push = function (sock)
		return
	end

	-- @brief 取出 sock
	-- @return [lightuserdata|nil]
	o.pop = function ()
		return
	end

	-- @brief 队列长度
	-- @return [number]
	o.size = function ()
		return 0
	end

	-- @brief 清空并关闭未取走的 sock
	o.clear = function ()
		return
	end

	return o
end


return kmuxlist

-- end
