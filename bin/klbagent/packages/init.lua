--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbagent 可选包 (packages) 发现与入口
-- @stability stable
-- @impl evolving
-- @attention
--   \n @stability stable: AI 禁止擅自添加/删除/变更对外接口 (含导出名, 参数, 返回与已文档含义); 须用户点名
--   \n @impl evolving: 内部可重构, 只要不破坏本文件导出签名与可观察语义
-- @note   非必须包根; tier=optional 默认不进嵌入式安装; CLI: alua pkg
--   \n kind: daemon|cli 走 entry()→mod.entry(); tools 走 register_tools(); lib 仅可 list
-- @history 修改历史
--  \n [2026-09] 创建文件 (桩)
--]]
local lfs = require("lfs")
local codes = require("klbagent.code")


local M = {}


-- @brief 本模块所在目录 (…/klbagent/packages)
local function _root_dir()
	local src = debug.getinfo(1, "S").source
	if type(src) == "string" and string.sub(src, 1, 1) == "@" then
		src = string.sub(src, 2)
	end

	if type(src) ~= "string" or src == "" then
		return nil
	end

	local dir = string.match(src, "^(.*)[/\\][^/\\]+$")
	return dir
end


-- @brief 读取包清单 package.lua
-- @return manifest[table|nil]
-- @return err[string|nil]
local function _load_manifest(name)
	if type(name) ~= "string" or name == "" then
		return nil, "need package name"
	end

	if string.find(name, "[^%w_%-]") ~= nil then
		return nil, "bad package name"
	end

	local mod_name = "klbagent.packages." .. name .. ".package"
	local ok, man = pcall(require, mod_name)
	if not ok or type(man) ~= "table" then
		return nil, "package not installed: " .. name
	end

	return man
end


-- @brief 列出已安装包 (有 package.lua 的子目录)
-- @return list[table]  manifest 数组; 无则为 {}
M.list = function ()
	local out = {}
	local root = _root_dir()
	if root == nil then
		return out
	end

	for name in lfs.dir(root) do
		if name ~= "." and name ~= ".." and name ~= "init.lua" then
			local path = root .. "/" .. name
			local attr = lfs.attributes(path)
			if attr ~= nil and attr.mode == "directory" then
				local man = _load_manifest(name)
				if type(man) == "table" then
					out[#out + 1] = {
						name = man.name or name,
						tier = man.tier or "optional",
						kind = man.kind or "",
						version = man.version or "",
						summary = man.summary or "",
					}
				end
			end
		end
	end

	table.sort(out, function (a, b)
		return tostring(a.name) < tostring(b.name)
	end)

	return out
end


-- @brief 包入口 (进程/CLI/serve 分派; 命名 entry*)
-- @param [in] name[string]
-- @param [in] argv[table|nil]  额外参数
-- @return ok[boolean]
-- @return code[number]
-- @return message[string|nil]
M.entry = function (name, argv)
	local man, err = _load_manifest(name)
	if man == nil then
		return false, codes.loader.BAD_REQUEST, tostring(err)
	end

	local kind = tostring(man.kind or "daemon")
	local mod_name = "klbagent.packages." .. (man.name or name)
	local ok, mod = pcall(require, mod_name)
	if not ok or type(mod) ~= "table" then
		return false, codes.loader.TOOL_NOT_FOUND, "package module missing"
	end

	if kind == "lib" then
		return false, codes.loader.NOT_IMPLEMENTED, "lib package has no entry(); require only"
	end

	if kind == "tools" then
		if type(mod.register_tools) ~= "function" then
			return false, codes.loader.NOT_IMPLEMENTED, "tools package has no register_tools()"
		end

		return mod.register_tools(argv or {}, man)
	end

	-- daemon / cli (及未识别 kind): 统一 mod.entry()
	if type(mod.entry) ~= "function" then
		return false, codes.loader.NOT_IMPLEMENTED, "package has no entry()"
	end

	return mod.entry(argv or {}, man)
end


return M

-- end
