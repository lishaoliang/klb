local kco = require("kco")
local klbsmp = require("klbcore.klbsmp")


local port = 3457


-- 服务连接
local function CoOnSmpServeConnRPC(ser_conn)
	
	
	ser_conn:disconnect()
end


kco.fork(function ()
	-- 监听
	local listen = klbsmp.listen_rpc(port)
	print('klbsmp.listen_rpc', port)
	
	while true do
		-- 接收连接
		local ser_conn = listen:co_accept()	
		print('smp-rpc serve connect :', ser_conn)
		
		-- 新建一个协程 处理新连接
		kco.fork(CoOnSmpServeConnRPC, ser_conn)
	end
	
end)
