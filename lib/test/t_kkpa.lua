
local ksys = require("ksys")
local kkpa = require("kkpa")
local krand = require("krand")



local data = {}

for i = 1, 300 do
	data[i] = krand.rand_string(64)
end

local kpa_w = kkpa.open_w('./tmp_package.kpa')

if kpa_w then
	for i = 1, #data do
		kpa_w:write(tostring(i), data[i])
	end

	kpa_w:close()
end

local kpa_r = kkpa.open_r('./tmp_package.kpa')

if kpa_r then
	
	assert(#data == kpa_r:size())
	
	for i = 1, kpa_r:size() do
		local k, v = kpa_r:read(i)
		print(k, v)
		assert(data[tonumber(k)] == v)	
	end	

	kpa_r:close()
end


ksys.exit()
