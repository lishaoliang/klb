--[[
-- Copyright(c) 2020, LGPL All Rights Reserved
-- @brief   GUI接口
-- @version 0.1
--]]

local kgui = {}




-- @brief 设置默认全局CSS参数
-- @param [in] [任意]...			设置参数
-- @return [number(int)] 	0.成功; 非0.失败
-- @note 注意必须在控件创建之前, 才能在初始化控件时生效;
kgui.set_default_css = function (...)
	return 0
end


-- @brief 获取默认全局CSS参数
-- @param [in] [任意]...			获取参数
-- @return [任意]...				值
kgui.get_default_css = function (...)
	return ...
end


-- @brief 加载图片
-- @param [in] key[string]			关键字
-- @param [in] path[string]			图片路径
-- @return [number(int)] 	0.成功; 非0.失败
kgui.load_image = function (key, path)
	return 0
end

-- @brief 添加窗口
-- @param [in] t[string]			窗口/控件类型: eg. "kbutton"
-- @param [in] path[string]			窗口路径名: eg. "/home/btn1"
-- @param [in] x[number(int)]		相对父窗口x坐标
-- @param [in] y[number(int)]		相对父窗口y坐标
-- @param [in] w[number(int)]		宽
-- @param [in] h[number(int)]		高
-- @return [number(int)] 	0.成功; 非0.失败
kgui.append = function (t, path, x, y, w, h)
	return 0
end


-- @brief 移除窗口
-- @return 无
kgui.remove = function (path)
	return
end


-- @brief 清理所有窗口
-- @return int 0.成功; 非0.失败(错误码)
-- @note 当修改分辨率等需要将所有UI清理掉时
--     清理内容:
--       1. klb_gui_append 添加的所有窗口
--       2. klb_gui_load_image 加载的所有图片资源
--       3. klb_gui_push_shwnd 添加的所有共享窗口
--     不清理内容
--       a. klb_gui_register 注册的控件类型
--       b. klb_gui_register_extension 注册的扩展
kgui.clear = function ()
	return 0
end

-- @brief 绑定消息(事件)处理函数
-- @param [in] path[string]			窗口路径名: eg. "/home/btn1"
-- @param [in] func[function]		lua函数
-- @return [number(int)] 	0.成功; 非0.失败
-- @note 
--  @brief 窗口(控件)处理函数
--  @param [in] obj[lightuserdata]	klb_wnd_t*指针
--  @param [in] msg[number(int)]	消息/事件
--  @param [in] x1[number(int)]		点1的x坐标
--  @param [in] y1[number(int)]		点1的y坐标
--  @param [in] x2[number(int)]		点2的x坐标
--  @param [in] y2[number(int)]		点2的y坐标
--  @param [in] lparam[number(int)]	参数1
--  @param [in] wparam[number(int)]	参数2
--  @return 0. 继续执行
--  		-1. 消息终止
--  local func = function (obj, msg, x1, y1, x2, y2, lparam, wparam)
--		...
--		return 0
--  end
kgui.bind_command = function (path, func)
	return 0
end


-- @brief 触发控件(窗口)某个事件
-- @param [in] path[string]			窗口路径名: eg. "/home/btn1"
--  @param [in] msg[number(int)]	消息/事件
--  @param [in] x1[number(int)]		[可选]点1的x1坐标
--  @param [in] y1[number(int)]		[可选]点1的y1坐标
--  @param [in] x2[number(int)]		[可选]点2的x2坐标
--  @param [in] y2[number(int)]		[可选]点2的y2坐标
--  @param [in] lparam[number(int)]	[可选]参数1
--  @param [in] wparam[number(int)]	[可选]参数2
-- @return [number(int)] 	0.成功; -1.失败
kgui.on_control_and_command = function (path, msg, x1, x2, y1, y2, lparam, wparam)
	return 0
end


-- @brief 向窗口(控件)设置数据: 样式\显示\状态等等
-- @param [in] path[string]			窗口路径(类unix): eg."/home/btn1"
-- @param [in] [任意]...			设置参数
-- @return [number(int)] 	0.成功; 非0.失败
kgui.set = function (path, ...)
	return 0
end


-- @brief 向窗口(控件)获取数据: 样式\显示\状态等等
-- @param [in] path[string]			窗口路径(类unix): eg."/home/btn1"
-- @param [in] [任意]...			获取参数
-- @return [任意]...				值
kgui.get = function (path, ...)
	return ...
end


-- @brief 以model方式的显示一个对话框
-- @param [in] all[boolean]			是否关闭全部popup: 默认true
-- @param [in] path[string]			窗口路径(类unix): 默认nil
-- @return [number(int)] 	0.成功; 非0.失败
kgui.model = function (path)
	return 0
end


-- @brief 结束一个model方式的对话框
-- @param [in] all[boolean]			是否关闭全部popup: 默认true
-- @param [in] path[string]			窗口路径(类unix): 默认nil
-- @return [number(int)] 	0.成功; 非0.失败
kgui.model_end = function (all, path)
	return 0
end

-- @brief 以popup方式显示一个菜单
-- @param [in] path[string]			窗口路径(类unix): eg."/menu"
-- @return [number(int)] 	0.成功; 非0.失败
kgui.popup = function (path)
	return 0
end


-- @brief 结束一个popup方式的菜单
-- @param [in] all[boolean]			是否关闭全部popup: 默认true
-- @return [number(int)] 	0.成功; 非0.失败
kgui.popup_end = function (all)
	return 0
end


-- @brief 以messagebox方式弹出一个消息提示框
-- @param [in] path[string]			窗口路径(类unix): eg."/messagebox"
-- @return [number(int)] 	0.成功; 非0.失败
kgui.messagebox = function (path)
	return 0
end


-- @brief 关闭messagebox消息提示框
-- @return [number(int)] 	0.成功; 非0.失败
kgui.messagebox_end = function ()
	return 0
end


-- @brief 显示或隐藏窗口
-- @param [in] path[string]			窗口路径(类unix): eg."/home/btn1"
-- @param [in] show[boolean]		true.显示; false.隐藏
-- @return [number(int)] 	0.成功; 非0.失败
kgui.show = function (path, show)
	return 0
end


-- @brief 移动窗口位置
-- @param [in] path[string]			窗口路径(类unix): eg."/home/btn1"
-- @param [in] x[number(int)]		相对父窗口x坐标
-- @param [in] y[number(int)]		相对父窗口y坐标
-- @return [number(int)] 	0.成功; 非0.失败
kgui.move = function (path, x, y)
	return 0
end


-- @brief 修改窗口大小
-- @param [in] path[string]			窗口路径(类unix): eg."/home/btn1"
-- @param [in] w[number(int)]		宽
-- @param [in] h[number(int)]		高
-- @return [number(int)] 	0.成功; 非0.失败
kgui.resize = function (path, w, h)
	return 0
end


-- @brief 获取主窗口(画布)的宽高
-- @return [number(int)] 	宽
--			[number(int)] 	高
kgui.wh = function ()
	return 1280, 720
end


return kgui
