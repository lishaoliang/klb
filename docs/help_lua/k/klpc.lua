--[[
-- Copyright(c) 2022, LGPL All Rights Reserved
-- @file   klpc.lua
-- @brief  C klpc, Local Procedure Call Protocol
--   \n require("klpc")
--   \n C导出文件: ./klb/src_c/klua/klua_base/klua_klpc.c
--   \n 本地夸线程通信
--   \n 定义模块, 访问模块
-- @version 0.1
--]]

local klpc = {}


-- @brief 新建一个导出模块(newmetatable)
-- @param [in]	name[string]		模块名称(不可重复)
-- @return module对象
-- @note 
klpc.new_module = function (name)
	local mo = {}
	
	-- @brief 关闭
	-- @return 无
	-- @note 显示关闭, 可提前释放非Lua相关的资源(内存, 文件句柄等)
	--		不显示关闭, 则需要等待gc才释放
	mo:close = function ()
		return
	end
	
	-- @brief 读取(LPC)消息
	-- @return [string]src		请求(call/post)来源名称
	--			[string]msg		消息: 通过 ksys.unpack 解压出具体信息
	-- @note 仅在协程中使用
	--		eg. local src, msg = mo:co_recv()
	--			local r1, r2 = ksys.unpack(msg)
	--			...
	--			mo:response(src, '123', true)
	mo:co_recv = function ()
		local src ='aaa'	-- 请求(call/post)来源名称
		local msg = '111'	-- 消息: 通过 ksys.unpack 解压出具体信息
		return src, msg
	end

	-- @brief 回复请求
	-- @param [in]	src[string]		请求来源名称: 
	-- @param [in]	...[任意类型]	回复的数据
	-- @return 无
	mo:response = function (src, ...)
		return
	end
	
	return mo
end


-- @brief 新建一个lpc对象
-- @return lpc对象
klpc.new = function ()
	local lpc = {}

	-- @brief 关闭
	-- @return 无
	-- @note 显示关闭, 可提前释放非Lua相关的资源(内存, 文件句柄等)
	--		不显示关闭, 则需要等待gc才释放
	lpc:close = function ()
		return
	end

	-- @brief 向某个模块发送消息,并等待返回数据
	-- @param [in]	mo_name[string]	目标模块名称
	-- @param [in]	...[任意类型]	请求数据
	-- @return [...] 对方回复的数据
	-- @note 仅在协程中使用
	lpc:co_call = function (mo_name, ...)
		return ...
	end
	
	return lpc
end


-- @brief 向某个模块post消息(不等待返回)
-- @param [in]	mo_name[string]	目标模块名称
-- @return [boolean] 是否成功
klpc.post = function (mo_name, ...)
	return true
end


return klpc


-- demo
--[[

local kco = require("kco")
local klpc = require("klpc")
local ksys = require("ksys")

local mo_name = 'aaa'
local mo_aaa = klpc.new_module(mo_name)
print('new_module', mo_aaa, mo_name)

kco.fork(function ()
	local loop = true
	
	while loop do
		local src, recv = mo_aaa:co_recv()
		
		-- 收到消息, 另开协程处理, 以免阻塞接收数据协程
		kco.fork(function ()
			
			local func_map = {
				strcat = function (...)
					local param = {...}
					local s = ''
					for _, v in pairs(param) do
						if 'string' == type(v) then
							s = s .. v
						end
					end
					return s
				end,
				
				add = function (...)
					local param = {...}
					local ret = 0
					for _, v in pairs(param) do
						if 'number' == type(v) then
							ret = ret + v
						end
					end
					return ret
				end,
				
				printf = function (...)
					print(mo_name .. '.printf', ...)
				end
			}
			
			func_map['end'] = function ()
				print('end')
				loop = false
			end
			
			local on_msg = function (src, msg, ...)	
				if 'string' == type(msg) and func_map[msg] then	
					mo_aaa:response(src, func_map[msg](...))
				else
					mo_aaa:response(src, ...)
				end
			end
			
			on_msg(src, ksys.unpack(recv))
		end)
	end
	
	mo_aaa:close()
end)


kco.fork(function ()
	
	klpc.post(mo_name, 'printf', '1', 'a', true)
	
	local lpc = klpc.new()

	print('co_call(strcat)', lpc:co_call(mo_name, 'strcat', '123', '456', '789', 'abc'))
	print('co_call(add)', lpc:co_call(mo_name, 'add', 1, 2, 3, 4, 5, 6, 7, 8, 9, 10))
	
	lpc:close()
	
	klpc.post(mo_name, 'end')
end)

--]]
