local ka_mdc = require("ka_mdc")
local ka_ipc = require("ka_ipc")
local kg_time = require("kg_time")

ka_mdc.init()
ka_ipc.init()

ka_ipc.set_stream_cb(ka_mdc.get_cb_push_stream())

ka_mdc.start('test_mdc', 'demoi.entry_mdc', 'klua_go_main_openlibs', '')
ka_ipc.start()


kexit = function ()
	print('kexit.')

	ka_ipc.stop()
	ka_mdc.stop()

	ka_ipc.quit()
	ka_mdc.quit()
	
	print('kexit...')
	return 0
end
