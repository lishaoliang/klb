local io = require("io")
local ksys = require("ksys")
local http = require("base.http")

local host = 'https://www.baidu.com'
local host = 'https://www.163.com'

http.get(host, '', function (body, status)
	print(status)
	print(body)
	
	local f = io.open('tmp_khttp.html', 'wb')
	if f then
		f:write(body)
		f:close()
	end
	
	ksys.exit()
end)
