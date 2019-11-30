
local ka_mdc = require("ka_mdc")
local kg_stream = require("kg_stream")

print('demo.demo.lua,start...')

ka_mdc.init()
kg_stream.init()


kg_stream.set_stream_cb(ka_mdc.get_cb_push_stream())


ka_mdc.start('test_mdc', 'demo.entry_ser', 'klua_go_main_openlibs', '')
kg_stream.start()


kexit = function ()
	kg_stream.stop()
	ka_mdc.stop()

	kg_stream.quit()
	ka_mdc.quit()
	
	return 0
end

kgo = function (msg, msgex, lparam, wparam)
	print('lua:', msg, msgex, lparam, wparam)
	
	return 0
end
