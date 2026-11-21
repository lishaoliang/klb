--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   demo/server.lua
-- @brief  xpc 双进程手测 - 服务端
-- @note   LD_LIBRARY_PATH=<klb>/lib <klb>/lib/klua server.lua [mod_name] [transport]
--         transport=msgq(默认)|unix|npipe|sysmq
-- @history 修改历史
--		[2026-11] 创建文件
--]]

local function setup_path()
	local src = debug.getinfo(1, "S").source
	local dir = src:match("^@(.+)/[^/]+$") or "."
	local bin = dir .. "/../../.."
	package.path = bin .. "/?.lua;" .. bin .. "/?/init.lua;" .. package.path
end

setup_path()

local kxpc = require("kxpc")
local ksys = require("ksys")

local _prog, _script, NAME, TR = ksys.get_args()
NAME = NAME or "xdemo1"
TR = TR or "msgq"
local NAME_MAX = 107
do
	local ok, cfger = pcall(require, "klbcore.klbxpc.cfger")
	if ok and "table" == type(cfger) and "number" == type(cfger.NAME_MAX) then
		NAME_MAX = cfger.NAME_MAX
	end
end
if NAME_MAX < #NAME then
	print("FAIL name too long", NAME, NAME_MAX)
	ksys.exit(1)
	return
end

local mo = nil
if "unix" == TR then
	mo = kxpc.new_module_unix(NAME)
elseif "npipe" == TR then
	mo = kxpc.new_module_npipe(NAME)
elseif "sysmq" == TR then
	mo = kxpc.new_module_sysmq(NAME)
else
	mo = kxpc.new_module(NAME)
	TR = "msgq"
end

if not mo then
	print("FAIL new_module", NAME, TR)
	ksys.exit(1)
	return
end

print("[xpc-demo server] ready name=", mo:name(), "owner=", mo:is_owner(), "tr=", TR)

while true do
	local t, src, corr, name, a = mo:recv(500)
	if nil == t then
		-- timeout
	elseif kxpc.REQUEST == t then
		print("[xpc-demo server] REQUEST", name, a)
		if "echo" == name then
			mo:response(src, corr, true, a)
		else
			mo:response(src, corr, false, "unknown")
		end
	elseif kxpc.POST == t then
		print("[xpc-demo server] POST", name, a)
		if "quit" == name then
			print("[xpc-demo server] quit")
			mo:close()
			ksys.exit(0)
			return
		end
	elseif kxpc.NOTIFY == t then
		print("[xpc-demo server] NOTIFY", name, a)
	else
		print("[xpc-demo server] other type=", t, "src=", src, "name=", name)
	end
end
