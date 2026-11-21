--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   listen/kmux_sink.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klblisten → kmuxlist 入队 (可按协议面分 path)
-- @stability internal
-- @impl evolving
-- @note   非公开 require 面; cfg.kmuxlist 见 setup 注释
-- @history 修改历史
--  \n [2026-11] 创建文件
--]]


local M = {}


-- face → 分队键 (TLS/明文同队; RPC 独立)
local FACE_KEY = {
	http = "http",
	https = "http",
	ws = "http",
	wss = "http",
	smp = "smp",
	smps = "smp",
	mnp = "mnp",
	mnps = "mnp",
	smprpc = "smp_rpc",
	smprpcs = "smp_rpc",
	mnprpc = "mnp_rpc",
	mnprpcs = "mnp_rpc",
	rtsp = "rtsp",
	rtsps = "rtsp",
}


-- cfg 键 → 分队键 (含别名)
local CFG_KEY = {
	http = "http",
	smp = "smp",
	mnp = "mnp",
	rtsp = "rtsp",
	smp_rpc = "smp_rpc",
	mnp_rpc = "mnp_rpc",
	smprpc = "smp_rpc",
	mnprpc = "mnp_rpc",
}


local function NormMax(n, default_max)
	if "number" ~= type(n) then
		return default_max
	end

	if 0 > n then
		return 0
	end

	return n
end


-- @brief 单条: string 或 { path, max|max_size }
local function EntryPathMax(v, default_max)
	if "string" == type(v) and "" ~= v then
		return v, default_max
	end

	if "table" ~= type(v) then
		return nil, 0
	end

	if "string" ~= type(v.path) or "" == v.path then
		return nil, 0
	end

	local max = default_max
	if "number" == type(v.max) then
		max = v.max
	elseif "number" == type(v.max_size) then
		max = v.max_size
	end

	return v.path, NormMax(max, default_max)
end


function M.face_key(face)
	if "string" ~= type(face) then
		return nil
	end

	return FACE_KEY[face]
end


-- @brief 解析并打开 kmuxlist 队列
-- @param [in]      opt[string|table|nil]
--   \n "/path"                         单队列 (全协议)
--   \n { path="/path", max=N }         单队列 + 上限
--   \n { max=N, http=, smp=, mnp=, rtsp=, smp_rpc=, mnp_rpc=, path=回落 }
--   \n   分队值: "/path" 或 { path="/path", max=N }
-- @return sink[table|nil]		push(face,sock) / info() / enabled
-- @return err[string|nil]
function M.setup(opt)
	if nil == opt then
		return nil, nil
	end

	local ok_m, kmuxlist = pcall(require, "kmuxlist")
	if not ok_m or "table" ~= type(kmuxlist) or "function" ~= type(kmuxlist.new) then
		return nil, "kmuxlist not available"
	end

	local default_max = 0
	local default_path = nil
	local by_key = {}
	local single = false

	if "string" == type(opt) then
		if "" == opt then
			return nil, nil
		end

		default_path = opt
		single = true
	elseif "table" == type(opt) then
		default_max = NormMax(opt.max or opt.max_size, 0)

		local has_face = false
		for alias, _ in pairs(CFG_KEY) do
			if nil ~= opt[alias] then
				has_face = true
				break
			end
		end

		if has_face then
			if "string" == type(opt.path) and "" ~= opt.path then
				default_path = opt.path
			else
				local p = select(1, EntryPathMax(opt.path, default_max))
				if p then
					default_path = p
				end
			end

			for alias, key in pairs(CFG_KEY) do
				if nil ~= opt[alias] and nil == by_key[key] then
					local p, m = EntryPathMax(opt[alias], default_max)
					if p then
						by_key[key] = { path = p, max = m }
					end
				end
			end

			if not next(by_key) and not default_path then
				return nil, "kmuxlist path required"
			end
		else
			-- 仅 path/max → 单队列
			local p, m = EntryPathMax(opt, default_max)
			if not p and "string" == type(opt.path) and "" ~= opt.path then
				p, m = opt.path, default_max
			end

			if not p then
				return nil, "kmuxlist path required"
			end

			default_path = p
			default_max = m
			single = true
		end
	else
		return nil, nil
	end

	local queues = {}
	local paths = {}

	if single then
		queues["*"] = kmuxlist.new(default_path, { max = default_max })
		paths["*"] = default_path
	else
		for key, e in pairs(by_key) do
			queues[key] = kmuxlist.new(e.path, { max = e.max })
			paths[key] = e.path
		end

		if default_path then
			queues["*"] = kmuxlist.new(default_path, { max = default_max })
			paths["*"] = default_path
		end
	end

	local sink = {
		enabled = true,
		paths = paths,
		single = single,
	}

	sink.push = function (face, sock)
		if nil == sock then
			return false
		end

		local q = nil
		if single then
			q = queues["*"]
		else
			local key = FACE_KEY[face]
			if key then
				q = queues[key]
			end

			if not q then
				q = queues["*"]
			end
		end

		if not q then
			return false
		end

		q:push(sock)
		return true
	end

	sink.info = function ()
		if single then
			return paths["*"]
		end

		local out = {}
		for k, p in pairs(paths) do
			if "*" == k then
				out.default = p
			else
				out[k] = p
			end
		end

		return out
	end

	return sink, nil
end


return M

-- end
