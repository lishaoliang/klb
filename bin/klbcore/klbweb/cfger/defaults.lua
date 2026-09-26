--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   defaults.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb 站点配置默认值
-- @history 修改历史
--		[2026-09] 创建文件
--]]


local defaults = {}


defaults.server = "klbweb"
defaults.keep_alive = true
defaults.max_body = 256 * 1024
defaults.max_keep_alive = 100
defaults.max_file = 2 * 1024 * 1024
defaults.index = "index.html"
defaults.listing = false
defaults.gzip = true
defaults.gzip_dynamic = false
defaults.listen_port = 8000


-- @brief 一份新的站点生效表 (未 apply 用户 cfg)
-- @return site[table]
defaults.site = function ()
	return {
		server = defaults.server,
		keep_alive = defaults.keep_alive,
		max_body = defaults.max_body,
		max_body_paths = nil,
		max_keep_alive = defaults.max_keep_alive,
		cors = nil,
		log = nil,
		session = nil,
		session_secret = nil,
		request_id = nil,
		csrf = nil,
		healthz = nil,
		spa = nil,
		pages = nil,
		on_error = nil,
		upload_dir = nil,
		max_file = nil,
	}
end


return defaults
