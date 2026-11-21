--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
--
-- @file   init.lua
-- @brief  klbstore 客户端 (HTTP REST)
-- @stability experimental
-- @impl evolving
-- @note   桩: 本文件仅契约; 全部接口回 nil, E_NOTIMPL
--   \n 协议前缀: /store/v1/... ; 下载 /store/blob/<group>/M00/xx/yy/<name>
--   \n 传输: klbcore.klbhttp client (kco 协程); 大文件须分片, 整包走 chunk.lua
--   \n 鉴权: token.lua 签发 / 校验; 失败 E_AUTH
--]]


local klbcode = require("klbcore.klbcode")
local base = klbcode.base


local mod = {}


-- @brief 创建客户端
-- @param [in] cfg[table]			{ endpoint, token, timeout_ms }
-- @return [table] 客户端对象; 失败 nil, code
-- @stability experimental
mod.new = function (cfg)
	return nil, base.NOTIMPL
end


-- @brief 上传 (整包或分片会话)
-- @param [in] cl[table]			客户端对象
-- @param [in] data[string]			数据
-- @param [in] opts[table|nil]		{ group, ext, chunk_size, name }
-- @return [string] file_id; 失败 nil, code
-- @stability experimental
mod.upload = function (cl, data, opts)
	return nil, base.NOTIMPL
end


-- @brief 下载
-- @param [in] cl[table]			客户端对象
-- @param [in] file_id[string]		文件标识
-- @param [in] opts[table|nil]		{ offset, length }
-- @return [string] data; 失败 nil, code
-- @stability experimental
mod.download = function (cl, file_id, opts)
	return nil, base.NOTIMPL
end


-- @brief 删除
-- @param [in] cl[table]			客户端对象
-- @param [in] file_id[string]		文件标识
-- @return [boolean] true 成功; 失败 nil, code
-- @stability experimental
mod.del = function (cl, file_id)
	return nil, base.NOTIMPL
end


-- @brief 取元信息
-- @param [in] cl[table]			客户端对象
-- @param [in] file_id[string]		文件标识
-- @return [table] { size, mtime, sha256 }; 失败 nil, code
-- @stability experimental
mod.stat = function (cl, file_id)
	return nil, base.NOTIMPL
end


-- @brief 取下载 URL (交给 klbweb static)
-- @param [in] cl[table]			客户端对象
-- @param [in] file_id[string]		文件标识
-- @return [string] url; 失败 nil, code
-- @stability experimental
mod.url = function (cl, file_id)
	return nil, base.NOTIMPL
end


return mod
