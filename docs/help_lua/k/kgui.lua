--[[
-- Copyright(c) 2020, LGPL All Rights Reserved
-- @brief   GUI接口
-- @author  李绍良
-- @version 0.1
--]]

local kgui = {}


-- @brief 初始化GUI
-- @param [in] json[string]				JSON参数
-- @return 无
kgui.init = function (json)
	return
end


-- @brief 创建线程
-- @param [in] name[string]				线程名称
-- @param [in] entry[string]			加载入口脚本; eg.'xxx.xxx'
-- @param [in] preload[lightuserdata]	预加载库C函数; 决定UI启动后使用那些预加载库
-- @param [in] loader[lightuserdata]	Lua脚本加载函数; 从自定义函数加载Lua脚本; 默认使用fopen从文件加载
-- @return [boolean] 	true.成功; false.失败
-- 			[string]	错误字符串
kgui.start = function (name, entry, preload, loader)
	return true, ''
end


-- @brief 停止GUI
-- @return 无
kgui.stop = function ()
	return
end


-- @brief 退出GUI
-- @return 无
kgui.quit = function ()
	return
end


-- @brief 附加到显存画布
-- @param [in] ptr[lightuserdata]	显存画布(C指针)
-- @return 无
kgui.attach_canvas = function (ptr)
	return 
end


kgui.get_msg_callback = function ()
	return ptr
end


-- @brief 放入消息
-- @param [in] msg[number(int)]		消息
-- @param [in] x1[number(int)]		点1的x坐标
-- @param [in] y1[number(int)]		点1的y坐标
-- @param [in] x2[number(int)]		点2的x坐标
-- @param [in] y2[number(int)]		点2的y坐标
-- @param [in] lparam[number(int)]	参数1
-- @param [in] wparam[number(int)]	参数2
-- @return [number(int)] 	0.成功; 非0.失败
kgui.push = function (msg, x1, y1, x2, y2, lparam, wparam)
	return 0
end


-- @brief 添加窗口
-- @param [in] t[string]			窗口/控件类型
-- @param [in] path_name[string]	窗口路径名: eg. "/home/btn1"
-- @param [in] x[number(int)]		相对父窗口x坐标
-- @param [in] y[number(int)]		相对父窗口y坐标
-- @param [in] w[number(int)]		相对父窗口宽
-- @param [in] h[number(int)]		相对父窗口高
-- @return [number(int)] 	0.成功; 非0.失败
kgui.append = function (t, path_name, x, y, w, h)
	return 0
end

-- @brief 移除窗口
kgui.remove = function (path_name)
	
end


-- @brief 绑定消息(事件)处理函数
-- @param [in] path_name[string]	窗口路径名: eg. "/home/btn1"
-- @param [in] func[function]		lua函数
-- @return [number(int)] 	0.成功; 非0.失败
-- @note 
--  @brief 窗口(控件)处理函数
--  @param [in] obj[lightuserdata]	klb_wnd_t*指针
--  @param [in] msg[number(int)]	GUI消息
--  @param [in] x1[number(int)]		点1的x坐标
--  @param [in] y1[number(int)]		点1的y坐标
--  @param [in] x2[number(int)]		点2的x坐标
--  @param [in] y2[number(int)]		点2的y坐标
--  @param [in] lparam[number(int)]	参数1
--  @param [in] wparam[number(int)]	参数2
--  @return 0. 消息终止,不再"冒泡"
--  		msg. 任然以msg"冒泡"
--  		非0. 转换为其他消息"冒泡"
--  local func = function (obj, msg, x1, y1, x2, y2, lparam, wparam)
--		...
--		return 0
--  end
kgui.bind_command = function (path_name, func)
	return 0
end


-- @brief 向窗口(控件)设置数据: 样式\显示\状态等等
-- @param [in] path_name[string]	窗口路径(类unix): eg."/home/btn1"
-- @param [in] json[string]			JSON数据
-- @return [number(int)] 	0.成功; 非0.失败
kgui.set = function (path_name, json)
	return 0
end


-- @brief 向窗口(控件)获取数据: 样式\显示\状态等等
-- @param [in] path_name[string]	窗口路径(类unix): eg."/home/btn1"
-- @param [in] json[string]			JSON数据
-- @return [string] JSON回复
kgui.get = function (path_name, json)
	return '{}'
end

return kgui
