--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   util.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  net.* 公共辅助
-- @note   截断 / 路径与 URL / 文件 IO / curl·klbhttp 探测 / cookie·header / 正向代理
-- @history 修改历史
--  \n [2026-10] 从 init.lua 拆出; proxy_resolve / proxy_status / url_host_port; N2-3 no_proxy_env_raw / *.dom; 与入口共用
--]]


local cjson = require("cjson")


local M = {}


M.DEFAULT_MAX_BYTES = 1048576
M.DEFAULT_TIMEOUT_SEC = 15
M.MAX_TIMEOUT_SEC = 120
M.MAX_MAX_BYTES = 8 * 1048576
M.MAX_REQ_BODY = 8 * 1048576


function M.curl_available()
	local cmd = "command -v curl 2>/dev/null"
	if package.config:sub(1, 1) == "\\" then
		-- Win: 勿用 /dev/null (cmd 会报「系统找不到指定的路径」)
		cmd = "where curl 2>NUL"
	end

	local f = io.popen(cmd)
	if f == nil then
		return false
	end

	local path = f:read("*l")
	f:close()
	return type(path) == "string" and path ~= ""
end


function M.klbhttp_available()
	local ok_k, khttp = pcall(require, "khttp")
	if not ok_k or khttp == nil then
		return false
	end

	local ok_h, klbhttp = pcall(require, "klbcore.klbhttp")
	if not ok_h or type(klbhttp) ~= "table" then
		return false
	end

	if type(klbhttp.co_get) ~= "function" or type(klbhttp.co_post) ~= "function" then
		return false
	end

	local ok_c, kco = pcall(require, "kco")
	if not ok_c or type(kco) ~= "table" or type(kco.fork) ~= "function" then
		return false
	end

	local ok_e, kenv = pcall(require, "kenv")
	if not ok_e or type(kenv) ~= "table" or type(kenv.loop_once) ~= "function" then
		return false
	end

	return true
end


function M.shell_quote(s)
	return "'" .. string.gsub(tostring(s), "'", "'\\''") .. "'"
end


function M.clamp_timeout(v)
	local n = tonumber(v)
	if n == nil or n <= 0 then
		return M.DEFAULT_TIMEOUT_SEC
	end

	n = math.floor(n)
	if n > M.MAX_TIMEOUT_SEC then
		return M.MAX_TIMEOUT_SEC
	end

	return n
end


function M.clamp_max_bytes(v)
	local n = tonumber(v)
	if n == nil or n <= 0 then
		return M.DEFAULT_MAX_BYTES
	end

	n = math.floor(n)
	if n > M.MAX_MAX_BYTES then
		return M.MAX_MAX_BYTES
	end

	return n
end


function M.url_ok(url)
	if type(url) ~= "string" or url == "" then
		return false
	end

	if string.find(url, "^https?://") == nil then
		return false
	end

	if string.find(url, "[\r\n]") ~= nil then
		return false
	end

	return true
end


function M.path_ok(path)
	if type(path) ~= "string" or path == "" then
		return false
	end

	if string.find(path, "[\r\n\0]") ~= nil then
		return false
	end

	return true
end


function M.tmp_path(suffix)
	return tostring(os.tmpname()) .. "_" .. tostring(suffix)
end


function M.read_file(path, max_n)
	local f = io.open(path, "rb")
	if f == nil then
		return "", 0, false
	end

	local data = f:read("*a") or ""
	f:close()
	local n = #data
	local truncated = false
	if type(max_n) == "number" and n > max_n then
		data = string.sub(data, 1, max_n)
		truncated = true
	end

	return data, n, truncated
end


function M.write_file(path, data)
	local f = io.open(path, "wb")
	if f == nil then
		return false
	end

	f:write(data or "")
	f:close()
	return true
end


function M.append_file(path, data)
	local f = io.open(path, "ab")
	if f == nil then
		return false
	end

	f:write(data or "")
	f:close()
	return true
end


function M.build_header_args(headers)
	local parts = {}
	if type(headers) ~= "table" then
		return parts
	end

	for k, v in pairs(headers) do
		if type(k) == "string" and type(v) == "string"
			and k ~= "" and v ~= ""
			and string.find(k, "[\r\n]") == nil
			and string.find(v, "[\r\n]") == nil
		then
			parts[#parts + 1] = "-H " .. M.shell_quote(k .. ": " .. v)
		end
	end

	return parts
end


function M.resolve_body(args)
	local headers = {}
	if type(args.headers) == "table" then
		for k, v in pairs(args.headers) do
			headers[k] = v
		end
	end

	local body = nil
	if args.json ~= nil then
		if type(args.json) ~= "table" and type(args.json) ~= "string"
			and type(args.json) ~= "number" and type(args.json) ~= "boolean"
		then
			error("json must be table/string/number/boolean")
		end

		local ok_j, text = pcall(cjson.encode, args.json)
		if not ok_j or type(text) ~= "string" then
			error("json encode fail")
		end

		body = text
		if headers["Content-Type"] == nil and headers["content-type"] == nil then
			headers["Content-Type"] = "application/json"
		end
	elseif type(args.body) == "string" then
		body = args.body
	elseif args.body ~= nil then
		error("body must be string (or use json)")
	end

	if type(args.content_type) == "string" and args.content_type ~= ""
		and string.find(args.content_type, "[\r\n]") == nil
	then
		headers["Content-Type"] = args.content_type
	end

	if body ~= nil and #body > M.MAX_REQ_BODY then
		error("request body too large")
	end

	return body, headers
end


function M.cookie_args(args)
	local parts = {}
	local jar = args.cookie_jar
	if jar ~= nil then
		if not M.path_ok(jar) then
			error("cookie_jar path invalid")
		end

		parts[#parts + 1] = "-b " .. M.shell_quote(jar)
		parts[#parts + 1] = "-c " .. M.shell_quote(jar)
	end

	local cookie = args.cookie
	if cookie ~= nil then
		if type(cookie) ~= "string" or cookie == ""
			or string.find(cookie, "[\r\n]") ~= nil
		then
			error("cookie must be a single-line string")
		end

		parts[#parts + 1] = "-H " .. M.shell_quote("Cookie: " .. cookie)
	end

	return parts, jar
end


function M.jar_load_cookie_header(jar)
	local f = io.open(jar, "rb")
	if f == nil then
		return nil
	end

	local cookies = {}
	for line in f:lines() do
		if line ~= "" and string.sub(line, 1, 1) ~= "#" then
			local fields = {}
			for field in string.gmatch(line, "[^\t]+") do
				fields[#fields + 1] = field
			end

			if #fields >= 7 then
				cookies[#cookies + 1] = fields[6] .. "=" .. fields[7]
			end
		end
	end

	f:close()
	if #cookies == 0 then
		return nil
	end

	return table.concat(cookies, "; ")
end


function M.url_host(url)
	local host, _ = M.url_host_port(url)
	return host
end


-- @brief 从 URL 取 host 与端口 (缺省按 scheme)
-- @param [in]  url[string]		http(s) URL
-- @return [string, number]	host, port
function M.url_host_port(url)
	local scheme, hostport = string.match(url or "", "^(https?)://([^/]+)")
	if hostport == nil or hostport == "" then
		return "localhost", 80
	end

	local host, port = string.match(hostport, "^([^:]+):(%d+)$")
	if host == nil then
		host = hostport
		if scheme == "https" then
			return host, 443
		end

		return host, 80
	end

	return host, tonumber(port) or 80
end


function M.jar_store_set_cookie(jar, head, url)
	if type(head) ~= "string" or head == "" then
		return
	end

	local host = M.url_host(url)
	for setc in string.gmatch(head, "[Ss][Ee][Tt]%-[Cc][Oo][Oo][Kk][Ii][Ee]:%s*([^\r\n]+)") do
		local name, value = string.match(setc, "^([^=;%s]+)=([^;]*)")
		if name ~= nil and name ~= "" then
			local line = string.format("%s\tTRUE\t/\tFALSE\t0\t%s\t%s\n",
				host, name, value or "")
			M.append_file(jar, line)
		end
	end
end


function M.parse_status(head)
	local code = string.match(head or "", "^HTTP/%d+%.%d+%s+(%d+)")
	return tonumber(code) or 0
end


function M.header_value(head, name)
	if type(head) ~= "string" or type(name) ~= "string" or name == "" then
		return nil
	end

	local lower_name = string.lower(name)
	for line in string.gmatch(head, "[^\r\n]+") do
		local k, v = string.match(line, "^([^:]+):%s*(.*)$")
		if k ~= nil and string.lower(k) == lower_name then
			return v
		end
	end

	return nil
end


function M.resolve_redirect(base_url, location)
	if type(location) ~= "string" or location == "" then
		return nil
	end

	if string.find(location, "^https?://") ~= nil then
		return location
	end

	local scheme_host = string.match(base_url, "^(https?://[^/]+)")
	if scheme_host == nil then
		return nil
	end

	if string.sub(location, 1, 1) == "/" then
		return scheme_host .. location
	end

	local base_path = string.match(base_url, "^(https?://.*/)")
	if base_path == nil then
		base_path = scheme_host .. "/"
	end

	return base_path .. location
end


function M.merge_cookie_headers(headers, args)
	local out = {}
	if type(headers) == "table" then
		for k, v in pairs(headers) do
			out[k] = v
		end
	end

	local parts = {}
	if type(args.cookie) == "string" and args.cookie ~= "" then
		if string.find(args.cookie, "[\r\n]") ~= nil then
			error("cookie must be a single-line string")
		end

		parts[#parts + 1] = args.cookie
	end

	if args.cookie_jar ~= nil then
		if not M.path_ok(args.cookie_jar) then
			error("cookie_jar path invalid")
		end

		local from_jar = M.jar_load_cookie_header(args.cookie_jar)
		if from_jar ~= nil then
			parts[#parts + 1] = from_jar
		end
	end

	if #parts > 0 then
		out["Cookie"] = table.concat(parts, "; ")
	end

	return out
end


function M.content_type_of(headers)
	if type(headers) ~= "table" then
		return nil
	end

	if type(headers["Content-Type"]) == "string" then
		return headers["Content-Type"]
	end

	if type(headers["content-type"]) == "string" then
		return headers["content-type"]
	end

	return nil
end


-- @brief 解析代理串; eg. `http://127.0.0.1:8080` / `socks5h://127.0.0.1:12818`
-- @param [in]  value[string]		代理串; 无 scheme 时按 http
-- @return [table|nil]	`{ host, port, tls, type }`; 非法为 nil
-- @note type: `http-connect` | `socks5`; 无端口时 http=80 / https=443 / socks=1080
function M.proxy_parse(value)
	if type(value) ~= "string" or value == "" then
		return nil
	end

	if string.find(value, "[\r\n]") ~= nil then
		return nil
	end

	-- scheme 允许字母开头 + 字母数字 (socks5 等); 未知 scheme 须拒绝, 勿当 host
	local scheme, rest = string.match(value, "^([%a][%w+.-]*)://(.+)$")
	if scheme == nil then
		if string.find(value, "://", 1, true) ~= nil then
			return nil
		end

		scheme, rest = "http", value
	end

	scheme = string.lower(scheme)
	local ptype = nil
	local tls = false
	local default_port = "80"

	if scheme == "http" then
		ptype = "http-connect"
		default_port = "80"
	elseif scheme == "https" then
		ptype = "http-connect"
		tls = true
		default_port = "443"
	elseif scheme == "socks5" or scheme == "socks5h" or scheme == "socks" then
		ptype = "socks5"
		default_port = "1080"
	else
		return nil
	end

	local hostport = string.match(rest, "^([^/]+)")
	if hostport == nil or hostport == "" then
		return nil
	end

	local host, port = string.match(hostport, "^([^:]+):(%d+)$")
	if host == nil then
		if string.find(hostport, ":", 1, true) ~= nil then
			return nil
		end

		host = hostport
		port = default_port
	end

	local nport = tonumber(port)
	if host == "" or nport == nil or nport < 1 or nport > 65535 then
		return nil
	end

	-- host 禁空白 / 控制字符
	if string.find(host, "[%s%c]") ~= nil then
		return nil
	end

	return { host = host, port = nport, tls = tls, type = ptype }
end


-- @brief 读 ALUA_PROXY 并解析
-- @return [table|nil]	`{ host, port, tls }`; 未设或非法为 nil
function M.proxy_from_env()
	return M.proxy_parse(os.getenv("ALUA_PROXY"))
end


-- @brief 规范化调用方传入的 proxy (table 或串)
-- @param [in]  value[any]		`{host,port,tls?,type?}` 或 `http://` / `socks5h://`
-- @return [table|nil]	合法代理表; 非法为 nil
function M.proxy_normalize(value)
	if type(value) == "string" then
		return M.proxy_parse(value)
	end

	if type(value) ~= "table" then
		return nil
	end

	if type(value.host) ~= "string" or value.host == "" then
		return nil
	end

	local port = tonumber(value.port)
	if port == nil or port < 1 or port > 65535 then
		return nil
	end

	local ptype = value.type or value.kind
	if type(ptype) == "string" then
		ptype = string.lower(ptype)
		if ptype == "socks" or ptype == "socks5h" then
			ptype = "socks5"
		elseif ptype == "http" or ptype == "http_connect" or ptype == "https" then
			ptype = "http-connect"
		end
	else
		ptype = "http-connect"
	end

	if ptype ~= "socks5" and ptype ~= "http-connect" then
		return nil
	end

	return {
		host = value.host,
		port = port,
		tls = value.tls == true,
		type = ptype,
		auth = value.auth,
	}
end


-- @brief 按 URL 解析应使用的正向代理 (args.proxy > ALUA_PROXY; 尊重 ALUA_NO_PROXY)
-- @param [in]  url[string]		目标 URL
-- @param [in]  args[table|nil]	可选; 读 args.proxy
-- @return [table|nil]	`{ host, port, tls, type }`; 直连为 nil
function M.proxy_resolve(url, args)
	local proxy = nil
	if type(args) == "table" then
		proxy = M.proxy_normalize(args.proxy)
	end

	if proxy == nil then
		proxy = M.proxy_from_env()
	end

	if proxy == nil then
		return nil
	end

	local host, port = M.url_host_port(url)
	local nop = M.no_proxy_parse(M.no_proxy_env_raw())
	if M.no_proxy_match(host, port, nop) then
		return nil
	end

	return proxy
end


-- @brief 解析链 hops (ALUA_PROXY 简写收进链引擎)
-- @param [in]  url[string]
-- @param [in]  args[table|nil]
-- @return [table|nil] hops
-- @return [table|nil] proxy 原表
function M.chain_hops_resolve(url, args)
	local proxy = M.proxy_resolve(url, args)
	if proxy == nil then
		return nil, nil
	end

	local ok_c, chain = pcall(require, "klbagent.packages.proxy.chain")
	if not ok_c or type(chain) ~= "table" or type(chain.hops_from_proxy) ~= "function" then
		return nil, proxy
	end

	return chain.hops_from_proxy(proxy), proxy
end


-- @brief 代理环境状态 (供 net.status / runtime.selfcheck)
-- @return [table]	configured / invalid / proxy / env / no_proxy
function M.proxy_status()
	local env = os.getenv("ALUA_PROXY")
	local nop = M.no_proxy_env_raw()
	local parsed = M.proxy_from_env()
	local has_env = type(env) == "string" and env ~= ""
	return {
		configured = parsed ~= nil,
		invalid = has_env and parsed == nil,
		proxy = parsed,
		env = has_env and env or "",
		no_proxy = (type(nop) == "string" and nop) or "",
	}
end


-- @brief 解析 ALUA_NO_PROXY 豁免列表; 逗号分隔
-- @param [in]  value[string]		豁免串; 未设时传 nil
-- @return [array]	小写模式串数组; 无内容为 {}
function M.no_proxy_parse(value)
	local list = {}
	if type(value) ~= "string" or value == "" then
		return list
	end

	for item in string.gmatch(value, "[^,]+") do
		local pat = string.lower(string.gsub(item, "^%s*(.-)%s*$", "%1"))
		if pat ~= "" then
			list[#list + 1] = pat
		end
	end

	return list
end


-- @brief 某 host:port 是否命中豁免列表
-- @param [in]  host[string]		目标主机
-- @param [in]  port[number]		目标端口
-- @param [in]  list[array]		no_proxy_parse 结果
-- @return [boolean]	true. 应直连 (豁免)
-- @note 支持 `*` 全豁免 / `.dom` 域后缀 / `host` / `host:port`
function M.no_proxy_match(host, port, list)
	if type(list) ~= "table" then
		return false
	end

	local lhost = string.lower(tostring(host))
	local lport = tostring(tonumber(port) or port)

	for i = 1, #list do
		local pat = list[i]
		if pat == "*" then
			return true
		end

		local phost, pport = string.match(pat, "^([^:]+):(%d+)$")
		if phost == nil then
			phost = pat
			pport = nil
		end

		if pport == nil or pport == lport then
			-- 前缀点 / *. 为域后缀; 否则精确主机
			if string.sub(phost, 1, 2) == "*." then
				local suf = string.sub(phost, 2)
				if lhost == string.sub(phost, 3) or string.sub(lhost, -#suf) == suf then
					return true
				end
			elseif string.sub(phost, 1, 1) == "." then
				if lhost == string.sub(phost, 2) or string.sub(lhost, -#phost) == phost then
					return true
				end
			elseif lhost == phost then
				return true
			end
		end
	end

	return false
end


-- @brief 读环境豁免串 (ALUA_NO_PROXY > NO_PROXY > no_proxy)
-- @return [string|nil]
function M.no_proxy_env_raw()
	local v = os.getenv("ALUA_NO_PROXY")
	if type(v) == "string" and v ~= "" then
		return v
	end

	v = os.getenv("NO_PROXY")
	if type(v) == "string" and v ~= "" then
		return v
	end

	v = os.getenv("no_proxy")
	if type(v) == "string" and v ~= "" then
		return v
	end

	return nil
end


return M

-- end
