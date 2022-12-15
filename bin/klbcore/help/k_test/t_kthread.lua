
local kthread = require("kthread")
local ksys = require("ksys")
local ktime = require("ktime")


local t1 = kthread.start('help_lua.k_test.t_kthread_entry', true, -1, 'abc', 123, 3.1415926, {a=1,b=2})
print('kthread.start', t1)


ktime.timer(3000, function ()
	print('kthread.stop', t1)
	kthread.stop(t1)
	
	print('t_kthread ksys.exit()')
	ksys.exit()
end)
