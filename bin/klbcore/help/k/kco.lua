--[[
-- Copyright(c) 2022, LGPL All Rights Reserved
-- @file   kco.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  C kco
--   \n require("kco")
--   \n C导出文件: ./klb/src_c/klua/klua_base/klua_kcoro.c
--   \n 协程(coroutine)
--   \n 与lua自带的协程库冲突, 使用本库替代即可!
-- @version 0.2
--]]

local kco = {}


-- @brief fork(创建)一个新协程(coroutine)
-- @param [in]	func[function]		协程入口函数
-- @param [in]	...[任意]			参数列表
-- @return h[userdata]				协程句柄; 可用 status/is_done/is_error/close/co_wait
-- @note eg. local h = kco.fork(function (...) print('123456', ...) end, 'a', 'b')
kco.fork = function (func, ...)
	return nil
end


-- @brief 等待一段时间后, 启动一个新的协程(coroutine)
-- @param [in]	tc[number(int)]		等待时间(单位毫秒)
-- @param [in]	func[function]		协程入口函数
-- @param [in]	...[任意]			参数列表
-- @return h[userdata]				协程句柄
-- @note eg. local h = kco.timeout(1000, function (...) print('123456', ...) end, 'a', 1)
kco.timeout = function (tc, func, ...)
	return nil
end


-- @brief 协程休眠一段时间
-- @param [in]	tc[number(int)]		休眠时间(单位毫秒)
-- @return 无
-- @note eg. kco.co_sleep(1000)
--  仅协程中使用
kco.co_sleep = function (tc)
	return
end


-- @brief 等待句柄对应协程结束
-- @param [in]	h[userdata]			fork/timeout 返回的句柄
-- @param [in]	tc[number(int)|nil]	最长等待(毫秒); nil/0 表示一直等
-- @return ok[boolean]				true=已结束; false=超时未结束
-- @return err[boolean]				是否异常结束 (仅 ok 为 true 时有意义)
-- @note 仅协程中使用; eg. local ok, err = kco.co_wait(h, 5000)
kco.co_wait = function (h, tc)
	return false, false
end


-- @brief 获取当前已登记协程数目
-- @return n[number(int)]			fork/timeout 登记后计入; 协程结束后减一
-- @note eg. local n = kco.count()
kco.count = function ()
	return 0
end


-- @brief 关闭所有协程
-- @return 无
-- @note quit 唤醒 + 丢弃计时/排空队列后, 强制收尸残留 (含纯 co_sleep), count 可归零; eg. kco.close_all()
kco.close_all = function ()
	return
end


-- @brief 协程快照 (诊断用)
-- @return info[table]				count[number(int)]; list[table] 数组
-- @note list[i]: id[string], status[string], in_timeout[boolean], in_wakeup[boolean], has_cb[boolean]
--   \n eg. local snap = kco.snapshot(); print(snap.count, #snap.list)
kco.snapshot = function ()
	return { count = 0, list = {} }
end


-- @brief 查询句柄状态
-- @param [in]	h[userdata]			协程句柄
-- @return s[string]				running/dead/suspended/normal/error
-- @note eg. print(kco.status(h)) 或 h:status()
kco.status = function (h)
	return "dead"
end


-- @brief 句柄是否已结束 (正常/异常/close 后均为 true)
-- @param [in]	h[userdata]			协程句柄
-- @return b[boolean]
-- @note eg. if kco.is_done(h) then ... end
kco.is_done = function (h)
	return false
end


-- @brief 句柄是否异常结束
-- @param [in]	h[userdata]			协程句柄
-- @return b[boolean]				status=="error" 时为 true
-- @note eg. if kco.is_error(h) then ... end
kco.is_error = function (h)
	return false
end


-- @brief 关闭单个协程 (suspended 可关; running/normal 报错)
-- @param [in]	h[userdata]			协程句柄
-- @return ok[boolean]				true 成功; false + err 时 reset 失败
-- @note 已结束则直接 true; eg. kco.close(h) 或 h:close()
kco.close = function (h)
	return true
end


-- @brief 当前运行的协程 thread 与是否主线程
-- @return co[thread]				当前 thread
-- @return ismain[boolean]			是否主线程
-- @note eg. local co, ismain = kco.running()
kco.running = function ()
	return nil, true
end


-- @brief 当前(或指定 thread)是否可 yield
-- @param [in]	co[thread|nil]		可选; 缺省为当前
-- @return b[boolean]
-- @note eg. if kco.isyieldable() then ... end
kco.isyieldable = function (co)
	return false
end

return kco
