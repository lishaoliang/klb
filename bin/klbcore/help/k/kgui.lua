--[[
-- Copyright(c) 2020, LGPL All Rights Reserved
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  GUI接口
--   \n require("kgui")
--   \n C导出文件: ./klb/src_c/klua/klua_base/klua_kgui.c
-- @version 0.1
--]]

local kgui = {}


-- @brief 设置默认CSS参数
-- @param [in] [任意]...			设置参数
-- @return [number(int)] 	0.成功; 非0.失败
-- @note 注意必须在控件创建之前, 才能在初始化控件时生效;
kgui.set_default_css = function (...)
	return 0
end


-- @brief 获取默认CSS参数
-- @param [in] [任意]...			获取参数
-- @return [任意]...				值
kgui.get_default_css = function (...)
	return ...
end


-- @brief 设置控件全局CSS参数
-- @param [in] t[string]			窗口/控件类型: eg. "kbutton"
-- @param [in] [任意]...			设置参数
-- @return [number(int)] 	0.成功; 非0.失败
kgui.set_global_css = function (t, ...)
	return 0
end


-- @brief 获取控件全局CSS参数
-- @param [in] t[string]			窗口/控件类型: eg. "kbutton"
-- @param [in] [任意]...			获取参数
-- @return [任意]...				值
kgui.get_global_css = function (t, ...)
	return ...
end


-- @brief 获取控件全局CSS参数
-- @param [in] t[string]			窗口/控件类型: eg. "kbutton"
-- @return [boolean] 	true.有全局CSS; false.无全局CSS
kgui.has_global_css = function (t)
	return true
end


-- @brief 设置共享窗口CSS
-- @param [in] path[string]			共享窗口路径: eg. "/klbui/combomenu"
-- @param [in] [任意]...			设置参数
-- @return [number(int)] 	0.成功; 非0.失败
kgui.set_shwnd_css = function (path, ...)
	return 0
end


-- @brief 获取共享窗口CSS
-- @param [in] path[string]			共享窗口路径: eg. "/klbui/combomenu"
-- @param [in] [任意]...			获取参数
-- @return [任意]...				值
kgui.get_shwnd_css = function (path, ...)
	return ...
end


-- @brief 是否多画布图层模式 (modal/popup/messagebox 分画布)
-- @return [boolean] true 多画布; false 单画布
kgui.is_multi_canvas_layer = function ()
	return false
end


-- @brief 加载图片
-- @param [in] key[string]			关键字
-- @param [in] path[string]			图片路径
-- @return [number(int)] 	0.成功; 非0.失败
kgui.load_image = function (key, path)
	return 0
end


-- @brief 清空消息事件队列
-- @return 无
kgui.clear_msg = function ()
	return
end


-- @brief 获取窗口 kwnd 操作接口
-- @param [in] path[string]			窗口路径: eg. "/home/btn1"
-- @return [userdata] kwnd; 失败 nil
kgui.get_kwnd = function (path)
	return nil
end


-- @brief 添加窗口
-- @param [in] t[string]			窗口/控件类型: eg. "kbutton"
-- @param [in] path[string]			窗口路径名: eg. "/home/btn1"
-- @param [in] x[number(int)]		相对父窗口x坐标
-- @param [in] y[number(int)]		相对父窗口y坐标
-- @param [in] w[number(int)]		宽
-- @param [in] h[number(int)]		高
-- @param [in] style[number(int)]	[可选] 样式, 默认 0
-- @return [number(int)] 	0.成功; 非0.失败
kgui.append = function (t, path, x, y, w, h, style)
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


-- @brief 调用(触发)控件(窗口)某个事件
-- @param [in] path[string]			窗口路径名: eg. "/home/btn1"
--  @param [in] msg[number(int)]	消息/事件
--  @param [in] x1[number(int)]		[可选]点1的x1坐标
--  @param [in] y1[number(int)]		[可选]点1的y1坐标
--  @param [in] x2[number(int)]		[可选]点2的x2坐标
--  @param [in] y2[number(int)]		[可选]点2的y2坐标
--  @param [in] lparam[number(int)]	[可选]参数1
--  @param [in] wparam[number(int)]	[可选]参数2
-- @return [number(int)] 	0.成功; -1.失败
kgui.call_control_and_command = function (path, msg, x1, x2, y1, y2, lparam, wparam)
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


-- @brief 以modal方式的显示一个对话框
-- @param [in] all[boolean]			是否关闭全部modal: 默认true
-- @param [in] path[string]			窗口路径(类unix): 默认nil
-- @return [number(int)] 	0.成功; 非0.失败
kgui.modal = function (path)
	return 0
end


-- @brief 结束一个modal方式的对话框
-- @param [in] all[boolean]			是否关闭全部modal: 默认true
-- @param [in] path[string]			窗口路径(类unix): 默认nil
-- @return [number(int)] 	0.成功; 非0.失败
kgui.modal_end = function (all, path)
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


-- @brief 当前 modal 窗口数量
-- @return [number(int)] 数量
kgui.modal_num = function ()
	return 0
end


-- @brief 当前 popup 窗口数量
-- @return [number(int)] 数量
kgui.popup_num = function ()
	return 0
end


-- @brief 当前 messagebox 窗口数量
-- @return [number(int)] 数量
kgui.messagebox_num = function ()
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


-- @brief 获取窗口位置
-- @param [in] path[string]				窗口路径(类unix): eg."/home/btn1"
-- @param [in] is_in_canvas[boolean		[可选]相对于画布; 默认true. 相对于画布; false.相对于父窗口
-- @return [table] 	窗口位置
kgui.wndpos = function (path, is_in_canvas)
	return {
		x = 0,
		y = 0,
		w = 32,
		h = 32,
	}
end


-- @brief 获取窗口(控件)建议宽度
-- @param [in] path[string]				窗口路径(类unix): eg."/home/btn1"
-- @return [number(int)] 	窗口(控件)建议宽度
kgui.suggestw = function (path)
	return 100
end


-- @brief 获取窗口(控件)建议高度
-- @param [in] path[string]				窗口路径(类unix): eg."/home/btn1"
-- @return [number(int)] 	窗口(控件)建议高度
kgui.suggesth = function (path)
	return 100
end


-- @brief 窗口刷新(仅标记, UI框架决定刷新时机)
-- @return 无
kgui.refresh = function ()
	return
end


-- @brief 更新全局 tip
-- @param [in] tip[string]			[可选] tip 文本; 省略则刷新当前 tip
-- @return 无
kgui.update_tip = function (tip)
	return
end


-- @brief 获取主窗口(画布)的宽高
-- @return [number(int)] 	宽
--			[number(int)] 	高
kgui.wh = function ()
	return 1280, 720
end


-- @brief 获取不含特殊标记的事件类型
-- @param [in] e[number(int)]			含有特殊标记的事件类型, eg. 0x40000406
-- @return [number(int)] 	事件类型
kgui.to_event = function (e)
	-- eg. mousemove
	-- 带标记可能为: 0x40000406
	-- 转换后为: 0x406
	return 0x406
end


-- @brief 获取是否含有 b1 比特位标记
-- @param [in] e[number(int)]			含有特殊标记的事件类型, eg. 0x40000406
-- @return [boolean 	事件类型
kgui.b1_event = function (e)
	-- 事件是否含有: KLBUI_event_bit1(0x40000000) 比特位标记
	return false
end


-- @brief 获取是否含有 b2 比特位标记
-- @param [in] e[number(int)]			含有特殊标记的事件类型, eg. 0x20000406
-- @return [boolean 	事件类型
kgui.b2_event = function (e)
	-- 事件是否含有: KLBUI_event_bit2(0x20000000) 比特位标记
	return false
end


-- @brief 获取是否含有 b3 比特位标记
-- @param [in] e[number(int)]			含有特殊标记的事件类型, eg. 0x10000406
-- @return [boolean 	事件类型
kgui.b3_event = function (e)
	-- 事件是否含有: KLBUI_event_bit3(0x10000000) 比特位标记
	return false
end



-- @brief 设置 聚焦延时消息 的时间(单位毫秒ms, 默认600, 范围[0, ~])
-- @return 无
-- @note 指鼠标聚焦一段时间后, 配合控件样式(KLB_WND_STYLE_FOCUS_DELAY), 会产生一个 focusdelay(KLBUI_focusdelay = 0x710)事件
-- 		同时影响 tip 弹出的时间
kgui.focusdelay = function (tc)
	return
end


-- @brief 获取系统当前 系统滴答数
-- @return [number(int)] 	系统滴答数
kgui.tick_count = function ()
	return 1000
end


-- @brief 设置 内部控件定时器运行间隔 (单位毫秒ms, 默认500, 范围[10, ~])
-- @return 无
-- 		调小,精度略微提高, 降低框架性能
--		调大,精度略微降低, 提高框架性能
kgui.ticker_interval = function (interval)
	return
end


-- @brief 绑定/解绑用户图层对应窗口
-- @param [in] path[string]			[可选] 窗口路径; 省略则解绑
-- @return [number(int)] 0 成功; 非 0 失败
kgui.bind_udatalayer = function (path)
	return 0
end


-- @brief 移动用户图层
-- @param [in] x[number(int)]		[可选] x 坐标, 默认 0
-- @param [in] y[number(int)]		[可选] y 坐标, 默认 0
-- @return 无
kgui.move_udatalayer = function (x, y)
	return
end


-- @brief 显示/隐藏用户图层
-- @param [in] show[boolean]		[可选] 默认 false
-- @return 无
kgui.show_udatalayer = function (show)
	return
end


-- @brief 绑定/解绑等待图层对应窗口
-- @param [in] path[string]			[可选] 窗口路径; 省略则解绑
-- @return [number(int)] 0 成功; 非 0 失败
kgui.bind_waitlayer = function (path)
	return 0
end


-- @brief 移动等待图层
-- @param [in] x[number(int)]		[可选] x 坐标, 默认 0
-- @param [in] y[number(int)]		[可选] y 坐标, 默认 0
-- @return 无
kgui.move_waitlayer = function (x, y)
	return
end


-- @brief 开启/关闭等待图层
-- @param [in] on[boolean]			[可选] 默认 false
-- @return 无
kgui.wait = function (on)
	return
end


-- @brief 设置/获取是否完整绘制事件流程
-- @param [in] is_full[boolean]		[可选] 设置时传入; 省略则只读当前值
-- @return [boolean] 是否完整绘制
kgui.redraw_full_event = function (is_full)
	return false
end


return kgui
