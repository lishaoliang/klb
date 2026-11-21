--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua 错误码门面 (聚合 core/packages; 对外统一入口)
-- @stability stable
-- @impl evolving
-- @attention
--   \n @stability stable: AI 禁止擅自添加/删除/变更对外接口 (含导出名, 参数, 返回与已文档含义); 须用户点名
--   \n @impl evolving: 内部可重构, 只要不破坏本文件导出签名与可观察语义
-- @note   分表挂载: M.core / M.packages; 并扁平导出域表 (M.serve / M.proxy …) 便于既有调用
--   \n 总窗 [0x10000+4000, 0x20000); core 前 500; 包 SEG=100
--   \n 完整码名 M.strerror -> alua.core.<域>.<码> / alua.packages.<包>.<码> / alua.OK / alua.UNKNOWN
--   \n require("klbagent.code")
-- @history 修改历史
--  \n [2026-11] 创建文件; 迁入原 core/code.lua 与 packages/code.lua
--]]


local M = {}


M.core = require("klbagent.code.core")
M.packages = require("klbagent.code.packages")

-- 别名 (与文件名一致)
M.corecode = M.core
M.pkgcode = M.packages

-- 全模块成功 / 非法哨兵
M.OK = 0
M.INVALID = -1

-- 总窗与 core 跨度 (真源在 core)
M.BASE = M.core.BASE
M.SPAN = M.core.SPAN
M.PKG_BASE = M.core.PKG_BASE


local _UNKNOWN = "alua.UNKNOWN"
local _OK_NAME = "alua.OK"

-- 扁平导出: core 域表 + 包表 (跳过元字段, 避免覆盖门面 API)
local _SKIP = {
	OK = true,
	INVALID = true,
	BASE = true,
	SPAN = true,
	SEG = true,
	PKG_BASE = true,
	PREFIX = true,
	strerror = true,
	str = true,
	make_error = true,
	packages = true,
	core = true,
	corecode = true,
	pkgcode = true,
}


local function _flat_export(src)
	for k, v in pairs(src) do
		if not _SKIP[k] and type(v) == "table" then
			M[k] = v
		end
	end
end


_flat_export(M.core)
_flat_export(M.packages)


-- @brief 按数值区间判域
-- @param [in]  code[number]	错误码
-- @return [string]			"ok" / "core" / "packages" / "unknown"
M.domain = function (code)
	if type(code) ~= "number" then
		return "unknown"
	end

	if 0 == code then
		return "ok"
	end

	if code >= M.BASE and code < M.BASE + M.SPAN then
		return "core"
	end

	if code >= M.PKG_BASE and code < 0x20000 then
		return "packages"
	end

	return "unknown"
end


-- @brief 码名 (完整串)
-- @param [in]  code[number]			码值
-- @return name[string]				`alua.core.<域>.<码名>` / `alua.packages.<包>.<码名>` / `alua.OK` / `alua.UNKNOWN`
-- @stability stable
-- @note 先 domain 按值判域, 再只查对应子表 (与 klbcore.klbcode 同形)
--   \n eg. code.strerror(code.serve.BAD_ARGV) -> "alua.core.serve.BAD_ARGV"
--   \n     code.strerror(code.proxy.BAD_REQUEST) -> "alua.packages.proxy.BAD_REQUEST"
--   \n     code.strerror(0) -> "alua.OK"
M.strerror = function (code)
	if type(code) ~= "number" then
		return _UNKNOWN
	end

	local dom = M.domain(code)

	if "ok" == dom then
		return _OK_NAME
	end

	if "core" == dom then
		return M.core.strerror(code)
	end

	if "packages" == dom then
		return M.packages.strerror(code)
	end

	return _UNKNOWN
end


-- @brief 码短名 (码名末段; 未知则 nil); 与 klbcode.str 同名
-- @param [in]  code[number]			码值
-- @return name[string|nil]			eg. "BAD_REQUEST" / "BAD_ARGV"
-- @stability stable
-- @note 完整码名用 strerror
M.str = function (code)
	local full = M.strerror(code)
	if full == _UNKNOWN then
		return nil
	end

	if full == _OK_NAME then
		return "OK"
	end

	return full:match("([^.]+)$")
end


-- @brief 构造错误表
-- @param [in]  code[number]			码值; 非数字取 M.INVALID
-- @param [in]  message[string]			描述; 空则取码短名
-- @return err[table]					{ code, message, name }
-- @stability stable
M.make_error = function (code, message)
	local c = code
	if type(c) ~= "number" then
		c = M.INVALID
	end

	local msg = message
	if type(msg) ~= "string" or msg == "" then
		msg = M.str(c) or "error"
	end

	return {
		code = c,
		message = msg,
		name = M.str(c),
	}
end


return M

-- end
