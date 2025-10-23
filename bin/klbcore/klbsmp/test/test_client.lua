local kco = require("kco")
local klbsmp = require("klbcore.klbsmp")


local host = '127.0.0.1'
local port = 3456



kco.fork(function ()
	
	local client = klbsmp.connect(host, port)
	print('smp client', client)

	client:send_text('123', '456')


end)

