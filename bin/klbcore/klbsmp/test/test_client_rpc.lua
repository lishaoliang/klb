local kco = require("kco")
local klbsmp = require("klbcore.klbsmp")


local host = '127.0.0.1'
local port = 3457



kco.fork(function ()
	
	local client = klbsmp.connect_rpc(host, port)
	print('smp-rpc client', client)

	--client:post('abc', 'fgh', 789, 3.1415926)

	client:post('add', 1, 2, 3, 4, 5, 6)
	client:post('strcat', '1', '2', '3', '4', '5', '6')

end)

