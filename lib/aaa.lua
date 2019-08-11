

local string = require("string")
local lfs = require("lfs")

local kg_thread = require("kg_thread")
local kg_os = require("kg_os")
local kg_time = require("kg_time")

print(lfs.currentdir())


--kg_os.shell()

--local count = 4
--while 0 < count do
--	count = count - 1
	
	kg_thread.create('test_1', 'bbb')
--end




kin = function ()
	return 0
end


--kgo = function ()
--	print('aaa, kgo')
--	
--	print('aaa, kgo')
--	
--	return 0
--end


kexit = function ()

	return 0
end

for i = 0, 100 do
	kg_thread.post('test_1', 'post', tostring(i), '123456')
end

	
--kg_time.sleep(1000)


kg_thread.destroy('test_1')


print('post', 100)
for i = 100, 200 do
	kg_thread.post('test_1', 'post', tostring(i), '123456')
end


print('sleep', 1000)
kg_time.sleep(1000)
print('sleep over', 1000)