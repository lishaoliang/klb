--[[
-- Copyright(c) 2020, LGPL All Rights Reserved
-- @brief   模拟设备硬件
--   \n kg simulate
-- @author  李绍良
-- @version 0.1
--]]

local kg_simulate = {}


-- @brief 初始化模块
-- @param [in] cfg[string]	   json格式配置参数
-- @return [boolean] true.成功; false.失败
--   \n [string] error  错误字符串
kg_simulate.init = function (cfg)
	return true, ''
	--return false, 'error'
end


-- @brief 退出模块
-- @return 无
kg_simulate.quit = function ()

end


-- @brief 启动模块
-- @return [boolean] true.成功; false.失败
--   \n [string] error  错误字符串
kg_simulate.start = function ()
	return true, ''
	--return false, 'error'
end


-- @brief 停止模块
-- @return 无
kg_simulate.stop = function ()

end


-- @brief 设置媒体数据接收处理函数
-- @param [in] receiver[userdata(lightuserdata,function)]    数据接收处理函数(C函数)
-- @return 无
-- @note 接收C函数原型: 
-- @brief 码流回调函数定义
-- @param [in] chnn        通道
-- @param [in] sidx        流序号: klb_mnp_sidx_e
-- @param [in] *p_stream   码流数据
-- @return int 0.成功; 非0.失败
-- typedef int(*klb_mnp_stream_cb)(int chnn, int sidx, klb_mnp_stream_t* p_stream);
-- @see klb_mnp_stream.h
kg_simulate.set_stream_receiver = function (receiver)

end


-- @brief 控制命令: 设置等
-- @param [in] cmd[string]      命令名称
-- @param [in] chnn[number]     通道
-- @param [in] sidx[number]     流序号
-- @param [in] lparam[string]   参数1
-- @param [in] wparam[string]   参数2
-- @return [number] 0.成功; 非0.错误码
--   \n [string] error  错误字符串
kg_simulate.command = function (cmd, chnn, sidx, lparam, wparam)
	return 0, ''
end

return kg_simulate
