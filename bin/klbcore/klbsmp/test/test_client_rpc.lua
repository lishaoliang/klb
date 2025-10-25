local kco = require("kco")
local klbsmp = require("klbcore.klbsmp")


local host = '127.0.0.1'
local port = 3457



kco.fork(function ()
	
	local client = klbsmp.connect_rpc(host, port)
	print('smp-rpc client', client)

	--client:post('abc', 'fgh', 789, 3.1415926)

	client:post('add', 1, 2, 3, 4, 5, 6)
	client:post('strcat', 'abc', '1', '2', '3', '4', '5', '6')

	print('strcat', client:co_call('strcat', 'abc', 'qwe', 'iop'))
	print('add', client:co_call('add', 10, 20, 30, 40))
	print('echo', client:co_call('echo', 1, 2, 3, 'abc', 3.1415926))
	print('aaa', client:co_call('aaa', 1, 2, 3))
	
	local url = 'smprpc://127.0.0.1:3457'
	
	
end)
