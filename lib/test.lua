
local kos = require("kos")
local ktime = require("ktime")
local kthread = require("kthread")

local gothread = require("gothread")

print(kos.os, kos.arch)


kin = function ()
	print('kin')
	
	return 0
end


kgo = function ()
	print('kgo')
	--gothread.hello()


	for i = 1, 1 do
		gothread.create("./aaa.lua")
	end	
	
	return 0
end


kexit = function ()
	print('kexit')
end
