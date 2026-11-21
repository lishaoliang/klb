--[[
-- Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   kuart.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  C kuart, 串口 (Windows / POSIX)
--   \n require("kuart")
--   \n C导出文件: ./klb/src_c/klua/klua_platform/klua_kuart.c
--   \n 失败形状为字符串描述: nil, "err"
-- @version 0.1
--]]

local kuart = {}


-- @brief 是否可用与后端
-- @return [boolean] true
-- @return backend[string]	"windows" / "posix"
kuart.available = function ()
	return true, "posix"
end


-- @brief 列举候选串口
-- @param [in] max_entries[number(int)]	[可选] 上限; 默认内部上限, 钳到 [1, 上限]
-- @param [in] include_legacy[boolean]	[可选] 是否含传统设备名; 默认 false
-- @return [table] 串口名数组
kuart.list = function (max_entries, include_legacy)
	return {}
end


-- @brief 打开串口
-- @param [in] path[string]	设备路径 (Win: "COM3"; POSIX: "/dev/ttyUSB0")
-- @param [in] opts[table]		[可选] baud(默认115200) / databits(8) / stop(1) / parity / flow
-- @return port[userdata]	串口对象
-- @return nil, err[string]	失败: "open fail"
-- @note 对象方法: close / write / read / flush
kuart.open = function (path, opts)
	local port = {}

	-- @brief 关闭串口 (幂等)
	-- @return [boolean] true
	-- @note __gc / __close 亦调此
	port.close = function (self)
		return true
	end

	-- @brief 写数据
	-- @param [in] data[string]	数据
	-- @return n[number(int)]	已写字节数
	-- @return nil, err[string]	失败: "closed" / "write fail"
	port.write = function (self, data)
		return 0
	end

	-- @brief 读数据
	-- @param [in] max_bytes[number(int)]	最大字节数
	-- @param [in] timeout_ms[number(int)]	[可选] 超时
	-- @return data[string]	数据; 暂无数据为空串 ""
	-- @return nil, err[string]	失败: "closed" / "bad max" / "select fail" / "read fail"
	port.read = function (self, max_bytes, timeout_ms)
		return ""
	end

	-- @brief 清空收发缓冲
	-- @return [boolean] true
	-- @return nil, err[string]	失败: "closed" / "flush fail"
	port.flush = function (self)
		return true
	end

	return port
end


return kuart
