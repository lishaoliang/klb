--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @brief  klbxpc 模块名/传输配置
-- @note   模块名须非空; 长度上限同 C 缓冲 KLB_XPC_NAME_BUF(=PATH_MAX=108, 含0 → strlen<=107);
--         业务短名由脚本约定, 无再硬编 15; transport/path/group 见下
-- @history 修改历史
--		[2026-11] 创建文件
--]]

local cfger = {}

-- 与 C KLB_XPC_NAME_BUF / KLB_XPC_PATH_MAX 一致 (含结尾0); 可用名 strlen < NAME_BUF
cfger.NAME_BUF = 108
cfger.PATH_MAX = cfger.NAME_BUF
cfger.NAME_MAX = cfger.NAME_BUF - 1

-- 默认传输: POSIX 走 unix socket (/tmp/klb.xpc.<模块名>); Windows 无 unix 端点 (klb_xpc_create_unix 返 NULL), 退回 msgq
cfger.DEFAULT_TRANSPORT = ("/" == string.sub(package.config, 1, 1)) and "unix" or "msgq"

-- group 可用传输全集 (顺序稳定, 便于 info / 冒烟)
cfger.ALL_TRANSPORTS = { "msgq", "unix", "npipe", "sysmq" }

-- group 读模式: recv=使用者 co_recv; route=内置 call/post/notify (与 recv 互斥)
cfger.MODE_RECV = "recv"
cfger.MODE_ROUTE = "route"

cfger.parse_name = function (name)
	if "string" ~= type(name) then
		return nil
	end

	if "" == name then
		return nil
	end

	if cfger.NAME_MAX < #name then
		return nil
	end

	return name
end

cfger.parse_transport = function (tr)
	if nil == tr then
		return cfger.DEFAULT_TRANSPORT
	end

	if "string" ~= type(tr) then
		return nil
	end

	if "msgq" == tr or "unix" == tr or "npipe" == tr or "sysmq" == tr then
		return tr
	end

	return nil
end

cfger.parse_path = function (path)
	if nil == path then
		return ""
	end

	if "string" ~= type(path) then
		return nil
	end

	return path
end

cfger.parse_setup = function (cfg)
	if nil == cfg then
		return { name = "", transport = cfger.DEFAULT_TRANSPORT, path = "" }, true
	end

	if "table" ~= type(cfg) then
		return nil, false
	end

	local name = ""
	if nil ~= cfg.name then
		name = cfger.parse_name(cfg.name)
		if not name then
			return nil, false
		end
	end

	local transport = cfger.parse_transport(cfg.transport)
	if not transport then
		return nil, false
	end

	local path = cfger.parse_path(cfg.path)
	if not path then
		return nil, false
	end

	return { name = name, transport = transport, path = path }, true
end

--- 是否走 unix 具名 API
cfger.is_unix = function (transport)
	return "unix" == transport
end

--- 是否走 npipe 具名 API
cfger.is_npipe = function (transport)
	return "npipe" == transport
end

--- 是否走 sysmq 具名 API
cfger.is_sysmq = function (transport)
	return "sysmq" == transport
end

--- 路径; 空串转 nil 给 C 默认路径
cfger.path_arg = function (path)
	if nil == path or "" == path then
		return nil
	end

	return path
end

-- 兼容旧名
cfger.unix_path_arg = cfger.path_arg


--- 解析 transports: nil→默认单传输; "all"→全集; string→单项; string 数组→去重保序
-- @return list[table]|nil
cfger.parse_transports = function (tr)
	if nil == tr then
		return { cfger.DEFAULT_TRANSPORT }
	end

	if "string" == type(tr) then
		if "all" == tr then
			local all = {}
			for i = 1, #cfger.ALL_TRANSPORTS do
				all[i] = cfger.ALL_TRANSPORTS[i]
			end
			return all
		end

		local one = cfger.parse_transport(tr)
		if not one then
			return nil
		end

		return { one }
	end

	if "table" ~= type(tr) then
		return nil
	end

	local list = {}
	local seen = {}
	for i = 1, #tr do
		local one = cfger.parse_transport(tr[i])
		if not one then
			return nil
		end

		if not seen[one] then
			seen[one] = true
			list[#list + 1] = one
		end
	end

	if 0 == #list then
		return nil
	end

	return list
end


--- 解析 group 规格
-- @param [in] cfg[table]  name; transports?|"all"; transport?(兼容单值);
--   \n paths?[table] 按传输覆盖 path; path?(单 path 仅当单传输时用); mode?=recv|route
-- @return parsed[table]|nil, ok[boolean]
cfger.parse_group = function (cfg)
	if nil == cfg then
		return nil, false
	end

	if "table" ~= type(cfg) then
		return nil, false
	end

	local name = cfger.parse_name(cfg.name)
	if not name then
		return nil, false
	end

	local tr_src = cfg.transports
	if nil == tr_src then
		tr_src = cfg.transport
	end

	local transports = cfger.parse_transports(tr_src)
	if not transports then
		return nil, false
	end

	local mode = cfg.mode
	if nil == mode then
		mode = cfger.MODE_RECV
	end

	if "string" ~= type(mode) then
		return nil, false
	end

	if cfger.MODE_RECV ~= mode and cfger.MODE_ROUTE ~= mode then
		return nil, false
	end

	local paths = {}
	if nil ~= cfg.paths then
		if "table" ~= type(cfg.paths) then
			return nil, false
		end

		for k, v in pairs(cfg.paths) do
			if "string" ~= type(k) then
				return nil, false
			end

			local tr = cfger.parse_transport(k)
			if not tr then
				return nil, false
			end

			local p = cfger.parse_path(v)
			if not p then
				return nil, false
			end

			paths[tr] = p
		end
	end

	-- 单 path: 仅当仅一传输且该传输未在 paths 中指定
	if nil ~= cfg.path then
		local p = cfger.parse_path(cfg.path)
		if not p then
			return nil, false
		end

		if 1 == #transports and nil == paths[transports[1]] then
			paths[transports[1]] = p
		elseif "" ~= p and 1 ~= #transports then
			-- 多传输时禁止模糊单 path
			return nil, false
		end
	end

	-- transports=="all" 标记: 显式列表失败须整组失败; all 时 create nil 可 dropped
	local want_all = ("all" == cfg.transports)

	return {
		name = name,
		transports = transports,
		paths = paths,
		mode = mode,
		want_all = want_all,
	}, true
end


--- 取某传输的 path 覆盖 (空→nil)
cfger.path_for = function (paths, transport)
	if "table" ~= type(paths) then
		return nil
	end

	return cfger.path_arg(paths[transport])
end

return cfger
