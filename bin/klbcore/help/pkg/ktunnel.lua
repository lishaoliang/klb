--[[
-- Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   ktunnel.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  C ktunnel, 隧道握手/加密封包 (纯函数)
--   \n require("ktunnel")
--   \n C导出文件: ./klb/src_packages/klbtunnel/tunnel_klua/klbtunnel_klua.c
--   \n 不触网, 无协程要求; 仅拼装/解帧与加解密
--   \n 帧头 6B: MAGIC0 MAGIC1 VER type payload_len_be16
-- @version 0.1
--]]

local ktunnel = {}


-- @brief 是否可用 (编译期已启用本包)
-- @return [boolean] 恒为 true
ktunnel.available = function ()
	return true
end


-- @brief 组一帧
-- @param [in] typ[number(int)]	帧类型 (T_*)
-- @param [in] payload[string]	[可选] 载荷; 上限 65535
-- @return [string]	帧
-- @return nil, "payload too large"|"encode fail"
ktunnel.encode = function (typ, payload)
	return ''
end


-- @brief 增量解帧
-- @param [in] buf[string]	[可选] 已收字节; 默认 ""
-- @return ok[boolean]	true.解出一帧; false.数据不足或帧坏
-- @return frame[table]	(type, ver, payload)
-- @return rest[string]	剩余未消费字节 (失败时为原 buf)
-- @note 失败第二值为 "short"(可继续收) 或 "bad magic"
ktunnel.try_decode = function (buf)
	return false, "short", buf
end


-- @brief 组 HELLO 帧
-- @param [in] token[string]	[可选] 令牌; 超 65535 截断
-- @return [string]	帧
-- @return nil, "encode fail"
ktunnel.encode_hello = function (token)
	return ''
end


-- @brief 组 CONNECT 帧
-- @param [in] host[string]	目标主机 (截断到 255)
-- @param [in] port[number(int)]	目标端口 (钳到 [0,65535])
-- @return [string]	帧
-- @return nil, "encode fail"
ktunnel.encode_connect = function (host, port)
	return ''
end


-- @brief 解析 CONNECT 载荷
-- @param [in] payload[string]	载荷
-- @return host[string], port[number(int)], nil
-- @return nil, nil, "bad connect payload"
ktunnel.parse_connect_payload = function (payload)
	return '', 0
end


-- @brief SHA-256
-- @param [in] data[string]	数据
-- @return [string]	摘要 (32B)
ktunnel.sha256 = function (data)
	return ''
end


-- @brief 派生会话密钥
-- @param [in] psk[string]	预共享密钥
-- @param [in] nc[string]	客户端 nonce
-- @param [in] ns[string]	服务端 nonce
-- @return [string]	会话密钥 (32B)
-- @note SHA256("alua-tunnel-sess|" + psk + "|" + nc + "|" + ns); 总长超 512 时截断 psk
ktunnel.derive_session_key = function (psk, nc, ns)
	return ''
end


-- @brief 加密并加认证标签
-- @param [in] key[string]	会话密钥 (>= 32B)
-- @param [in] seq[number(int)]	序号
-- @param [in] plain[string]	[可选] 明文; 默认 ""
-- @return [string]	seq_be32(4) + 密文 + tag(16)
-- @return nil, "bad key"|"seal fail"
ktunnel.seal = function (key, seq, plain)
	return ''
end


-- @brief 校验标签并解密
-- @param [in] key[string]	会话密钥 (>= 32B)
-- @param [in] blob[string]	seal 产物 (>= 20B)
-- @return plain[string], seq[number(int)]
-- @return nil, "bad key"|"short seal"|"open fail"
ktunnel.open = function (key, blob)
	return '', 0
end


-- @brief 生成随机 nonce
-- @param [in] len[number(int)]	[可选] 长度 1..64; 默认 16
-- @return [string]	nonce
-- @return nil, "bad nonce len"|"nonce fail"
-- @note 弱熵源, 非 CSPRNG; 仅供握手 nonce 使用
ktunnel.random_nonce = function (len)
	return ''
end


-- @brief 组 HELLO(加密标记) 帧
-- @param [in] token[string]	[可选] 令牌; 默认 ""
-- @param [in] nonce_c[string]	[可选] 客户端 nonce, 必须恰好 16B; 缺省随机
-- @return frame[string], nonce_c[string]
-- @return nil, "nonce fail"|"bad nonce"|"encode fail"
ktunnel.encode_hello_crypto = function (token, nonce_c)
	return '', ''
end


-- @brief 解析 HELLO 载荷
-- @param [in] payload[string]	[可选] 载荷; 默认 ""
-- @return [table] crypto[boolean], token[string], nonce_c[string|nil]
-- @return nil, "token too long"|"short hello crypto"
ktunnel.parse_hello_payload = function (payload)
	return { crypto = false, token = '', nonce_c = nil }
end


-- @brief 组 HELLO_OK(加密标记) 帧
-- @param [in] nonce_s[string]	[可选] 服务端 nonce, 缺省随机 16B
-- @return frame[string], nonce_s[string]
-- @return nil, "nonce fail"|"bad nonce"|"encode fail"
ktunnel.encode_hello_ok_crypto = function (nonce_s)
	return '', ''
end


-- @brief 解析 HELLO_OK 载荷
-- @param [in] payload[string]	[可选] 载荷; 默认 ""
-- @return [table] crypto[boolean], nonce_s[string|nil]
-- @note 永不失败
ktunnel.parse_hello_ok_payload = function (payload)
	return { crypto = false, nonce_s = nil }
end


-- 常量 (klb_tunnel_wire.h / crypto.h)
ktunnel.MAGIC0 = 0xA7
ktunnel.MAGIC1 = 0x01
ktunnel.VER = 0x01
ktunnel.T_HELLO = 0x01
ktunnel.T_HELLO_OK = 0x02
ktunnel.T_HELLO_NAK = 0x03
ktunnel.T_CONNECT = 0x10
ktunnel.T_CONNECT_OK = 0x11
ktunnel.T_CONNECT_NAK = 0x12
ktunnel.T_DATA = 0x20
ktunnel.aead = true				-- 是否启用自定义 AEAD (EtM)
ktunnel.FLAG_CRYPTO = 0x01		-- HELLO/HELLO_OK 的加密标记位
ktunnel.NONCE_LEN = 16			-- nonce 字节数

-- @note 未导出常量(需硬编码): HDR_LEN=6, HOST_MAX=255, SHA256_LEN=32, TAG_LEN=16


return ktunnel
