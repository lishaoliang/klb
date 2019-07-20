

local string = require("string")
local lfs = require("lfs")

local kos = require("kos")
local ktime = require("ktime")
local kthread = require("kthread")

local gothread = require("gothread")

print('aaa', kos.os, kos.arch)

print('123456')
print(lfs.currentdir())


kin = function ()
	print('aaa, kin')

	return 0
end


kgo = function ()
	print('aaa, kgo')
	--gothread.hello()
	return 0
end


kexit = function ()
	print('aaa, kexit')
end
