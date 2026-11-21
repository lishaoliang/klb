--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   demo/client.lua
-- @brief  xpc 双进程手测 - 客户端
-- @note   <klb>/lib/klua client.lua [mod_name] [transport]
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
local cli = nil

local function open_peer()
	if "unix" == TR then
		return kxpc.open_unix(NAME)
	end

	if "npipe" == TR then
		return kxpc.open_npipe(NAME)
	end

	if "sysmq" == TR then
		return kxpc.open_sysmq(NAME)
	end

	TR = "msgq"
	return kxpc.open(NAME)
end

for i = 1, 100 do
	cli = open_peer()
	if cli then
		break
	end
	os.execute("sleep 0.05")
end

if not cli then
	print("FAIL open", NAME, TR)
	ksys.exit(1)
	return
end

print("[xpc-demo client] opened local=", cli:name(), "peer=", NAME, "tr=", TR)

local ok = cli:post(NAME, "hello", 1)
print("[xpc-demo client] post", ok)
if not ok then
	cli:close()
	print("FAIL post")
	ksys.exit(2)
	return
end

local ok2, v = cli:request(NAME, 3000, "echo", "world")
print("[xpc-demo client] echo", ok2, v)
if true ~= ok2 or "world" ~= v then
	cli:close()
	print("FAIL echo")
	ksys.exit(3)
	return
end

ok = cli:notify(NAME, "bye", 0)
print("[xpc-demo client] notify", ok)
if not ok then
	cli:close()
	print("FAIL notify")
	ksys.exit(4)
	return
end

ok = cli:post(NAME, "quit")
print("[xpc-demo client] quit post", ok)

cli:close()
print("[xpc-demo client] OK")
ksys.exit(0)
return
