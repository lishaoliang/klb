--[[
-- Copyright(c) 2020, LGPL All Rights Reserved
-- @brief  服务端入口
-- @author 李绍良
--]]
local kos = require("kos")
local kg_ctx = require("kg_ctx")
local kg_lsmfw = require("kg_lsmfw")
local kg_smcfw = require("kg_smcfw")
local kg_simulate = require("kg_simulate")

local kmnp_dev = require("kmnp_dev")
local kgui = require("kgui")

local kg_time = require("kg_time")


local conf = require("conf")
local worker = require("server.worker")


-- 打印系统信息
print('os', kos.os, kos.arch)


kin = function ()
	
	
	if 'windows' == kos.os then
		kmnp_dev.dlopen('./libsdl_dev.dll')
	end

	print(kmnp_dev.version(), kmnp_dev.project())


	worker.init()

	local lsmfw = 'server.worker.lsmfw'
	local smcfw = 'server.worker.smcfw'
	local preload = 'klua_go_main_openlibs'

	kg_lsmfw.init('')
	kg_smcfw.init('')
	--kg_simulate.init('')
	kmnp_dev.init('')

	--kg_simulate.set_path_media(conf.path_media)

	local cb = kg_smcfw.get_stream_cb()
	--kg_simulate.set_stream_receiver(cb)
	kmnp_dev.set_stream_receiver(0, cb)
	
	
	kg_lsmfw.start(lsmfw, lsmfw, preload, '')
	
	kg_lsmfw.listen('tcp:80','tcp', ':' .. tostring(conf.port))
	kg_lsmfw.listen_tls('tcp:443','tcp', ':' .. tostring(conf.tls_port), conf.tls_cert, conf.tls_key)	

	-- todo 线程启动并需要一定时间, demo在初始化里面干了一下活
	kg_time.sleep(10)
	
	kg_smcfw.start(smcfw, smcfw, preload, '')
	--kg_simulate.start()
	kmnp_dev.start('')

	-- ui
	local canvas = kmnp_dev.get_canvas(0)
	if canvas then
		kgui.init('')
		kgui.attach_canvas(kmnp_dev.get_canvas(0))
		
		kgui.start('server.entry_gui', 'server.entry_gui', kg_ctx.get_preload('klua_go_main_openlibs'), nil)
		kmnp_dev.set_msg_receiver(0, kgui.get_msg_callback())	
	end
	
	return 0
end


kgo = function (msg, msgex, lparam, wparam)
	
	return 0
end


kexit = function ()
	
	kgui.stop()
	kgui.quit()

	kg_lsmfw.close_listen('tcp:80')
	kg_lsmfw.close_listen('tcp:443')
	
	kmnp_dev.stop()
	--kg_simulate.stop()
	kg_smcfw.stop()
	kg_lsmfw.stop()
	
	
	kmnp_dev.quit()
	--kg_simulate.quit()
	kg_smcfw.quit()
	kg_lsmfw.quit()
	worker.quit()

	kmnp_dev.dlclose()
	print('server exit.')
	return 0
end
