local kco = require("kco")
local klbsmp = require("klbcore.klbsmp")


local port = 3457


local function CoEcho(ser_conn, ...)
	local args = {...}
	
	return ...
end


local function CoAdd(ser_conn, ...)
	local args = {...}
	
	local num = 0
	for _, v in ipairs(args) do
		num = tonumber(v)
	end
	
	return num
end


local function CoStrCat(ser_conn, ...)
	local args = {...}
	
	local t = {}
	for _, v in ipairs(args) do
		table.insert(t, tostring(v))
	end
	
	local s = table.concat(t)
	
	return s
end


local function CoUnsupported(ser_conn, ...)
	return ''
end


-- 服务连接
local function CoOnSmpServeConnRPC(ser_conn)
	
	local func_map = {
		['echo'] = CoEcho,
		['add'] = CoAdd,	
		['strcat'] = CoStrCat,
	}
	
	local CoRecv = function (func_name, ...)
		print('smp-rpc recv : ', func_name, ...)	
		local state = ser_conn:status()
		
		local func = func_map[tostring(func_name)] or CoUnsupported
		
		if klbsmp.RPC_REQUEST == state.method then
			ser_conn:response(func(ser_conn, ...))
		else
			func(ser_conn, ...)
		end
	end

	while true do
		CoRecv(ser_conn:co_recv())
	end
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
		kco.fork(CoOnSmpServeConnRPC, ser_conn, 'rpc')
	end
	
end)
