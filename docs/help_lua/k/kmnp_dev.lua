--[[
-- Copyright(c) 2020, LGPL v3 All Rights Reserved
-- @brief	设备硬件接口
--   \n "kmnp_dev"
-- @author  李绍良
-- @version 0.1
--]]

local kmnp_dev = {}


-- @brief 打开设备硬件接口
-- @param [in]	path[string]	动态库路径(采用动态加载时有效)
-- @return [string] 0.成功; 非0.错误码
--  \n 若采用动态加载: 加载动态库,并加载函数地址: "klb_mnp_dev_open"
--  \n 使用函数"klb_mnp_dev_open"打开库提供的接口函数
kmnp_dev.dlopen = function (path)
	return 0
end

-- @brief 关闭设备硬件接口
-- @return 无
--  \n 卸载动态库, 并清接口函数地址
kmnp_dev.dlclose = function ()
	return
end

-- @brief 获取库版本
-- @return [string] 字符串版本号
--  \n [number(int)] 数值版本号
--  \n eg. v1.1.1 = 10101
kmnp_dev.version = function ()
	return 'v1.1.1', 10101
end

-- @brief 获取项目代号
-- @return [string] 项目代号
--  \n 'simulator'为模拟器
kmnp_dev.project = function ()
	return 'simulator'
end

-- @brief 初始化
-- @param [in]	json[string]	JSON参数
-- @return [string] 0.成功; 非0.错误码
kmnp_dev.init = function (json)
	return 0
end

-- @brief 启动线程(开始工作)
-- @param [in]	json[string]	JSON参数
-- @return [string] 0.成功; 非0.错误码
kmnp_dev.start = function (json)
	return 0
end

-- @brief 关闭线程(停止工作)
-- @return 无
kmnp_dev.stop = function ()
	return
end

-- @brief 退出
-- @return 无
kmnp_dev.quit = function ()
	return
end

-- @brief 协议控制
-- @param [in]	cmd[string]		命令名称
-- @param [in]	lparam[string]	JSON参数1
-- @param [in]	wparam[string]	JSON参数2
-- @return [string] '' 或 JSON回复
kmnp_dev.control = function (cmd, lparam, wparam)
	return ''
end


-- @brief 设置数据流接收者
-- @param [in]	index[number(int)]				序号[0,16)
-- @param [in]	ptr[userdata(lightuserdata)]	数据接收函数
-- @return 无
-- @note 数据接收函数参考: "./klb/inc/mnp/klb_mnp_stream.h" 的 定义 klb_mnp_stream_cb
--	@brief 码流回调函数定义
-- 	@param [in] chnn        通道
-- 	@param [in] sidx        流序号: klb_mnp_sidx_e
-- 	@param [in] *p_stream   码流数据
-- 	@return int 0.成功; 非0.失败
-- 	typedef int(*klb_mnp_stream_cb)(uint32_t chnn, uint32_t sidx, const klb_mnp_stream_t* p_stream);
kmnp_dev.set_stream_receiver = function (index, ptr)
	return
end

return kmnp_dev
