--[[
-- Copyright(c) 2020, LGPL All Rights Reserved
-- @brief   GUI接口
-- @version 0.1
--]]

local kgui = {}


-- @brief 放入消息
-- @param [in] msg[number(int)]		消息
-- @param [in] x1[number(int)]		点1的x坐标
-- @param [in] y1[number(int)]		点1的y坐标
-- @param [in] x2[number(int)]		点2的x坐标
-- @param [in] y2[number(int)]		点2的y坐标
-- @param [in] lparam[number(int)]	参数1
-- @param [in] wparam[number(int)]	参数2
-- @return [number(int)] 	0.成功; 非0.失败
kgui.push_msg = function (msg, x1, y1, x2, y2, lparam, wparam)
	return 0
end


-- @brief 加载图片
-- @param [in] key[string]			关键字
-- @param [in] path[string]			图片路径
-- @return [number(int)] 	0.成功; 非0.失败
kgui.load_image = function (key, path)
	return 0
end

-- @brief 添加窗口
-- @param [in] t[string]			窗口/控件类型
-- @param [in] path[string]			窗口路径名: eg. "/home/btn1"
-- @param [in] x[number(int)]		相对父窗口x坐标
-- @param [in] y[number(int)]		相对父窗口y坐标
-- @param [in] w[number(int)]		相对父窗口宽
-- @param [in] h[number(int)]		相对父窗口高
-- @return [number(int)] 	0.成功; 非0.失败
kgui.append = function (t, path, x, y, w, h)
	return 0
end


-- @brief 移除窗口
-- @return 无
kgui.remove = function (path)
	return
end


-- @brief 绑定消息(事件)处理函数
-- @param [in] path[string]			窗口路径名: eg. "/home/btn1"
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
kgui.bind_command = function (path, func)
	return 0
end


-- @brief 向窗口(控件)设置数据: 样式\显示\状态等等
-- @param [in] path[string]			窗口路径(类unix): eg."/home/btn1"
-- @param [in] json[string]			JSON数据
-- @return [number(int)] 	0.成功; 非0.失败
kgui.set = function (path, json)
	return 0
end


-- @brief 向窗口(控件)获取数据: 样式\显示\状态等等
-- @param [in] path[string]			窗口路径(类unix): eg."/home/btn1"
-- @param [in] json[string]			JSON数据
-- @return [string] JSON回复
kgui.get = function (path, json)
	return '{}'
end


kgui.do_model = function (path)

end

kgui.end_model = function (path)

end

kgui.end_model_all = function ()
	
end


-- @brief 显示或隐藏窗口
-- @param [in] path[string]			窗口路径(类unix): eg."/home/btn1"
-- @param [in] show[boolean]		true.显示; false.隐藏
-- @return [number(int)] 	0.成功; 非0.失败
kgui.show = function (path, show)
	return 0
end

return kgui
