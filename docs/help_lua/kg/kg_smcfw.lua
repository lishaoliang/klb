--[[
-- Copyright(c) 2020, LGPL All Rights Reserved
-- @brief   流媒体客户端
--   \n kg stream media client framework
-- @author  李绍良
-- @version 0.1
--]]

local kg_smcfw = {}


-- @brief 初始化模块
-- @param [in] cfg[string]	   json格式配置参数
-- @return [boolean] true.成功; false.失败
--   \n [string] error  错误字符串
kg_smcfw.init = function (cfg)
	return true, ''
	--return false, 'error'
end


-- @brief 退出模块
-- @return 无
kg_smcfw.quit = function ()

end


-- @brief 启动模块: 加载启动脚本
-- @param [in] name[string]	   线程名称
-- @param [in] entry[string]   加载入口脚本; eg.'xxx.lua'
-- @param [in] preload[string] 预加载库函数名称; 决定线程创建后使用那些预加载库
-- @param [in] loader[string]  Lua脚本加载函数; 从自定义函数加载Lua脚本; 默认使用fopen从文件加载
-- @return [boolean] true.成功; false.失败
--   \n [string] error  错误字符串
-- @note see kg_thread.create
kg_smcfw.start = function (name, entry, preload, loader)
	return true, ''
	--return false, 'error'
end


-- @brief 停止模块
-- @return 无
kg_smcfw.stop = function ()

end


-- @brief 设置新连接加入的情况: 正常加入, 加入失败
-- @param [in] newconn[function]    处理函数
-- @return 无
-- @note 接收函数原型: 
-- local newconn = function (name, protocol, ok, err)
--    return 0
-- end
-- @param [in] name[string]	    连接名称
-- @param [in] protocol[string] 协议名称
-- @param [in] ok[boolean]      true.成功; false.失败
-- @param [in] err[string]      错误字符串
-- @return 0
kg_smcfw.set_newconn = function (newconn)

end


-- @brief 设置txt数据接收处理函数
-- @param [in] receiver[function]    数据接收处理函数
-- @return 无
-- @note 接收函数原型: 
-- local receiver = function (name, protocol, sequence, uid, extra, txt)
--    return 0
-- end
-- @param [in] name[string]	    连接名称
-- @param [in] protocol[string] 协议名称
-- @param [in] sequence[number] 序列号: [0,2^32)
-- @param [in] uid[number]      UID: [0,2^32)
-- @param [in] extra[string]    附加数据(或头数据)
-- @param [in] txt[string]      正式文本数据
-- @return 0
kg_smcfw.set_receiver = function (receiver)

end


-- @brief 设置连接异常等断开处理函数
-- @param [in] disconnect[function]   异常等断开处理函数
-- @return 无
-- @note 处理函数原型: 
-- local disconnect = function (name, protocol, err)
--    return 0
-- end
-- @param [in] name[string]	    连接名称
-- @param [in] protocol[string] 协议名称
-- @param [in] err[string]      错误字符串
-- @return 0
kg_smcfw.set_disconnect = function (disconnect)

end


-- @brief 发起连接
-- @param [in] name[string]	    连接名称
-- @param [in] protocol[string] 协议名称
-- @param [in] network[string]  网络类型: 'tcp'
-- @param [in] address[string]  网络地址: ':8000'
-- @return 无
kg_smcfw.dial = function (name, protocol, network, address)

end


-- @brief 发送文本数据
-- @param [in] name[string]	    连接名称
-- @param [in] protocol[string] 协议名称
-- @param [in] sequence[number] 序列号: [0,2^32)
-- @param [in] uid[number]      UID: [0,2^32)
-- @param [in] extra[string]    附加数据(或头数据)
-- @param [in] txt[string]      正式文本数据
-- @return [boolean] true.成功; false.失败
kg_smcfw.send_txt = function (name, protocol, sequence, uid, extra, txt)
	return true
	--return false
end

return kg_smcfw
