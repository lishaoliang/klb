--[[
-- Copyright(c) 2026, LGPL All Rights Reserved
-- @file   kxpc.lua
-- @brief  C kxpc, Cross-Process Call
--   \n require("kxpc")
--   \n C导出文件: ./klb/src_c/klua/klua_base/klua_kxpc.c
--   \n 本机跨进程通信; 对标 klpc(本进程跨线程); 底层 klb_xpc
--   \n 传输: msgq(默认; SHM 环缓) / unix(Unix domain socket) / npipe(命名管道) / sysmq(系统 mqueue)
--   \n 语义: POST/REQUEST/RESPONSE/NOTIFY 四类消息; 载荷为任意 Lua 值列表(自动序列化)
-- @version 0.3
--]]

local kxpc = {}


-- @brief 消息类型: POST (无响应)
kxpc.POST = 10

-- @brief 消息类型: REQUEST (须 response 回应)
kxpc.REQUEST = 11

-- @brief 消息类型: RESPONSE (对 REQUEST 的回应)
kxpc.RESPONSE = 12

-- @brief 消息类型: NOTIFY (通知)
kxpc.NOTIFY = 13


-- @brief 传输类型: 命名消息队列 (默认; SHM 环缓; 全平台)
kxpc.TRANSPORT_MSGQ = 0

-- @brief 传输类型: Unix domain socket (仅非 Win; 默认路径 "/tmp/klb.xpc.<name>")
kxpc.TRANSPORT_UNIX = 1

-- @brief 传输类型: 命名管道 (全平台; 默认路径见 klb_xpc.h)
kxpc.TRANSPORT_NPIPE = 2

-- @brief 传输类型: 系统级命名消息队列 (POSIX mqueue; Win/无 mqueue 创建失败)
kxpc.TRANSPORT_SYSMQ = 3


-- @brief 新建服务侧模块(创建方; 独占 inbox)
-- @param [in]	name[string]		模块名(1..107 字节); 不可与已存在模块重复
-- @return kxpc对象; 失败 nil
-- @note 对象方法: close / name / is_owner / post / notify / response / request / recv
--   \n eg. local mo = kxpc.new_module('xdemo1')
kxpc.new_module = function (name)
	local mo = {}

	-- @brief 关闭
	-- @return 无
	-- @note 销毁本端句柄; 创建方同时拆除内核对象(inbox/路径)
	--		不显示关闭则等待 gc (__gc/__close 亦调此)
	function mo:close()
		return
	end

	-- @brief 本端模块名
	-- @return name[string|nil]		名; 已关闭为 nil
	function mo:name()
		return nil
	end

	-- @brief 是否为创建方(服务侧)
	-- @return [boolean]			true=new_module* 创建; false=open* 对端
	function mo:is_owner()
		return true
	end

	-- @brief POST 消息(不等待返回)
	-- @param [in]	dst[string]		目标模块名
	-- @param [in]	...[任意]		载荷(任意 Lua 值列表)
	-- @return [boolean]			是否成功
	function mo:post(dst, ...)
		return true
	end

	-- @brief NOTIFY 通知(不等待返回)
	-- @param [in]	dst[string]		目标模块名
	-- @param [in]	...[任意]		载荷(任意 Lua 值列表)
	-- @return [boolean]			是否成功
	function mo:notify(dst, ...)
		return true
	end

	-- @brief 对 REQUEST 回 RESPONSE
	-- @param [in]	dst[string]		请求来源名(recv 首返回的 src)
	-- @param [in]	corr[number(int)]	与 recv 取得的 corr 一致
	-- @param [in]	...[任意]		载荷(任意 Lua 值列表)
	-- @return [boolean]			是否成功
	-- @note 仅收方对已收到的 REQUEST 回复; corr 不可自造
	function mo:response(dst, corr, ...)
		return true
	end

	-- @brief REQUEST 请求并等待 RESPONSE
	-- @param [in]	dst[string]		目标模块名
	-- @param [in]	timeout_ms[number(int)]	超时; 0.不等待; -1.一直等
	-- @param [in]	...[任意]		请求载荷(任意 Lua 值列表)
	-- @return [...]				对方 response 的载荷列表
	-- @return nil, err[string]			失败; err='closed'(已关闭) / 'failed'(失败或超时)
	-- @note 阻塞等待; 等待期间收到的其它消息入本端 pending, 供后续 recv 取出
	--		eg. local ok, v = cli:request('xdemo1', 3000, 'echo', 'world')
	function mo:request(dst, timeout_ms, ...)
		return ...
	end

	-- @brief 接收一条消息
	-- @param [in]	timeout_ms[number(int)]	[可选] 超时; 默认 -1(一直等); 0.不等待
	-- @return type[number(int)|nil]		消息类型: kxpc.POST/REQUEST/RESPONSE/NOTIFY; 超时或已关闭为 nil
	-- @return src[string]				来源模块名
	-- @return corr[number(int)]			REQUEST/RESPONSE 的关联 id; 其它类型为 0
	-- @return [...]					载荷列表(发送方 post/notify/request/response 传入的 ...)
	-- @note 单条载荷上限 8K 字节, 超出视为失败
	--		eg. local t, src, corr, name, a = mo:recv(500)
	--			if kxpc.REQUEST == t then mo:response(src, corr, true, a) end
	function mo:recv(timeout_ms)
		return nil
	end

	return mo
end


-- @brief 新建服务侧模块(unix domain socket 传输)
-- @param [in]	name[string]		模块名(1..107 字节)
-- @param [in]	path[string]		[可选] 监听路径; nil/空=默认 "/tmp/klb.xpc.<name>"
-- @return kxpc对象; 失败 nil(含 Win)
-- @note 对象方法同 new_module 返回对象
kxpc.new_module_unix = function (name, path)
	local mo = {}

	return mo
end


-- @brief 新建服务侧模块(命名管道传输)
-- @param [in]	name[string]		模块名(1..107 字节)
-- @param [in]	path[string]		[可选] 管道路径; nil/空=默认见 klb_xpc.h
-- @return kxpc对象; 失败 nil
-- @note 对象方法同 new_module 返回对象
kxpc.new_module_npipe = function (name, path)
	local mo = {}

	return mo
end


-- @brief 新建服务侧模块(系统级 mqueue 传输)
-- @param [in]	name[string]		模块名(1..107 字节)
-- @param [in]	path[string]		[可选] mq 名; nil/空=默认 "xpc.in.<name>"
-- @return kxpc对象; 失败 nil(含 Win/无 mqueue)
-- @note 对象方法同 new_module 返回对象
kxpc.new_module_sysmq = function (name, path)
	local mo = {}

	return mo
end


-- @brief 打开对端模块(客户端; msgq)
-- @param [in]	name[string]		对端已创建的模块名
-- @return kxpc对象; 对端不存在或失败 nil
-- @note 本地另建临时 inbox 收 RESPONSE; is_owner=false
--   \n 对象方法同 new_module 返回对象
--   \n eg. local cli = kxpc.open('xdemo1')
kxpc.open = function (name)
	local mo = {}

	return mo
end


-- @brief 打开对端模块(客户端; unix domain socket)
-- @param [in]	name[string]		对端模块名
-- @param [in]	path[string]		[可选] 对端路径; nil/空=默认 "/tmp/klb.xpc.<name>"
-- @return kxpc对象; 失败 nil(含 Win)
-- @note 对象方法同 new_module 返回对象
kxpc.open_unix = function (name, path)
	local mo = {}

	return mo
end


-- @brief 打开对端模块(客户端; 命名管道)
-- @param [in]	name[string]		对端模块名
-- @param [in]	path[string]		[可选] 对端管道路径; nil/空=默认见 klb_xpc.h
-- @return kxpc对象; 失败 nil
-- @note 对象方法同 new_module 返回对象
kxpc.open_npipe = function (name, path)
	local mo = {}

	return mo
end


-- @brief 打开对端模块(客户端; 系统级 mqueue)
-- @param [in]	name[string]		对端模块名
-- @param [in]	path[string]		[可选] 对端 mq 名; nil/空=默认 "xpc.in.<name>"
-- @return kxpc对象; 失败 nil(含 Win/无 mqueue)
-- @note 对象方法同 new_module 返回对象
kxpc.open_sysmq = function (name, path)
	local mo = {}

	return mo
end


-- @brief 打开对端模块(等价 kxpc.open; msgq)
-- @param [in]	name[string]		对端模块名
-- @return kxpc对象; 失败 nil
kxpc.new = kxpc.open


return kxpc
