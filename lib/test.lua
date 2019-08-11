
local kos = require("kos")
local ktime = require("ktime")
local kthread = require("kthread")

print(kos.os, kos.arch)


kin = function ()
	print('kin')
	
	return 0
end


kgo = function ()
	print('kgo')
	--gothread.hello()
	
	return 0
end


kexit = function ()
	print('kexit')
	
	return 0
end
