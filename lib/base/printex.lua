
local cjson = require("cjson")


local printex = function (...)
	local arg = {...}
	print(cjson.encode(arg))
end

return printex
