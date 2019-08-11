

local string = require("string")
local kos = require("kos")
local kd_demo = require("kd_demo")
local ka_net = require("ka_net")
local kx_net = require("kx_net")


print(_KLUA_ENV_PTR_)

kin = function ()
	print('aaa, kin')
	print(kos.os, kos.arch)

	return 0
end


kgo = function ()
	print('aaa, kgo')
	--gothread.hello()
	
	local str, b = kd_demo.hello()
	print("lua:", str, b)
	
	local a1, a2 = kd_demo.test("123456", "789987")
	print("lua:", a1, a2)
	
	ka_net.hello()
	kx_net.hello()
	return 0
end


kexit = function ()
	print('aaa, kexit')
	
	return 0
end
