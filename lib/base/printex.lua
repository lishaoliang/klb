
local cjson = require("cjson")


local copy_to_string_table = nil
copy_to_string_table = function(t)
	local dst = {}
	
	for k, v in pairs(t) do
		if type(v) == 'table' then
			dst[k] = copy_to_string_table(v)
		elseif type(v) == 'string' or type(v) == 'number' or type(v) == 'boolean' then
			dst[k] = v
		else
			dst[k] = tostring(v)
		end
	end
	
	return dst
end

local printex = function (...)
	local arg = {...}	
	local tmp = copy_to_string_table(arg)
	
	print(cjson.encode(tmp))
end

return printex
