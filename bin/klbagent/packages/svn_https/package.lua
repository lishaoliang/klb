--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   package.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  svn_https 包清单 (optional)
-- @note   方案 B: 对外 https:// 替 mod_dav_svn; 对内 svnserve; P0 契约见 docs/packages.md
-- @history 修改历史
--  \n [2026-09] 创建文件 (桩); 反代主路径与 bridge HTTP-SVN↔svnserve (python helper); 修 inherited-props / PROPFIND Depth:1 (Bug.1/2); 记 checkout E200007 未修
--  \n [2026-11] 改用 klblisten 端口组投递; ALUA_SVN_ADDR 多地址监听与 REPO_ROOT 401/404 映射; 修 DeltaV/LOCK/Content-Length 与子目录 checkout/多段 Range; PROPFIND 输出 svn: 版本属性; replay textdelta 上限; 修 update-report 根目录属性与根锚点 propchanges (Bug.2026-23/24)
--]]

return {
	name = "svn_https",
	version = "0.2.15",
	tier = "optional",
	kind = "daemon",
	summary = "HTTPS SVN edge (scheme B: replace mod_dav_svn; upstream svnserve)",
	main = "klbagent.packages.svn_https",
}

-- end
