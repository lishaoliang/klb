--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   machine.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  ssid serve 本机 machine_key / digest / 生成 alua_ssid
-- @stability internal
-- @impl evolving
-- @note   require("klbagent.core.ssid.serve.machine")
-- @history 修改历史
--  \n [2026-11] 自 serve/init 迁出
--]]
local const = require("klbagent.core.ssid.base.const")
local fingerprint = require("klbagent.core.ssid.base.fingerprint")
local store = require("klbagent.core.ssid.base.store")
local cjson = require("cjson")


local M = {}


local g_machine = nil


local function _cfg_store()
	local ok, s = pcall(require, "klbagent.core.config.store")
	if ok and type(s) == "table" then
		return s
	end

	return nil
end


local function _rand_hex(nbytes)
	local ok_k, krand = pcall(require, "krand")
	if ok_k and type(krand) == "table" and type(krand.rand_bytes) == "function" then
		local ok_b, raw = pcall(krand.rand_bytes, nbytes)
		if ok_b and type(raw) == "string" and #raw == nbytes then
			local parts = {}
			for i = 1, nbytes do
				parts[#parts + 1] = string.format("%02x", string.byte(raw, i))
			end

			return table.concat(parts)
		end
	end

	local ok_f, f = pcall(io.open, "/dev/urandom", "rb")
	if ok_f and f ~= nil then
		local raw = f:read(nbytes)
		f:close()
		if type(raw) == "string" and #raw == nbytes then
			local parts = {}
			for i = 1, nbytes do
				parts[#parts + 1] = string.format("%02x", string.byte(raw, i))
			end

			return table.concat(parts)
		end
	end

	-- 弱回退 (仅 machine 首次生成; 生产宜有 CSPRNG)
	local parts = {}
	for i = 1, nbytes do
		parts[#parts + 1] = string.format("%02x", math.random(0, 255))
	end

	return table.concat(parts)
end


-- @brief 加载/创建本机 machine_key + salt4 + digest
-- @return mk[table]
M.get = function ()
	if type(g_machine) == "table" then
		return g_machine
	end

	local store_mod = _cfg_store()
	local path = ""
	if store_mod ~= nil and type(store_mod.path) == "function" then
		path = store_mod.path("ssid.machine.json")
	end

	if path ~= "" then
		local f = io.open(path, "rb")
		if f ~= nil then
			local text = f:read("*a") or ""
			f:close()
			local ok_d, obj = pcall(cjson.decode, text)
			if ok_d and type(obj) == "table"
				and type(obj.machine_key) == "string" and #obj.machine_key >= 16
				and type(obj.salt4) == "string" and #obj.salt4 == const.SALT_LEN then
				-- 旧机器键无 digest: 保持 fnv1a32 兼容, 不迁移既有 SSID
				local digest = "fnv1a32"
				if obj.digest == "fnv1a64" then
					digest = "fnv1a64"
				end

				g_machine = {
					machine_key = obj.machine_key,
					salt4 = string.lower(obj.salt4),
					digest = digest,
				}
				return g_machine
			end
		end
	end

	local key = _rand_hex(16)
	local salt4 = string.sub(_rand_hex(2), 1, const.SALT_LEN)
	g_machine = {
		machine_key = key,
		salt4 = salt4,
		digest = "fnv1a64",
	}

	if path ~= "" and store_mod ~= nil then
		local dir = store_mod.dir()
		if type(dir) == "string" and dir ~= "" then
			store_mod.ensure_dir(dir)
		end

		local f = io.open(path, "wb")
		if f ~= nil then
			f:write(cjson.encode({
				version = 1,
				machine_key = key,
				salt4 = salt4,
				digest = g_machine.digest,
			}))
			f:write("\n")
			f:close()
			store.set_private(path)
		end
	end

	return g_machine
end


-- @brief 推 digest12 (machine_key + material); 字符集 [0-9a-f]
-- @note 新装机器键用 fnv1a64; 旧机器键保持 fnv1a32 三轮, 不迁移既有 SSID
-- @param [in] mat_str[string]
-- @return digest12[string]
M.digest12 = function (mat_str)
	local mk = M.get()
	local s = tostring(mk.machine_key) .. "|" .. tostring(mat_str)
	if mk.digest == "fnv1a64" then
		return string.sub(fingerprint.fnv1a64_hex(s), 1, const.DIGEST_LEN)
	end

	-- 多轮 FNV 扩成 12 hex (兼容既有)
	local a = fingerprint.fnv1a_hex(s)
	local b = fingerprint.fnv1a_hex(a .. "|" .. s)
	local c = fingerprint.fnv1a_hex(b .. "|ssid")
	local hex = (a .. b .. c)
	return string.sub(hex, 1, const.DIGEST_LEN)
end


-- @brief 由 material 串生成 16 位 alua_ssid
-- @param [in] mat_str[string]
-- @return sid[string]
M.make_ssid = function (mat_str)
	local mk = M.get()
	return M.digest12(mat_str) .. mk.salt4
end


return M

-- end
