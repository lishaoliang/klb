
local kg_time = require("kg_time")
local ka_mdc = require("ka_mdc")


ka_mdc.init()
ka_mdc.start()


kexit = function ()
	ka_mdc.stop()
	ka_mdc.quit()
	
	return 0
end


local count = 30
while 0 < count do
	--count = count - 1
	
	kg_time.sleep(1000)
end
