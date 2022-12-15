

local kco = require("kco")
local ksys = require("ksys")


kco.fork(function (...)
	
	print('-->>', ...)
	
	kco.co_sleep(1000)
	
	print('-->>', ...)
	
	kco.co_sleep(1000)
	
	print('-->>', ...)

end, 'kco.fork', 1, 'abc', 3.1415926, true, {})



kco.timeout(3000, function (...)
	
	print('==>>', ...)

end, 'kco.timeout', 4000, 'abc', 3.1415926)


kco.timeout(4000, function ()
	print('====>>>> ksys.exit()')
	ksys.exit()
end)
