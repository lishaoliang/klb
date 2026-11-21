--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   parallel.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  runtime.parallel
-- @note   进程内多工具编排 (同进程 dispatch; Lua 单线程故串行执行 calls)
-- @history 修改历史
--  \n [2026-10] 从 init.lua 拆出
--]]


local session = require("klbagent.core.session")


local M = {}


local PARALLEL_MAX = 32


-- @brief 进程内多工具编排 (同进程 dispatch; Lua 单线程故串行执行 calls)
local function _parallel(args)
	args = args or {}
	if session.parallel_depth() > 0 then
		error("nested runtime.parallel denied")
	end

	local calls = args.calls
	if type(calls) ~= "table" then
		error("need calls array")
	end

	local n = #calls
	if n < 1 then
		error("calls empty")
	end

	if n > PARALLEL_MAX then
		error("calls max " .. tostring(PARALLEL_MAX))
	end

	local dispatch = require("klbagent.core.dispatch")
	local sid = args.session_id
	if type(sid) == "string" and sid ~= "" then
		if session.get(sid) == nil then
			error("session not found")
		end
	else
		sid = nil
	end

	session.parallel_enter(1)
	local results = {}
	local ok_all = true

	for i = 1, n do
		local c = calls[i]
		if type(c) ~= "table" then
			session.parallel_enter(-1)
			error("calls[" .. tostring(i) .. "] must be table")
		end

		local name = c.name
		if type(name) ~= "string" or name == "" then
			session.parallel_enter(-1)
			error("calls[" .. tostring(i) .. "].name required")
		end

		if name == "runtime.parallel" then
			session.parallel_enter(-1)
			error("nested runtime.parallel denied")
		end

		local call_args = c.args
		if type(call_args) ~= "table" then
			call_args = {}
		end

		local req = {
			name = name,
			args = call_args,
			id = c.id,
		}
		if sid ~= nil then
			req.session_id = sid
		end

		local ok, result = dispatch.call(req)
		local item = {
			index = i,
			name = name,
			ok = ok,
		}
		if c.id ~= nil then
			item.id = c.id
		end

		if ok then
			if type(result) == "table" then
				item.result = result
			else
				item.result = { value = result }
			end
		else
			ok_all = false
			if type(result) == "table" then
				item.error = result
			else
				item.error = { message = tostring(result) }
			end
		end

		results[#results + 1] = item
	end

	session.parallel_enter(-1)

	return {
		ok = ok_all,
		mode = "inproc",
		count = #results,
		results = results,
	}
end


M.register = function (catalog, dispatch)
	local ok, err = catalog.register({
		name = "runtime.parallel",
		category = "runtime",
		title = "in-process multi-tool fan-in",
		summary = "Run calls[] via same-process dispatch (Lua single-thread: serial inproc). Optional session_id. Max 32. No nesting.",
		args_schema = {
			calls = "array of {name, args?, id?}",
			session_id = "string optional bind all child calls",
		},
		examples = {
			{
				args = {
					calls = {
						{ name = "runtime.ping", id = 1 },
						{ name = "runtime.cwd", id = 2 },
					},
				},
			},
		},
		docs = "tools/runtime.md",
		side_effects = {},
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("runtime.parallel", _parallel)
	if not ok then
		return false, err
	end

	return true
end


return M

-- end
