--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb init.lua
-- @note   轻量 HTTP 站点框架; 路由 / 静态 / API; 内部使用 klbhttp
--	       适用场景: 嵌入式设备 / 中小型站点 / 个人网站
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local weber = require("klbcore.klbweb.weber")
local coder = require("klbcore.klbweb.coder")
local mw = require("klbcore.klbweb.mw")


local klbweb = {}


--------------------------------------------------------------------------------------------
-- 包入口

-- HTTP 状态码与 reason-phrase / 状态行
klbweb.coder = coder

-- 内置中间件工厂: cors / basic / log / limit / https / bearer / session
klbweb.mw = mw


-- @brief 新建一个独立站点对象
-- @param [in]      cfg[table]			[可选] 站点配置
-- @return [table]	站点对象
klbweb.new = function (cfg)
	return weber.new(cfg)
end


--------------------------------------------------------------------------------------------
-- 内置站点 (一份; 与 new 互不影响)
-- 默认 nil; 首次 setup / listen / fork_accept / 路由等再创建

local builtin = nil


-- 确保内置站点已创建
local function ensure_builtin()
	if not builtin then
		builtin = weber.new()
	end

	return builtin
end


-- @brief 配置内置站点
-- @param [in]      cfg[table]			[可选] 站点配置; 见 weber.new
-- @return ok[boolean]					true 成功; 已 listen 为 false
-- @note 须在路由 / listen 之前调用
klbweb.setup = function (cfg)
	local site = ensure_builtin()
	if 0 < #site._listeners then
		return false
	end

	builtin = weber.new(cfg)
	return true
end


-- @brief 内置站点打开端口
-- @param [in]      port[number(int)]	端口
-- @param [in]      cfg[table]			[可选] 监听配置; 见内联 opts 注释
-- @return ok[boolean]					true 成功; 失败为 false
-- @note 只 bind, 不启动 accept 循环; 可多次; 须再调用 klbweb.fork_accept
klbweb.listen = function (port, cfg)
	-- cfg = {
	--   tls[boolean]			[可选] 是否 TLS, 默认 `false`
	--   cert[string]			tls 时必填, 证书 PEM 原文或文件路径
	--   key[string]			tls 时必填, 私钥 PEM 原文或文件路径
	-- }
	return ensure_builtin():listen(port, cfg)
end


-- @brief 启动内置站点的 accept 循环
-- @return 无
-- @note 幂等; 已启动的 listener 跳过
klbweb.fork_accept = function ()
	ensure_builtin():fork_accept()
end


-- @brief 关闭内置站点全部监听
-- @return 无
klbweb.close = function ()
	if builtin then
		builtin:close()
	end
end


-- 路由 / 静态 / 中间件转到内置站点; 签名同 weber W:*
local FORWARD = {
	"route", "get", "post", "put", "delete", "patch", "head", "options",
	"use", "static",
}

for i = 1, #FORWARD do
	local name = FORWARD[i]
	klbweb[name] = function (...)
		local site = ensure_builtin()
		return site[name](site, ...)
	end
end


return klbweb
