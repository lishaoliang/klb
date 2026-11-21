--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   caps.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua serve 子模块: 按 TLS/mux 面宣告 transport 列表 (health 共用)
-- @note   纯查询, 不绑定端口; worker 正式面 mux_on 恒为 true
--   \n @stability internal: 实现文件, 契约面见 core/serve/init.lua
-- @history 修改历史
--  \n [2026-11] 创建文件; 从 core/serve/init.lua 拆出 transport_caps / mux_active / serve_transport_info
--]]


local M = {}


-- @brief 按实宣告 transport 列表 (health 共用)
-- @param [in] tls_listen[boolean]		是否 TLS 同口
-- @param [in] mux_on[boolean]			主口同口多协议是否已接线
-- @param [in] tls_over[string|nil]		"http"|"smp"|"mnp"; nil=http(默认)
-- @return transport[table]				string 数组
M.transport_caps = function (tls_listen, mux_on, tls_over)
	local over = "http"
	if type(tls_over) == "string" and tls_over ~= "" then
		over = tls_over
	elseif tls_over == false then
		over = "smp"
	end

	local transport = { "http" }
	if tls_listen == true then
		if over == "smp" then
			transport = { "smps", "http" }
		elseif over == "mnp" then
			transport = { "mnps", "http" }
		else
			transport = { "https", "http" }
		end
	end

	if mux_on == true then
		if not (tls_listen == true and (over == "smp" or over == "mnp")) then
			transport[#transport + 1] = "ws"
		end

		transport[#transport + 1] = "smp"
		transport[#transport + 1] = "mnp"
		transport[#transport + 1] = "smprpc"
		transport[#transport + 1] = "mnprpc"
	end

	return transport
end


return M

-- end
