--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   env_overlay.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  跨进程环境 overlay (env_overlay.json)
-- @note   供 runtime.env_set/get 与 unsafe.exec; 动态键不用 store.normalize
-- @history 修改历史
--  \n [2026-09] 创建文件
--]]
local store = require("klbagent.core.config.store")
local cjson = require("cjson")
local lfs = require("lfs")


local M = {}


M.FILE = "env_overlay.json"


local function _empty()
	return {
		set = {},
		unset = {},
	}
end


local function _ensure_pref_dir()
	local dir = store.dir()
	if dir == "" then
		return false
	end

	if lfs.attributes(dir, "mode") == "directory" then
		return true
	end

	-- touch via runtime module (creates pref dir)
	local runtime = require("klbagent.core.config.runtime")
	runtime.load()
	return lfs.attributes(dir, "mode") == "directory"
end


-- @brief 读取 overlay
-- @return data[table]		{ set={k=v}, unset={k=true} }
M.load = function ()
	local path = store.path(M.FILE)
	if path == "" then
		return _empty()
	end

	local f = io.open(path, "rb")
	if f == nil then
		return _empty()
	end

	local raw = f:read("*a")
	f:close()
	if type(raw) ~= "string" or raw == "" then
		return _empty()
	end

	local ok, obj = pcall(cjson.decode, raw)
	if not ok or type(obj) ~= "table" then
		return _empty()
	end

	local set = {}
	if type(obj.set) == "table" then
		for k, v in pairs(obj.set) do
			if type(k) == "string" and type(v) == "string" then
				set[k] = v
			end
		end
	end

	local unset = {}
	if type(obj.unset) == "table" then
		for _, k in ipairs(obj.unset) do
			if type(k) == "string" and k ~= "" then
				unset[k] = true
			end
		end

		for k, v in pairs(obj.unset) do
			if type(k) == "string" and v == true then
				unset[k] = true
			end
		end
	end

	return {
		set = set,
		unset = unset,
	}
end


-- @brief 写入 overlay
-- @param [in] data[table]
-- @return ok[boolean]
M.save = function (data)
	if type(data) ~= "table" then
		data = _empty()
	end

	if not _ensure_pref_dir() then
		return false
	end

	local unset_arr = {}
	if type(data.unset) == "table" then
		for k, v in pairs(data.unset) do
			if type(k) == "string" and v then
				unset_arr[#unset_arr + 1] = k
			end
		end
	end

	table.sort(unset_arr)

	local set = {}
	if type(data.set) == "table" then
		for k, v in pairs(data.set) do
			if type(k) == "string" and type(v) == "string" then
				set[k] = v
			end
		end
	end

	local path = store.path(M.FILE)
	if path == "" then
		return false
	end

	local ok_j, text = pcall(cjson.encode, {
		set = set,
		unset = unset_arr,
	})
	if not ok_j or type(text) ~= "string" then
		return false
	end

	local f = io.open(path, "wb")
	if f == nil then
		return false
	end

	f:write(text)
	f:write("\n")
	f:close()
	return true
end


return M

-- end
