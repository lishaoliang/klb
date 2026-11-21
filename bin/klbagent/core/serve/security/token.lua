--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   token.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua serve 子模块: serve token 读取 / 生成 / 比较
-- @note   来源优先级: 嵌入式传入 > ALUA_SERVE_TOKEN 环境变量 > pref/serve.env
--   \n 嵌入式缺 token 时自动生成并写 pref/serve.env (chmod 600); 禁伪随机回退
--   \n @stability internal: 实现文件, 契约面见 core/serve/init.lua
-- @history 修改历史
--  \n [2026-11] 从 core/serve/init.lua 拆出本文件
--]]
local hostinfo = require("klbagent.core.hostinfo")
local store = require("klbagent.core.config.store")
local log = require("klbagent.core.log")


local M = {}


-- 嵌入式缺原始 token 自动生成的长度 (十六进制字符数)
M.EMBED_TOKEN_LEN = 16


-- 从 pref 目录 serve.env 读 token (支持 export 前缀与引号; 无 shell export 亦可)
local function _token_from_serve_env()
	local dir = store.dir()
	if type(dir) ~= "string" or dir == "" then
		return nil
	end

	dir = string.gsub(dir, "[/\\]+$", "")
	local path = dir .. "/serve.env"
	local f = io.open(path, "rb")
	if f == nil then
		return nil
	end

	local data = f:read("*a") or ""
	f:close()

	for line in string.gmatch(data, "[^\r\n]+") do
		local v = string.match(line, "^%s*export%s+ALUA_SERVE_TOKEN%s*=%s*(.-)%s*$")
		if v == nil then
			v = string.match(line, "^%s*ALUA_SERVE_TOKEN%s*=%s*(.-)%s*$")
		end

		if type(v) == "string" and v ~= "" then
			local q = string.match(v, "^\"(.*)\"$")
			if q == nil then
				q = string.match(v, "^'(.*)'$")
			end

			if q ~= nil then
				v = q
			end

			if v ~= "" then
				return v
			end
		end
	end

	return nil
end


-- @brief 生成随机十六进制 token (仅 CSPRNG; 无 /dev/urandom 则失败)
-- @param [in] n[number]		字符数 (偶数; 缺省 M.EMBED_TOKEN_LEN)
-- @return token[string|nil]	生成成功时的十六进制串
-- @return err[string|nil]	CSPRNG 不可用时的原因
local function _gen_hex_token(n)
	if type(n) ~= "number" or n < 4 then
		n = M.EMBED_TOKEN_LEN
	end

	n = math.floor(n)
	if n % 2 ~= 0 then
		n = n + 1
	end

	local nb = math.floor(n / 2)
	local ok_f, f = pcall(io.open, "/dev/urandom", "rb")
	if ok_f and f ~= nil then
		local raw = f:read(nb)
		f:close()
		if type(raw) == "string" and #raw == nb then
			local parts = {}
			for i = 1, nb do
				parts[#parts + 1] = string.format("%02x", string.byte(raw, i))
			end

			return table.concat(parts)
		end
	end

	-- 禁时间种子伪随机回退: 生成失败即拒发
	return nil, "csprng unavailable (/dev/urandom)"
end


-- @brief 嵌入式缺原始 token: 自动生成并追加 pref/serve.env (chmod 600)
-- @note  仅 host_kind=embedded; 非嵌入式仍须显式 token
-- @return token[string|nil]		生成并落盘成功时返回; 其余 nil
M.autogen_embedded = function ()
	local ok_h, info = pcall(hostinfo.probe)
	if not ok_h or type(info) ~= "table" or info.host_kind ~= "embedded" then
		return nil
	end

	local dir = store.dir()
	if type(dir) ~= "string" or dir == "" then
		return nil
	end

	dir = string.gsub(dir, "[/\\]+$", "")
	if not store.ensure_dir(dir) then
		return nil
	end

	local token, err_tok = _gen_hex_token(M.EMBED_TOKEN_LEN)
	if type(token) ~= "string" or token == "" then
		log.write("warn", "serve token autogen fail", {
			module = "serve",
			err = tostring(err_tok or "csprng unavailable"),
		})
		return nil
	end

	local path = dir .. "/serve.env"
	local tail = ""
	local fr = io.open(path, "rb")
	if fr ~= nil then
		local size = fr:seek("end")
		if type(size) == "number" and size > 0 then
			fr:seek("set", size - 1)
			tail = fr:read(1) or ""
		end

		fr:close()
	end

	local f = io.open(path, "ab")
	if f == nil then
		return nil
	end

	-- 文件已存在时补换行, 避免与既有键粘连
	if tail ~= "" and tail ~= "\n" then
		f:write("\n")
	end

	f:write("ALUA_SERVE_TOKEN=" .. token .. "\n")
	f:close()
	pcall(function ()
		os.execute(string.format("chmod 600 %q", path))
	end)

	-- 日志仅记长度与路径, 禁打印 token 真值
	log.write("info", "serve token autogen (embedded)", {
		module = "serve",
		path = path,
		len = #token,
	})

	-- 让 hostinfo 缓存中的 token_set 反映新落盘
	pcall(hostinfo.reset_cache)

	return token
end


-- @brief 解析 serve token (嵌入式传入 / 环境变量 / serve.env)
-- @param [in] embed_token[string|nil]	宿主进程内传入 (g2de 等); 优先
-- @return token[string|nil]
M.token_expect = function (embed_token)
	if type(embed_token) == "string" and embed_token ~= "" then
		return embed_token
	end

	local expect = os.getenv("ALUA_SERVE_TOKEN")
	if type(expect) == "string" and expect ~= "" then
		-- systemd EnvironmentFile / CRLF 残留
		expect = string.gsub(expect, "[\r\n]", "")
		expect = string.match(expect, "^%s*(.-)%s*$") or expect
		if expect ~= "" then
			return expect
		end
	end

	return _token_from_serve_env()
end


-- @brief 常量时间比较, 降低时序侧信道
-- @param [in] a[string]
-- @param [in] b[string]
-- @return eq[boolean]
M.token_eq = function (a, b)
	if type(a) ~= "string" or type(b) ~= "string" then
		return false
	end

	local la = #a
	local lb = #b
	local n = la
	if lb > n then
		n = lb
	end

	local diff = la ~ lb
	for i = 1, n do
		local ca = (i <= la) and string.byte(a, i) or 0
		local cb = (i <= lb) and string.byte(b, i) or 0
		diff = diff | (ca ~ cb)
	end

	return diff == 0
end


return M

-- end
