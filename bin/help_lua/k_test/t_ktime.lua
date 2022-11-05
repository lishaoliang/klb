

local ktime = require("ktime")
local ksys = require("ksys")


-- step1 
print('-->', ktime.tick_count())
ktime.sleep(1000)
print('-->', ktime.tick_count())
ktime.sleep(1000)
print('-->', ktime.tick_count())


-- step2
ktime.timer(500, function ()
	print('--> timer', ktime.tick_count())
end)


-- step3
local abc_count = 1
ktime.ticker('abc', 1000, function ()
	print('--> ticker.abc.' .. tostring(abc_count), ktime.tick_count())
	
	abc_count = abc_count + 1
	if 10 < abc_count then
		ktime.stop_ticker('abc')
		
		-- 等待1000毫秒后, 退出
		ktime.timer(1000, function ()
			print('====>>>> ksys.exit()')
			ksys.exit()
		end)
	end
end)

