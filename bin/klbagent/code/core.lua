--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   core.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua core 域错误码表 (按模块域分段)
-- @stability stable
-- @impl evolving
-- @attention
--   \n @stability stable: AI 禁止擅自添加/删除/变更对外接口 (含导出名, 参数, 返回与已文档含义); 须用户点名
--   \n @impl evolving: 内部可重构, 只要不破坏本文件导出签名与可观察语义
-- @note   各域使用本域独立错误码 (码名带域段): alua.core.<域>.<码名>
--   \n 无跨域共享错误码 (仅 M.OK = 0 各域共用); 可选包码见 code/packages.lua
--   \n 总窗: [0x10000+4000, 0x20000); core 仅占前 500 (M.BASE .. M.BASE+M.SPAN); 其后归 packages
--   \n 普通域按目录名字母序紧排: 空壳宽 10 / 有码中等域宽 20 (M.SEG) / serve 宽 100; 不再用 400 步长与 16k 锚点
--   \n 码名: alua.core.<域>.<码名> (例 alua.core.serve.BAD_ARGV); 成功码 alua.OK; 未登记 alua.UNKNOWN
--   \n 码值一经发布不得改号
--   \n require("klbagent.code.core"); 对外统一入口 require("klbagent.code")
--   \n 码表随模块落地增补 (只增码, 已发布码值不改); 通用 0 = OK; 各域基数已定
--   \n 已落: envelope / ssid / stdio / dispatch / tools / serve(相对 +0..+42) / cli; 包表见 code/packages.lua
-- @history 修改历史
--  \n [2026-11] 创建文件; 先落 alua 模块错误码骨架
--]]


local M = {}


-- 全模块成功码; 各域成功均返回 0 (各域独立错误码见下方域表)
M.OK = 0

-- 码非法哨兵 (make_error 容错用; 非业务码, 不占域段; 未登记故 str 返回 nil)
M.INVALID = -1

-- alua 错误码总窗起点 (紧接 base/net 私有窗末)
M.BASE = 0x10000 + 4000

-- core 占用跨度; 包域自 M.BASE + M.SPAN 起
M.SPAN = 500

-- 有码中等域默认宽 (空壳 10; serve 100); 新域按需取宽, 勿越过 M.BASE+M.SPAN
M.SEG = 20


-- ai_load_gate 域错误码 (域基址 M.BASE+0, 宽 10); 码随 core/ai_load_gate 子模块落地按语义补名
M.ai_load_gate = {
	base = 69536,				-- 域基址
}


-- auth 域错误码 (域基址 M.BASE+10, 宽 10); 码随 core/auth 子模块落地按语义补名
M.auth = {
	base = 69546,				-- 域基址
}


-- catalog 域错误码 (域基址 M.BASE+20, 宽 10); 码随 core/catalog 子模块落地按语义补名
M.catalog = {
	base = 69556,				-- 域基址
}


-- config 域错误码 (域基址 M.BASE+30, 宽 10); 码随 core/config 子模块落地按语义补名
M.config = {
	base = 69566,				-- 域基址
}


-- envelope 域错误码 (域基址 M.BASE+40, 宽 20)
M.envelope = {
	base = 69576,				-- 域基址
	TOOL_FAILED = 69576,		-- 信封构造 / 缺字段失败
}


-- hostinfo 域错误码 (域基址 M.BASE+60, 宽 10); 码随 core/hostinfo 子模块落地按语义补名
M.hostinfo = {
	base = 69596,				-- 域基址
}


-- log 域错误码 (域基址 M.BASE+70, 宽 10); 码随 core/log 子模块落地按语义补名
M.log = {
	base = 69606,				-- 域基址
}


-- lpc_bridge 域错误码 (域基址 M.BASE+80, 宽 10); 码随 core/lpc_bridge 子模块落地按语义补名
M.lpc_bridge = {
	base = 69616,				-- 域基址
}


-- profiles 域错误码 (域基址 M.BASE+90, 宽 10); 码随 core/profiles 子模块落地按语义补名
M.profiles = {
	base = 69626,				-- 域基址
}


-- registry 域错误码 (域基址 M.BASE+100, 宽 10); 码随 core/registry 子模块落地按语义补名
M.registry = {
	base = 69636,				-- 域基址
}


-- schema 域错误码 (域基址 M.BASE+110, 宽 10); 码随 core/schema 子模块落地按语义补名
M.schema = {
	base = 69646,				-- 域基址
}


-- session 域错误码 (域基址 M.BASE+120, 宽 10); 码随 core/session 子模块落地按语义补名
M.session = {
	base = 69656,				-- 域基址
}


-- ssid 域错误码 (域基址 M.BASE+130, 宽 20)
M.ssid = {
	base = 69666,				-- 域基址
	OWNER_UNAVAIL = 69666,		-- ssid owner / LPC|XPC 客户端不可用
	LPC_FAIL = 69667,			-- LPC/XPC 传输调用失败 (未达 owner 业务)
}


-- stdio 域错误码 (域基址 M.BASE+150, 宽 20)
M.stdio = {
	base = 69686,				-- 域基址
	BAD_REQUEST = 69686,		-- stdio 请求非法 (缺字段 / 类型错)
	TOOL_FAILED = 69687,		-- stdio 工具执行失败
}


-- dispatch 域错误码 (域基址 M.BASE+170, 宽 20)
M.dispatch = {
	base = 69706,				-- 域基址
	BAD_REQUEST = 69706,		-- 请求非法 (缺工具名 / 参数错)
	DENIED = 69707,				-- 权限拒绝
	TOOL_NOT_FOUND = 69708,		-- 工具未登记
	TOOL_FAILED = 69709,		-- 工具执行失败
}


-- tools 域错误码 (域基址 M.BASE+190, 宽 20)
M.tools = {
	base = 69726,				-- 域基址
	TOOL_FAILED = 69726,		-- 远端工具调用 / 传输失败
}


-- serve 域错误码 (域基址 M.BASE+210, 宽 100; 落 core/serve 启动 / 选口 / TLS / 嵌入失败面)
-- 入口鉴权拒绝用 core.auth 的 AUTH_ERR_*; 请求类错用本域码 (BAD_REQUEST / DENIED), 此处不重复
-- 失败来源: serve/init(entry) | worker | proc/embed | proc/restart | port | mux | security/tls | security/profile | security/token | base/boot
-- 相对偏移保留: 启动 0..19 / 嵌入 20..39 / 请求 40..59
M.serve = {
	base = 69746,				-- 域基址
	-- 启动 / 生命周期 (+0..+19)
	BAD_ARGV = 69746,			-- serve 参数非法 (未知参数 / 端口越界 / 档位名错)
	LOAD_TOOLS_FAIL = 69747,	-- 内置 tools 注册失败 (catalog / dispatch 装载错)
	TOKEN_REQUIRED = 69748,		-- 全档强制 token; 环境变量 / serve.env / 嵌入均无
	TOKEN_CSPRNG = 69749,		-- CSPRNG 不可用; 拒生成 / 拒签发 token
	TLS_RESOLVE_FAIL = 69750,	-- TLS 面解析失败 (证书 / 私钥不可用)
	TLS_NEED_PEM = 69751,		-- 自签 / 热换缺 PEM cert/key
	TLS_RELOAD_FAIL = 69752,	-- 进程内热换证书失败 (klbweb.reload_tls fail)
	TLS_KTLS_MISSING = 69753,	-- ktls 不可用 (require fail / unavailable)
	PROFILE_FAIL = 69754,		-- 进程档应用 / 运行中调整失败
	NEED_KLBWEB = 69755,		-- 缺 klbcore.klbweb (klbhttp 被裁剪)
	NEED_KLBLISTEN = 69756,		-- 缺 klbcore.klblisten (listen 被裁剪)
	BIND_FAIL = 69757,			-- 选口全失败 / mux bind 失败
	HANDLE_FAIL = 69758,		-- 主口 HTTP 会话投递失败 (app:handle)
	NO_LIVE_APP = 69759,		-- 无活动 serve app (resign / reload 前置不满足)
	RESTART_FAIL = 69760,		-- 异步重启调度失败 (systemctl / schtasks / exec 均不可用)
	UNAVAILABLE = 69761,		-- 包外契约面已收口; 仅 entry 可用 (直调 token/tls/port 等失败)

	-- 宿主嵌入 (+20..+39)
	EMBED_BAD_PORT = 69766,		-- 嵌入端口越界
	EMBED_BIND_FAIL = 69767,	-- 嵌入 app:serve bind 失败 (端口占用等)

	-- 请求面 (+40..+59)
	BAD_REQUEST = 69786,		-- 请求行 / 报文体 / 参数非法
	DENIED = 69787,				-- 会话鉴权拒绝
	TOOL_FAILED = 69788,		-- 请求内工具执行失败
}


-- cli 域错误码 (域基址 M.BASE+310, 宽 20)
M.cli = {
	base = 69846,				-- 域基址
	BAD_REQUEST = 69846,		-- 命令行参数非法 (缺参 / 越界 / 未知旗标)
	DENIED = 69847,				-- 权限 / 确认拒绝
	TOOL_FAILED = 69848,		-- 命令执行失败 (工具调用错)
	NOT_IMPLEMENTED = 69849,	-- 命令尚未实现
}


-- 包域起点 (仅指针; 包表见 klbagent.code.packages)
M.PKG_BASE = M.BASE + M.SPAN


-- 码名反查表: 按域登记 (域前缀 + 表内名); 新增域/码只改上表, 勿另处硬写
-- 码名格式: alua.core.<域>.<码名>; 成功码 alua.OK; 包码见 code/packages.lua
local _CORE_PREFIX = "alua.core."
local _UNKNOWN = "alua.UNKNOWN"
local _names = {}
local _domains = {
	{ "ai_load_gate.", M.ai_load_gate },
	{ "auth.", M.auth },
	{ "catalog.", M.catalog },
	{ "config.", M.config },
	{ "envelope.", M.envelope },
	{ "hostinfo.", M.hostinfo },
	{ "log.", M.log },
	{ "lpc_bridge.", M.lpc_bridge },
	{ "profiles.", M.profiles },
	{ "registry.", M.registry },
	{ "schema.", M.schema },
	{ "session.", M.session },
	{ "ssid.", M.ssid },
	{ "stdio.", M.stdio },
	{ "dispatch.", M.dispatch },
	{ "tools.", M.tools },
	{ "serve.", M.serve },
	{ "cli.", M.cli },
}


for i = 1, #_domains do
	local prefix = _domains[i][1]
	local codes = _domains[i][2]
	for name, code in pairs(codes) do
		-- base 为域基址常量, 非码
		if name ~= "base" then
			_names[code] = _CORE_PREFIX .. prefix .. name
		end
	end
end

-- 成功码不带域段
_names[M.OK] = "alua.OK"


-- @brief 码名 (完整串)
-- @param [in]  code[number]			码值
-- @return name[string]				`alua.core.<域>.<码名>`; 成功码 `alua.OK`; 未登记码 `alua.UNKNOWN`
-- @stability stable
-- @note eg. code.strerror(code.serve.BAD_ARGV) -> "alua.core.serve.BAD_ARGV"
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
-- @return name[string|nil]			eg. "BAD_REQUEST" / "BAD_ARGV"
-- @stability stable
-- @note 完整码名用 strerror (eg. alua.core.cli.BAD_REQUEST)
M.str = function (code)
	local full = M.strerror(code)
	if full == _UNKNOWN then
		return nil
	end

	return full:match("([^.]+)$")
end


-- @brief 构造错误表
-- @param [in]  code[number]			本域码值; 非数字取 M.INVALID (码非法)
-- @param [in]  message[string]			描述; 空则取码短名
-- @return err[table]					{ code, message, name }
-- @stability stable
-- @note code 须传本模块域码 (eg. code.cli.BAD_REQUEST); err.name 为码短名, 未登记码为 nil
--   \n eg. code.make_error(code.cli.BAD_REQUEST, "need tool name") -> { code=69846, ..., name="BAD_REQUEST" }
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
