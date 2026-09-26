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


local klbweb = {}


--------------------------------------------------------------------------------------------
-- 包入口

-- @brief 新建一个独立站点对象
-- @param [in]      cfg[table]			[可选] 站点配置
-- @return [table]	站点对象
klbweb.new = function (cfg)
	return weber.new(cfg)
end


--------------------------------------------------------------------------------------------
-- 内置站点 (一份; 与 new 互不影响)
-- 默认 nil; 首次 setup / serve / 路由等再创建

local builtin = nil


-- 确保内置站点已创建
local function ensure_builtin()
	if not builtin then
		builtin = weber.new()
	end

	return builtin
end


-- @brief 配置内置站点; 可多次; 后写字段覆盖
-- @param [in]      cfg[table]			[可选] 站点配置; 见 cfger.new
-- @return ok[boolean]					true 成功; 服务中未 stop 或参数非法为 false
-- @note 省略的键不变. 不重建站点, 已注册路由/中间件保留.
--   \n cors/log/session/request_id/csrf/healthz 在 serve 时按汇总结果安装; stop 后再 serve 会重装.
--   \n `listen` 只记规格; serve 时自行 bind. 未设时默认 8000 HTTP.
--   \n 同端口 HTTP+HTTPS 写一项 `tls=true, plain=true`, 或两项同 port 由 cfger 合并.
klbweb.setup = function (cfg)
	return ensure_builtin():setup(cfg)
end


-- @brief 按 setup.listen 自行 bind, 再启动 accept 循环
-- @return ok[boolean]					true 全部 bind 成功; 任一项失败为 false
-- @note 幂等; 已启动的 listener 跳过
--   \n 未配置 listen 时默认 8000 HTTP
klbweb.serve = function ()
	return ensure_builtin():serve()
end


-- @brief 停止内置站点对外服务; 许可再 setup / serve
-- @return ok[boolean]					true
-- @note 关全部监听; 保留路由 / 静态 / 汇总配置
klbweb.stop = function ()
	if builtin then
		builtin:stop()
	end

	return true
end


-- 路由 / 静态 / 中间件转到内置站点; 签名同 weber W:*
local FORWARD = {
	"route", "get", "post", "put", "delete", "patch", "head", "options",
	"use", "static", "apply_cfg",
}

for i = 1, #FORWARD do
	local name = FORWARD[i]
	klbweb[name] = function (...)
		local site = ensure_builtin()
		return site[name](site, ...)
	end
end


return klbweb
