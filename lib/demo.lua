
local kos = require("kos")
print(kos.os, kos.arch)


kin = function ()
	--print('kin')
	
	return 0
end


kgo = function (msg, msgex, lparam, wparam)
	print('lua:', msg, msgex, lparam, wparam)
	return 0
end


kexit = function ()
	print('kexit.')
	print('kexit..')
	
	print('kexit...')
	return 0
end
