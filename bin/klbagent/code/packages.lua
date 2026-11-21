--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   packages.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua 可选包 (packages) 业务错误码 (按扩展分包)
-- @stability stable
-- @impl evolving
-- @attention
--   \n @stability stable: AI 禁止擅自添加/删除/变更对外接口 (含导出名, 参数, 返回与已文档含义); 须用户点名
--   \n @impl evolving: 内部可重构, 只要不破坏本文件导出签名与可观察语义
-- @note   各可选包使用本包独立错误码 (码名带包段, 便于追溯): alua.packages.<包名>.<码名>
--   \n 无跨包共享错误码 (仅 M.OK = 0 各包共用); 旧 core/errors 目录已删除, 其通用码按包拆分到本表
--   \n 分包: 每个可选包预留 100 号 (M.SEG), 从 M.BASE (0x10000+4000+500) 起, 与 code.core.PKG_BASE 对齐
--   \n 总窗上界 0x20000 (不含); 包加载器 (packages/init) 用 M.loader 段 (第 7 段)
--   \n 码名: alua.packages.<包名>.<码名> (例 alua.packages.proxy.BAD_ARGS); 成功码 alua.OK; 未登记码 alua.UNKNOWN
--   \n require("klbagent.code.packages"); 对外统一入口 require("klbagent.code")
--   \n 码表随包落地增补 (只增码, 已发布码值不改)
-- @history 修改历史
--  \n [2026-11] 创建文件; 落 6 个可选包错误码骨架 (alua_ui / alua_web / g2de_agent / mcp / proxy / svn_https)
--]]


local M = {}


-- 全包成功码; 各包成功均返回 0 (各包独立错误码见下方包表)
M.OK = 0

-- 码非法哨兵 (make_error 容错用; 非业务码, 不占包段; 未登记故 str 返回 nil)
M.INVALID = -1

-- 分包步长; 包基址 = M.BASE + 包序号 * SEG
M.SEG = 100

-- 包域基址 (与 code.core.PKG_BASE 一致; = 0x10000+4000+500)
M.BASE = 70036

-- 码名前缀; 完整码名 = PREFIX.<包名>.<码名>
M.PREFIX = "alua.packages."


-- 包加载器错误码 (packages/init; 第 7 段)
M.loader = {
	base = 70636,				-- 加载器基址
	BAD_REQUEST = 70636,		-- 包入口参数 / 模块返回值非法
	NOT_IMPLEMENTED = 70637,	-- 包未提供该入口 (run / register_tools)
	TOOL_NOT_FOUND = 70638,		-- 包目录 / 模块缺失
}


-- alua_ui 包错误码 (包基址 M.BASE+0*SEG)
M.alua_ui = {
	base = 70036,				-- 包基址
	TOOL_FAILED = 70036,		-- UI 启动 / 页面运行失败
}


-- alua_web 包错误码 (包基址 M.BASE+1*SEG)
M.alua_web = {
	base = 70136,				-- 包基址
	BAD_REQUEST = 70136,		-- HTTP 请求非法 (缺字段 / 报文体错)
	DENIED = 70137,				-- 会话 / CSRF 鉴权拒绝
	TOOL_NOT_FOUND = 70138,		-- 配置模块 / 资源未找到
	TOOL_FAILED = 70139,		-- 业务动作执行失败
}


-- g2de_agent 包错误码 (包基址 M.BASE+2*SEG)
M.g2de_agent = {
	base = 70236,				-- 包基址
	TOOL_FAILED = 70236,		-- 引擎侧工具注册 / 调用失败
}


-- mcp 包错误码 (包基址 M.BASE+3*SEG); 码随 packages/mcp 落地按语义补名
M.mcp = {
	base = 70336,				-- 包基址
}


-- proxy 包错误码 (包基址 M.BASE+4*SEG)
M.proxy = {
	base = 70436,				-- 包基址
	BAD_REQUEST = 70436,		-- 代理命令 / 配置参数非法
	TOOL_FAILED = 70437,		-- 代理检查 / 起停失败
}


-- svn_https 包错误码 (包基址 M.BASE+5*SEG)
M.svn_https = {
	base = 70536,				-- 包基址
	BAD_REQUEST = 70536,		-- 监听端口 / 参数非法
	TOOL_FAILED = 70537,		-- TLS / web 依赖缺失或起服失败
}


-- 码名反查表: 按包登记 (包前缀 + 表内名); 新增包/码只改上表, 勿另处硬写
local _UNKNOWN = "alua.UNKNOWN"
local _names = {}
local _domains = {
	{ "loader.", M.loader },
	{ "alua_ui.", M.alua_ui },
	{ "alua_web.", M.alua_web },
	{ "g2de_agent.", M.g2de_agent },
	{ "mcp.", M.mcp },
	{ "proxy.", M.proxy },
	{ "svn_https.", M.svn_https },
}


for i = 1, #_domains do
	local prefix = _domains[i][1]
	local codes = _domains[i][2]
	for name, code in pairs(codes) do
		-- base 为包基址常量, 非码
		if name ~= "base" then
			_names[code] = M.PREFIX .. prefix .. name
		end
	end
end

-- 通用成功码不带包段
_names[M.OK] = "alua.OK"


-- @brief 码名 (完整串)
-- @param [in]  code[number]			码值
-- @return name[string]				`alua.packages.<包名>.<码名>`; 成功码 `alua.OK`; 未登记码 `alua.UNKNOWN`
-- @stability stable
-- @note eg. code.strerror(code.proxy.BAD_ARGS) -> "alua.packages.proxy.BAD_ARGS"
--   \n     code.strerror(code.OK) -> "alua.OK"
--   \n     code.strerror(9999) -> "alua.UNKNOWN"
M.strerror = function (code)
	if type(code) ~= "number" then
		return _UNKNOWN
	end

	return _names[code] or _UNKNOWN
end


-- @brief 码短名 (码名末段; 未知则 nil); 与 klbcode.str 同名
-- @param [in]  code[number]			码值
-- @return name[string|nil]			eg. "BAD_REQUEST"
-- @stability stable
-- @note 完整码名用 strerror (eg. alua.packages.alua_web.BAD_REQUEST)
M.str = function (code)
	local full = M.strerror(code)
	if full == _UNKNOWN then
		return nil
	end

	return full:match("([^.]+)$")
end


-- @brief 构造错误表
-- @param [in]  code[number]			本包码值; 非数字取 M.INVALID (码非法)
-- @param [in]  message[string]			描述; 空则取码短名
-- @return err[table]					{ code, message, name }
-- @stability stable
-- @note code 须传本包码 (eg. code.proxy.BAD_REQUEST); err.name 为码短名, 未登记码为 nil
--   \n eg. code.make_error(code.proxy.BAD_REQUEST, "bad args") -> { code=70436, ..., name="BAD_REQUEST" }
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
