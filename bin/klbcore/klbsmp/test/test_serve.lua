local kco = require("kco")
local klbsmp = require("klbcore.klbsmp")


local port = 3456


-- 服务连接
local function CoOnSmpServeConn(ser_conn)
	
	
	ser_conn:disconnect()
end


kco.fork(function ()
	-- 监听
	local listen = klbsmp.listen(port)
	print('klbsmp.listen', port)
	
	while true do
		-- 接收连接
		local ser_conn = listen:co_accept()	
		print('smp serve connect :', ser_conn)
		
		-- 新建一个协程 处理新连接
		kco.fork(CoOnSmpServeConn, ser_conn)
	end
	
end)
