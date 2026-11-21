--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   ktls.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  C ktls
--   \n require("ktls")
--   \n C导出文件: ./klb/src_c/klua/klua_net/klua_ktls.c
--   \n mbedtls 自签 RSA PEM
-- @version 0.1
--]]

local ktls = {}


-- @brief 是否可用 (编入 mbedtls / 非 no-ssl)
-- @return [boolean]
ktls.available = function ()
	return false
end


-- @brief 生成自签证书与私钥 PEM
-- @param [in] opts[table]		[可选] bits/days/subject/san
-- @return cert_pem[string]|nil
-- @return key_pem[string]|err[string]
-- @note 默认 subject=`O=klb,OU=alua,CN=alua-serve`
ktls.selfsign = function (opts)
	return nil, "stub"
end


-- @brief 生成自签并写入文件
-- @param [in] opts[table]		须含 path_cert / path_key; 其余同 selfsign
-- @return ok[boolean]
-- @return err[string|nil]
ktls.selfsign_to_files = function (opts)
	return false, "stub"
end


-- @brief 解析 PEM 文本有效期
-- @param [in] pem[string]		证书 PEM
-- @return info[table]|nil		{ not_after, remaining_sec, expired }
-- @return err[string|nil]
ktls.not_after = function (pem)
	return nil, "stub"
end


-- @brief 解析证书文件有效期
-- @param [in] path[string]		PEM 路径
-- @return info[table]|nil		同 not_after
-- @return err[string|nil]
ktls.not_after_file = function (path)
	return nil, "stub"
end


return ktls
