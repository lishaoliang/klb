
local ksys = require("ksys")
local kkpa = require("kkpa")
local krand = require("krand")


local kpa_w = kkpa.open_w('./tmp_package.kpa')

if kpa_w then	
	for i = 1, 100 do
		kpa_w:write(tostring(i), krand.rand_string(33))
	end

	kpa_w:close()
end

local kpa_r = kkpa.open_r('./tmp_package.kpa')

if kpa_r then
	

	kpa_r:close()
end


ksys.exit()
