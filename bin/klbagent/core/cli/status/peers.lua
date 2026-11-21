--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   peers.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua status peers — 对端清单 / 状态计数
-- @note   脱敏: 只出 token_set / cert pin 是否设置, 禁 token 真值
-- @history 修改历史
--  \n [2026-10] 创建文件 (由 status.lua 拆分)
--]]


local version = require("klbagent.version")
local common = require("klbagent.core.cli.status.common")


local M = {}


-- @brief peers 模块详情 (脱敏: 无 token)
-- @param [in] path_rest[table]		未用 (模块无下级)
-- @param [in] opts[table|nil]		未用
-- @return report[table]
M.collect = function (path_rest, opts)
	local rows = {}
	local counts = { total = 0, trusted = 0, pending = 0, revoked = 0 }
	local path = nil
	local file_ok = false
	do
		local ok_p, peers_mod = pcall(require, "klbagent.core.config.peers")
		if ok_p and type(peers_mod) == "table" then
			if type(peers_mod.path) == "function" then
				path = peers_mod.path()
				file_ok = common.file_exists(path)
			end

			if type(peers_mod.load) == "function" then
				local cfg = peers_mod.load()
				if type(peers_mod.counts) == "function" then
					counts = peers_mod.counts(cfg)
				end

				if type(cfg) == "table" and type(cfg.peers) == "table" then
					for i = 1, #cfg.peers do
						local p = cfg.peers[i]
						if type(p) == "table" then
							rows[#rows + 1] = {
								id = p.id,
								state = p.state,
								host = p.host,
								port = p.port,
								scheme = p.scheme,
								token_set = (type(p.token_ref) == "string" and p.token_ref ~= "")
									or (type(p.token) == "string" and p.token ~= ""),
								has_cert_pin = type(p.cert_sha256) == "string" and p.cert_sha256 ~= "",
							}
						end
					end
				end
			end
		end
	end

	return {
		ok = true,
		command = "status",
		schema = common.SCHEMA,
		module = "peers",
		level = "root",
		path = { "peers" },
		version = version.string,
		counts = counts,
		peers = rows,
		pref_path = path,
		file = file_ok,
	}
end


-- @brief 打印 peers 模块详情
-- @param [in] report[table]
M.print_text = function (report)
	if report.ok == false then
		common.print_kv("error", report.message or "fail")
		return
	end

	local c = report.counts or {}
	common.print_kv("counts", string.format("total=%s trusted=%s pending=%s revoked=%s",
		tostring(c.total or 0), tostring(c.trusted or 0),
		tostring(c.pending or 0), tostring(c.revoked or 0)))
	common.print_kv("pref", string.format("%s  file=%s",
		tostring(report.pref_path or ""), common.yn(report.file == true)))
	local peers = report.peers or {}
	if #peers > 0 then
		print("peers:")
		for i = 1, #peers do
			local p = peers[i]
			print(string.format("  %-10s %-8s %s:%s %s token=%s pin=%s",
				tostring(p.id), tostring(p.state),
				tostring(p.host or "?"), common.port_str(p.port),
				tostring(p.scheme or ""),
				common.yn(p.token_set == true),
				common.yn(p.has_cert_pin == true)))
		end
	end
end


return M

-- end
