local kco = require("kco")
local klbsmp = require("klbcore.klbsmp")


local host = '127.0.0.1'
local port = 3457



kco.fork(function ()
	
	local client = klbsmp.connect_rpc(host, port)
	print('smp-rpc client', client)

	--client:send_text('123', '456')


end)

