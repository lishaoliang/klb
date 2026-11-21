--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   store.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbagent 配置 JSON 存盘公共层
-- @note   pref_path(klua,alua); 错误/未知键/缺键均按模块 defaults+normalize 替代
-- @history 修改历史
--  \n [2026-09] 创建文件
--  \n [2026-10] 可选 mod.prepare_save (proxy 剥 active_chain)
--]]
local kenv = require("kenv")
local cjson = require("cjson")
local lfs = require("lfs")


local S = {}


S.ORG = "klua"
S.APP = "alua"


local function _join(base, rel)
	if base == nil or base == "" then
		return rel
	end

	local last = base:sub(-1)
	if last == "/" or last == "\\" then
		return base .. rel
	end

	return base .. "/" .. rel
end


local function _mkdir_one(path)
	local mode = lfs.attributes(path, "mode")
	if mode ~= nil then
		return true
	end

	local ok, err = lfs.mkdir(path)
	if ok then
		return true
	end

	if lfs.attributes(path, "mode") ~= nil then
		return true
	end

	return false, err
end


local function _ensure_dir(path)
	if path == nil or path == "" then
		return false, "empty path"
	end

	local is_abs = path:sub(1, 1) == "/" or path:match("^%a:[/\\]")
	local parts = {}
	for seg in string.gmatch(path, "[^/\\]+") do
		parts[#parts + 1] = seg
	end

	local cur = ""
	if is_abs then
		if path:match("^%a:[/\\]") then
			cur = parts[1]
			table.remove(parts, 1)
		else
			cur = "/"
		end
	end

	for i = 1, #parts do
		local seg = parts[i]
		if cur == "" or cur == "/" then
			cur = cur .. seg
		elseif cur:sub(-1) == "/" or cur:sub(-1) == "\\" then
			cur = cur .. seg
		else
			cur = cur .. "/" .. seg
		end

		local ok, err = _mkdir_one(cur)
		if not ok then
			return false, err
		end
	end

	return true
end


local function _encode(obj)
	local ok, text = pcall(cjson.encode, obj)
	if not ok or type(text) ~= "string" then
		return nil
	end

	return text
end


-- 未知键 / 缺键 / 值被 normalize 替换 → 需要回写干净 JSON
local function _need_rewrite(obj, cfg)
	for k, _ in pairs(obj) do
		if cfg[k] == nil then
			return true
		end
	end

	for k, v in pairs(cfg) do
		if obj[k] ~= v then
			return true
		end
	end

	return false
end


-- @brief 逐级创建目录 (供非 JSON 落盘复用)
-- @param [in] path[string]
-- @return ok[boolean]
-- @return err[string|nil]
S.ensure_dir = function (path)
	return _ensure_dir(path)
end


-- @brief 配置根目录
-- @return dir[string]
S.dir = function ()
	local dir = kenv.pref_path(S.ORG, S.APP)
	if type(dir) ~= "string" then
		return ""
	end

	return dir
end


-- @brief 模块 JSON 全路径
-- @param [in] file[string]		如 runtime.json
-- @return path[string]
S.path = function (file)
	local dir = S.dir()
	if dir == "" or type(file) ~= "string" or file == "" then
		return ""
	end

	return _join(dir, file)
end


-- @brief normalize 后可选 prepare_save (模块可剥仅进程内字段)
-- @param [in] mod[table]
-- @param [in] cfg[table]
-- @return cfg[table]
local function _after_normalize(mod, cfg)
	if type(mod) == "table" and type(mod.prepare_save) == "function" then
		cfg = mod.prepare_save(cfg)
	end

	return cfg
end


-- @brief 写入模块 JSON (仅 normalize 后的已知键)
-- @param [in] mod[table]		须含 FILE / normalize; 可选 prepare_save
-- @param [in] cfg[table]
-- @return ok[boolean]
S.save = function (mod, cfg)
	cfg = _after_normalize(mod, mod.normalize(cfg))

	local dir = S.dir()
	if dir == "" then
		return false
	end

	if lfs.attributes(dir, "mode") ~= "directory" then
		local ok = _ensure_dir(dir)
		if not ok then
			return false
		end
	end

	local path = S.path(mod.FILE)
	if path == "" then
		return false
	end

	local text = _encode(cfg)
	if text == nil then
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


-- @brief 读取模块 JSON
-- @note  缺文件/空/坏 JSON → 整表默认并回写; 非法值与未知/新键 → 默认替代并回写
-- @param [in] mod[table]		须含 FILE / defaults / normalize; 可选 prepare_save
-- @param [in] legacy_file[string|nil]	[可选] 旧单文件名迁移
-- @return cfg[table]
S.load = function (mod, legacy_file)
	local def = mod.defaults()
	local path = S.path(mod.FILE)
	if path == "" then
		return def
	end

	local f = io.open(path, "rb")
	local from_legacy = false

	if f == nil and type(legacy_file) == "string" and legacy_file ~= "" then
		local legacy_path = S.path(legacy_file)
		if legacy_path ~= "" then
			f = io.open(legacy_path, "rb")
			if f ~= nil then
				from_legacy = true
			end
		end
	end

	if f == nil then
		S.save(mod, def)
		return def
	end

	local raw = f:read("*a")
	f:close()

	if type(raw) ~= "string" or raw == "" then
		S.save(mod, def)
		return def
	end

	local ok, obj = pcall(cjson.decode, raw)
	if not ok or type(obj) ~= "table" then
		S.save(mod, def)
		return def
	end

	local cfg = _after_normalize(mod, mod.normalize(obj))
	if from_legacy or _need_rewrite(obj, cfg) then
		S.save(mod, cfg)
	end

	return cfg
end


return S
