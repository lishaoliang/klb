--[[
-- Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   ksocks5.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  C ksocks5, SOCKS5 报文编解码 (纯函数)
--   \n require("ksocks5")
--   \n C导出文件: ./klb/src_packages/klbsocks5/socks5_klua/klbsocks5_klua.c
--   \n 不触网, 无协程要求; 仅拼装/解析 wire 字节
--   \n 失败统一为 nil, "描述串" (无数字错误码)
-- @version 0.1
--]]

local ksocks5 = {}


-- @brief 是否可用 (编译期已启用本包)
-- @return [boolean] 恒为 true
ksocks5.available = function ()
	return true
end


-- @brief 编码客户端问候 (VER/METHODS)
-- @param [in] with_auth[boolean]	[可选] true.带用户名口令; 默认 false
-- @return [string]	报文: 真为 "05 02 00 02"(4B); 假为 "05 01 00"(3B)
-- @return nil, "encode fail"
ksocks5.encode_greeting = function (with_auth)
	return ''
end


-- @brief 解析服务端方法选择
-- @param [in] data[string]	报文 (需 2B, VER 必须 0x05)
-- @return method[number(int)]	选中的方法
-- @return nil, "short"|"bad method select"
ksocks5.parse_method_select = function (data)
	return 0
end


-- @brief 编码用户名口令认证请求
-- @param [in] user[string]	用户名 (长度 <= 255)
-- @param [in] pass[string]	口令 (长度 <= 255)
-- @return [string]	报文
-- @return nil, "encode fail"
-- @note 内部缓冲 520B
ksocks5.encode_userpass = function (user, pass)
	return ''
end


-- @brief 解析用户名口令认证结果
-- @param [in] data[string]	报文 (需 2B, p[0] 必须 0x01)
-- @return status[number(int)]	0.成功
-- @return nil, "short"|"bad auth status"
ksocks5.parse_userpass_status = function (data)
	return 0
end


-- @brief 编码 CONNECT 请求
-- @param [in] host[string]	目标主机: 点分 IPv4 或域名
-- @param [in] port[number(int)]	目标端口 (钳到 [0,65535])
-- @return [string]	报文; IPv4 用 ATYP=0x01, 其余用 ATYP=0x03(域名)
-- @return nil, "encode fail"
ksocks5.encode_connect = function (host, port)
	return ''
end


-- @brief 编码方法选择 (服务端应答)
-- @param [in] method[number(int)]	方法号
-- @return [string]	报文: "05 method"(2B)
-- @return nil, "encode fail"
ksocks5.encode_method_select = function (method)
	return ''
end


-- @brief 解析客户端问候
-- @param [in] data[string]	报文
-- @return [table] methods[string], consumed[number(int)]
-- @return nil, "short"|"bad greeting"
ksocks5.parse_greeting = function (data)
	return { methods = '', consumed = 0 }
end


-- @brief 从客户端方法列表中选择方法
-- @param [in] methods[string]	方法字节串
-- @param [in] want_user[boolean]	[可选] true.只认用户名口令(0x02); 默认 false.只认 0x00
-- @return [number(int)]	方法号; 未命中为 0xFF
-- @note 不会失败
ksocks5.choose_method = function (methods, want_user)
	return 0xFF
end


-- @brief 解析用户名口令认证请求
-- @param [in] data[string]	报文
-- @return [table] user[string], pass[string], consumed[number(int)]
-- @return nil, "short"|"bad auth"
-- @note user/pass 缓冲各 256B
ksocks5.parse_userpass = function (data)
	return { user = '', pass = '', consumed = 0 }
end


-- @brief 编码用户名口令认证结果
-- @param [in] status[number(int)]	状态
-- @return [string]	报文: "01 status"(2B)
-- @return nil, "encode fail"
ksocks5.encode_userpass_status = function (status)
	return ''
end


-- @brief 解析 CONNECT 请求
-- @param [in] data[string]	报文
-- @return [table] cmd, atyp, host[string], port, consumed
-- @return nil, "short"|"bad request"
ksocks5.parse_request = function (data)
	return { cmd = 1, atyp = 1, host = '', port = 0, consumed = 0 }
end


-- @brief 解析应答
-- @param [in] data[string]	报文
-- @return [table] rep, atyp, host[string], port, consumed
-- @return nil, "short"|"bad reply"
ksocks5.parse_reply = function (data)
	return { rep = 0, atyp = 1, host = '', port = 0, consumed = 0 }
end


-- @brief 编码应答
-- @param [in] rep[number(int)]	应答码 REP
-- @return [string]	报文: "05 rep 00 01 00 00 00 00 00 00"(10B, BND 置 0.0.0.0:0)
-- @return nil, "encode fail"
ksocks5.encode_reply = function (rep)
	return ''
end


-- 常量 (klb_socks5_wire.h)
ksocks5.VER = 0x05				-- SOCKS 版本
ksocks5.METHOD_NONE = 0x00		-- 无需认证
ksocks5.METHOD_USERPASS = 0x02	-- 用户名口令认证
ksocks5.METHOD_NO_ACCEPT = 0xFF	-- 无可接受方法
ksocks5.CMD_CONNECT = 0x01		-- CONNECT 命令
ksocks5.REP_CMD_NOT_SUPP = 0x07	-- 命令不支持
ksocks5.REP_ATYP_NOT_SUPP = 0x08	-- 地址类型不支持

-- @note 未导出常量(需硬编码): AUTH_VER=0x01, ATYP_IPV4=0x01, ATYP_DOMAIN=0x03,
--   \n ATYP_IPV6=0x04, REP_SUCCEEDED=0x00, HOST_MAX=255, ENCODE_MAX=520


return ksocks5
