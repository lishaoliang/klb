
local kos = require("kos")
local kd_demo = require("kd_demo")

print(kos.os, kos.arch)


kin = function ()
	--print('kin')
	
	return 0
end


kgo = function ()
	--print('kd_demo')
	
	print('lua:', kd_demo.hello())
	
	print('lua:', kd_demo.test('test1', 'test2'))
	
	return 0
end


kexit = function ()
	--print('kexit')
	
	return 0
end