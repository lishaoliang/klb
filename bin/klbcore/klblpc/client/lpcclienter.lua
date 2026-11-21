--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   client/lpcclienter.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klblpc 长期 LPC 客户端
-- @note   包装 klpc.new; 可多次 :co_call / :post; 结束 :close
-- @history 修改历史
--		[2026-10] 创建文件
--]]
local klpc = require("klpc")


local lpcclienter = {}


-- @brief 新建可长期复用的 LPC 客户端
-- @return [table]						klpc 客户端对象; 失败为 nil
-- @note 对象方法: `:co_call(mo_name, ...)` / `:post(mo_name, ...)` / `:close`
lpcclienter.new_client = function ()
	return klpc.new()
end


return lpcclienter
