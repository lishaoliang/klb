--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   status.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  unsafe.status
-- @history 修改历史
--  \n [2026-10] 从 init.lua 拆出
--]]


local M = {}


local function _status(_args)
	return {
		available = true,
		tools = {
			"unsafe.run_file",
			"unsafe.exec",
			"unsafe.process_list",
			"unsafe.process_kill",
		},
		message = "run_file uses kthread isolated env; exec/process_* require profile unsafe; process_* POSIX-only; win32 exec: cmd semantics, no timeout enforcement",
	}
end


M.register = function (catalog, dispatch)
	local ok, err = catalog.register({
		name = "unsafe.status",
		category = "unsafe",
		title = "unsafe capability status",
		summary = "Report unsafe tools (run_file, exec). Needs profile unsafe.",
		min_profile = "unsafe",
		args_schema = {},
		examples = {},
		docs = "tools/unsafe.md",
		side_effects = { "unsafe" },
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("unsafe.status", _status)
	if not ok then
		return false, err
	end

	return true
end


return M

-- end
