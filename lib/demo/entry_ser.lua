local ka_mdc = require("ka_mdc")
local cjson = require("cjson.safe")


kgo = function (name, msgex, extra, msg)
	--print('loader_ser.lua', name, msgex, extra, msg)	
	local ex = cjson.decode(msgex)
	local req = cjson.decode(msg)
	
	local E = {}
	local chnn = ((req or E).open_stream or E).chnn
	local idx = ((req or E).open_stream or E).idx

	if nil ~= chnn and nil ~= idx then
		ka_mdc.open_stream(name, chnn, idx)
	end	

	ka_mdc.send(name, ex['sequence'], ex['uid'], extra, msg)
	
	return 0
end
