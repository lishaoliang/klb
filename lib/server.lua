--[[
-- Copyright(c) 2020, LGPL All Rights Reserved
-- @brief  服务端入口
-- @author 李绍良
--]]
local kos = require("kos")
local kg_lsmfw = require("kg_lsmfw")
local kg_smcfw = require("kg_smcfw")
local kg_simulate = require("kg_simulate")

local kg_time = require("kg_time")


local conf = require("conf")
local worker = require("server.worker")


-- 打印系统信息
print('os', kos.os, kos.arch)


kin = function ()
	worker.init()

	local lsmfw = 'server.worker.lsmfw'
	local smcfw = 'server.worker.smcfw'
	local preload = 'klua_go_main_openlibs'

	kg_lsmfw.init('')
	kg_smcfw.init('')
	kg_simulate.init('')

	kg_simulate.set_path_media(conf.path_media)

	local cb = kg_smcfw.get_stream_cb()
	kg_simulate.set_stream_receiver(cb)
	
	
	kg_lsmfw.start(lsmfw, lsmfw, preload, '')
	
	kg_lsmfw.listen('tcp:80','tcp', ':' .. tostring(conf.port))
	kg_lsmfw.listen_tls('tcp:443','tcp', ':' .. tostring(conf.tls_port), conf.tls_cert, conf.tls_key)	

	-- todo 线程启动并需要一定时间, demo在初始化里面干了一下活
	kg_time.sleep(10)
	
	kg_smcfw.start(smcfw, smcfw, preload, '')
	kg_simulate.start()
	
	return 0
end


kgo = function (msg, msgex, lparam, wparam)
	
	return 0
end


kexit = function ()

	kg_lsmfw.close_listen('tcp:80')
	kg_lsmfw.close_listen('tcp:443')
	
	kg_simulate.stop()
	kg_smcfw.stop()
	kg_lsmfw.stop()
	
	
	kg_simulate.quit()
	kg_smcfw.quit()
	kg_lsmfw.quit()
	worker.quit()

	print('server exit.')
	return 0
end
