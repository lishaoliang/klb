--[[
-- Copyright(c) 2020, LGPL All Rights Reserved
-- @brief  �����߳�
-- @author ������
--]]
local kg_thread = require("kg_thread")

local worker = {}


worker.init = function ()

end


worker.quit = function ()

end


worker.post = function (name, msg, msgex, lparam, wparam)
	return kg_thread.post(name, msg, msgex, lparam, wparam)
end


return worker
