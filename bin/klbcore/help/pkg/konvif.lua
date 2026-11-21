--[[
-- Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   konvif.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  C konvif, ONVIF 发现 (connect/listen 为桩)
--   \n require("konvif")
--   \n C导出文件: ./klb/src_packages/klbonvif/onvif_klua/klbonvif_klua.c
--   \n 现状: connect/listen 仅返回 NOTSUP; discover/discover_listen 可用
-- @version 0.1
--]]

local konvif = {}


-- @brief ONVIF 客户端连接 (未实现)
-- @param [in] ...	[任意]	当前桩实现忽略入参
-- @return nil, code[number(int)]	恒为 nil, -95 (KLB_NETCODE_EOPNOTSUPP)
konvif.connect = function (...)
	return nil, -95
end


-- @brief ONVIF 服务端监听 (未实现)
-- @param [in] ...	[任意]	当前桩实现忽略入参
-- @return nil, code[number(int)]	恒为 nil, -95 (KLB_NETCODE_EOPNOTSUPP)
konvif.listen = function (...)
	-- 返回的句柄对象方法: close / send / co_recv
	return nil, -95
end


-- @brief 搜索端: 发 WS-Discovery Probe 组播, 收 ProbeMatches
-- @return discovery[userdata]	发现对象
-- @return nil, code[number(int)]	失败 (无 netmulti 为 INVAL; 开 socket 失败为 BIND_FAIL)
-- @note 对象方法: close / send / co_recv
--   \n eg. local d = konvif.discover(); d:send(probe_xml); local k, head, body = d:co_recv()
konvif.discover = function ()
	local d = {}

	-- @brief 关闭发现 socket (幂等)
	-- @return 无
	-- @note __gc 亦调此
	d.close = function (self)
		return
	end

	-- @brief 发送组播/单播报文
	-- @param [in] data[string]	报文
	-- @param [in] ip[string]		[可选] 目标 IP; 缺省为组播地址
	-- @param [in] port[number(int)]	[可选] 目标端口; 缺省为默认端口
	-- @return [number(int)] 0.成功; 非0.失败 (已关闭恒 1)
	d.send = function (self, data, ip, port)
		return 0
	end

	-- @brief 接收一条报文
	-- @param [in] self[userdata]
	-- @return kind[string]	"datagram" / "error" / "exit"
	-- @return head[string]	报文头 (MNP TEXT 帧头, 通常为空)
	-- @return body[string]	报文体; kind 为 "error" 时为错误码 number
	-- @note 仅协程内调用; 同一时刻只允许一个等待者
	--   \n 队列空时挂起, 直到收包 / 出错 / 环境退出
	d.co_recv = function (self)
		return "datagram", "", ""
	end

	return d
end


-- @brief 服务端: 加入组播, 收 Probe, 回 ProbeMatches
-- @param [in] port[number(int)]	[可选] 监听端口; 缺省 0 = 默认端口
-- @return discovery[userdata]	发现对象 (对象方法同 discover)
-- @return nil, code[number(int)]	失败
konvif.discover_listen = function (port)
	local d = {}

	return d
end


return konvif
