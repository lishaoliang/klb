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
--   \n cors/https/log/session/request_id/csrf/healthz 在 serve 时按汇总结果安装; stop 后再 serve 会重装.
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


--------------------------------------------------------------------------------------------
-- 路由 / 静态 / 中间件 / 路由组 (转到内置站点)

-- @brief 注册通用路由; 可 `route(method, path, mw, handler)`
-- @param [in]      method[string]		HTTP 方法; eg. `POST`
-- @param [in]      path[string]		URL 路径; eg. `/api/v1/:id`
-- @param [in]      ...[function]		路由级 mw..., 末项 handler
-- @return ok[boolean]					true 成功; 参数非法为 false
klbweb.route = function (method, path, ...)
	return ensure_builtin():route(method, path, ...)
end


-- @brief 注册 GET 路由
-- @param [in]      path[string]		URL 路径
-- @param [in]      ...[function]		路由级 mw..., 末项 handler
-- @return ok[boolean]
klbweb.get = function (path, ...)
	return ensure_builtin():get(path, ...)
end


-- @brief 注册 POST 路由
-- @param [in]      path[string]		URL 路径
-- @param [in]      ...[function]		路由级 mw..., 末项 handler
-- @return ok[boolean]
klbweb.post = function (path, ...)
	return ensure_builtin():post(path, ...)
end


-- @brief 注册 PUT 路由
-- @param [in]      path[string]		URL 路径
-- @param [in]      ...[function]		路由级 mw..., 末项 handler
-- @return ok[boolean]
klbweb.put = function (path, ...)
	return ensure_builtin():put(path, ...)
end


-- @brief 注册 DELETE 路由
-- @param [in]      path[string]		URL 路径
-- @param [in]      ...[function]		路由级 mw..., 末项 handler
-- @return ok[boolean]
klbweb.delete = function (path, ...)
	return ensure_builtin():delete(path, ...)
end


-- @brief 注册 PATCH 路由
-- @param [in]      path[string]		URL 路径
-- @param [in]      ...[function]		路由级 mw..., 末项 handler
-- @return ok[boolean]
klbweb.patch = function (path, ...)
	return ensure_builtin():patch(path, ...)
end


-- @brief 注册 HEAD 路由
-- @param [in]      path[string]		URL 路径
-- @param [in]      ...[function]		路由级 mw..., 末项 handler
-- @return ok[boolean]
-- @note 无 HEAD 路由时回退匹配 GET; 响应不带 body
klbweb.head = function (path, ...)
	return ensure_builtin():head(path, ...)
end


-- @brief 注册 OPTIONS 路由
-- @param [in]      path[string]		URL 路径
-- @param [in]      ...[function]		路由级 mw..., 末项 handler
-- @return ok[boolean]
klbweb.options = function (path, ...)
	return ensure_builtin():options(path, ...)
end


-- @brief 注册 WebSocket 路由
-- @param [in]      path[string]		URL 路径
-- @param [in]      ...[function|table]	路由级 mw..., 可选 `{ protocol = "..." }`, 末项 `fn(req, ws)`
-- @return ok[boolean]					true 成功; 参数非法为 false
klbweb.ws = function (path, ...)
	return ensure_builtin():ws(path, ...)
end


-- @brief 注册中间件
-- @param [in]      path_or_fn[string|function]		前缀或处理函数
-- @param [in]      fn[function]					[可选] 前缀形式时的处理函数
-- @return ok[boolean]								true 成功; 参数非法为 false
-- @note `fn(req, res, next)`; 不调用 next 且未发送则 204; 与 get/static 按注册序
klbweb.use = function (path_or_fn, fn)
	return ensure_builtin():use(path_or_fn, fn)
end


-- @brief 打开路由组
-- @param [in]      prefix[string]		URL 前缀; 静态路径, 不含 `:name` / `*`
-- @return group[table]					组对象; 前缀非法为 false
-- @note 组内 path 相对前缀. eg. `local api = klbweb.group("/api"); api:get("/v1/:id", fn)`
klbweb.group = function (prefix)
	return ensure_builtin():group(prefix)
end


-- @brief 挂载静态目录
-- @param [in]      prefix[string]		URL 前缀; eg. `/`
-- @param [in]      dir[string]			本地目录
-- @param [in]      opts[table]			[可选] listing/index/max_file/gzip/gzip_dynamic
-- @return ok[boolean]					true 成功; 参数非法为 false
klbweb.static = function (prefix, dir, opts)
	return ensure_builtin():static(prefix, dir, opts)
end


-- @brief 修改已生效的站点配置
-- @param [in]      patch[table]		要覆盖的字段; 省略的键不变
-- @return ok[boolean]					true 成功; 参数非法为 false
-- @note 立即影响后续请求读 `_cfg` 的项. cors/https/log 等中间件不重装.
klbweb.apply_cfg = function (patch)
	return ensure_builtin():apply_cfg(patch)
end


-- @brief 处理一条外部已建立的 HTTP 服务连接 (不 bind)
-- @param [in]      serve[table]		klbhttp/httpserver 会话; 或兼容 :co_recv/:send 的对象
-- @return ok[boolean]					true 已 fork 处理; serve 非法为 false
-- @note 须在 kco 可用的 env 内; 用于 klblisten / klisten 单端口分发后的 HTTP 投递
klbweb.handle = function (serve)
	return ensure_builtin():handle(serve)
end


return klbweb
