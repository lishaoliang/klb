--[[
-- Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   ksrtp.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  C ksrtp, SRTP/SRTCP 保护 (单套件)
--   \n require("ksrtp")
--   \n C导出文件: ./klb/src_packages/klbsrtp/srtp_klua/klbsrtp_klua.c
--   \n 仅支持 KLB_SRTP_PROFILE_AES128_CM_HMAC_SHA1_80 (0)
-- @version 0.1
--]]

local ksrtp = {}


-- @brief 创建 SRTP 上下文
-- @param [in] profile[number(int)]	[可选] 保护套件; 默认 0; 其它值失败
-- @return ctx[userdata]	上下文对象
-- @return nil, code[number(int)]	失败; 描述串用 knetcode.strerror(code)
-- @note 对象方法: close / destroy / set_key / set_mki / protect / unprotect
--   \n eg. local s = ksrtp.create(0); s:set_key(key, salt)
ksrtp.create = function (profile)
	local srtp = {}

	-- @brief 关闭并销毁上下文 (幂等)
	-- @return 无
	-- @note destroy 为 close 别名; __gc / __close 亦调此
	srtp.close = function (self)
		return
	end

	-- @brief 设置主密钥与主盐
	-- @param [in] key[string]		主密钥
	-- @param [in] salt[string]		主盐
	-- @return [boolean] 成功
	-- @return nil, code[number(int)]	失败
	srtp.set_key = function (self, key, salt)
		return true
	end

	-- @brief 设置 MKI
	-- @param [in] mki[string|nil]	[可选] MKI 字节; nil/缺省表示清除
	-- @return [boolean] 成功
	-- @return nil, code[number(int)]	失败
	-- @note MKI 长度 0 ~ 16 (KLB_SRTP_MKI_MAX)
	srtp.set_mki = function (self, mki)
		return true
	end

	-- @brief 保护一个 RTP/RTCP 包
	-- @param [in] pkt[string]	明文包 (须自带完整 RTP/RTCP 头)
	-- @return [string]	SRTP/SRTCP 包
	-- @return nil, code[number(int)]	失败
	-- @note 内部缓冲 = 输入长度 + MKI 上限 + 32; 不改调用方数据
	srtp.protect = function (self, pkt)
		return ''
	end

	-- @brief 解保护一个 SRTP/SRTCP 包
	-- @param [in] pkt[string]	SRTP/SRTCP 包
	-- @return [string]	明文包
	-- @return nil, code[number(int)]	失败 (认证不过/长度不足)
	srtp.unprotect = function (self, pkt)
		return ''
	end

	return srtp
end


return ksrtp
