--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  进程内 session (关联 id / 调用上下文)
-- @note   仅本进程有效 (serve/stdio/MCP 多轮); 一次性 alua call 退出即失
--   \n 仅宿主内调; 无 runtime.session_* 对外工具; 身份回显见 ssid.whoami
--   \n 并行编排见 runtime.parallel; 勿预埋重层
-- @history 修改历史
--  \n [2026-09] 创建文件
--  \n [2026-11] 对外工具删除; 本模块仅内调
--]]


local M = {}


local sessions = {}
local seq = 0
local parallel_depth = 0
local current_id = nil


local function _now_ms()
	return math.floor(os.time() * 1000)
end


local function _new_id()
	seq = seq + 1
	return string.format("s%x-%d-%d", os.time(), seq, math.random(1000, 9999))
end


-- @brief 创建 session
-- @param [in] opts[table|nil]		可选 attrs[table]
-- @return info[table]				{ id, created_ms, attrs, call_count, last_tool }
M.begin = function (opts)
	opts = opts or {}
	local attrs = {}
	if type(opts.attrs) == "table" then
		for k, v in pairs(opts.attrs) do
			if type(k) == "string" then
				attrs[k] = v
			end
		end
	end

	local id = _new_id()
	local info = {
		id = id,
		created_ms = _now_ms(),
		attrs = attrs,
		call_count = 0,
		last_tool = "",
	}
	sessions[id] = info
	if opts.set_current ~= false then
		current_id = id
	end

	return {
		id = info.id,
		created_ms = info.created_ms,
		attrs = info.attrs,
		call_count = info.call_count,
		last_tool = info.last_tool,
	}
end


-- @brief 设置当前 session (本进程默认 whoami 目标)
-- @param [in] id[string|nil]
-- @return ok[boolean]
-- @return err[string|nil]
M.set_current = function (id)
	if id == nil or id == "" then
		current_id = nil
		return true
	end

	if type(id) ~= "string" then
		return false, "need session_id"
	end

	if sessions[id] == nil then
		return false, "session not found"
	end

	current_id = id
	return true
end


-- @brief 当前 session id
-- @return id[string|nil]
M.current_id = function ()
	return current_id
end


-- @brief 结束 session
-- @param [in] id[string]
-- @return ok[boolean]
-- @return err[string|nil]
M.end_session = function (id)
	if type(id) ~= "string" or id == "" then
		return false, "need session_id"
	end

	if sessions[id] == nil then
		return false, "session not found"
	end

	sessions[id] = nil
	if current_id == id then
		current_id = nil
	end

	return true
end


-- @brief 查询 session (副本)
-- @param [in] id[string]
-- @return info[table|nil]
M.get = function (id)
	if type(id) ~= "string" or id == "" then
		return nil
	end

	local s = sessions[id]
	if s == nil then
		return nil
	end

	local attrs = {}
	for k, v in pairs(s.attrs) do
		attrs[k] = v
	end

	return {
		id = s.id,
		created_ms = s.created_ms,
		attrs = attrs,
		call_count = s.call_count,
		last_tool = s.last_tool,
	}
end


-- @brief 合并 attrs
-- @param [in] id[string]
-- @param [in] attrs[table]
-- @return ok[boolean]
-- @return err[string|nil]
M.set_attrs = function (id, attrs)
	if type(id) ~= "string" or id == "" then
		return false, "need session_id"
	end

	local s = sessions[id]
	if s == nil then
		return false, "session not found"
	end

	if type(attrs) ~= "table" then
		return false, "attrs must be table"
	end

	for k, v in pairs(attrs) do
		if type(k) == "string" then
			if v == nil then
				s.attrs[k] = nil
			else
				s.attrs[k] = v
			end
		end
	end

	return true
end


-- @brief 记录一次工具调用 (dispatch 接线)
-- @param [in] id[string|nil]
-- @param [in] tool_name[string|nil]
-- @return ok[boolean]
M.note_call = function (id, tool_name)
	if type(id) ~= "string" or id == "" then
		return false
	end

	local s = sessions[id]
	if s == nil then
		return false
	end

	s.call_count = s.call_count + 1
	if type(tool_name) == "string" then
		s.last_tool = tool_name
	end

	return true
end


-- @brief 并行编排深度 (防 runtime.parallel 嵌套)
-- @return depth[number]
M.parallel_depth = function ()
	return parallel_depth
end


-- @brief 进入/退出 parallel 编排
-- @param [in] delta[number]		+1 / -1
M.parallel_enter = function (delta)
	parallel_depth = parallel_depth + (delta or 0)
	if parallel_depth < 0 then
		parallel_depth = 0
	end
end


-- @brief 回显会话摘要 (当前或指定 id; 含 attrs 指纹字段)
-- @param [in] opts[table|nil]		session_id?
-- @return info[table|nil]
-- @return err[string|nil]
M.whoami = function (opts)
	if type(opts) ~= "table" then
		opts = {}
	end

	local id = opts.session_id
	if type(id) ~= "string" or id == "" then
		id = current_id
	end

	if type(id) ~= "string" or id == "" then
		return nil, "no current session"
	end

	local info = M.get(id)
	if info == nil then
		return nil, "session not found"
	end

	local attrs = info.attrs or {}
	return {
		session_id = info.id,
		created_ms = info.created_ms,
		call_count = info.call_count,
		last_tool = info.last_tool,
		via = attrs.via,
		caller_kind = attrs.caller_kind,
		host_tool = attrs.host_tool,
		alua_ssid = attrs.alua_ssid,
		client_name = attrs.client_name,
		protocol_version = attrs.protocol_version,
		attrs = attrs,
	}
end


-- @brief 清空 (测试)
M.clear = function ()
	sessions = {}
	seq = 0
	parallel_depth = 0
	current_id = nil
end


return M
