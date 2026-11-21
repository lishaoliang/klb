--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   transfer.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  remote.pull / remote.push
-- @history 修改历史
--  \n [2026-10] 从 init.lua 拆出; scheme 文档/契约扩 ws|wss (经 transport.peer_tool); scheme 扩 smp|mnp|smps|mnps; ws|wss 分片走 encoding=base64 (Bug.2026-9)
--]]


local transport = require("klbagent.core.tools.remote.transport")
local util = require("klbagent.core.tools.remote.util")
local fs_util = require("klbagent.core.tools.fs.util")


local M = {}


-- @brief ws/wss 文本帧不容非 UTF-8; 分片改 base64
local function _wire_b64(scheme)
	return util.scheme_is_ws(scheme)
end


-- 从对端拉文件到本机 (分片 fs.read + 本地 append)
local function _pull(args)
	local host, port, token, timeout_sec, chunk, scheme, pin, ca_path = util.resolve_peer(args)
	local alua_ssid = args._resolved_ssid or util.current_alua_ssid(args)
	local remote_path = args.remote_path
	local local_path = args.local_path
	if not util.path_ok(remote_path) or not util.path_ok(local_path) then
		error("need remote_path and local_path")
	end

	local st, used_scheme = transport.peer_tool(
		host, port, token, timeout_sec, "fs.stat", { path = remote_path }, 65536, scheme, pin, ca_path, alua_ssid)
	if st.exists ~= true or st.mode ~= "file" then
		error("remote path not a file")
	end

	-- 首包已选定 scheme 后固定, 避免每片再探 https
	if type(used_scheme) == "string" and used_scheme ~= "" then
		scheme = used_scheme
	end

	local out = io.open(local_path, "wb")
	if out == nil then
		error("open local_path fail")
	end

	local offset = 0
	local chunks = 0
	local total = 0
	local size = tonumber(st.size) or 0
	local use_b64 = _wire_b64(scheme)

	while true do
		local read_args = {
			path = remote_path,
			offset = offset,
			max_bytes = chunk,
		}
		if use_b64 then
			read_args.encoding = "base64"
		end

		local piece = transport.peer_tool(host, port, token, timeout_sec, "fs.read", read_args,
			chunk + 65536, scheme, pin, ca_path, alua_ssid)

		local content = piece.content or ""
		if use_b64 or piece.encoding == "base64" then
			local decoded = fs_util.b64_decode(content)
			if decoded == nil then
				out:close()
				error("remote fs.read bad base64")
			end

			content = decoded
		end

		local n = #content
		if n > 0 then
			out:write(content)
			total = total + n
			offset = offset + n
			chunks = chunks + 1
		end

		if piece.truncated ~= true or n == 0 then
			break
		end
	end

	out:close()

	return {
		ok = true,
		host = host,
		port = port,
		scheme = scheme,
		remote_path = remote_path,
		local_path = local_path,
		bytes = total,
		chunks = chunks,
		size = size,
	}
end


-- 本机推文件到对端 (分片本地读 + 对端 fs.write append)
local function _push(args)
	local host, port, token, timeout_sec, chunk, scheme, pin, ca_path = util.resolve_peer(args)
	local alua_ssid = args._resolved_ssid or util.current_alua_ssid(args)
	local local_path = args.local_path
	local remote_path = args.remote_path
	if not util.path_ok(remote_path) or not util.path_ok(local_path) then
		error("need remote_path and local_path")
	end

	local inf = io.open(local_path, "rb")
	if inf == nil then
		error("open local_path fail")
	end

	local size = inf:seek("end") or 0
	inf:seek("set", 0)

	local first = true
	local offset = 0
	local chunks = 0
	local total = 0
	local used_scheme = scheme
	local use_b64 = _wire_b64(scheme)

	while true do
		local content = inf:read(chunk) or ""
		local n = #content
		if n == 0 then
			break
		end

		local mode = "append"
		if first then
			mode = "overwrite"
			first = false
		end

		local write_args = {
			path = remote_path,
			content = content,
			mode = mode,
		}
		if use_b64 then
			write_args.content = fs_util.b64_encode(content)
			write_args.encoding = "base64"
		end

		local _, s = transport.peer_tool(host, port, token, timeout_sec, "fs.write", write_args,
			65536, scheme, pin, ca_path, alua_ssid)
		if type(s) == "string" and s ~= "" then
			used_scheme = s
			scheme = s
			use_b64 = _wire_b64(scheme)
		end

		total = total + n
		offset = offset + n
		chunks = chunks + 1
	end

	inf:close()

	return {
		ok = true,
		host = host,
		port = port,
		scheme = used_scheme,
		remote_path = remote_path,
		local_path = local_path,
		bytes = total,
		chunks = chunks,
		size = size,
	}
end


M.push = _push


M.register = function (catalog, dispatch_reg)
	local ok, err = catalog.register({
		name = "remote.pull",
		category = "net",
		title = "pull remote file in chunks",
		summary = "Download remote file via chunked fs.read to local_path. Needs profile net; peer fs.read.",
		min_profile = "net",
		args_schema = {
			host = "string optional (or use peer=)",
			peer = "string optional trusted peer id",
			port = "number optional",
			remote_path = "string peer file",
			local_path = "string local dest",
			chunk_bytes = "number optional default 524288 max 2097152",
			token = "string optional",
			scheme = "string optional auto|https|http|ws|wss|smp|mnp|smps|mnps|smprpc|mnprpc (raw token needs explicit TLS scheme: https|wss|smps|mnps)",
			cert_sha256 = "string optional peer cert SHA256 pin (else ALUA_SERVE_CERT_SHA256)",
			ca_path = "string optional CA PEM (else ALUA_SERVE_CA)",
			timeout_sec = "number optional",
		},
		examples = {
			{
				args = {
					host = "127.0.0.1",
					port = 12817,
					remote_path = "/tmp/a.bin",
					local_path = "/tmp/a.bin",
					scheme = "wss",
				},
			},
		},
		docs = "tools/remote.md",
		side_effects = { "net", "write_fs" },
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch_reg.register("remote.pull", _pull)
	if not ok then
		return false, err
	end

	ok, err = catalog.register({
		name = "remote.push",
		category = "net",
		title = "push local file in chunks",
		summary = "Upload local file via chunked fs.write to remote_path. Needs profile net; peer write_fs.",
		min_profile = "net",
		args_schema = {
			host = "string optional (or use peer=)",
			peer = "string optional trusted peer id",
			port = "number optional",
			local_path = "string local file",
			remote_path = "string peer dest",
			chunk_bytes = "number optional default 524288 max 2097152",
			token = "string optional",
			scheme = "string optional auto|https|http|ws|wss|smp|mnp|smps|mnps|smprpc|mnprpc (raw token needs explicit TLS scheme: https|wss|smps|mnps)",
			cert_sha256 = "string optional peer cert SHA256 pin (else ALUA_SERVE_CERT_SHA256)",
			ca_path = "string optional CA PEM (else ALUA_SERVE_CA)",
			timeout_sec = "number optional",
		},
		examples = {
			{
				args = {
					host = "127.0.0.1",
					port = 12817,
					local_path = "/tmp/a.bin",
					remote_path = "/tmp/a.bin",
					scheme = "ws",
				},
			},
		},
		docs = "tools/remote.md",
		side_effects = { "net" },
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch_reg.register("remote.push", _push)
	if not ok then
		return false, err
	end

	return true
end


return M

-- end
