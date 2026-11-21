--[[
-- Copyright(c) 2026, LGPL All Rights Reserved
-- @file   kcrypto.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  C kcrypto
--   \n require("kcrypto")
--   \n C导出文件: ./klb/src_c/klua/klua_util/klua_kcrypto.c
-- @version 0.1
--]]

local kcrypto = {}


-- @brief MD5 摘要 (16 字节 binary)
-- @param [in]  data[string]
-- @return [string] binary
kcrypto.md5 = function (data)
	return ''
end


-- @brief MD5 十六进制小写
-- @param [in]  data[string]
-- @return [string] 32 字符 hex
kcrypto.md5_hex = function (data)
	return '900150983cd24fb0d6963f7d28e17f72'
end


-- @brief SHA-1 摘要 (20 字节 binary)
-- @param [in]  data[string]
-- @return [string] binary
kcrypto.sha1 = function (data)
	return ''
end


-- @brief SHA-1 十六进制小写
-- @param [in]  data[string]
-- @return [string] hex
kcrypto.sha1_hex = function (data)
	return ''
end


-- @brief SHA-256 摘要 (32 字节 binary)
-- @param [in]  data[string]
-- @return [string] binary
kcrypto.sha256 = function (data)
	return ''
end


-- @brief SHA-256 十六进制小写
-- @param [in]  data[string]
-- @return [string] hex
kcrypto.sha256_hex = function (data)
	return ''
end


-- @brief HMAC-MD5
-- @param [in]  key[string]
-- @param [in]  data[string]
-- @return [string] binary
kcrypto.hmac_md5 = function (key, data)
	return ''
end


-- @brief HMAC-MD5 十六进制
-- @param [in]  key[string]
-- @param [in]  data[string]
-- @return [string] hex
kcrypto.hmac_md5_hex = function (key, data)
	return ''
end


-- @brief HMAC-SHA1
-- @param [in]  key[string]
-- @param [in]  data[string]
-- @return [string] binary
kcrypto.hmac_sha1 = function (key, data)
	return ''
end


-- @brief HMAC-SHA1 十六进制
-- @param [in]  key[string]
-- @param [in]  data[string]
-- @return [string] hex
kcrypto.hmac_sha1_hex = function (key, data)
	return ''
end


-- @brief HMAC-SHA256
-- @param [in]  key[string]
-- @param [in]  data[string]
-- @return [string] binary
kcrypto.hmac_sha256 = function (key, data)
	return ''
end


-- @brief HMAC-SHA256 十六进制
-- @param [in]  key[string]
-- @param [in]  data[string]
-- @return [string] hex
kcrypto.hmac_sha256_hex = function (key, data)
	return ''
end


-- @brief CRC16
-- @param [in]  data[string]
-- @param [in]  variant[string]	[可选] 默认 modbus; modbus|ansi_le|ansi|ccitt
-- @return [number(int)]
-- @note 默认 Modbus RTU; "123456789" -> 0x4B37
kcrypto.crc16 = function (data, variant)
	return 0x4B37
end


-- @brief CRC32
-- @param [in]  data[string]
-- @param [in]  variant[string]	[可选] 默认 ieee(=zlib); ieee|zlib|ieee_be
-- @return [number(int)]
-- @note 默认 IEEE LE zlib 风格; "123456789" -> 0xCBF43926
kcrypto.crc32 = function (data, variant)
	return 0xCBF43926
end


-- @brief Base64 编码
-- @param [in]  data[string]
-- @return [string]
kcrypto.base64_encode = function (data)
	return ''
end


-- @brief Base64 解码
-- @param [in]  s[string]
-- @return [string] 成功
-- @return nil, err[string] 失败
kcrypto.base64_decode = function (s)
	return ''
end


-- @brief 流式上下文工厂
-- @param [in]  typ[string]		md5|sha1|sha256|hmac_md5|hmac_sha1|hmac_sha256|crc16|crc32
-- @param [in]  arg2[string]	[可选] hmac 的 key; 或 crc variant
-- @return [userdata] 句柄 (update/final/final_hex/close)
-- @note eg. local h = kcrypto.new("md5"); h:update(chunk); local dig = h:final_hex()
kcrypto.new = function (typ, arg2)
	local h = {}

	-- @brief 追加数据; 可链式
	-- @param [in]  data[string]
	-- @return self
	h.update = function (self, data)
		return self
	end

	-- @brief 结束; 摘要 binary; crc 为 integer
	-- @return [string|number(int)]
	h.final = function (self)
		return ''
	end

	-- @brief 结束为 hex; 仅摘要/HMAC
	-- @return [string]
	h.final_hex = function (self)
		return ''
	end

	-- @brief 释放; 同 __gc / __close
	-- @return 无
	h.close = function (self)
		return
	end

	return h
end


return kcrypto
