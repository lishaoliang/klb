
local kg_thread = require("kg_thread")
local kg_time = require("kg_time")
local kos = require("kos")

local kg_stream = require("kg_stream")


print(kos.os, kos.arch)
print(lfs.currentdir())

kg_stream.init()
kg_stream.start()

print('sleep', 3000)
kg_time.sleep(3000)

kg_stream.stop()
kg_stream.quit()


assert(false)


local b, sid = kg_thread.create('bbb')

for i = 0, 100 do
	kg_thread.post(sid, 'post', tostring(i), '123456', '123456')
end

--[[
kgo = function (msg, msgex, lparam, wparam)
	print('lua:', msg, msgex, lparam, wparam)
	return 0
end
--]]

kg_thread.destroy(sid)


print('post', 100)
for i = 100, 200 do
	kg_thread.post(sid, 'post', tostring(i), '123456', '123456')
end


print('sleep', 1000)
kg_time.sleep(1000)



