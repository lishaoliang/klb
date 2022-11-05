

local kco = require("kco")
local klpc = require("klpc")
local ksys = require("ksys")


local mo_name = '/aaa'

-- 创建模块
local mo = klpc.new_module(mo_name)

kco.fork(function ()
	
	local dispatch = function (name, msg, ...)
		print('mo:\"'.. mo_name .. '\",recv:', msg, ...)
		
		mo:response(name, msg, ...) -- 回应消息
		
		if 'close' == msg then
			return false
		else
			
		end
		
		return true
	end	
	
	while true do
		-- 分派消息
		if  not dispatch(mo:co_recv()) then
			break
		end
	end	
	
	mo:close()	-- 关闭
end)



-- lpc 发起调用方
-- 可以夸线程使用, 测试例子仅为方便了解
kco.fork(function ()
	local lpc = klpc.new()

	print(lpc:co_call(mo_name, 'a', 123, true, 'abcde', 3.14))
	print(lpc:co_call(mo_name, 'b', 234, false, 'abcde', 3.14))
	print(lpc:co_call(mo_name, 'c', 345, true, 'abcde', 3.14))
	print(lpc:co_call(mo_name, 'd', 456, false, 'abcde', 3.1415926))
	
	lpc:co_call(mo_name,'close')
	
	print('====>>>> ksys.exit()')
	ksys.exit()
end)
