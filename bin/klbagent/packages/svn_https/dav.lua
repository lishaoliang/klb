--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   dav.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  svn_https 同进程 HTTP-SVN (ksvnra)
-- @note   读: OPTIONS/PROPFIND/REPORT/GET; 写: HTTPv2 POST/MKCOL/PUT/DELETE/COPY/MOVE/MERGE
--   \n Bug.1-Bug.12; update-report 增量/skelta; copy/move/propset commit
-- @history 修改历史
--  \n [2026-10] 创建文件 (补充实现读路径); HTTPv2 写路径 (txn 缓冲 + ksvnra.commit)
--  \n [2026-11] LOCK 补 X-SVN-Lock-Owner + owner href; HEAD 去重复 Content-Length; 写方法加仓级互斥 (txn / 锁表串行); M9: ra 缓存健康探测与失效重连; GET 无 Range 时走 ksvnra cat_open/cat_next 真流式 (64KiB 聚合分块); 单段 Range 亦真流式 (skip/limit, co_chunk 传 206); 流式路径去重复头; 失败体统一带 errcode 数字码 (全量透出); 去掉重复 Server 头; update-report 按 <S:src-path> 截断 base-path, 修子目录 checkout 多一层; 多段 Range 亦真流式 (逐段 cat_open/cat_next → multipart/byteranges); PROPFIND 经 ksvnra dir_props 输出 svn: 版本属性 (mergeinfo 等); S 前缀命名空间取自请求; mergeinfo-report 走 ksvnra mergeinfo 真取数; 修正 protocol 路径语义
--   \n   (location-segment@path 与 mergeinfo-path 均为相对 REPORT 目标, 无前导 '/')
--  \n [2026-11] P1-5: location-segments-report 走 ksvnra location_segments 真历史段
--   \n   (取代占位单段; 修 svn mergeinfo 摘要图 oldest_rev 断言)
--  \n [2026-11] M3: replay-report 走 ksvnra replay 真编辑流; 失败透出 svn 码/消息 (非 401 假体)
--  \n [2026-11] M4: PUT body spool 到进程临时文件 (ALUA_SVN_SPOOL_DIR), MERGE 前读回;
--   \n   txn 只留路径+字节数, 超期残留按 SPOOL_TTL 清理
--  \n [2026-11] P1-6/P1-7: update-report 按 <S:dst-path> 双树差分 (skelta; 编辑器根=src-path,
--   \n   update-target 子树保留源侧名); log-report <S:path> 相对 REPORT 目标解析
--  \n [2026-11] M1/M4: update-report 文件增量改走 ksvnra.txdelta_file (取文/svndiff/digest/b64
--   \n   全在 C 侧单次完成); 删除纯 Lua sha1/md5/svndiff/base64 兜底实现
--  \n [2026-11] M2: replay/file-revs/diff 响应接入分块直写; replay 改 replay_begin/
--   \n   replay_next 批量取事件, 批次间 flush, 断流后关闭会话停止 replay
--  \n [2026-11] Bug.2026-22: update-report 目录条目统一输出, 修空目录 checkout/update 丢失
--  \n [2026-11] Bug.2026-24: 目标为子项时根目录锚点不带 propchanges (修单文件 up -r 断言)
--]]
local ksvnra = require("ksvnra")
local kcrypto = require("kcrypto")
local log = require("klbagent.core.log")
local ok_kco, kco = pcall(require, "kco")
if not ok_kco then
	kco = nil
end
local ok_lfs, lfs = pcall(require, "lfs")
if not ok_lfs then
	lfs = nil
end


-- @brief 失效 ra 会话标记 (弱键; M9 失败后下一次请求强制重连)
local RA_STALE = setmetatable({}, { __mode = "k" })


-- @brief 调试输出 (仅 ALUA_SVN_DAV_DEBUG=1; 生产默认静默)
local function _dbg(fmt, ...)
	if os.getenv("ALUA_SVN_DAV_DEBUG") ~= "1" then
		return
	end

	local msg = string.format(fmt, ...)
	pcall(function ()
		io.stderr:write(msg, "\n")
	end)
end


local M = {}


local DAV_NS = "DAV:"
local SVN_DAV_NS = "http://subversion.tigris.org/xmlns/dav/"
local SVN_NS = "svn:"


local function _xml_escape(s)
	s = tostring(s or "")
	s = string.gsub(s, "&", "&amp;")
	s = string.gsub(s, "<", "&lt;")
	s = string.gsub(s, ">", "&gt;")
	s = string.gsub(s, "\"", "&quot;")
	return s
end


local function _parse_basic(auth)
	if type(auth) ~= "string" then
		return nil
	end

	local b64 = string.match(auth, "^[Bb][Aa][Ss][Ii][Cc]%s+(%S+)$")
	if not b64 then
		return nil
	end

	b64 = string.gsub(b64, "[^A-Za-z0-9+/=]", "")
	local raw = kcrypto.base64_decode(b64)
	if type(raw) ~= "string" then
		return nil
	end

	local u, p = string.match(raw, "^([^:]+):(.*)$")
	if not u then
		return nil
	end

	return u, p
end


local function _parse_path(path)
	path = string.match(path or "", "^([^?]*)") or ""
	local repo, rest = string.match(path, "^/svn/([^/]+)(/.*)$")
	if not repo then
		repo = string.match(path, "^/svn/([^/]+)/?$")
		if not repo then
			return nil
		end
		rest = "/"
	end

	return repo, rest or "/", path
end


local function _header_get(headers, name)
	if type(headers) ~= "table" then
		return nil
	end

	name = string.lower(name)
	local v = headers[name]
	if type(v) == "string" then
		return v
	end

	return nil
end


local function _send(res, code, body, headers)
	body = body or ""
	headers = headers or {}
	-- co_status 默认 mime=text/plain; 须把 Content-Type 作 mime 传入, 避免双头干扰 serf (Bug.18)
	-- Server 由 pack_head 默认输出, 额外再加会重复 (HEAD 曾出现两行 Server)
	local mime = nil
	for i = 1, #headers do
		local n = string.lower(tostring(headers[i][1] or ""))
		if n == "content-type" then
			mime = headers[i][2]
		elseif n ~= "server" then
			res:header(headers[i][1], headers[i][2])
		end
	end
	res:co_status(code, mime or "text/plain", body)
end


local function _send_no_body(res, code, headers)
	headers = headers or {}
	local mime = nil
	for i = 1, #headers do
		local n = string.lower(tostring(headers[i][1] or ""))
		if n == "content-type" then
			mime = headers[i][2]
		elseif n ~= "server" then
			res:header(headers[i][1], headers[i][2])
		end
	end
	res:co_status(code, mime or "text/plain", "")
end


-- @brief DAV error XML (P1-3): 失败回 svn 语义错误体, 不再只有 500 纯文本
-- @param [in] code[int]			HTTP 状态
-- @param [in] err_id[string]	svn 错误标签 (如 svn:commit-failed)
-- @param [in] msg[string]		说明
-- @param [in] svn_code[number]	[可选] 上游 svn 错误码; 非 0 时以 errcode 属性全量透出
-- @note svn 客户端按 m:human-readable 的 errcode 取码, 故任意码可透出, 无需逐码符号名表
local function _send_dav_error(res, code, err_id, msg, svn_code)
	-- 调用方传 svn:<local>; XML 元素已带 S: 前缀, 须剥掉, 否则 <S:svn:x/> 双冒号非法
	local local_id = string.gsub(tostring(err_id or ""), "^svn:", "")
	local num = tonumber(svn_code) or 0
	local body = table.concat({
		'<?xml version="1.0" encoding="utf-8"?>\n',
		'<D:error xmlns:D="DAV:" xmlns:S="svn:" xmlns:m="http://apache.org/dav/xmlns">\n',
		"<S:", local_id, "/>\n",
		"<D:responsedescription>", _xml_escape(msg or ""), "</D:responsedescription>\n",
		(num > 0)
			and string.format('<m:human-readable errcode="%d">%s</m:human-readable>\n',
				num, _xml_escape(msg or ""))
			or "",
		"</D:error>\n",
	})
	_send(res, code, body, {
		{ "Content-Type", 'text/xml; charset="utf-8"' },
		{ "Server", "alua-svn-https" },
	})
end


-- @brief svn 错误码 → DAV error 标签 (P1-2)
-- @note 仅收录可确证码; 其余由上游消息透传 + 通用 svn:ra-error 兜底
local SVN_ERR_TAGS = {
	[160006] = "svn:revision-not-found",
	[160013] = "svn:path-not-found",
	[160016] = "svn:not-directory",
	[160017] = "svn:not-file",
	[160020] = "svn:already-exists",
	[165000] = "svn:repository-locked",
	[170001] = "svn:not-authorized",
	[185005] = "svn:repository-not-found",
	[185008] = "svn:edit-aborted",
	[195012] = "svn:node-kind-mismatch",
}


-- @brief 取会话最近失败 (旧绑定无 last_error 时返回 0, "")
local function _ra_err(ra)
	local ok, code, msg = pcall(function ()
		return ra:last_error()
	end)
	if not ok then
		return 0, ""
	end

	return tonumber(code) or 0, tostring(msg or "")
end


-- @brief 失败体: 依 svn 错误码直出标签 (P1-2)
-- @param [in] code[number]  已捕获错误码; nil 时从 ra 读取
-- @param [in] err[string]   已捕获错误消息; code 非 nil 时使用
-- @param [in] http[number]  [可选] HTTP 状态, 默认 500 (路径缺失类可传 404)
local function _send_ra_error(res, ra, fallback, msg, code, err, http)
	if nil == code then
		code, err = _ra_err(ra)
	end
	local tag = SVN_ERR_TAGS[code] or fallback or "svn:ra-error"
	-- 上游 svn 消息比本地通用文案更精确 (如 Path 'x' not found in revision 6)
	if type(err) == "string" and "" ~= err then
		msg = err
	end

	-- M9: 标记会话失效; 下次 _ra 取用前强制关闭重开
	if nil ~= ra then
		RA_STALE[ra] = true
	end

	-- Bug.26/27: ra_serf 把底层错误码(160013/160020)包装成 E175002; 需从 err 消息字符串提取类型
	-- 关键词匹配: parent not found / already exists / not found 等
	if nil == http then
		local _RA_CODE_HTTP = {
			[160013] = 409,   -- path-not-found / parent missing
			[160020] = 409,   -- already-exists / parent already exists
			[160006] = 404,   -- object-not-found
			[170003] = 403,   -- access-denied
			[195012] = 409,   -- fs_root_mismatch
		}
		http = _RA_CODE_HTTP[code]
		-- code 不在映射表 (如 175002 ra_serf wrapper) 时, 尝试从 err 消息关键词推断
		if not http and type(err) == "string" and err ~= "" then
			local lower_err = string.lower(err)
			if string.find(lower_err, "parent", 1, true)
				or string.find(lower_err, "not found", 1, true) then
				http = 409
			elseif string.find(lower_err, "already exist", 1, true)
				or string.find(lower_err, "file exist", 1, true) then
				http = 409
			elseif string.find(lower_err, "not found", 1, true) then
				http = 404
			elseif string.find(lower_err, "access denied", 1, true)
				or string.find(lower_err, "authorization", 1, true) then
				http = 403
			end
		end
		http = http or 500
	end

	if os.getenv("ALUA_SVN_DAV_DEBUG") == "1" then
		io.stderr:write(string.format("[dav] ra error code=%s tag=%s http=%s msg=%s\n",
			tostring(code), tostring(tag), tostring(http), tostring(msg)))
	end
	_send_dav_error(res, http, tag, msg, code)
end


-- @brief 仓库目录缺失 (读 <repo_root>/<repo>/format, 免 lfs 依赖; repo_root 未配置时恒 false)
local function _repo_missing(cfg, repo)
	local root = cfg.repo_root
	if type(root) ~= "string" or root == "" then
		return false
	end

	local f = io.open(root .. "/" .. tostring(repo) .. "/format", "r")
	if f then
		f:close()
		return false
	end

	return true
end


-- @brief ra 会话打开失败: 仓不存在→404; 其余(含口令错)→401 挑战 (P1-3 部分)
local function _send_ra_fail(res, cfg, repo, err)
	if _repo_missing(cfg, repo) then
		_send(res, 404, "repository not found: " .. tostring(repo), {
			{ "Content-Type", "text/plain" },
			{ "Server", "alua-svn-https" },
		})
		return
	end

	_send(res, 401, tostring(err or "authentication failed"), {
		{ "WWW-Authenticate", 'Basic realm="Private SVN"' },
		{ "Content-Type", "text/plain" },
		{ "Server", "alua-svn-https" },
	})
end


local function _rel_clean(rel)
	rel = tostring(rel or "")
	rel = string.gsub(rel, "^/+", "")
	rel = string.gsub(rel, "/+$", "")
	return rel
end


-- @brief DAV 属性名 → svn 属性名 (S:ignore→svn:ignore; C:foo→foo)
local function _dav_prop_svn_name(name)
	name = tostring(name or "")
	local pref, localn = string.match(name, "^([%w]+):([%w%.:%-]+)$")
	if not pref then
		if string.sub(name, 1, 4) == "svn:" then
			return name
		end
		return nil
	end

	if pref == "S" or pref == "s" then
		return "svn:" .. localn
	end

	if pref == "C" or pref == "c" then
		return localn
	end

	return nil
end


local function _xml_unescape(s)
	s = tostring(s or "")
	s = string.gsub(s, "&lt;", "<")
	s = string.gsub(s, "&gt;", ">")
	s = string.gsub(s, "&quot;", "\"")
	s = string.gsub(s, "&apos;", "'")
	s = string.gsub(s, "&amp;", "&")
	return s
end


-- @brief 缺失资源 PROPFIND: HTTP 404 (ra_serf check_path→svn_node_none; Bug.18/17/16)
-- @note  勿回 207+404 propstat: serf 会当成功解析却无 resourcetype → E175004
local function _send_propfind_missing(res, href)
	local body = table.concat({
		'<?xml version="1.0" encoding="utf-8"?>\n',
		'<D:multistatus xmlns:D="DAV:">\n',
		"<D:response>\n<D:href>", _xml_escape(href or ""), "</D:href>\n",
		"<D:propstat>\n<D:prop><D:resourcetype></D:resourcetype></D:prop>\n",
		"<D:status>HTTP/1.1 404 Not Found</D:status>\n</D:propstat>\n",
		"</D:response>\n</D:multistatus>\n",
	})
	_send(res, 404, body, {
		{ "Content-Type", 'text/xml; charset="utf-8"' },
		{ "Server", "alua-svn-https" },
	})
end


local function _lock_esc(s)
	s = tostring(s or "")
	s = string.gsub(s, "\\", "\\\\")
	s = string.gsub(s, "\t", "\\t")
	s = string.gsub(s, "\n", "\\n")
	return s
end


local function _lock_unesc(s)
	local out = {}
	local i = 1
	while i <= #s do
		local c = string.sub(s, i, i)
		if c == "\\" then
			local n = string.sub(s, i + 1, i + 1)
			if n == "t" then
				out[#out + 1] = "\t"
			elseif n == "n" then
				out[#out + 1] = "\n"
			elseif n == "\\" then
				out[#out + 1] = "\\"
			else
				out[#out + 1] = n
			end
			i = i + 2
		else
			out[#out + 1] = c
			i = i + 1
		end
	end
	return table.concat(out)
end


-- @brief 锁表落盘文件 (P1-2): ALUA_SVN_LOCK_DB 未配置时仅内存
local function _lock_db_path()
	local p = os.getenv("ALUA_SVN_LOCK_DB")
	if type(p) ~= "string" or p == "" then
		return nil
	end
	return p
end


local function _lock_key(repo, rel)
	return tostring(repo or "") .. "\0" .. _rel_clean(rel)
end


local function _lock_load(cfg)
	local path = _lock_db_path()
	if not path then
		return
	end

	local f = io.open(path, "r")
	if not f then
		return
	end

	local store = cfg._locks
	for line in f:lines() do
		local f1, f2, f3, f4, f5, f6 = string.match(line,
			"^([^\t]*)\t([^\t]*)\t([^\t]*)\t([^\t]*)\t([^\t]*)\t([^\t]*)$")
		if f1 then
			store[_lock_key(_lock_unesc(f1), _lock_unesc(f2))] = {
				token = _lock_unesc(f3),
				owner = _lock_unesc(f4),
				created = tonumber(f5) or 0,
				expires = tonumber(f6) or 0,
			}
		end
	end
	f:close()
end


local function _lock_save(cfg)
	local path = _lock_db_path()
	if not path then
		return
	end

	local lines = {}
	for key, rec in pairs(cfg._locks or {}) do
		local repo, rel = string.match(key, "^(.-)\0(.*)$")
		if repo then
			lines[#lines + 1] = table.concat({
				_lock_esc(repo),
				_lock_esc(rel),
				_lock_esc(rec.token),
				_lock_esc(rec.owner),
				tostring(rec.created or 0),
				tostring(rec.expires or 0),
			}, "\t")
		end
	end

	local tmp = path .. ".tmp"
	local f = io.open(tmp, "w")
	if not f then
		return
	end
	f:write(table.concat(lines, "\n"))
	if 0 < #lines then
		f:write("\n")
	end
	f:close()
	os.rename(tmp, path)
end


local function _lock_store(cfg)
	cfg._locks = cfg._locks or {}
	if not cfg._locks_loaded then
		cfg._locks_loaded = true
		_lock_load(cfg)
	end
	return cfg._locks
end


local function _lock_get(cfg, repo, rel)
	local store = _lock_store(cfg)
	local key = _lock_key(repo, rel)
	local info = store[key]
	if not info then
		return nil
	end

	local exp = tonumber(info.expires) or 0
	if 0 < exp and exp <= os.time() then
		store[key] = nil
		_lock_save(cfg)
		return nil
	end

	return info
end


local function _lock_set(cfg, repo, rel, info)
	local store = _lock_store(cfg)
	store[_lock_key(repo, rel)] = info
	_lock_save(cfg)
end


local function _lock_del(cfg, repo, rel)
	local store = _lock_store(cfg)
	store[_lock_key(repo, rel)] = nil
	_lock_save(cfg)
end


-- @brief 按 depth 裁剪 update 行集 (只保留 base 及其对应层级)
-- @param rows  list_r 结果 (仓内相对路径)
-- @param base  本次报告根 (仓内相对路径)
-- @param depth empty/files/immediates
local function _rows_by_depth(rows, base, depth)
	local keep = {}
	local bp = ("" ~= base) and (base .. "/") or ""

	for i = 1, #rows do
		local e = rows[i]
		local n = e.name or ""
		local rel = n
		if "" ~= bp then
			if n == base then
				rel = ""
			elseif string.sub(n, 1, #bp) == bp then
				rel = string.sub(n, #bp + 1)
			else
				rel = nil
			end
		end

		if nil ~= rel then
			local d = 0
			for _ in string.gmatch(rel, "[^/]+") do
				d = d + 1
			end
			if "empty" == depth then
				if 0 == d then
					keep[#keep + 1] = e
				end
			elseif "files" == depth then
				if 0 == d or (1 == d and "dir" ~= (e.kind or "file")) then
					keep[#keep + 1] = e
				end
			elseif "immediates" == depth then
				if 1 >= d then
					keep[#keep + 1] = e
				end
			end
		end
	end

	return keep
end


local function _path_depth(p)
	local n = 0
	for _ in string.gmatch(p or "", "[^/]+") do
		n = n + 1
	end
	return n
end


-- @brief 解析 Destination 头到仓内 !svn/txr/txn/rel
local function _parse_destination_txr(dest, repo)
	if type(dest) ~= "string" or dest == "" then
		return nil
	end

	local path = dest
	if string.match(dest, "^https?://") then
		path = string.match(dest, "^https?://[^/]+(/.*)$") or dest
	end

	local txn, rel = string.match(path, "^/svn/" .. repo .. "/!svn/txr/([^/]+)/?(.*)$")
	if not txn then
		return nil
	end

	return txn, _rel_clean(rel)
end


local function _need_auth(req, res)
	local u, p = _parse_basic(_header_get(req.headers, "authorization"))
	if not u then
		_send(res, 401, "Unauthorized", {
			{ "WWW-Authenticate", 'Basic realm="Private SVN"' },
			{ "Content-Type", "text/plain" },
		})
		return nil
	end

	return u, p
end


local function _ra(cfg, repo, user, pass)
	cfg._ra_cache = cfg._ra_cache or {}
	local key = tostring(user) .. "\0" .. tostring(repo) .. "\0" .. tostring(pass)
	local ent = cfg._ra_cache[key]
	if ent then
		-- M9: 健康探测 (youngest) + 失败会话强制重连
		local ok_y, y = pcall(function ()
			return ent:youngest()
		end)
		if ok_y and y ~= nil and not RA_STALE[ent] then
			return ent
		end

		pcall(function ()
			ent:close()
		end)
		cfg._ra_cache[key] = nil
		RA_STALE[ent] = nil
	end

	local ra, err = ksvnra.open({
		hostport = cfg.svnserve or "127.0.0.1:3690",
		user = user,
		pass = pass,
		repo = repo,
	})
	if not ra then
		return nil, err or "ksvnra open failed"
	end

	cfg._ra_cache[key] = ra
	return ra
end


local function _dav_headers(ra, repo)
	local young, yerr = ra:youngest()
	if not young then
		return nil, yerr
	end

	local uuid, uerr = ra:uuid()
	if not uuid then
		return nil, uerr
	end

	local root = "/svn/" .. repo
	return {
		{ "DAV", "1,2" },
		{ "DAV", "http://subversion.tigris.org/xmlns/dav/svn/depth" },
		{ "DAV", "http://subversion.tigris.org/xmlns/dav/svn/log-revprops" },
		{ "DAV", "http://subversion.tigris.org/xmlns/dav/svn/atomic-revprops" },
		{ "DAV", "http://subversion.tigris.org/xmlns/dav/svn/partial-replay" },
		{ "DAV", "http://subversion.tigris.org/xmlns/dav/svn/inherited-props" },
		{ "DAV", "http://subversion.tigris.org/xmlns/dav/svn/inline-props" },
		{ "DAV", "http://subversion.tigris.org/xmlns/dav/svn/mergeinfo" },
		{ "MS-Author-Via", "DAV" },
		{ "Allow", "OPTIONS,GET,HEAD,POST,DELETE,PROPFIND,PROPPATCH,COPY,MOVE,LOCK,UNLOCK,MKCOL,MERGE,REPORT,PUT" },
		{ "SVN-Youngest-Rev", tostring(young) },
		{ "SVN-Repository-UUID", uuid },
		{ "SVN-Repository-Root", root },
		{ "SVN-Me-Resource", root .. "/!svn/me" },
		{ "SVN-Rev-Root-Stub", root .. "/!svn/rvr" },
		{ "SVN-Rev-Stub", root .. "/!svn/rev" },
		{ "SVN-Txn-Root-Stub", root .. "/!svn/txr" },
		{ "SVN-Txn-Stub", root .. "/!svn/txn" },
		{ "SVN-VTxn-Root-Stub", root .. "/!svn/vtxr" },
		{ "SVN-VTxn-Stub", root .. "/!svn/vtxn" },
		{ "SVN-Allow-Bulk-Updates", "On" },
		{ "SVN-Supported-Posts", "create-txn" },
		{ "SVN-Supported-Posts", "create-txn-with-props" },
		{ "Server", "alua-svn-https" },
	}, young, uuid
end


-- @brief 从 PROPFIND 请求体取 S 前缀命名空间 (兼容 "svn:" 与 tigris URI)
local function _propfind_svn_ns(body)
	local ns = string.match(tostring(body or ""), 'xmlns:[Ss]%s*=%s*"([^"]*)"')
	if type(ns) == "string" and "" ~= ns then
		return ns
	end
	return SVN_NS
end


-- @brief PROPFIND 单条 response; props 为目录 svn: 属性 (可空); svn_ns 为 S 前缀命名空间
local function _propfind_response(repo, uuid, href, brp, kind, last_rev, author, date, size, checked_href, lock_info, props, svn_ns)
	local is_dir = (kind == "dir")
	if is_dir and not string.match(href, "/$") then
		href = href .. "/"
	end

	local rt = is_dir and "<D:collection/>" or ""
	local brp_esc = _xml_escape(brp)
	local checked = checked_href
	if not checked then
		checked = string.format("/svn/%s/!svn/rvr/%s/%s", repo, tostring(last_rev), brp_esc)
		if is_dir and brp ~= "" then
			checked = checked .. "/"
		elseif is_dir then
			checked = string.format("/svn/%s/!svn/rvr/%s/", repo, tostring(last_rev))
		end
	elseif is_dir and not string.match(checked, "/$") then
		checked = checked .. "/"
	end

	-- Bug.25: 收集 props 中所有属性前缀, 构建额外 xmlns 声明
	-- prefix "svn" 已用 S 前缀声明 xmlns:S; 其他 prefix (如 custom) 需要动态声明
	local extra_xmlns = {}  -- {prefix=uri, ...}
	if type(props) == "table" then
		for i = 1, #props do
			local pname = tostring(props[i].name or "")
			local colon = string.find(pname, ":", 1, true)
			if colon then
				local prefix = string.sub(pname, 1, colon - 1)
				if prefix ~= "svn" and prefix ~= "" then
					if not extra_xmlns[prefix] then
						extra_xmlns[prefix] = prefix .. ":"  -- URI = prefix + ":" (svn DAV 惯例)
					end
				end
			end
		end
	end
	-- 构建 response 根元素的 xmlns 声明字符串
	local extra_xmlns_str = ""
	for prefix, uri in pairs(extra_xmlns) do
		extra_xmlns_str = extra_xmlns_str .. string.format(' xmlns:%s="%s"', prefix, uri)
	end

	local parts = {
		string.format('<D:response xmlns:lp1="%s" xmlns:lp2="%s" xmlns:S="%s"%s>\n',
			DAV_NS, SVN_DAV_NS, svn_ns or SVN_NS, extra_xmlns_str),
		"<D:href>", _xml_escape(href), "</D:href>\n",
		"<D:propstat>\n<D:prop>\n",
		"<lp1:version-controlled-configuration><D:href>/svn/", repo, "/!svn/vcc/default</D:href></lp1:version-controlled-configuration>\n",
		"<lp1:resourcetype>", rt, "</lp1:resourcetype>\n",
		"<lp2:baseline-relative-path>", brp_esc, "</lp2:baseline-relative-path>\n",
		"<lp2:repository-uuid>", _xml_escape(uuid), "</lp2:repository-uuid>\n",
		"<lp1:getcontentlength>", tostring(size or 0), "</lp1:getcontentlength>\n",
	}
	if last_rev ~= nil then
		parts[#parts + 1] = "<lp1:version-name>"
		parts[#parts + 1] = tostring(last_rev)
		parts[#parts + 1] = "</lp1:version-name>\n"
	end

	-- 空 creationdate 会导致客户端 E125003 Bogus date; 勿填 getlastmodified(HTTP-date)
	-- 以免 serf 用 ISO 解析器误读
	if type(date) == "string" and date ~= "" then
		parts[#parts + 1] = "<lp1:creationdate>"
		parts[#parts + 1] = _xml_escape(date)
		parts[#parts + 1] = "</lp1:creationdate>\n"
	end

	if type(author) == "string" and author ~= "" then
		parts[#parts + 1] = "<lp1:creator-displayname>"
		parts[#parts + 1] = _xml_escape(author)
		parts[#parts + 1] = "</lp1:creator-displayname>\n"
	end

	parts[#parts + 1] = "<lp1:checked-in><D:href>"
	parts[#parts + 1] = checked
	parts[#parts + 1] = "</D:href></lp1:checked-in>\n"

	if type(lock_info) == "table" and type(lock_info.token) == "string" and lock_info.token ~= "" then
		parts[#parts + 1] = "<D:lockdiscovery><D:activelock>\n"
		parts[#parts + 1] = "<D:locktype><D:write/></D:locktype>\n"
		parts[#parts + 1] = "<D:lockscope><D:exclusive/></D:lockscope>\n"
		parts[#parts + 1] = "<D:depth>0</D:depth>\n"
		parts[#parts + 1] = "<D:creator-displayname>"
		parts[#parts + 1] = _xml_escape(lock_info.owner or "")
		parts[#parts + 1] = "</D:creator-displayname>\n"
		parts[#parts + 1] = "<D:owner><D:href>"
		parts[#parts + 1] = _xml_escape(lock_info.owner or "")
		parts[#parts + 1] = "</D:href></D:owner>\n"
		parts[#parts + 1] = "<D:locktoken><D:href>"
		parts[#parts + 1] = _xml_escape(lock_info.token)
		parts[#parts + 1] = "</D:href></D:locktoken>\n"
		parts[#parts + 1] = "</D:activelock></D:lockdiscovery>\n"
	else
		parts[#parts + 1] = "<D:lockdiscovery/>\n"
	end

	if type(props) == "table" then
		for i = 1, #props do
			local pname = tostring(props[i].name or "")
			if pname ~= "" then
				-- Bug.25: 属性名 "svn:ignore" / "custom:prop" / "svn:entry:committed-rev"
				-- prefix = 第一个冒号前, local = 第一个冒号后全部 (可含更多冒号)
				local xml_prefix, local_name
				local colon = string.find(pname, ":", 1, true)
				if colon then
					xml_prefix = string.sub(pname, 1, colon - 1)
					local_name = string.sub(pname, colon + 1)
				else
					xml_prefix = nil  -- 无前缀
					local_name = pname
				end
				-- svn: 前缀用 S XML 前缀, 其他用自身 prefix, 无前缀则直接用 local_name
				local elem_prefix
				if xml_prefix == "svn" then
					elem_prefix = "S"
				elseif xml_prefix and xml_prefix ~= "" then
					elem_prefix = xml_prefix
				else
					elem_prefix = nil
				end
				if local_name ~= "" then
					if elem_prefix then
						parts[#parts + 1] = "<" .. elem_prefix .. ":" .. local_name .. ">"
						parts[#parts + 1] = _xml_escape(props[i].value or "")
						parts[#parts + 1] = "</" .. elem_prefix .. ":" .. local_name .. ">\n"
					else
						parts[#parts + 1] = "<" .. local_name .. ">"
						parts[#parts + 1] = _xml_escape(props[i].value or "")
						parts[#parts + 1] = "</" .. local_name .. ">\n"
					end
				end
			end
		end
	end

	parts[#parts + 1] = "</D:prop>\n<D:status>HTTP/1.1 200 OK</D:status>\n</D:propstat>\n</D:response>\n"
	return table.concat(parts)
end


local function _do_options(req, res, cfg, repo, user, pass)
	local ra, err = _ra(cfg, repo, user, pass)
	if not ra then
		_send_ra_fail(res, cfg, repo, err)
		return
	end

	local hs, e2 = _dav_headers(ra, repo)
	if not hs then
		_send_ra_error(res, ra, "svn:ra-error", tostring(e2))
		return
	end

	local body = table.concat({
		'<?xml version="1.0" encoding="utf-8"?>\n',
		'<D:options-response xmlns:D="DAV:">\n',
		"<D:activity-collection-set><D:href>/svn/", repo, "/!svn/act/</D:href>",
		"</D:activity-collection-set></D:options-response>\n",
	})
	hs[#hs + 1] = { "Content-Type", 'text/xml; charset="utf-8"' }
	_send(res, 200, body, hs)
end


local function _do_propfind(req, res, cfg, repo, rest, user, pass)
	local ra, err = _ra(cfg, repo, user, pass)
	if not ra then
		_send_ra_fail(res, cfg, repo, err)
		return
	end

	local young = ra:youngest()
	local uuid = ra:uuid()
	if not young or not uuid then
		_send_ra_error(res, ra, "svn:ra-error", "youngest/uuid failed")
		return
	end

	local depth = _header_get(req.headers, "depth") or "0"
	local txr_txn, txr_rel = string.match(rest, "^/!svn/txr/([^/]+)/?(.*)$")
	if txr_txn then
		_do_propfind_txr(req, res, cfg, repo, user, pass, ra, young, uuid, depth, txr_txn, txr_rel or "")
		return
	end

	-- 允许 /!svn/rvr/2 与 /!svn/rvr/2/path (无尾斜杠亦可)
	local rvr_rev_s, rvr_rel = string.match(rest, "^/!svn/rvr/(%d+)/?(.*)$")
	local is_rvr = (rvr_rev_s ~= nil)
	local rev = young
	local brp = ""

	if is_rvr then
		rev = tonumber(rvr_rev_s) or young
		brp = string.gsub(rvr_rel or "", "/$", "")
	elseif not string.match(rest, "^/!svn/") then
		brp = string.gsub(rest, "^/", "")
		brp = string.gsub(brp, "/$", "")
	end

	local author = ""
	local date = ""
	local size = "0"
	local kind = "dir"
	local href
	local ents = nil

	if is_rvr then
		kind = "dir"
		href = string.format("/svn/%s/!svn/rvr/%d/%s", repo, rev, brp)
		if brp == "" and not string.match(href, "/$") then
			href = href .. "/"
		end
		ents = ra:list(brp, rev)
		if ents then
			kind = "dir"
			if #ents > 0 and ents[1].author and ents[1].author ~= "" then
				author = ents[1].author
			end
			if #ents > 0 and ents[1].date and ents[1].date ~= "" then
				date = ents[1].date
			end
		elseif brp ~= "" then
			local data = ra:cat(brp, rev)
			if not data then
				_send_propfind_missing(res, href)
				return
			end
			kind = "file"
			size = tostring(#data)
		else
			kind = "dir"
		end
	elseif string.match(rest, "^/!svn/") then
		kind = "dir"
		href = string.format("/svn/%s%s", repo, rest)
		if not string.match(href, "/$") then
			href = href .. "/"
		end
		brp = ""
	else
		href = string.format("/svn/%s/%s", repo, brp)
		if brp == "" then
			ents = ra:list(brp, rev) or {}
			kind = "dir"
			if #ents > 0 and ents[1].author and ents[1].author ~= "" then
				author = ents[1].author
			end
			if #ents > 0 and ents[1].date and ents[1].date ~= "" then
				date = ents[1].date
			end
		else
			ents = ra:list(brp, rev)
			if ents then
				kind = "dir"
				if #ents > 0 and ents[1].author and ents[1].author ~= "" then
					author = ents[1].author
				end
				if #ents > 0 and ents[1].date and ents[1].date ~= "" then
					date = ents[1].date
				end
			else
				local data = ra:cat(brp, rev)
				if data then
					kind = "file"
					size = tostring(#data)
				else
					_send_propfind_missing(res, href)
					return
				end
			end
		end
	end

	local lock_info = _lock_get(cfg, repo, brp)
	local svn_ns = _propfind_svn_ns(req.body)
	local props = nil
	local perr = nil
	props, perr = ra:dir_props(brp, rev)
	local parts = {
		'<?xml version="1.0" encoding="utf-8"?>\n',
		'<D:multistatus xmlns:D="DAV:">\n',
		_propfind_response(repo, uuid, href, brp, kind, rev, author, date, size, nil, lock_info, props, svn_ns),
	}

	if depth == "1" and kind == "dir" and ents then
		for i = 1, #ents do
			local e = ents[i]
			local name = e.name or ""
			if name ~= "" and not string.find(name, "/", 1, true) then
				local child_rel = (brp ~= "" and (brp .. "/" .. name)) or name
				local child_href
				if is_rvr then
					child_href = string.format("/svn/%s/!svn/rvr/%d/%s", repo, rev, child_rel)
				else
					child_href = string.format("/svn/%s/%s", repo, child_rel)
				end
				local child_props, _ = ra:dir_props(child_rel, rev)
				local child_lock = _lock_get(cfg, repo, child_rel)
				parts[#parts + 1] = _propfind_response(
					repo, uuid, child_href, child_rel, e.kind or "file",
					e.created_rev or rev, e.author or author, e.date or date, e.size or 0,
					nil, child_lock, child_props, svn_ns)
			end
		end
	end

	parts[#parts + 1] = "</D:multistatus>\n"
	_send(res, 207, table.concat(parts), {
		{ "Content-Type", 'text/xml; charset="utf-8"' },
		{ "Server", "alua-svn-https" },
	})
end


local function _collect_log_deletions(ra, entry_rev, rev)
	local del = {}

	if type(entry_rev) ~= "number" or type(rev) ~= "number" or entry_rev >= rev then
		return del
	end

	local ents = ra:log({
		start = entry_rev + 1,
		["end"] = rev,
		changed_paths = true,
	}) or {}

	for i = 1, #ents do
		local cps = ents[i].changed_paths
		if type(cps) == "table" then
			for j = 1, #cps do
				local cp = cps[j]
				-- 仅纯删除; R(replace) 路径仍存在, 由增量 open-file + delta 更新
				if "D" == (cp.action or "") then
					local p = _rel_clean(string.gsub(cp.path or "", "^/", ""))
					if p ~= "" then
						del[p] = true
					end
				end
			end
		end
	end

	return del
end


local function _parse_update_scope(text, repo)
	local send_all = string.find(text, 'send-all="false"', 1, true) == nil
	local scopes = {}
	local seen = {}

	for href in string.gmatch(text or "", "/svn/" .. repo .. "/([^<\"%s]+)") do
		href = _rel_clean(string.gsub(href, "/$", ""))
		if href ~= "" and not string.match(href, "^!svn/") and not seen[href] then
			seen[href] = true
			scopes[#scopes + 1] = href
		end
	end

	return send_all, scopes
end


local function _path_in_scope(path, scopes)
	if type(scopes) ~= "table" or #scopes == 0 then
		return true
	end

	path = _rel_clean(path)
	for i = 1, #scopes do
		local s = scopes[i]
		if path == s or string.sub(path, 1, #s + 1) == s .. "/" then
			return true
		end
	end

	return false
end


local function _list_rows_scoped(ra, rev, scopes)
	if type(scopes) ~= "table" or #scopes == 0 then
		return ra:list_r("", rev) or {}
	end

	local out = {}
	local seen = {}

	for i = 1, #scopes do
		local rows = ra:list_r(scopes[i], rev) or {}
		for j = 1, #rows do
			local name = rows[j].name or ""
			if name ~= "" and not seen[name] then
				seen[name] = true
				out[#out + 1] = rows[j]
			end
		end
	end

	return out
end


-- @brief update-report 基准路径 (子目录 checkout): 取 <S:src-path>/<S:url> 的仓内相对前缀
-- @param [in] text[string] REPORT 请求体
-- @param [in] repo[string] 仓名
-- @return [string] 仓内相对路径 (仓根为空串)
local function _parse_update_base(text, repo)
	local raw = string.match(text or "", "<%a+:src%-path>(.-)</%a+:src%-path>")
		or string.match(text or "", "<%a+:url>(.-)</%a+:url>")
	raw = _xml_unescape(raw or "")
	if "" == raw then
		return ""
	end

	raw = string.gsub(raw, "^[%a]+://[^/]+", "")
	local prefix = "/svn/" .. repo
	if string.sub(raw, 1, #prefix) ~= prefix then
		return ""
	end

	return _rel_clean(string.sub(raw, #prefix + 1))
end


local TXN_TTL_SEC = tonumber(os.getenv("ALUA_SVN_TXN_TTL")) or 3600
-- 单文件上限与整体提交上限分开: FILE 只看单个 PUT 的明文字节, TXN 看本事务累计
local FILE_MAX_BYTES = tonumber(os.getenv("ALUA_SVN_FILE_MAX_BYTES")) or (512 * 1024 * 1024)
local TXN_MAX_BYTES = tonumber(os.getenv("ALUA_SVN_TXN_MAX_BYTES")) or (4 * 1024 * 1024 * 1024)
local CAT_MAX_BYTES = tonumber(os.getenv("ALUA_SVN_CAT_MAX")) or (64 * 1024 * 1024)
-- replay 单文件 textdelta 累积上限 (字节); 超限拒绝该 REPORT, 避免整文件累积失控
local REPLAY_DELTA_MAX_BYTES = tonumber(os.getenv("ALUA_SVN_REPLAY_DELTA_MAX")) or (64 * 1024 * 1024)


-- PUT spool: 请求体落进程临时文件, txn 只留路径; MERGE 前再逐个读回
-- 目的: 多文件 txn 期间不把全部 body 常驻内存 (L2 一次性交 body, 无法真正的边收边写)
local SPOOL_DIR = os.getenv("ALUA_SVN_SPOOL_DIR")
	or ((os.getenv("TMPDIR") or "/tmp") .. "/alua_svn_spool")
local SPOOL_TTL_SEC = (tonumber(os.getenv("ALUA_SVN_SPOOL_TTL_MIN")) or 1440) * 60
local SPOOL_ON = (os.getenv("ALUA_SVN_SPOOL") ~= "0")
local _SPOOL_SEQ = 0
local _SPOOL_GC_DONE = false


-- @brief 确保 spool 目录存在 (lfs, 跨平台)
local function _spool_dir()
	if not lfs then
		return nil
	end

	if "directory" == lfs.attributes(SPOOL_DIR, "mode") then
		return SPOOL_DIR
	end

	lfs.mkdir(SPOOL_DIR)
	if "directory" == lfs.attributes(SPOOL_DIR, "mode") then
		return SPOOL_DIR
	end

	return nil
end


-- @brief 清理超期残留 (进程崩溃或客户端放弃 txn 的遗留)
local function _spool_gc()
	local d = nil
	local now = os.time()

	if _SPOOL_GC_DONE or not SPOOL_ON or SPOOL_TTL_SEC <= 0 then
		return
	end

	_SPOOL_GC_DONE = true
	d = _spool_dir()
	if not d then
		return
	end

	for name in lfs.dir(d) do
		if "." ~= string.sub(name, 1, 1) then
			local p = d .. "/" .. name
			local mtime = lfs.attributes(p, "modification")
			if mtime and (now - mtime) > SPOOL_TTL_SEC then
				os.remove(p)
			end
		end
	end
end


-- @brief 请求体落盘; 返回 path, size (目录不可用时返回 nil)
local function _spool_write(txn, tag, body)
	local d = _spool_dir()
	local name = nil
	local p = nil
	local f = nil

	if not d then
		return nil
	end

	_SPOOL_SEQ = _SPOOL_SEQ + 1
	name = string.format("%s_%s_%d_%d_%d", tostring(txn or "txn"):gsub("[^%w]", "_"),
		tag or "put", os.time(), _SPOOL_SEQ, math.random(1, 65535))
	p = d .. "/" .. name

	f = io.open(p, "wb")
	if not f then
		return nil
	end

	f:write(body or "")
	f:close()

	return p, #(body or "")
end


local function _spool_read(p)
	local f = io.open(p, "rb")
	local s = nil

	if not f then
		return nil
	end

	s = f:read("*a")
	f:close()
	return s
end


local function _spool_drop(p)
	if type(p) == "string" and "" ~= p then
		os.remove(p)
	end
end


-- @brief 释放 txn 内全部 spool 文件 (路径去重, 复制 op 会共享同一文件)
local function _txn_spool_free(t)
	local seen = {}

	if type(t) ~= "table" or type(t.ops) ~= "table" then
		return
	end

	for i = 1, #t.ops do
		local p = t.ops[i].spool_path
		if type(p) == "string" and "" ~= p and not seen[p] then
			seen[p] = true
			_spool_drop(p)
		end
	end
end


-- @brief 把 spool 的 op 读回内存 (保留 spool_path 供 txn 收尾统一删除)
-- @return op | nil, err
local function _op_materialize(op)
	local p = nil
	local s = nil

	if type(op) ~= "table" or type(op.spool_path) ~= "string" then
		return op
	end

	p = op.spool_path
	s = _spool_read(p)
	if nil == s then
		return nil, "spool read failed: " .. p
	end

	if op.spool_svndiff then
		op.svndiff = s
	else
		op.data = s
	end

	return op
end


local function _safe_cat(ra, path, rev)
	local data = ra:cat(path, rev)
	if data == nil then
		return nil
	end

	if CAT_MAX_BYTES > 0 and #data > CAT_MAX_BYTES then
		return nil, "too_large"
	end

	return data
end


-- @brief update-report 目录属性差分 (inline-props)
-- @param [in] base_rev[number] 客户端基版本; nil 表示新增目录, 不读取旧属性
-- @note mod_dav_svn 使用 S:set-prop/S:remove-prop, name 为原始 svn 属性名 (如 svn:ignore/x:note)
local function _update_emit_dir_props(parts, ra, path, base_rev, rev)
	if nil ~= base_rev and base_rev == rev then
		return
	end

	local new_props = ra:dir_props(path, rev) or {}
	local old_props = {}
	if nil ~= base_rev and base_rev >= 0 then
		old_props = ra:dir_props(path, base_rev) or {}
	end

	local function _prop_map(props)
		local map = {}
		for i = 1, #props do
			local name = tostring(props[i].name or "")
			-- 排除 entry/wc 属性: mod_dav_svn send_propchange 亦跳过
			if "" ~= name and "svn:entry:" ~= string.sub(name, 1, 10)
				and "svn:wc:" ~= string.sub(name, 1, 7) then
				map[name] = tostring(props[i].value or "")
			end
		end
		return map
	end

	local new_map = _prop_map(new_props)
	local old_map = _prop_map(old_props)

	local names = {}
	for name in pairs(new_map) do
		names[#names + 1] = name
	end
	for name in pairs(old_map) do
		if nil == new_map[name] then
			names[#names + 1] = name
		end
	end
	table.sort(names)

	for i = 1, #names do
		local name = names[i]
		local escaped = _xml_escape(name)
		if nil == new_map[name] then
			parts[#parts + 1] = string.format('<S:remove-prop name="%s"/>\n', escaped)
		elseif old_map[name] ~= new_map[name] then
			local value = new_map[name]
			-- 控制字符等非 XML 安全值走 base64 (mod_dav_svn send_propchange)
			if string.find(value, "[%z\1-\8\11\12\14-\31]") then
				parts[#parts + 1] = string.format('<S:set-prop name="%s" encoding="base64">%s</S:set-prop>\n',
					escaped, kcrypto.base64_encode(value))
			else
				parts[#parts + 1] = string.format('<S:set-prop name="%s">%s</S:set-prop>\n',
					escaped, _xml_escape(value))
			end
		end
	end
end


local function _update_emit_tree(parts, ra, repo, uuid, rev, entry_rev, view, author, cdate, rows, bulk, scopes, base, flush)
	local have = view.have or {}
	local depth_map = view.depth or {}
	local missing = view.missing or {}
	local emitted = {}
	local open_stack = {}
	local open_tag = {}
	base = base or ""

	-- 大树 update 逐文件在 Lua 内算 SHA1/MD5/svndiff, 周期性让出事件循环,
	-- 避免单个请求把 8443 全部会话一起卡住
	local function _yield_update()
		if type(kco) == "table" and type(kco.co_sleep) == "function" then
			kco.co_sleep(1)
		end
	end

	-- 子目录 checkout: 仅输出 base 子树, 根目录即 base 本身
	local function in_base(p)
		if "" == base or p == base then
			return true
		end

		return string.sub(p, 1, #base + 1) == base .. "/"
	end

	-- 条目在指定版本是否存在 (按父目录列目录缓存; 返回 kind, 缺失为 nil)
	local list_cache = {}
	local function _stat_at(path, at_rev)
		local parent = string.match(path, "^(.*)/[^/]+$") or ""
		local name = string.match(path, "([^/]+)$") or path
		local key = tostring(at_rev) .. "\0" .. parent
		local ents = list_cache[key]
		if nil == ents then
			ents = ra:list(parent, at_rev)
			if type(ents) ~= "table" then
				ents = {}
			end
			list_cache[key] = ents
		end

		for i = 1, #ents do
			if ents[i].name == name then
				return ents[i].kind or ""
			end
		end

		return nil
	end

	-- 客户端在 path 上的基版本; nil 表示客户端无该条目 (missing/depth 之外/不在 base 内)
	local function _client_base(path)
		local cur = path
		while true do
			if nil ~= missing[cur] then
				return nil
			end

			local rv = have[cur]
			if nil ~= rv then
				if cur == path then
					return rv
				end

				-- 未上报子项: 按最近上报祖先的 depth 推断是否可见
				local dp = depth_map[cur] or "infinity"
				local dist = _path_depth(path) - _path_depth(cur)
				if "empty" == dp then
					return nil
				end
				if ("files" == dp or "immediates" == dp) and dist > 1 then
					return nil
				end

				-- 继承版本下不存在的路径客户端不可能持有 (如上一轮 update 已删除的节点)
				local kind = _stat_at(path, rv)
				if nil == kind then
					return nil
				end
				-- depth=files 只含文件, 直接子目录不在客户端
				if "files" == dp and "dir" == kind then
					return nil
				end
				return rv
			end

			if cur == base or "" == cur then
				return nil
			end
			cur = string.match(cur, "^(.*)/[^/]+$") or ""
		end
	end

	-- 待删除项 (在 open_dir 就绪后再发; delete-entry 须处于其父目录的 open-directory 作用域)
	local pending_deletes = {}
	if bulk then
		-- 客户端持有的条目若目标版本已无, 须 delete-entry; 候选来源:
		--  上报条目 (客户端领先于目标版本) / 前向区间日志删除项 (未上报的继承子项)
		--  / 回退时客户端基树 (entry_rev) 全部条目
		local cand = {}
		for p in pairs(have) do
			cand[p] = true
		end
		if entry_rev < rev then
			for p in pairs(_collect_log_deletions(ra, entry_rev, rev)) do
				cand[p] = true
			end
		elseif entry_rev > rev then
			local rows_base = _list_rows_scoped(ra, entry_rev, scopes)
			for i = 1, #rows_base do
				cand[rows_base[i].name or ""] = true
			end
		end

		local dels = {}
		for p in pairs(cand) do
			if p ~= base and in_base(p) and _path_in_scope(p, scopes)
				and nil ~= _client_base(p) and nil == _stat_at(p, rev)
			then
				dels[#dels + 1] = p
			end
		end

		-- 删除目录已涵盖其子项: 仅保留最顶层删除项, 避免子项在错误父目录下输出 (E155010)
		table.sort(dels, function(a, b)
			return _path_depth(a) < _path_depth(b)
		end)
		local kept_dirs = {}
		for i = 1, #dels do
			local p = dels[i]
			local covered = false
			for k = 1, #kept_dirs do
				if string.sub(p, 1, #kept_dirs[k] + 1) == kept_dirs[k] .. "/" then
					covered = true
					break
				end
			end
			if not covered then
				pending_deletes[#pending_deletes + 1] = p
				local cb = _client_base(p)
				if nil ~= cb and "dir" == _stat_at(p, cb) then
					kept_dirs[#kept_dirs + 1] = p
				end
			end
		end
	end

	local function ensure_dirs(dpath)
		if "" == dpath or emitted[dpath] or not in_base(dpath) then
			return
		end

		-- base 即本响应的 open-directory 根, 不再作为子目录输出
		if dpath == base then
			emitted[dpath] = true
			return
		end

		local parent = string.match(dpath, "^(.*)/[^/]+$") or ""
		if parent ~= "" then
			ensure_dirs(parent)
		end

		local drev = rev
		for i = 1, #rows do
			local p, k, lr = rows[i].name, rows[i].kind, rows[i].created_rev
			if k == "dir" and p == dpath then
				drev = lr or rev
				break
			end
		end

		local name = string.match(dpath, "([^/]+)$") or dpath
		-- 增量 update: 客户端已持有的目录须 open-directory;
		-- 否则客户端把 add-directory 视作障碍工作副本, 整棵子树被跳过 (新增文件丢失)
		local tag = "add-directory"
		local rev_attr = ""
		local base_rev = _client_base(dpath)
		if bulk and nil ~= base_rev then
			tag = "open-directory"
			rev_attr = string.format(' rev="%d"', base_rev)
		end

		parts[#parts + 1] = string.format('<S:%s%s name="%s">\n', tag, rev_attr, _xml_escape(name))
		parts[#parts + 1] = string.format(
			"<D:checked-in><D:href>/svn/%s/!svn/rvr/%d/%s/</D:href></D:checked-in>\n",
			repo, drev, _xml_escape(dpath))
		parts[#parts + 1] = string.format('<S:set-prop name="svn:entry:committed-rev">%d</S:set-prop>\n', drev)
		parts[#parts + 1] = string.format('<S:set-prop name="svn:entry:uuid">%s</S:set-prop>\n', _xml_escape(uuid))
		_update_emit_dir_props(parts, ra, dpath, ("open-directory" == tag) and base_rev or nil, rev)
		emitted[dpath] = true
		open_tag[dpath] = tag
	end

	local function close_to(prefix)
		while #open_stack > 0 do
			local top = open_stack[#open_stack]
			if prefix == top or string.sub(prefix, 1, #top + 1) == top .. "/" then
				break
			end
			open_stack[#open_stack] = nil
			parts[#parts + 1] = "</S:" .. (open_tag[top] or "add-directory") .. ">\n"
		end
	end

	local function append_file(path, frev, fdate, fauth, is_open, base_rev)
		local old_rev = is_open and (base_rev or entry_rev) or -1
		local td, sha1, md5 = ra:txdelta_file(path, rev, old_rev, CAT_MAX_BYTES)

		if nil == td then
			if "too_large" == sha1 then
				io.stderr:write("alua svn_https: skip large file in update: " .. path .. "\n")
			end
			return
		end

		local name = string.match(path, "([^/]+)$") or path
		local tag = is_open and "open-file" or "add-file"
		local rev_attr = ""
		if is_open then
			rev_attr = string.format(' rev="%d"', base_rev or entry_rev)
		end
		local file_parts = {
			string.format('<S:%s%s name="%s" sha1-checksum="%s">\n', tag, rev_attr, _xml_escape(name), sha1),
			string.format(
				"<D:checked-in><D:href>/svn/%s/!svn/rvr/%d/%s</D:href></D:checked-in>\n",
				repo, frev, _xml_escape(path)),
			string.format('<S:set-prop name="svn:entry:committed-rev">%d</S:set-prop>\n', frev),
		}
		if type(fdate) == "string" and fdate ~= "" then
			file_parts[#file_parts + 1] = string.format(
				'<S:set-prop name="svn:entry:committed-date">%s</S:set-prop>\n', _xml_escape(fdate))
		end
		if type(fauth) == "string" and fauth ~= "" then
			file_parts[#file_parts + 1] = string.format(
				'<S:set-prop name="svn:entry:last-author">%s</S:set-prop>\n', _xml_escape(fauth))
		end
		file_parts[#file_parts + 1] = string.format(
			'<S:set-prop name="svn:entry:uuid">%s</S:set-prop>\n', _xml_escape(uuid))
		if "" == td then
			file_parts[#file_parts + 1] = string.format(
				"<S:prop><V:md5-checksum>%s</V:md5-checksum></S:prop></S:%s>\n",
				md5, tag)
		else
			-- 大 b64 单独成段, 避免 format/拼接多一次整串复制
			file_parts[#file_parts + 1] = "<S:txdelta>"
			file_parts[#file_parts + 1] = td
			file_parts[#file_parts + 1] = string.format(
				"</S:txdelta><S:prop><V:md5-checksum>%s</V:md5-checksum></S:prop></S:%s>\n",
				md5, tag)
		end
		parts[#parts + 1] = table.concat(file_parts)
	end

	-- 目录与文件统一按路径序输出; 旧逻辑只在文件的父链上 ensure_dirs,
	-- 无子文件的目录 (空目录) 永不输出 add-directory, 客户端不建该目录
	local entries = {}
	for i = 1, #rows do
		local kind = rows[i].kind
		if "file" == kind or "dir" == kind then
			entries[#entries + 1] = rows[i]
		end
	end
	table.sort(entries, function(a, b) return (a.name or "") < (b.name or "") end)

	-- 打开 dpath 的目录链并置于 open_stack 顶; 空/base 表示响应根 (不输出目录标签)
	local function open_dir(dpath)
		if "" == dpath or dpath == base then
			close_to("")
			return
		end

		-- 已在栈顶: 目录保持打开, 直接复用
		if open_stack[#open_stack] == dpath then
			return
		end

		-- 保留 dpath 及其祖先, 闭合更深项与兄弟 (回退时 dpath 本身不被闭合)
		close_to(dpath)

		-- 由浅到深逐层确保并入栈 (ensure_dirs 只在首次输出目录标签)
		local chain = {}
		local cur = dpath
		while "" ~= cur and cur ~= base do
			chain[#chain + 1] = cur
			cur = string.match(cur, "^(.*)/[^/]+$") or ""
		end

		-- 已在栈中的祖先不重复入栈 (重复 push 会多出闭合标签, XML 失配)
		local instack = {}
		for si = 1, #open_stack do
			instack[open_stack[si]] = true
		end
		for ci = #chain, 1, -1 do
			cur = chain[ci]
			if not emitted[cur] then
				ensure_dirs(cur)
			end
			if emitted[cur] and not instack[cur] then
				open_stack[#open_stack + 1] = cur
				instack[cur] = true
			end
		end
	end

	-- 先发删除项 (深到浅): delete-entry 必须处于其父目录的 open-directory 作用域,
	-- 否则客户端会按当前目录解析名字, 报 E155010 找不到节点
	table.sort(pending_deletes, function(a, b)
		return _path_depth(a) > _path_depth(b)
	end)
	for i = 1, #pending_deletes do
		local p = pending_deletes[i]
		open_dir(string.match(p, "^(.*)/[^/]+$") or "")
		local name = string.match(p, "([^/]+)$") or p
		parts[#parts + 1] = string.format('<S:delete-entry name="%s"/>\n', _xml_escape(name))
	end

	for i = 1, #entries do
		-- 定期让出事件循环, 避免大树 update 独占 8443 (每条目让出会破坏响应写出)
		if 0 == (i % 16) then
			_yield_update()
		end

		-- M2: 每条目后落一次响应; 客户端断开时写失败 → 立即中止整树构建
		if flush and not flush() then
			return false
		end

		local path = entries[i].name or ""
		if not in_base(path) or not _path_in_scope(path, scopes) then
			-- base 外 / 不在 REPORT scope: 跳过
		elseif "dir" == entries[i].kind then
			-- 空目录靠此显式输出; 非空目录的父链由文件路径顺带打开
			if path ~= base then
				open_dir(path)
			end
		else
			local frev = entries[i].created_rev or rev
			local is_open = false
			local skip = false
			local base_rev = nil
			if bulk then
				-- 客户端视图 (上报/继承) 持有条目即 open-file;
				-- 按根 entry_rev 推断会把混合版本 WC 已持有的文件误判为 add-file (障碍工作副本)
				base_rev = _client_base(path)
				is_open = (nil ~= base_rev)
				-- 前向增量且客户端未上报缺失: 客户端不持有该旧文件属 sparse checkout, 跳过;
				-- 显式 missing / 回退到旧版本时须 add-file 补出
				if not is_open and not missing[path] and entry_rev < rev
					and (frev or 0) <= entry_rev
				then
					skip = true
				end
			end

			if not skip then
				open_dir(string.match(path, "^(.*)/[^/]+$") or "")
				append_file(path, frev, entries[i].date or cdate, entries[i].author or author,
					is_open, base_rev)
			end
		end
	end

	while #open_stack > 0 do
		local top = open_stack[#open_stack]
		open_stack[#open_stack] = nil
		parts[#parts + 1] = "</S:" .. (open_tag[top] or "add-directory") .. ">\n"
	end

	return true
end


-- @brief 拼接仓内相对路径 (空串视为仓根)
local function _rel_join(a, b)
	a = _rel_clean(a)
	b = _rel_clean(b)
	if "" == a then
		return b
	end
	if "" == b then
		return a
	end
	return a .. "/" .. b
end


-- @brief 解析 do_diff 请求: <S:dst-path>/<S:versus-url> + update-target + depth
--  @return [table|nil] { dst=仓内目的路径, target=源侧相对目标, depth=深度 }
local function _parse_update_diff(text, repo)
	local raw = string.match(text or "", "<%a+:dst%-path>(.-)</%a+:dst%-path>")
		or string.match(text or "", "<%a+:versus%-url>(.-)</%a+:versus%-url>")
	if not raw then
		return nil
	end

	raw = _xml_unescape(raw)
	raw = string.gsub(raw, "^[%a]+://[^/]+", "")
	local prefix = "/svn/" .. repo
	if string.sub(raw, 1, #prefix) ~= prefix then
		return nil
	end

	local target = _xml_unescape(string.match(text or "",
		"<%a+:update%-target>(.-)</%a+:update%-target>") or "")
	local depth = string.match(text or "", "<%a+:depth>(.-)</%a+:depth>") or "infinity"
	if "" == depth then
		depth = "infinity"
	end

	return {
		dst = _rel_clean(string.sub(raw, #prefix + 1)),
		target = _rel_clean(target),
		depth = depth,
	}
end


-- @brief 列出 root 子树, 键为相对 root 的路径 (root 自身为 "")
local function _diff_row_map(ra, root, rev)
	local map = {}
	local ok, rows = pcall(function()
		return ra:list_r(root, rev)
	end)

	if not ok or type(rows) ~= "table" then
		return map
	end

	for i = 1, #rows do
		local row = rows[i]
		local p = _rel_clean(row.name or "")
		local rel = nil
		if "" == root then
			rel = p
		elseif p == root then
			rel = ""
		elseif string.sub(p, 1, #root + 1) == root .. "/" then
			rel = string.sub(p, #root + 2)
		end
		if rel and not map[rel] then
			map[rel] = row
		end
	end

	return map
end


-- @brief 取相对路径的父目录 (顶层返回空串)
local function _rel_dirname(rel)
	local parent = string.match(_rel_clean(rel), "^(.*)/[^/]+$")
	return parent or ""
end


-- @brief 取 root 自身条目 (list_r 不含根条目, 需列其父层再匹配)
local function _diff_root_row(ra, root, rev)
	root = _rel_clean(root)
	if "" == root then
		return { name = "", kind = "dir", created_rev = rev }
	end

	local ok, rows = pcall(function()
		return ra:list_r(_rel_dirname(root), rev)
	end)
	if not ok or type(rows) ~= "table" then
		return nil
	end

	for i = 1, #rows do
		if _rel_clean(rows[i].name or "") == root then
			return rows[i]
		end
	end

	return nil
end


-- @brief 相对路径表 → 父路径下的直接子项索引
local function _diff_child_index(map)
	local idx = {}

	for rel, row in pairs(map) do
		if "" ~= rel then
			local parent, name = string.match(rel, "^(.*)/([^/]+)$")
			if not parent then
				parent = ""
				name = rel
			end
			idx[parent] = idx[parent] or {}
			idx[parent][name] = row
		end
	end

	return idx
end


-- @brief 输出单个文件的 add-file / open-file (skelta: 文本由客户端 fetch)
-- @return true 继续; false 已断流 (flush 检测到写入失败)
local function _diff_emit_file(parts, ra, repo, uuid, rev, entry_rev, name, drow, srow, srel, drel, flush)
	local data = _safe_cat(ra, drel, rev)
	if nil == data then
		return true
	end

	local is_open = (nil ~= srow)
	local old = ""
	local base_md5 = nil
	if is_open then
		old = _safe_cat(ra, srel, entry_rev) or ""
		if old == data then
			return true
		end
		base_md5 = ksvnra.md5_hex(old)
	end

	local sha1 = ksvnra.sha1_hex(data)
	local frev = drow.created_rev or rev
	local file_parts = {
		string.format('<S:%s%s name="%s" sha1-checksum="%s">\n',
			is_open and "open-file" or "add-file",
			is_open and string.format(' rev="%d"', entry_rev) or "",
			_xml_escape(name), sha1),
		string.format(
			"<D:checked-in><D:href>/svn/%s/!svn/rvr/%d/%s</D:href></D:checked-in>\n",
			repo, frev, _xml_escape(drel)),
		string.format('<S:set-prop name="svn:entry:committed-rev">%d</S:set-prop>\n', frev),
	}
	if type(drow.date) == "string" and drow.date ~= "" then
		file_parts[#file_parts + 1] = string.format(
			'<S:set-prop name="svn:entry:committed-date">%s</S:set-prop>\n', _xml_escape(drow.date))
	end
	if type(drow.author) == "string" and drow.author ~= "" then
		file_parts[#file_parts + 1] = string.format(
			'<S:set-prop name="svn:entry:last-author">%s</S:set-prop>\n', _xml_escape(drow.author))
	end
	file_parts[#file_parts + 1] = string.format(
		'<S:set-prop name="svn:entry:uuid">%s</S:set-prop>\n', _xml_escape(uuid))

	if is_open then
		file_parts[#file_parts + 1] = string.format(
			'<S:fetch-file base-checksum="%s" sha1-checksum="%s"/>\n', base_md5, sha1)
	end
	file_parts[#file_parts + 1] = string.format("</S:%s>\n",
		is_open and "open-file" or "add-file")

	parts[#parts + 1] = table.concat(file_parts)
	if flush and not flush() then
		return false
	end

	return true
end


-- @brief 两棵树差分: 源树 @entry_rev vs 目的树 @rev (update-report/do_diff 语义)
-- @return true 继续; false 已断流
local function _diff_walk(parts, ra, repo, uuid, rev, entry_rev,
		smap, sidx, didx, sroot, droot, rel_dir, descend, flush)
	local schildren = sidx[rel_dir] or {}
	local dchildren = didx[rel_dir] or {}
	local dnames = {}

	for name, drow in pairs(dchildren) do
		local srow = schildren[name]
		if srow and (srow.kind or "file") ~= (drow.kind or "file") then
			parts[#parts + 1] = string.format(
				'<S:delete-entry name="%s" rev="%d"/>\n', _xml_escape(name), entry_rev)
			srow = nil
		end
		dnames[#dnames + 1] = name
		dchildren[name] = { row = drow, src = srow }
	end

	for name in pairs(schildren) do
		if not dchildren[name] then
			parts[#parts + 1] = string.format(
				'<S:delete-entry name="%s" rev="%d"/>\n', _xml_escape(name), entry_rev)
		end
	end

	table.sort(dnames)
	for i = 1, #dnames do
		local name = dnames[i]
		local pair = dchildren[name]
		local drow = pair.row
		local srow = pair.src
		local drel = _rel_join(droot, _rel_join(rel_dir, name))
		local srel = _rel_join(sroot, _rel_join(rel_dir, name))

		if "dir" == (drow.kind or "") then
			local crev = drow.created_rev or rev
			local open_tag = srow and "open-directory" or "add-directory"
			parts[#parts + 1] = string.format('<S:%s name="%s"%s>\n', open_tag,
				_xml_escape(name), srow and string.format(' rev="%d"', entry_rev) or "")
			parts[#parts + 1] = string.format(
				"<D:checked-in><D:href>/svn/%s/!svn/rvr/%d/%s/</D:href></D:checked-in>\n",
				repo, crev, _xml_escape(drel))
			parts[#parts + 1] = string.format(
				'<S:set-prop name="svn:entry:committed-rev">%d</S:set-prop>\n', crev)
			parts[#parts + 1] = string.format(
				'<S:set-prop name="svn:entry:uuid">%s</S:set-prop>\n', _xml_escape(uuid))

			if descend then
				local sub_smap = srow and smap or {}
				local sub_sidx = srow and sidx or {}
				if not _diff_walk(parts, ra, repo, uuid, rev, entry_rev,
						sub_smap, sub_sidx, didx, sroot, droot, _rel_join(rel_dir, name), descend, flush) then
					return false
				end
			end

			parts[#parts + 1] = string.format("</S:%s>\n", open_tag)
		elseif "file" == (drow.kind or "file") then
			if not _diff_emit_file(parts, ra, repo, uuid, rev, entry_rev,
					name, drow, srow, srel, drel, flush) then
				return false
			end
		end
	end

	return true
end


-- @brief 输出 do_diff 的 update-target 子树 (首层名保留源侧名, 对齐 mod_dav_svn pathmap)
-- @return true 继续; false 已断流
local function _diff_emit_target(parts, ra, repo, uuid, rev, entry_rev, name,
		smap, sidx, didx, dmap, sroot, droot, walk, descend, flush)
	local srow = _diff_root_row(ra, sroot, entry_rev)
	local drow = _diff_root_row(ra, droot, rev)
	local skind = srow and (srow.kind or "file") or nil
	local dkind = drow and (drow.kind or "file") or nil

	if drow and "dir" == dkind then
		if srow and "dir" ~= skind then
			parts[#parts + 1] = string.format(
				'<S:delete-entry name="%s" rev="%d"/>\n', _xml_escape(name), entry_rev)
			srow = nil
		end

		local crev = drow.created_rev or rev
		local tag = srow and "open-directory" or "add-directory"
		parts[#parts + 1] = string.format('<S:%s name="%s"%s>\n', tag,
			_xml_escape(name), srow and string.format(' rev="%d"', entry_rev) or "")
		parts[#parts + 1] = string.format(
			"<D:checked-in><D:href>/svn/%s/!svn/rvr/%d/%s/</D:href></D:checked-in>\n",
			repo, crev, _xml_escape(droot))
		parts[#parts + 1] = string.format(
			'<S:set-prop name="svn:entry:committed-rev">%d</S:set-prop>\n', crev)
		parts[#parts + 1] = string.format(
			'<S:set-prop name="svn:entry:uuid">%s</S:set-prop>\n', _xml_escape(uuid))

		if walk then
			if not _diff_walk(parts, ra, repo, uuid, rev, entry_rev,
					smap, sidx, didx, sroot, droot, "", descend, flush) then
				return false
			end
		end

		parts[#parts + 1] = string.format("</S:%s>\n", tag)
	elseif drow and "file" == dkind then
		if srow and "file" ~= skind then
			parts[#parts + 1] = string.format(
				'<S:delete-entry name="%s" rev="%d"/>\n', _xml_escape(name), entry_rev)
			srow = nil
		end

		if not _diff_emit_file(parts, ra, repo, uuid, rev, entry_rev, name, drow,
				srow, sroot, droot, flush) then
			return false
		end
	elseif srow then
		parts[#parts + 1] = string.format(
			'<S:delete-entry name="%s" rev="%d"/>\n', _xml_escape(name), entry_rev)
	end

	return true
end


local function _update_report(res, cfg, repo, user, pass, body)
	local ra, err = _ra(cfg, repo, user, pass)
	if not ra then
		_send_ra_fail(res, cfg, repo, err)
		return
	end

	local text = body or ""
	local rev = tonumber(string.match(text, "<S:target%-revision>(%d+)</S:target%-revision>"))
		or tonumber(string.match(text, "target%-revision[^>]*>(%d+)"))
		or ra:youngest()
	-- plain find: 勿用 % 转义
	local start_empty = string.find(text, "start-empty", 1, true) ~= nil
	local entry_rev = tonumber(string.match(text, '<S:entry[^>]*rev="(%d+)"')) or rev
	local dif = _parse_update_diff(text, repo)
	local update_target = _xml_unescape(string.match(text,
		"<%a+:update%-target>(.-)</%a+:update%-target>") or "")
	local send_all, scopes = _parse_update_scope(text, repo)
	local base = _parse_update_base(text, repo)
	-- 普通 update 也带 depth; 不裁剪会把整树读一遍并算哈希, 大树直接卡死事件循环
	local depth = string.match(text, "<%a+:depth>(.-)</%a+:depth>") or "infinity"
	if "" == depth then
		depth = "infinity"
	end

	-- 客户端 REPORT 视图 (skelta): 仅列出版本与父项不一致的条目及本地缺失项;
	--  未列出的子项按最近上报祖先的版本与 depth 继承 (服务端据此推断客户端持有内容)
	local have = {}
	local depth_map = {}
	local missing = {}
	if not start_empty then
		have[base] = entry_rev
	end
	local function _add_entry(attrs, pname)
		local p = _rel_join(base, _rel_clean(_xml_unescape(pname)))
		if nil ~= string.find(attrs, 'presence="absent"', 1, true)
			or nil ~= string.find(attrs, 'presence="base-deleted"', 1, true)
		then
			missing[p] = true
			return
		end

		local r = tonumber(string.match(attrs, 'rev="(%d+)"')) or entry_rev
		have[p] = r
		depth_map[p] = string.match(attrs, 'depth="([%w]+)"') or "infinity"
	end
	for attrs, pname in string.gmatch(text, "<%a+:entry([^>]*)>(.-)</%a+:entry>") do
		_add_entry(attrs, pname)
	end
	for attrs in string.gmatch(text, "<%a+:entry([^>]*)/>") do
		_add_entry(attrs, "")
	end
	for pname in string.gmatch(text, "<%a+:missing>(.-)</%a+:missing>") do
		missing[_rel_join(base, _rel_clean(_xml_unescape(pname)))] = true
	end
	if "1" == os.getenv("ALUA_SVN_DAV_DEBUG") then
		local keys = {}
		for p, r in pairs(have) do
			keys[#keys + 1] = tostring(p) .. "@" .. tostring(r)
		end
		for p in pairs(missing) do
			keys[#keys + 1] = tostring(p) .. "!missing"
		end
		table.sort(keys)
		io.stderr:write(string.format("[dav] update rev=%s entry_rev=%s base=%s view=%s\n",
			tostring(rev), tostring(entry_rev), tostring(base), table.concat(keys, ",")))
	end
	local view = { have = have, depth = depth_map, missing = missing }
	local uuid = ra:uuid() or ""
	local author = user
	local cdate = ""
	local rows = {}
	if not dif then
		if send_all then
			rows = ra:list_r("", rev) or {}
		elseif #scopes > 0 then
			rows = _list_rows_scoped(ra, rev, scopes)
		else
			rows = ra:list_r("", rev) or {}
		end

		for i = 1, #rows do
			local e = rows[i]
			if type(e.date) == "string" and e.date ~= "" then
				if cdate == "" or (e.created_rev or 0) >= rev then
					cdate = e.date
				end
			end
			if type(e.author) == "string" and e.author ~= "" and (e.created_rev or 0) >= rev then
				author = e.author
			end
		end
	end

	-- depth=empty/files/immediates: 仅输出对应层级, 避免大树全量读取与哈希
	if not dif and "infinity" ~= depth and "unknown" ~= depth then
		rows = _rows_by_depth(rows, base, depth)
	end

	-- do_diff/switch: 编辑器根为源侧锚点 (src-path); update-target 子树保留源侧名
	--   (对齐 mod_dav_svn: src-path/update-target → dst-path 走 pathmap, 根仅在 target 为空时输出)
	local dif_target = dif and dif.target or ""
	local root_rel = base
	local root_is_tree = true
	if dif then
		root_rel = dif.dst
		root_is_tree = ("" == dif_target)
		send_all = false
	end

	local parts = {
		'<?xml version="1.0" encoding="utf-8"?>\n',
		string.format('<S:update-report xmlns:S="%s" xmlns:V="%s" xmlns:D="%s" send-all="%s" inline-props="true">\n',
			SVN_NS, SVN_DAV_NS, DAV_NS, send_all and "true" or "false"),
		string.format('<S:target-revision rev="%d"/>\n', rev),
		string.format('<S:open-directory rev="%d">\n', entry_rev),
	}

	-- M2: 分块直写响应; 客户端中断时写失败 → 中止整树构建 (原全程空转数十秒)
	local mime = 'text/xml; charset="utf-8"'
	local streaming = (type(res.co_chunk) == "function")
	local aborted = false

	local function _flush_parts()
		if not streaming then
			return true
		end
		if aborted then
			return false
		end
		if 0 == #parts then
			return true
		end

		local body = table.concat(parts)
		for i = #parts, 1, -1 do
			parts[i] = nil
		end

		res:co_chunk(body, mime)
		if res._stream_closed or not res._streaming then
			aborted = true
			return false
		end

		return true
	end

	if root_is_tree then
		parts[#parts + 1] = string.format(
			"<D:checked-in><D:href>/svn/%s/!svn/rvr/%d/%s</D:href></D:checked-in>\n",
			repo, rev, ("" ~= root_rel) and (root_rel .. "/") or "")
		-- Bug.2026-24: 目标为子项 (update-target 非空) 时根目录仅为锚点,
		--   客户端要求 propchanges 为 0 (update_editor.c close_directory 断言)
		if "" == update_target then
			parts[#parts + 1] = string.format(
				'<S:set-prop name="svn:entry:committed-rev">%d</S:set-prop>\n', rev)
			if cdate ~= "" then
				parts[#parts + 1] = string.format(
					'<S:set-prop name="svn:entry:committed-date">%s</S:set-prop>\n', _xml_escape(cdate))
			end
			if author ~= "" then
				parts[#parts + 1] = string.format(
					'<S:set-prop name="svn:entry:last-author">%s</S:set-prop>\n', _xml_escape(author))
			end
			parts[#parts + 1] = string.format(
				'<S:set-prop name="svn:entry:uuid">%s</S:set-prop>\n', _xml_escape(uuid))
			-- Bug.2026-23: 响应根目录属性 (仅变更项; 对齐 mod_dav_svn inline-props)
			_update_emit_dir_props(parts, ra, root_rel, (not start_empty and not dif) and entry_rev or nil, rev)
		end
	end

	if dif then
		-- 两 URL / WC↔URL 差分: 源树 @entry_rev vs 目的树 @rev
		local sroot = _rel_join(base, dif_target)
		local smap = _diff_row_map(ra, sroot, entry_rev)
		local dmap = _diff_row_map(ra, dif.dst, rev)
		local sidx = _diff_child_index(smap)
		local didx = _diff_child_index(dmap)

		-- depth=unknown 对 do_diff 等价于全递归
		local walk = ("empty" ~= dif.depth)
		local descend = ("files" ~= dif.depth and "immediates" ~= dif.depth)
		if "" == dif_target then
			if walk then
				_diff_walk(parts, ra, repo, uuid, rev, entry_rev,
					smap, sidx, didx, sroot, dif.dst, "", descend, _flush_parts)
			end
		else
			_diff_emit_target(parts, ra, repo, uuid, rev, entry_rev, dif_target,
				smap, sidx, didx, dmap, sroot, dif.dst, walk, descend, _flush_parts)
		end
	else
		-- skelta: send-all=false 时按 REPORT 内 href 限定 scope; 仍单包 bulk 响应
		local scope = (send_all and {}) or scopes
		if start_empty or entry_rev == 0 then
			_update_emit_tree(parts, ra, repo, uuid, rev, entry_rev, view, author, cdate, rows, false, scope, base, _flush_parts)
		elseif entry_rev < rev then
			_update_emit_tree(parts, ra, repo, uuid, rev, entry_rev, view, author, cdate, rows, true, scope, base, _flush_parts)
		else
			-- WC 已最新: 全量确认条目存在 (open-file 空 delta), 防客户端误删
			_update_emit_tree(parts, ra, repo, uuid, rev, entry_rev, view, author, cdate, rows, true, scope, base, _flush_parts)
		end
	end

	if aborted then
		return
	end

	parts[#parts + 1] = "</S:open-directory>\n</S:update-report>\n"
	if streaming then
		_flush_parts()
		return
	end

	_send(res, 200, table.concat(parts), {
		{ "Content-Type", mime },
		{ "Server", "alua-svn-https" },
	})
end


local function _log_changed_path_xml(parts, cp)
	local action = cp.action or "M"
	local tag = "modified-path"

	if "A" == action then
		tag = "added-path"
	elseif "D" == action then
		tag = "deleted-path"
	elseif "R" == action then
		tag = "replaced-path"
	end

	local cprev = tonumber(cp.copyfrom_rev) or -1
	local cfpath = cp.copyfrom_path
	if ("A" == action or "R" == action)
		and cprev >= 0
		and "string" == type(cfpath)
		and cfpath ~= "" then
		parts[#parts + 1] = string.format(
			'<S:%s copyfrom-path="%s" copyfrom-rev="%d"',
			tag, _xml_escape(cfpath), cprev)
	else
		parts[#parts + 1] = string.format("<S:%s", tag)
	end

	local nk = "file"
	if "string" == type(cp.node_kind) and cp.node_kind ~= "" then
		nk = cp.node_kind
	end

	local tm = "false"
	local pm = "false"
	if true == cp.text_mods or 1 == cp.text_mods then
		tm = "true"
	end
	if true == cp.prop_mods or 1 == cp.prop_mods then
		pm = "true"
	end

	parts[#parts + 1] = string.format(
		' node-kind="%s" text-mods="%s" prop-mods="%s">%s</S:%s>\n',
		_xml_escape(nk), tm, pm, _xml_escape(cp.path or ""), tag)
end


local function _log_report(res, cfg, repo, rest, user, pass, body)
	local text = body or ""
	local start_s = string.match(text, "<[Ss]:start%-revision>(%-?%d+)</[Ss]:start%-revision>")
	local end_s = string.match(text, "<[Ss]:end%-revision>(%-?%d+)</[Ss]:end%-revision>")
	local limit_s = string.match(text, "<[Ss]:limit>(%d+)</[Ss]:limit>")
	local discover = (nil ~= string.find(text, "discover%-changed%-paths", 1))
	local paths = {}

	-- REPORT 目标 (rest) 为 <S:path> 的解析基准; <path> 是相对目标的
	local target_rel = ""
	local rvr_rel = string.match(rest or "", "^/!svn/rvr/%d+/?(.*)$")
	if rvr_rel then
		target_rel = _rel_clean(rvr_rel)
	else
		local cleaned = _rel_clean(rest or "")
		if not string.match(cleaned, "^!svn/") then
			target_rel = cleaned
		end
	end

	for p in string.gmatch(text, "<[Ss]:path>(.-)</[Ss]:path>") do
		paths[#paths + 1] = _rel_join(target_rel, _rel_clean(_xml_unescape(p)))
	end

	if 0 == #paths then
		paths[1] = target_rel
	end

	local ra, err = _ra(cfg, repo, user, pass)
	if not ra then
		_send_ra_fail(res, cfg, repo, err)
		return
	end

	local opts = {
		paths = paths,
		changed_paths = discover and true or false,
	}
	if start_s then
		opts.start = tonumber(start_s)
	end
	if end_s then
		opts["end"] = tonumber(end_s)
	end
	if limit_s then
		opts.limit = tonumber(limit_s) or 0
	end

	local ents, lerr = ra:log(opts)
	if not ents then
		_send_ra_error(res, ra, "svn:ra-error", tostring(lerr or "log failed"))
		return
	end

	local parts = {
		'<?xml version="1.0" encoding="utf-8"?>\n',
		'<S:log-report xmlns:S="svn:" xmlns:D="DAV:">\n',
	}

	for i = 1, #ents do
		local e = ents[i]
		parts[#parts + 1] = "<S:log-item>\n"
		parts[#parts + 1] = string.format(
			"<D:version-name>%d</D:version-name>\n", tonumber(e.rev) or 0)
		if type(e.author) == "string" and e.author ~= "" then
			parts[#parts + 1] = "<D:creator-displayname>"
			parts[#parts + 1] = _xml_escape(e.author)
			parts[#parts + 1] = "</D:creator-displayname>\n"
		end
		if type(e.date) == "string" and e.date ~= "" then
			parts[#parts + 1] = "<S:date>"
			parts[#parts + 1] = _xml_escape(e.date)
			parts[#parts + 1] = "</S:date>\n"
		end
		if type(e.message) == "string" then
			parts[#parts + 1] = "<D:comment>"
			parts[#parts + 1] = _xml_escape(e.message)
			parts[#parts + 1] = "</D:comment>\n"
		end
		if type(e.changed_paths) == "table" then
			for j = 1, #e.changed_paths do
				_log_changed_path_xml(parts, e.changed_paths[j])
			end
		end
		parts[#parts + 1] = "</S:log-item>\n"
	end

	parts[#parts + 1] = "</S:log-report>\n"
	_send(res, 200, table.concat(parts), {
		{ "Content-Type", 'text/xml; charset="utf-8"' },
		{ "Server", "alua-svn-https" },
	})
end


-- @brief REPORT URL 上的相对路径 (!svn/rvr/REV[/rel] 或公开路径)
local function _report_rel_path(rest, body)
	local from_body = string.match(body or "", "<[Ss]:path>(.-)</[Ss]:path>")
	if type(from_body) == "string" and from_body ~= "" then
		return _rel_clean(from_body)
	end

	local rvr_rel = string.match(rest or "", "^/!svn/rvr/%d+/?(.*)$")
	if rvr_rel then
		return _rel_clean(rvr_rel)
	end

	if not string.match(rest or "", "^/!svn/") then
		return _rel_clean(rest)
	end

	return ""
end


local function _get_locations_report(res, cfg, repo, rest, user, pass, body)
	local text = body or ""
	local peg_s = string.match(text, "<[Ss]:peg%-revision>(%d+)</[Ss]:peg%-revision>")
	local peg = tonumber(peg_s)
	local path = _report_rel_path(rest, text)
	local ra, err = _ra(cfg, repo, user, pass)
	if not ra then
		_send_ra_fail(res, cfg, repo, err)
		return
	end

	if not peg then
		peg = ra:youngest()
	end

	local parts = {
		'<?xml version="1.0" encoding="utf-8"?>\n',
		'<S:get-locations-report xmlns:S="svn:" xmlns:D="DAV:">\n',
	}
	local abs_path = "/" .. path

	for rev_s in string.gmatch(text, "<[Ss]:location%-revision>(%d+)</[Ss]:location%-revision>") do
		local rev = tonumber(rev_s)
		if rev then
			local ok = false
			if path == "" then
				ok = true
			elseif ra:cat(path, rev) then
				ok = true
			else
				local ents = ra:list(path, rev)
				ok = (ents ~= nil)
			end
			if ok then
				parts[#parts + 1] = string.format(
					'<S:location rev="%d" path="%s"/>\n', rev, _xml_escape(abs_path))
			end
		end
	end

	parts[#parts + 1] = "</S:get-locations-report>\n"
	_send(res, 200, table.concat(parts), {
		{ "Content-Type", 'text/xml; charset="utf-8"' },
		{ "Server", "alua-svn-https" },
	})
end


local function _get_location_segments_report(res, cfg, repo, rest, user, pass, body)
	local text = body or ""
	local peg_s = string.match(text, "<[Ss]:peg%-revision>(%d+)</[Ss]:peg%-revision>")
	local start_s = string.match(text, "<[Ss]:start%-revision>(%d+)</[Ss]:start%-revision>")
	local end_s = string.match(text, "<[Ss]:end%-revision>(%d+)</[Ss]:end%-revision>")

	-- 报表体 <S:path> 相对 REPORT 目标 (mod_dav_svn 会与目标 fspath 拼接)
	local target = _report_rel_path(rest, "")
	local rel = _rel_clean(string.match(text, "<[Ss]:path>(.-)</[Ss]:path>") or "")
	local full = rel
	if "" ~= target then
		if "" ~= rel then
			full = target .. "/" .. rel
		else
			full = target
		end
	end

	local ra, err = _ra(cfg, repo, user, pass)
	if not ra then
		_send_ra_fail(res, cfg, repo, err)
		return
	end

	-- 缺省对齐 mod_dav_svn: peg / start 取 youngest, end 取 0
	local young = tonumber(ra:youngest()) or 0
	local peg = tonumber(peg_s) or young
	local start_rev = tonumber(start_s) or young
	local end_rev = tonumber(end_s) or 0

	-- M3: 经 ksvnra:location_segments (ra_svn get-location-segments) 取真历史段
	local segs, serr = ra:location_segments(full, peg, start_rev, end_rev)
	if not segs then
		_send_ra_fail(res, cfg, repo, serr)
		return
	end

	local parts = {
		'<?xml version="1.0" encoding="utf-8"?>\n',
		'<S:get-location-segments-report xmlns:S="svn:" xmlns:D="DAV:">\n',
	}
	for _, seg in ipairs(segs) do
		-- 协议要求 segment path 为「无前导 /」的仓内相对路径; 路径不存在时省略属性
		if seg.path then
			parts[#parts + 1] = string.format(
				'<S:location-segment path="%s" range-start="%d" range-end="%d"/>\n',
				_xml_escape(seg.path), seg.range_start, seg.range_end)
		else
			parts[#parts + 1] = string.format(
				'<S:location-segment range-start="%d" range-end="%d"/>\n',
				seg.range_start, seg.range_end)
		end
	end

	parts[#parts + 1] = "</S:get-location-segments-report>\n"
	_send(res, 200, table.concat(parts), {
		{ "Content-Type", 'text/xml; charset="utf-8"' },
		{ "Server", "alua-svn-https" },
	})
end


local function _mergeinfo_report(res, cfg, repo, rest, user, pass, body)
	local text = body or ""
	local rev_s = string.match(text, "<[Ss]:revision>(%d+)</[Ss]:revision>")
	local inherit_s = string.match(text, "<[Ss]:inherit>(%a+)</[Ss]:inherit>")
	local inherited = (inherit_s == "inherited")

	-- 报表体 <S:path> 相对 REPORT 目标 (实测一般为空); 目标路径取自 URL
	local target = _report_rel_path(rest, "")
	local ra, err = _ra(cfg, repo, user, pass)
	if not ra then
		_send_ra_fail(res, cfg, repo, err)
		return
	end

	local rev = tonumber(rev_s)
	if not rev then
		rev = tonumber(ra:youngest()) or 0
	end

	local req = {}
	for p in string.gmatch(text, "<[Ss]:path>(.-)</[Ss]:path>") do
		req[#req + 1] = _rel_clean(p)
	end
	if #req == 0 then
		req[1] = ""
	end

	local parts = {
		'<?xml version="1.0" encoding="utf-8"?>\n',
		'<S:mergeinfo-report xmlns:S="svn:" xmlns:D="DAV:">\n',
	}

	for _, rel in ipairs(req) do
		local full = rel
		if "" ~= target then
			if "" ~= rel then
				full = target .. "/" .. rel
			else
				full = target
			end
		end

		-- M2: 经 ksvnra:mergeinfo (ra_svn get-mergeinfo) 取真实 svn:mergeinfo
		local info, merr = ra:mergeinfo(full, rev, inherited)
		if not info then
			-- 路径/修订在该版本不存在时视为无 mergeinfo (对齐官方 r0 探测)
			if 160013 == merr or 160006 == merr then
				info = ""
			else
				_send_ra_fail(res, cfg, repo, merr)
				return
			end
		end

		if "" ~= info then
			-- M2: mergeinfo-path 为相对 REPORT 目标路径 (对齐 mod_dav_svn
			-- svn_fspath__skip_ancestor(fs_path, path)); 无前导 '/'
			parts[#parts + 1] = string.format(
				'<S:mergeinfo-item><S:mergeinfo-path>%s</S:mergeinfo-path><S:mergeinfo-info>%s</S:mergeinfo-info></S:mergeinfo-item>\n',
				_xml_escape(rel), _xml_escape(info))
		end
	end

	parts[#parts + 1] = "</S:mergeinfo-report>\n"

	_send(res, 200, table.concat(parts), {
		{ "Content-Type", 'text/xml; charset="utf-8"' },
		{ "Server", "alua-svn-https" },
	})
end


-- @brief replay-report: 经 ksvnra replay_begin/replay_next 的编辑器事件流生成 DAV replay XML
-- @note 事件类型号见 svn_ra_edit_evt_e; textdelta 分块累积后一次性 base64;
--   \n 批次之间 flush, 避免在 C 回调内 yield (across C-call boundary)
local function _replay_report(res, cfg, repo, rest, user, pass, body)
	local text = body or ""
	local rev_s = string.match(text, "<[Ss]:revision>(%d+)</[Ss]:revision>")
	local low_s = string.match(text, "<[Ss]:low%-water%-mark>(%d+)</[Ss]:low%-water%-mark>")
	local send_deltas = (nil ~= string.find(text, "<[Ss]:send%-deltas>1</[Ss]:send%-deltas>"))

	if not rev_s then
		_send_dav_error(res, 400, "svn:replay-report", "missing revision")
		return
	end

	local ra, err = _ra(cfg, repo, user, pass)
	if not ra then
		_send_ra_fail(res, cfg, repo, err)
		return
	end

	local parts = {
		'<?xml version="1.0" encoding="utf-8"?>\n',
		'<S:editor-report xmlns:S="svn:">\n',
	}
	local delta_tbl = nil
	local delta_total = 0
	local delta_checksum = nil

	-- M2: 同 update-report 分块直写; 批次之间 flush, 断流后停读 replay
	local mime = 'text/xml; charset="utf-8"'
	local streaming = (type(res.co_chunk) == "function")
	local aborted = false

	local function flush_parts()
		if not streaming then
			return true
		end
		if aborted then
			return false
		end
		if 0 == #parts then
			return true
		end

		local chunk = table.concat(parts)
		for i = #parts, 1, -1 do
			parts[i] = nil
		end

		res:co_chunk(chunk, mime)
		if res._stream_closed or not res._streaming then
			aborted = true
			return false
		end

		return true
	end

	-- 关闭当前文本增量: 累积的 svndiff 字节一次性 base64 输出
	local function close_delta()
		if nil == delta_tbl then
			return
		end

		local raw = table.concat(delta_tbl)
		delta_tbl = nil
		if 0 < #raw then
			if delta_checksum then
				parts[#parts + 1] = string.format('<S:apply-textdelta checksum="%s">', _xml_escape(delta_checksum))
			else
				parts[#parts + 1] = "<S:apply-textdelta>"
			end
			parts[#parts + 1] = ksvnra.b64enc(raw)
			parts[#parts + 1] = "</S:apply-textdelta>\n"
		end
		delta_checksum = nil
	end

	local function prop_xml(kind, evt)
		close_delta()
		if evt.value then
			parts[#parts + 1] = string.format('<S:change-%s-prop name="%s">%s</S:change-%s-prop>\n',
				kind, _xml_escape(evt.name or ""), ksvnra.b64enc(evt.value), kind)
		else
			parts[#parts + 1] = string.format('<S:change-%s-prop name="%s" del="true"/>\n',
				kind, _xml_escape(evt.name or ""))
		end
	end

	-- 单条编辑器事件 → editor-report XML 段
	local function on_evt(evt)
		local t = evt.type
		if 0 == t then
			parts[#parts + 1] = string.format('<S:target-revision rev="%d"/>\n', evt.rev)
		elseif 1 == t then
			close_delta()
			parts[#parts + 1] = string.format('<S:open-root rev="%d"/>\n', evt.rev)
		elseif 2 == t then
			close_delta()
			parts[#parts + 1] = string.format('<S:delete-entry name="%s" rev="%d"/>\n',
				_xml_escape(evt.path or ""), evt.rev)
		elseif 3 == t then
			close_delta()
			if evt.copy_path then
				parts[#parts + 1] = string.format(
					'<S:add-directory name="%s" copyfrom-path="%s" copyfrom-rev="%d"/>\n',
					_xml_escape(evt.path or ""), _xml_escape(evt.copy_path), evt.rev)
			else
				parts[#parts + 1] = string.format('<S:add-directory name="%s"/>\n',
					_xml_escape(evt.path or ""))
			end
		elseif 4 == t then
			close_delta()
			parts[#parts + 1] = string.format('<S:open-directory name="%s" rev="%d"/>\n',
				_xml_escape(evt.path or ""), evt.rev)
		elseif 5 == t then
			prop_xml("dir", evt)
		elseif 6 == t then
			parts[#parts + 1] = "<S:close-directory/>\n"
		elseif 7 == t then
			-- DAV replay 无 absent 元素 (mod_dav_svn 亦不下发), 跳过
		elseif 8 == t then
			close_delta()
			if evt.copy_path then
				parts[#parts + 1] = string.format(
					'<S:add-file name="%s" copyfrom-path="%s" copyfrom-rev="%d"/>\n',
					_xml_escape(evt.path or ""), _xml_escape(evt.copy_path), evt.rev)
			else
				parts[#parts + 1] = string.format('<S:add-file name="%s"/>\n',
					_xml_escape(evt.path or ""))
			end
		elseif 9 == t then
			close_delta()
			parts[#parts + 1] = string.format('<S:open-file name="%s" rev="%d"/>\n',
				_xml_escape(evt.path or ""), evt.rev)
		elseif 10 == t then
			close_delta()
			delta_tbl = {}
			delta_total = 0
			delta_checksum = evt.checksum
		elseif 11 == t then
			if delta_tbl then
				delta_total = delta_total + #(evt.data or "")
				if REPLAY_DELTA_MAX_BYTES > 0 and delta_total > REPLAY_DELTA_MAX_BYTES then
					error("replay: textdelta exceeds limit " .. tostring(REPLAY_DELTA_MAX_BYTES))
				end
				delta_tbl[#delta_tbl + 1] = evt.data or ""
			end
		elseif 12 == t then
			close_delta()
		elseif 13 == t then
			prop_xml("file", evt)
		elseif 14 == t then
			close_delta()
			if evt.checksum then
				parts[#parts + 1] = string.format('<S:close-file checksum="%s"/>\n', _xml_escape(evt.checksum))
			else
				parts[#parts + 1] = "<S:close-file/>\n"
			end
		elseif 15 == t then
			-- absent-file: 同 absent-dir, 跳过
		elseif 16 == t then
			error("replay: unexpected close-edit")
		end
	end

	-- ksvnra 批量接口: replay_next 返回 nil 时为 nil, svn 错误码, 错误消息
	local ok, rcode, rmsg = ra:replay_begin(tonumber(rev_s), tonumber(low_s) or 0, send_deltas)
	if not ok then
		_send_ra_error(res, ra, "svn:ra-error", tostring(rmsg or "replay failed"), rcode, nil)
		return
	end

	local done = false
	while not done do
		local evts, de, ncode, nmsg = ra:replay_next(256)
		if nil == evts then
			_send_ra_error(res, ra, "svn:ra-error", tostring(nmsg or "replay failed"), ncode, nil)
			return
		end

		for i = 1, #evts do
			on_evt(evts[i])
		end

		if not flush_parts() then
			-- 客户端断开: 标记会话失效并关闭 (replay 中途不可复用), 下次请求重连
			RA_STALE[ra] = true
			pcall(function ()
				ra:close()
			end)
			return
		end

		done = de
	end

	close_delta()
	if aborted then
		return
	end

	parts[#parts + 1] = "</S:editor-report>\n"
	if streaming then
		flush_parts()
		return
	end

	_send(res, 200, table.concat(parts), {
		{ "Content-Type", mime },
		{ "Server", "alua-svn-https" },
	})
end


local function _file_revs_report(res, cfg, repo, rest, user, pass, body)
	local text = body or ""
	local start_s = string.match(text, "<[Ss]:start%-revision>(%-?%d+)</[Ss]:start%-revision>")
	local end_s = string.match(text, "<[Ss]:end%-revision>(%-?%d+)</[Ss]:end%-revision>")
	local path = _report_rel_path(rest, text)
	if path == "" then
		_send(res, 400, "file-revs needs path")
		return
	end

	local ra, err = _ra(cfg, repo, user, pass)
	if not ra then
		_send_ra_fail(res, cfg, repo, err)
		return
	end

	local young = ra:youngest()
	local start_rev = tonumber(start_s) or 0
	local end_rev = tonumber(end_s) or young
	if start_rev < 0 then
		start_rev = 0
	end
	if end_rev < 0 then
		end_rev = young
	end

	local ents, lerr = ra:log({
		paths = { path },
		start = end_rev,
		["end"] = start_rev,
		changed_paths = false,
	})
	if not ents then
		_send_ra_error(res, ra, "svn:ra-error", tostring(lerr or "log failed"))
		return
	end

	-- log 通常新→旧; file-revs 要旧→新
	local revs = {}
	for i = #ents, 1, -1 do
		local r = tonumber(ents[i].rev)
		if r and r >= start_rev and r <= end_rev then
			revs[#revs + 1] = ents[i]
		end
	end

	local parts = {
		'<?xml version="1.0" encoding="utf-8"?>\n',
		'<S:file-revs-report xmlns:S="svn:" xmlns:V="http://subversion.tigris.org/xmlns/dav/" xmlns:D="DAV:">\n',
	}
	local prev_rev = -1
	local abs_path = "/" .. path

	-- M2: 分块直写响应; 客户端断开则停止余下版本构建
	local mime = 'text/xml; charset="utf-8"'
	local streaming = (type(res.co_chunk) == "function")

	local function flush_parts()
		if not streaming then
			return true
		end
		if 0 == #parts then
			return true
		end

		local body = table.concat(parts)
		for i = #parts, 1, -1 do
			parts[i] = nil
		end

		res:co_chunk(body, mime)
		if res._stream_closed or not res._streaming then
			return false
		end

		return true
	end

	for i = 1, #revs do
		local e = revs[i]
		local rev = tonumber(e.rev) or 0
		local b64, terr = ra:txdelta_file(path, rev, prev_rev, CAT_MAX_BYTES)
		if nil == b64 then
			if "too_large" == terr then
				_send(res, 413, "file too large", { { "Content-Type", "text/plain" } })
				return
			end
			-- 该版本无该路径: 跳过 (对齐 _safe_cat 失败)
		else
			if "" == b64 then
				-- 正文未变: 退回全新窗 (对齐原 _svndiff_new_file 兜底)
				b64 = ra:txdelta_file(path, rev, -1, CAT_MAX_BYTES)
			end
			if nil == b64 then
				return
			end

			prev_rev = rev
			parts[#parts + 1] = string.format(
				'<S:file-rev path="%s" rev="%d">\n', _xml_escape(abs_path), rev)
			if type(e.author) == "string" and e.author ~= "" then
				parts[#parts + 1] = '<S:rev-prop name="svn:author">'
				parts[#parts + 1] = _xml_escape(e.author)
				parts[#parts + 1] = "</S:rev-prop>\n"
			end
			if type(e.date) == "string" and e.date ~= "" then
				parts[#parts + 1] = '<S:rev-prop name="svn:date">'
				parts[#parts + 1] = _xml_escape(e.date)
				parts[#parts + 1] = "</S:rev-prop>\n"
			end
			if type(e.message) == "string" then
				parts[#parts + 1] = '<S:rev-prop name="svn:log">'
				parts[#parts + 1] = _xml_escape(e.message)
				parts[#parts + 1] = "</S:rev-prop>\n"
			end
			parts[#parts + 1] = "<S:txdelta>"
			parts[#parts + 1] = b64
			parts[#parts + 1] = "</S:txdelta>\n"
			parts[#parts + 1] = "</S:file-rev>\n"
		end

		if not flush_parts() then
			return
		end
	end

	parts[#parts + 1] = "</S:file-revs-report>\n"
	if streaming then
		flush_parts()
		return
	end

	_send(res, 200, table.concat(parts), {
		{ "Content-Type", mime },
		{ "Server", "alua-svn-https" },
	})
end


local function _get_locks_report(res, cfg, repo, rest, user, pass, body)
	local path = _report_rel_path(rest, body or "")
	local store = _lock_store(cfg)
	local parts = {
		'<?xml version="1.0" encoding="utf-8"?>\n',
		'<S:get-locks-report xmlns:S="svn:" xmlns:D="DAV:">\n',
	}
	local prefix = _lock_key(repo, "")
	local path_prefix = path == "" and "" or (path .. "/")

	for key, info in pairs(store) do
		if type(key) == "string" and string.sub(key, 1, #prefix) == prefix and type(info) == "table" then
			local rel = string.sub(key, #prefix + 1)
			local hit = false
			if path == "" then
				hit = true
			elseif rel == path or string.sub(rel, 1, #path_prefix) == path_prefix then
				hit = true
			end
			if hit and type(info.token) == "string" then
				parts[#parts + 1] = "<S:lock>\n"
				parts[#parts + 1] = "<S:path>"
				parts[#parts + 1] = _xml_escape("/" .. rel)
				parts[#parts + 1] = "</S:path>\n"
				parts[#parts + 1] = "<D:lockdiscovery><D:activelock>\n"
				parts[#parts + 1] = "<D:locktype><D:write/></D:locktype>\n"
				parts[#parts + 1] = "<D:lockscope><D:exclusive/></D:lockscope>\n"
				parts[#parts + 1] = "<D:depth>0</D:depth>\n"
				parts[#parts + 1] = "<D:creator-displayname>"
				parts[#parts + 1] = _xml_escape(info.owner or "")
				parts[#parts + 1] = "</D:creator-displayname>\n"
				parts[#parts + 1] = "<D:owner><D:href>"
				parts[#parts + 1] = _xml_escape(info.owner or "")
				parts[#parts + 1] = "</D:href></D:owner>\n"
				parts[#parts + 1] = "<D:locktoken><D:href>"
				parts[#parts + 1] = _xml_escape(info.token)
				parts[#parts + 1] = "</D:href></D:locktoken>\n"
				parts[#parts + 1] = "</D:activelock></D:lockdiscovery>\n"
				parts[#parts + 1] = "</S:lock>\n"
			end
		end
	end

	parts[#parts + 1] = "</S:get-locks-report>\n"
	_send(res, 200, table.concat(parts), {
		{ "Content-Type", 'text/xml; charset="utf-8"' },
		{ "Server", "alua-svn-https" },
	})
end


-- @brief REPORT 请求体元素文本 (匹配任意命名空间前缀; 无则 nil)
local function _report_elem(text, name)
	-- 名称中非字母数字须转义 (如 end-revision 的 '-')
	local pat_name = string.gsub(name, "[^%w_]", function (c)
		return "%" .. c
	end)
	local v = string.match(text or "", "<%a+:" .. pat_name .. ">(.-)</%a+:" .. pat_name .. ">")
	if nil == v then
		return nil
	end

	return _xml_unescape(v)
end


-- @brief ra 是否提供某方法 (旧二进制/未编入时为 false)
local function _ra_has(ra, name)
	local ok, has = pcall(function ()
		return type(ra[name]) == "function"
	end)

	return ok and has or false
end


-- @brief dated-rev-report → <S:dated-rev-report><D:version-name> (P1-3)
local function _dated_rev_report(res, cfg, repo, rest, user, pass, body)
	local date = _report_elem(body, "date") or _report_elem(body, "creationdate")
	if nil == date or "" == date then
		_send_dav_error(res, 400, "svn:bad-report", "dated-rev-report: missing date")
		return
	end

	local ra, err = _ra(cfg, repo, user, pass)
	if not ra then
		_send_ra_fail(res, cfg, repo, err)
		return
	end

	if not _ra_has(ra, "dated_rev") then
		_send_dav_error(res, 501, "svn:unsupported-feature", "dated-rev not supported by runtime")
		return
	end

	local rev, _, msg = ra:dated_rev(date)
	if not rev then
		_send_ra_error(res, ra, "svn:ra-error", tostring(msg or "dated-rev failed"))
		return
	end

	_send(res, 200, table.concat({
		'<?xml version="1.0" encoding="utf-8"?>\n',
		'<S:dated-rev-report xmlns:S="svn:" xmlns:D="DAV:">\n',
		"<D:version-name>", tostring(rev), "</D:version-name>\n",
		"</S:dated-rev-report>\n",
	}), {
		{ "Content-Type", 'text/xml; charset="utf-8"' },
		{ "Server", "alua-svn-https" },
	})
end


-- @brief get-deleted-rev-report → <S:get-deleted-rev-report><D:deleted-rev> (P1-3)
local function _deleted_rev_report(res, cfg, repo, rest, user, pass, body)
	local path = _report_elem(body, "path")
	if nil == path or "" == path then
		_send_dav_error(res, 400, "svn:bad-report", "get-deleted-rev-report: missing path")
		return
	end

	local rev_s = _report_elem(body, "revision")
		or _report_elem(body, "peg-revision")
		or _report_elem(body, "version-name")
	local peg = tonumber(rev_s)
	if nil == peg or 0 >= peg then
		_send_dav_error(res, 400, "svn:bad-report", "get-deleted-rev-report: bad revision")
		return
	end

	local ra, err = _ra(cfg, repo, user, pass)
	if not ra then
		_send_ra_fail(res, cfg, repo, err)
		return
	end

	if not _ra_has(ra, "deleted_rev") then
		_send_dav_error(res, 501, "svn:unsupported-feature", "get-deleted-rev not supported by runtime")
		return
	end

	local end_rev = tonumber(_report_elem(body, "end-revision")) or -1
	local del, _, msg = ra:deleted_rev(_rel_clean(path), peg, end_rev)
	if not del then
		_send_ra_error(res, ra, "svn:ra-error", tostring(msg or "get-deleted-rev failed"))
		return
	end

	_send(res, 200, table.concat({
		'<?xml version="1.0" encoding="utf-8"?>\n',
		'<S:get-deleted-rev-report xmlns:S="svn:" xmlns:D="DAV:">\n',
		"<D:deleted-rev>", tostring(del), "</D:deleted-rev>\n",
		"</S:get-deleted-rev-report>\n",
	}), {
		{ "Content-Type", 'text/xml; charset="utf-8"' },
		{ "Server", "alua-svn-https" },
	})
end


local function _do_report(req, res, cfg, repo, rest, user, pass, body)
	local text = body or ""

	if string.find(text, "inherited%-props%-report", 1) then
		_send(res, 200,
			'<?xml version="1.0" encoding="utf-8"?>\n'
				.. '<S:inherited-props-report xmlns:S="svn:"></S:inherited-props-report>\n',
			{ { "Content-Type", 'text/xml; charset="utf-8"' }, { "Server", "alua-svn-https" } })
		return
	end

	if string.find(text, "get%-locks%-report", 1) then
		_get_locks_report(res, cfg, repo, rest, user, pass, text)
		return
	end

	if string.find(text, "list%-report", 1) then
		_send(res, 501, "list-report not supported")
		return
	end

	-- M3: replay-report 经 ra_svn replay 编辑器事件流真实现
	if string.find(text, "replay%-report", 1) then
		_replay_report(res, cfg, repo, rest, user, pass, text)
		return
	end

	-- P1-3: svn log -r {DATE}
	if string.find(text, "dated%-rev%-report", 1) then
		_dated_rev_report(res, cfg, repo, rest, user, pass, text)
		return
	end

	-- P1-3: copy 源删除版本查询
	if string.find(text, "get%-deleted%-rev%-report", 1) then
		_deleted_rev_report(res, cfg, repo, rest, user, pass, text)
		return
	end

	-- Bug.4: svn log 对 !svn/rvr 发 log-report; 须先于路径限制处理
	if string.find(text, "log%-report", 1) then
		_log_report(res, cfg, repo, rest, user, pass, text)
		return
	end

	-- Bug.19: blame / cat -r
	if string.find(text, "file%-revs%-report", 1) then
		_file_revs_report(res, cfg, repo, rest, user, pass, text)
		return
	end

	if string.find(text, "get%-locations", 1) and not string.find(text, "get%-location%-segments", 1) then
		_get_locations_report(res, cfg, repo, rest, user, pass, text)
		return
	end

	if string.find(text, "get%-location%-segments", 1) then
		_get_location_segments_report(res, cfg, repo, rest, user, pass, text)
		return
	end

	-- Bug.17: mergeinfo-report
	if string.find(text, "mergeinfo%-report", 1) then
		_mergeinfo_report(res, cfg, repo, rest, user, pass, text)
		return
	end

	if string.gsub(rest, "/$", "") ~= "/!svn/me" then
		_send(res, 501, "REPORT path not supported")
		return
	end

	_update_report(res, cfg, repo, user, pass, text)
end


-- @brief 解析 Range 头 (bytes=a-b,a-b,...); nil=语法无效, 空表=全部不可满足
local function _parse_ranges(range, total)
	local spec = string.match(tostring(range or ""), "^bytes=(.+)$")
	if not spec then
		return nil
	end

	local out = {}
	for item in string.gmatch(spec, "[^,]+") do
		item = string.gsub(item, "^%s*(.-)%s*$", "%1")
		local from_s, to_s = string.match(item, "^(%d*)%-(%d*)$")
		if not from_s or (from_s == "" and to_s == "") then
			return nil
		end

		local from, to
		if from_s == "" then
			local n = tonumber(to_s) or 0
			from = math.max(0, total - n)
			to = total - 1
		else
			from = tonumber(from_s) or 0
			to = (to_s ~= "" and tonumber(to_s)) or (total - 1)
			to = math.min(to, total - 1)
		end

		if from < total and to >= from then
			out[#out + 1] = { from = from, to = to }
		end
	end

	return out
end


local GET_CHUNK_BYTES = 64 * 1024


-- @brief 真流式 GET: cat_open -> cat_next 逐块聚合输出; 失败时丢弃剩余并收尾
-- @param [in] skip[number]   [可选] 跳过首部字节数, 默认 0 (Range 起点)
-- @param [in] limit[number]  [可选] 最多输出字节数, 默认 nil=读到 EOF
-- @param [in] code[number]   [可选] 状态码, 默认 200 (单段 Range 用 206)
-- @return ok[boolean], err[string] (失败原因; 成功为 "")
local function _stream_cat(ra, res, rel, rev, hdrs, skip, limit, code)
	skip = skip or 0

	-- 先发起 get-file, 未成功前不写响应头, 便于回 404
	if 0 ~= ra:cat_open(rel, rev) then
		return false, "cat_open"
	end

	for i = 1, #hdrs do
		res:header(hdrs[i][1], hdrs[i][2])
	end

	local parts = {}
	local pending = 0
	local sent = 0
	local ok, err = pcall(function ()
		while true do
			local rc, chunk, msg = ra:cat_next()
			if 0 ~= rc then
				error(msg or "cat_next failed")
			end
			if "" == chunk then
				break
			end

			-- Range 起点: 丢弃 chunk 首部
			if skip > 0 then
				local n = math.min(skip, #chunk)
				chunk = string.sub(chunk, n + 1)
				skip = skip - n
			end

			if #chunk > 0 and (nil == limit or sent < limit) then
				if limit and #chunk > (limit - sent) then
					chunk = string.sub(chunk, 1, limit - sent)
				end
				parts[#parts + 1] = chunk
				pending = pending + #chunk
				sent = sent + #chunk
				if pending >= GET_CHUNK_BYTES then
					res:co_chunk(table.concat(parts), "application/octet-stream", code)
					parts = {}
					pending = 0
				end
			end

			if limit and sent >= limit then
				break
			end
		end

		if pending > 0 then
			res:co_chunk(table.concat(parts), "application/octet-stream", code)
		end
	end)

	ra:cat_close()

	if type(res.co_chunk_end) == "function" then
		res:co_chunk_end()
	end

	if not ok then
		return false, err or "stream"
	end
	return true, ""
end


-- @brief 取指定修订下文件大小 (list 父目录); nil=未知
local function _file_size(ra, rel, rev)
	local parent, name = string.match(rel, "^(.*)/([^/]+)$")
	if not parent then
		parent = ""
		name = rel
	end

	local ents = ra:list(parent, rev)
	if type(ents) ~= "table" then
		return nil
	end

	for i = 1, #ents do
		if ents[i].name == name and "file" == ents[i].kind then
			return tonumber(ents[i].size) or 0
		end
	end

	return nil
end


-- @brief 多段 Range 真流式: 逐段 cat_open/cat_next, 按 multipart/byteranges 输出
-- @param [in] hdrs[table]   额外响应头 (不含 Content-Type)
-- @param [in] ranges[table] `{ { from, to }, ... }` (已按 total 归一)
-- @param [in] total[number] 文件总长
-- @param [in] boundary[string] multipart 分隔串
-- @param [in] mime[string]  Content-Type (首次 co_chunk 生效)
-- @return ok[boolean], err[string] (`cat_open` 表示未发头, 可回 404)
local function _stream_multipart(ra, res, rel, rev, hdrs, ranges, total, boundary, mime)
	-- 先发起首段, 未成功前不写响应头, 便于回 404
	if 0 ~= ra:cat_open(rel, rev) then
		return false, "cat_open"
	end

	for i = 1, #hdrs do
		res:header(hdrs[i][1], hdrs[i][2])
	end

	local parts = {}
	local pending = 0
	local first = true

	-- 首次写出时带 206 与 multipart Content-Type; 之后 mime/code 由 start_mode 忽略
	local function emit(s)
		parts[#parts + 1] = s
		pending = pending + #s
		if pending >= GET_CHUNK_BYTES then
			res:co_chunk(table.concat(parts), mime, first and 206 or nil)
			first = false
			parts = {}
			pending = 0
		end
	end

	local ok, err = pcall(function ()
		for i = 1, #ranges do
			if i > 1 and 0 ~= ra:cat_open(rel, rev) then
				error("cat_open")
			end

			local r = ranges[i]
			emit("--" .. boundary .. "\r\n")
			emit("Content-Type: application/octet-stream\r\n")
			emit(string.format("Content-Range: bytes %d-%d/%d\r\n\r\n", r.from, r.to, total))

			local skip = r.from
			local remain = r.to - r.from + 1
			while true do
				local rc, chunk, msg = ra:cat_next()
				if 0 ~= rc then
					error(msg or "cat_next failed")
				end
				if "" == chunk then
					break
				end

				if skip > 0 then
					local n = math.min(skip, #chunk)
					chunk = string.sub(chunk, n + 1)
					skip = skip - n
				end

				if #chunk > 0 and 0 < remain then
					if #chunk > remain then
						chunk = string.sub(chunk, 1, remain)
					end
					emit(chunk)
					remain = remain - #chunk
				end

				if 0 >= remain then
					break
				end
			end

			ra:cat_close()
			emit("\r\n")
		end

		emit("--" .. boundary .. "--\r\n")
	end)

	-- 异常路径也确保收尾
	ra:cat_close()

	if pending > 0 then
		res:co_chunk(table.concat(parts), mime, first and 206 or nil)
	end

	if type(res.co_chunk_end) == "function" then
		res:co_chunk_end()
	end

	if not ok then
		return false, err or "stream"
	end

	return true, ""
end


local function _do_get_head(req, res, cfg, repo, rest, user, pass, head_only)
	local rev_s, rel = string.match(rest, "^/!svn/rvr/(%d+)/(.*)$")
	if not rev_s then
		_send(res, 404, "not found")
		return
	end

	local ra, err = _ra(cfg, repo, user, pass)
	if not ra then
		_send_ra_fail(res, cfg, repo, err)
		return
	end

	-- M4: 单段/多段 Range 均真流式 (先 list 父目录取长度); 长度未知仍整包
	local range_hdr = _header_get(req.headers, "range")
	if (not head_only) and range_hdr
		and type(res.co_chunk) == "function"
		and type(ra.cat_open) == "function" then
		local total = _file_size(ra, rel, tonumber(rev_s))
		if total then
			local ranges = _parse_ranges(range_hdr, total)
			if ranges and 0 == #ranges then
				_send(res, 416, "range not satisfiable", {
					{ "Content-Range", "bytes */" .. tostring(total) },
					{ "Content-Type", "text/plain" },
				})
				return
			end
			if ranges and 1 == #ranges then
				local r = ranges[1]
				-- Content-Type/Server 由 pack_head 默认输出, 此处勿重复; 状态码经 co_chunk 传入
				local hdrs = {
					{ "Content-Range", string.format("bytes %d-%d/%d", r.from, r.to, total) },
					{ "Accept-Ranges", "bytes" },
				}

				local ok, serr = _stream_cat(ra, res, rel, tonumber(rev_s), hdrs,
					r.from, r.to - r.from + 1, 206)
				if ok then
					return
				end
				if "cat_open" == serr then
					_send_ra_error(res, ra, "svn:path-not-found", "cat failed", nil, nil, 404)
				end
				return
			end
			if ranges and 2 <= #ranges then
				local boundary = string.format("alua-%d-%d", os.time(), math.random(1000000))
				local mime = 'multipart/byteranges; boundary="' .. boundary .. '"'
				local ok, serr = _stream_multipart(ra, res, rel, tonumber(rev_s),
					{ { "Accept-Ranges", "bytes" } }, ranges, total, boundary, mime)
				if ok then
					return
				end
				if "cat_open" == serr then
					_send_ra_error(res, ra, "svn:path-not-found", "cat failed", nil, nil, 404)
				end
				return
			end
		end
	end

	-- M3: 无 Range 的 GET 走真流式; HEAD / Range / 无 co_chunk 仍走整包 (需长度与随机段)
	if (not head_only) and not range_hdr
		and type(res.co_chunk) == "function"
		and type(ra.cat_open) == "function" then
		-- Content-Type/Server 由 pack_head 默认输出, 此处勿重复
		local hdrs = {
			{ "Accept-Ranges", "bytes" },
		}

		local ok, serr = _stream_cat(ra, res, rel, tonumber(rev_s), hdrs)
		if ok then
			return
		end

		-- 未发头时 open 失败 → 带上游错误码; 已发头后失败只能中止
		if "cat_open" == serr then
			_send_ra_error(res, ra, "svn:path-not-found", "cat failed", nil, nil, 404)
		end
		return
	end

	local data, cerr = _safe_cat(ra, rel, tonumber(rev_s))
	if cerr == "too_large" then
		_send(res, 413, "file too large", { { "Content-Type", "text/plain" } })
		return
	end

	if not data then
		_send_ra_error(res, ra, "svn:path-not-found", "cat failed", nil, nil, 404)
		return
	end

	local hdrs = {
		{ "Content-Type", "application/octet-stream" },
		{ "Accept-Ranges", "bytes" },
		{ "Server", "alua-svn-https" },
	}
	if head_only then
		hdrs[#hdrs + 1] = { "Content-Length", tostring(#data) }
		_send_no_body(res, 200, hdrs)
		return
	end

	-- P1-4: Range 请求 (单段/多段); 数据仍整包 cat 入内存, 响应按段或分块发送
	local ranges = _parse_ranges(_header_get(req.headers, "range"), #data)
	if ranges and 0 == #ranges then
		_send(res, 416, "range not satisfiable", {
			{ "Content-Range", "bytes */" .. tostring(#data) },
			{ "Content-Type", "text/plain" },
		})
		return
	end

	if ranges and 1 == #ranges then
		local r = ranges[1]
		_send(res, 206, string.sub(data, r.from + 1, r.to + 1), {
			{ "Content-Type", "application/octet-stream" },
			{ "Content-Range", string.format("bytes %d-%d/%d", r.from, r.to, #data) },
			{ "Accept-Ranges", "bytes" },
			{ "Server", "alua-svn-https" },
		})
		return
	end

	if ranges then
		local boundary = string.format("alua-%d-%d", os.time(), math.random(1000000))
		local parts = {}
		for i = 1, #ranges do
			local r = ranges[i]
			parts[#parts + 1] = "--" .. boundary .. "\r\n"
			parts[#parts + 1] = "Content-Type: application/octet-stream\r\n"
			parts[#parts + 1] = string.format("Content-Range: bytes %d-%d/%d\r\n\r\n",
				r.from, r.to, #data)
			parts[#parts + 1] = string.sub(data, r.from + 1, r.to + 1)
			parts[#parts + 1] = "\r\n"
		end
		parts[#parts + 1] = "--" .. boundary .. "--\r\n"

		_send(res, 206, table.concat(parts), {
			{ "Content-Type", 'multipart/byteranges; boundary="' .. boundary .. '"' },
			{ "Accept-Ranges", "bytes" },
			{ "Server", "alua-svn-https" },
		})
		return
	end

	-- P1-4: 有 co_chunk 时分块发送 (64KiB/块), 避免整包一次性写入
	local CHUNK_BYTES = 64 * 1024
	if 0 == #data then
		hdrs[#hdrs + 1] = { "Content-Length", "0" }
		_send(res, 200, "", hdrs)
	elseif type(res.co_chunk) == "function" then
		for i = 1, #hdrs do
			res:header(hdrs[i][1], hdrs[i][2])
		end

		local pos = 1
		while pos <= #data do
			local n = math.min(CHUNK_BYTES, #data - pos + 1)
			res:co_chunk(string.sub(data, pos, pos + n - 1), "application/octet-stream")
			pos = pos + n
		end

		if type(res.co_chunk_end) == "function" then
			res:co_chunk_end()
		end
	else
		hdrs[#hdrs + 1] = { "Content-Length", tostring(#data) }
		_send(res, 200, data, hdrs)
	end
end


local function _do_get(req, res, cfg, repo, rest, user, pass)
	_do_get_head(req, res, cfg, repo, rest, user, pass, false)
end


local function _do_head(req, res, cfg, repo, rest, user, pass)
	_do_get_head(req, res, cfg, repo, rest, user, pass, true)
end


-- //////////////////////////////////////////////////////////////////////////
-- HTTPv2 write path (txn buffer -> ksvnra.commit)


local function _txn_store(cfg)
	cfg._txns = cfg._txns or {}
	return cfg._txns
end


local function _txn_touch(t)
	local now = os.time()
	t.created_at = t.created_at or now
	t.last_touch = now
end


local function _txn_purge(cfg)
	local store = _txn_store(cfg)
	local now = os.time()

	for name, t in pairs(store) do
		local ts = t.last_touch or t.created_at or now
		if TXN_TTL_SEC > 0 and (now - ts) > TXN_TTL_SEC then
			_txn_spool_free(t)
			store[name] = nil
		end
	end
end


local function _txn_get(cfg, txn, repo)
	local store = _txn_store(cfg)
	local t = store[txn]
	if not t or t.repo ~= repo then
		return nil
	end

	_txn_touch(t)
	return t
end


-- @brief 单个 op 的 body 字节数 (spool 落盘后以 spool_size 计)
local function _txn_op_bytes(op)
	if type(op) ~= "table" or op.op ~= "put" then
		return 0
	end

	if type(op.spool_size) == "number" then
		return op.spool_size
	end

	if type(op.svndiff) == "string" then
		return #op.svndiff
	end

	if type(op.data) == "string" then
		return #op.data
	end

	return 0
end


local function _txn_bytes_total(t)
	local n = 0

	for i = 1, #t.ops do
		n = n + _txn_op_bytes(t.ops[i])
	end

	return n
end


local function _txn_has_children(t, dir_path)
	dir_path = _rel_clean(dir_path)
	if dir_path == "" then
		return false
	end

	local prefix = dir_path .. "/"
	for i = 1, #t.ops do
		local p = _rel_clean(t.ops[i].path or "")
		if string.sub(p, 1, #prefix) == prefix then
			return true
		end
	end

	return false
end


local function _txn_replicate_prefix(t, src, dst)
	src = _rel_clean(src)
	dst = _rel_clean(dst)
	t.ops[#t.ops + 1] = { op = "mkdir", path = dst }

	local prefix = src .. "/"
	local dst_prefix = dst .. "/"
	local done = {}

	for i = 1, #t.ops do
		local o = t.ops[i]
		local p = _rel_clean(o.path or "")
		if p ~= src and string.sub(p, 1, #prefix) == prefix then
			local rel = string.sub(p, #prefix + 1)
			local np = dst_prefix .. rel
			if not done[np] then
				done[np] = true
				local no = {}
				for k, v in pairs(o) do
					no[k] = v
				end
				no.path = np
				if no.op == "put" then
					no.add = true
				end
				t.ops[#t.ops + 1] = no
			end
		end
	end
end


-- @brief txn 内路径可见状态 (叠 ops 顺序)
local function _txn_path_state(t, path)
	local deleted = false
	local put_op = nil
	local mkdir_op = false
	local copy_op = nil

	path = _rel_clean(path)
	for i = 1, #t.ops do
		local o = t.ops[i]
		if _rel_clean(o.path or "") == path then
			if o.op == "delete" then
				deleted = true
				put_op = nil
				mkdir_op = false
				copy_op = nil
			elseif o.op == "put" then
				deleted = false
				put_op = o
				mkdir_op = false
				copy_op = nil
			elseif o.op == "mkdir" then
				deleted = false
				mkdir_op = true
				put_op = nil
				copy_op = nil
			elseif o.op == "copy" then
				deleted = false
				copy_op = o
				mkdir_op = (o.kind == "dir")
				put_op = nil
			end
		end
	end

	if deleted then
		return nil
	end
	if put_op then
		return "put", put_op
	end
	if copy_op and copy_op.kind ~= "dir" then
		return "copy", copy_op
	end
	if mkdir_op then
		return "mkdir"
	end
	if copy_op then
		return "copydir", copy_op
	end

	return "repos"
end


-- @brief PROPFIND !svn/txr: checked-in 指向 txn 自身 (勿嵌套 !svn/rvr; Bug.11)
local function _do_propfind_txr(req, res, cfg, repo, user, pass, ra, young, uuid, depth, txn_name, rel)
	rel = _rel_clean(rel)
	local href = string.format("/svn/%s/!svn/txr/%s", repo, txn_name)
	if rel ~= "" then
		href = href .. "/" .. rel
	end

	local t = _txn_get(cfg, txn_name, repo)
	if not t then
		_send(res, 404, "txn not found")
		return
	end

	local kind = "dir"
	local last_rev = young
	local author = user or ""
	local date = ""
	local size = "0"
	local vis, pop = nil, nil

	if rel ~= "" then
		vis, pop = _txn_path_state(t, rel)
		if vis == nil then
			_send(res, 404, "path deleted in txn")
			return
		elseif vis == "put" then
			kind = "file"
			if not pop.add then
				last_rev = tonumber(pop.rev) or young
			else
				last_rev = nil
			end
			if type(pop.data) == "string" then
				size = tostring(#pop.data)
			elseif type(pop.svndiff) == "string" then
				size = tostring(#pop.svndiff)
			end
		elseif vis == "copy" then
			kind = "file"
			last_rev = nil
			local cf = pop.copyfrom_path
			local crev = tonumber(pop.copyfrom_rev) or young
			if type(cf) == "string" and cf ~= "" then
				local data = ra:cat(cf, crev)
				if data then
					size = tostring(#data)
				end
			end
		elseif vis == "copydir" or vis == "mkdir" then
			kind = "dir"
			last_rev = nil
		else
			local data = ra:cat(rel, young)
			if data then
				kind = "file"
				size = tostring(#data)
			else
				local ents = ra:list(rel, young)
				if ents then
					kind = "dir"
				else
					_send_propfind_missing(res, href)
					return
				end
			end
		end
	end

	local checked = href
	if kind == "dir" and not string.match(checked, "/$") then
		checked = checked .. "/"
	end

	local lock_info = _lock_get(cfg, repo, rel)
	local parts = {
		'<?xml version="1.0" encoding="utf-8"?>\n',
		'<D:multistatus xmlns:D="DAV:">\n',
		_propfind_response(repo, uuid, href, rel, kind, last_rev, author, date, size, checked, lock_info),
	}
	parts[#parts + 1] = "</D:multistatus>\n"
	_send(res, 207, table.concat(parts), {
		{ "Content-Type", 'text/xml; charset="utf-8"' },
		{ "Server", "alua-svn-https" },
	})
end


local function _txn_new_name(cfg)
	cfg._txn_seq = (cfg._txn_seq or 0) + 1
	return string.format("%d-%d", math.floor(os.time()) % 100000, cfg._txn_seq)
end


local function _match_special(rest)
	if rest == "/!svn/me" or rest == "/!svn/me/" then
		return "me"
	end

	local txn = string.match(rest, "^/!svn/txn/([^/]+)/?$")
	if txn then
		return "txn", txn
	end

	local tn, rel = string.match(rest, "^/!svn/txr/([^/]+)/?(.*)$")
	if tn then
		return "txr", tn, rel or ""
	end

	return nil
end


local function _extract_svn_log(body)
	body = body or ""
	-- skel atom: svn:log <len> <payload>  (ra_serf create-txn-with-props)
	local nn, rest = string.match(body, "svn:log%s+(%d+)%s+(.*)")
	if nn and rest then
		local n = tonumber(nn) or 0
		if n >= 0 and #rest >= n then
			return string.sub(rest, 1, n)
		end
	end

	-- skel string form: svn:log ... N:payload
	nn, rest = string.match(body, "svn:log[^%w]+(%d+):(.*)")
	if nn and rest then
		local n = tonumber(nn) or 0
		if n >= 0 and #rest >= n then
			return string.sub(rest, 1, n)
		end
	end

	local xml = string.match(body, "<[Ss]:log>(.-)</[Ss]:log>")
	if xml then
		return xml
	end

	xml = string.match(body, "<log>(.-)</log>")
	if xml then
		return xml
	end

	return ""
end


local function _extract_proppatch_props(body)
	local out = {}
	body = body or ""

	for block in string.gmatch(body, "<[Dd]:set>.-</[Dd]:set>") do
		for name, _attrs, val in string.gmatch(block, "<([%w]+:[%w%.:%-]+)([^>]*)>([^<]*)</") do
			local svn_name = _dav_prop_svn_name(name)
			if svn_name then
				out[#out + 1] = { name = svn_name, value = _xml_unescape(val), remove = false }
			end
		end
	end

	-- 删除块元素可能带属性 (如 <S:ignore V:absent="1"></S:ignore>), 故名字后允许任意属性
	for block in string.gmatch(body, "<[Dd]:remove>.-</[Dd]:remove>") do
		for name in string.gmatch(block, "<([%w]+:[%w%.:%-]+)([^>]*)>") do
			local svn_name = _dav_prop_svn_name(name)
			if svn_name then
				out[#out + 1] = { name = svn_name, value = nil, remove = true }
			end
		end
	end

	return out
end


local function _node_kind_at(ra, path, rev)
	path = _rel_clean(path)
	if path == "" then
		return "dir"
	end

	local parent = string.match(path, "^(.*)/[^/]+$") or ""
	local base = string.match(path, "([^/]+)$") or path
	local ents = ra:list(parent, rev) or {}

	for i = 1, #ents do
		if ents[i].name == base then
			return ents[i].kind or "file"
		end
	end

	if ra:cat(path, rev) ~= nil then
		return "file"
	end

	local dir_ents = ra:list(path, rev)
	if dir_ents then
		return "dir"
	end

	return "file"
end


local function _do_post(req, res, cfg, repo, rest, user, pass, body)
	local kind = _match_special(rest)
	if kind ~= "me" then
		_send(res, 404, "POST path not supported")
		return
	end

	body = body or ""
	-- ra_serf: skel "( create-txn )" / "( create-txn-with-props ... )"; 兼容裸词/长度前缀
	local action = string.match(body, "^%s*%(%s*([%w%-]+)")
	if not action then
		action = string.match(body, "^%s*%(%s*%d+:([%w%-]+)")
	end
	if not action then
		action = string.match(body, "^%s*([%w%-]+)")
	end
	if not action then
		action = string.match(body, "^%s*%d+:([%w%-]+)")
	end
	if action ~= "create-txn" and action ~= "create-txn-with-props" then
		-- 部分客户端只靠 header; 体空也当 create-txn
		if body == "" or string.match(body, "^%s*$") then
			action = "create-txn"
		elseif string.find(body, "create-txn-with-props", 1, true) then
			action = "create-txn-with-props"
		elseif string.find(body, "create-txn", 1, true) then
			action = "create-txn"
		else
			_send(res, 501, "unsupported POST action")
			return
		end
	end

	local name = _txn_new_name(cfg)
	local logmsg = ""
	if action == "create-txn-with-props" then
		logmsg = _extract_svn_log(body)
	end

	local store = _txn_store(cfg)
	local now = os.time()
	store[name] = {
		repo = repo,
		user = user,
		pass = pass,
		log = logmsg,
		ops = {},
		created_at = now,
		last_touch = now,
	}
	_dbg("txn create %s action=%s loglen=%d", name, action, #logmsg)

	_send(res, 201, "", {
		{ "SVN-Txn-Name", name },
		{ "Cache-Control", "no-cache" },
		{ "Server", "alua-svn-https" },
	})
end


local function _do_proppatch(req, res, cfg, repo, rest, user, pass, body)
	local kind, txn, rel = _match_special(rest)

	if kind == "txr" then
		-- empty rel = txn/repo root dir props (svn:ignore 等; Bug.15)
		rel = _rel_clean(rel)

		local t = _txn_get(cfg, txn, repo)
		if not t then
			_send(res, 404, "txn not found")
			return
		end

		local ver = _header_get(req.headers, "x-svn-version-name")
		local props = _extract_proppatch_props(body)
		_dbg("proppatch txn=%s rel=%s ver=%s nprops=%d body=%s",
			txn, rel, tostring(ver), #props, string.sub(tostring(body or ""), 1, 400))
		local ra, err = _ra(cfg, repo, user, pass)
		local node_kind = "dir"

		if rel == "" then
			node_kind = "dir"
		elseif ra then
			node_kind = _node_kind_at(ra, rel, tonumber(ver) or ra:youngest())
		else
			node_kind = "file"
		end

		for i = 1, #props do
			local p = props[i]
			local op = {
				op = "prop",
				path = rel,
				name = p.name,
				kind = node_kind,
			}
			if not p.remove then
				op.value = p.value or ""
			end
			if ver and ver ~= "" then
				op.rev = tonumber(ver)
			end
			t.ops[#t.ops + 1] = op
		end

		_send(res, 207, table.concat({
			'<?xml version="1.0" encoding="utf-8"?>\n',
			'<D:multistatus xmlns:D="DAV:">\n',
			"<D:response><D:href>", _xml_escape(req.path or ""), "</D:href>\n",
			"<D:propstat><D:prop/><D:status>HTTP/1.1 200 OK</D:status></D:propstat>\n",
			"</D:response></D:multistatus>\n",
		}), {
			{ "Content-Type", 'text/xml; charset="utf-8"' },
			{ "Server", "alua-svn-https" },
		})
		return
	end

	if kind ~= "txn" then
		_send(res, 501, "PROPPATCH only on !svn/txn or !svn/txr")
		return
	end

	local t = _txn_get(cfg, txn, repo)
	if not t then
		_send(res, 404, "txn not found")
		return
	end

	local logmsg = _extract_svn_log(body)
	if logmsg ~= "" then
		t.log = logmsg
	end

	_send(res, 207, table.concat({
		'<?xml version="1.0" encoding="utf-8"?>\n',
		'<D:multistatus xmlns:D="DAV:">\n',
		"<D:response><D:href>", _xml_escape(req.path or ""), "</D:href>\n",
		"<D:propstat><D:prop/><D:status>HTTP/1.1 200 OK</D:status></D:propstat>\n",
		"</D:response></D:multistatus>\n",
	}), {
		{ "Content-Type", 'text/xml; charset="utf-8"' },
		{ "Server", "alua-svn-https" },
	})
end


local function _do_mkcol(req, res, cfg, repo, rest, user, pass)
	local kind, txn, rel = _match_special(rest)
	if kind ~= "txr" then
		_send(res, 501, "MKCOL only on !svn/txr")
		return
	end

	rel = _rel_clean(rel)
	if rel == "" then
		_send(res, 403, "cannot MKCOL txn root")
		return
	end

	local t = _txn_get(cfg, txn, repo)
	if not t then
		_send(res, 404, "txn not found")
		return
	end

	-- Bug.26/27: spool 前预检查父路径存在 + 目标不存在, 立即返回 409
	local ra, err = _ra(cfg, repo, user, pass)
	if ra then
		local young = ra:youngest()
		-- 内联 dirname/basename (无独立辅助函数)
		local parent_rel = ""
		local base_name = rel
		local last_slash = string.find(rel, "/[^/]*$")
		if last_slash then
			parent_rel = string.sub(rel, 1, last_slash - 1)
			base_name = string.sub(rel, last_slash + 1)
		end
		-- Bug.26: 父路径不存在 → 409 Conflict
		-- Bug.2026-28: txn 内已建/复制目录视为父路径已存在 (svn mkdir a a/b 同事务)
		if parent_rel ~= "" then
			local pvis = _txn_path_state(t, parent_rel)
			if pvis == nil or pvis == "put" or pvis == "copy" then
				ra:close()
				_send_dav_error(res, 409, "svn:path-not-found",
					string.format("Path '%s' not found in transaction; try svn mkdir --parents", parent_rel), 160013)
				return
			end
			if pvis == "repos" then
				local parent_ents = ra:list(parent_rel, young)
				if not parent_ents then
					ra:close()
					_send_dav_error(res, 409, "svn:path-not-found",
						string.format("Path '%s' not found in transaction; try svn mkdir --parents", parent_rel), 160013)
					return
				end
			end
		end
		-- Bug.27: 目标路径已存在 → 409 Conflict
		-- 用 ra:list 父目录 + 匹配 name 来判断 (避免 stat 接口缺失)
		-- Bug.2026-28: txn 内 mkdir/put/copy 同路径亦视为已存在; txn 内已删除则允许重建
		local target_exists = false
		local tvis = _txn_path_state(t, rel)
		if tvis ~= "repos" then
			if tvis ~= nil then
				target_exists = true
			end
		elseif parent_rel == "" then
			-- 目标是仓根下的直接子项, list 根目录
			local root_ents = ra:list("", young)
			if root_ents then
				for i = 1, #root_ents do
					if root_ents[i].name == base_name then
						target_exists = true
						break
					end
				end
			end
		else
			local parent_ents = ra:list(parent_rel, young)
			if parent_ents then
				for i = 1, #parent_ents do
					if parent_ents[i].name == base_name then
						target_exists = true
						break
					end
				end
			end
		end
		if target_exists then
			ra:close()
			_send_dav_error(res, 409, "svn:already-exists",
				string.format("Path '%s' already exists", rel), 160020)
			return
		end
		if ra.close then
			ra:close()
		end
	end

	t.ops[#t.ops + 1] = { op = "mkdir", path = rel }
	_send(res, 201, "", { { "Server", "alua-svn-https" } })
end


local function _do_put(req, res, cfg, repo, rest, user, pass, body)
	local kind, txn, rel = _match_special(rest)
	if kind ~= "txr" then
		_send(res, 501, "PUT only on !svn/txr")
		return
	end

	rel = _rel_clean(rel)
	if rel == "" then
		_send(res, 403, "cannot PUT txn root")
		return
	end

	local t = _txn_get(cfg, txn, repo)
	if not t then
		_send(res, 404, "txn not found")
		return
	end

	local ctype = _header_get(req.headers, "content-type") or ""
	local ver = _header_get(req.headers, "x-svn-version-name")
	local is_add = (ver == nil or ver == "")
	local payload = body or ""
	local sniff_ver = nil

	if string.sub(payload, 1, 3) == "SVN" then
		sniff_ver = string.byte(payload, 4)
	end

	-- 仅前三字节为 "SVN" 的明文不得判为 svndiff; 须见合法版本字节 0..3
	local payload_svndiff = string.find(ctype, "svndiff", 1, true) ~= nil
		or (sniff_ver ~= nil and sniff_ver >= 0 and sniff_ver <= 3)

	-- Bug.13: modify PUT 的 svndiff 在 txn 内解成明文, 避免 MERGE 时 apply 失败
	if not is_add and payload_svndiff and payload ~= "" then
		local ra_put, err_put = _ra(cfg, repo, user, pass)
		if ra_put then
			local put_rev = tonumber(ver) or ra_put:youngest()
			local old = ra_put:cat(rel, put_rev)
			if old == nil then
				old = ""
			end
			local plain = ksvnra.svndiff_apply(old, payload)
			if type(plain) == "string" then
				payload = plain
				payload_svndiff = false
			end
		end
	end

	local nbytes = #payload
	if FILE_MAX_BYTES > 0 and nbytes > FILE_MAX_BYTES then
		_send(res, 413, "PUT body too large", { { "Content-Type", "text/plain" } })
		return
	end

	if TXN_MAX_BYTES > 0 and (_txn_bytes_total(t) + nbytes) > TXN_MAX_BYTES then
		_send(res, 413, "txn buffer full", { { "Content-Type", "text/plain" } })
		return
	end

	local op = { op = "put", path = rel, add = is_add }
	local spool_path = nil
	local spool_size = nil

	if ver and ver ~= "" then
		op.rev = tonumber(ver)
	end

	_spool_gc()
	if SPOOL_ON then
		spool_path, spool_size = _spool_write(txn, "put", payload)
	end

	if spool_path then
		op.spool_path = spool_path
		op.spool_size = spool_size
		op.spool_svndiff = payload_svndiff
	elseif payload_svndiff then
		op.svndiff = payload
	else
		op.data = payload
	end

	t.ops[#t.ops + 1] = op
	-- ra_serf: add -> 201 Created; modify (X-SVN-Version-Name) -> 204 No Content
	-- Bug.3: modify 回 201 会 E175002 "unexpectedly created"
	local code = is_add and 201 or 204
	_send(res, code, "", { { "Server", "alua-svn-https" } })
end


local function _do_delete(req, res, cfg, repo, rest, user, pass)
	local kind, a, rel = _match_special(rest)

	if kind == "txn" then
		local store = _txn_store(cfg)
		_txn_spool_free(store[a])
		store[a] = nil
		_send(res, 204, "", { { "Server", "alua-svn-https" } })
		return
	end

	if kind ~= "txr" then
		_send(res, 501, "DELETE only on !svn/txn or !svn/txr")
		return
	end

	rel = _rel_clean(rel)
	if rel == "" then
		_send(res, 403, "cannot DELETE txn root path")
		return
	end

	local t = _txn_get(cfg, a, repo)
	if not t then
		_send(res, 404, "txn not found")
		return
	end

	local ver = _header_get(req.headers, "x-svn-version-name")
	local op = { op = "delete", path = rel }
	if ver and ver ~= "" then
		op.rev = tonumber(ver)
	end
	t.ops[#t.ops + 1] = op
	_send(res, 204, "", { { "Server", "alua-svn-https" } })
end


-- @brief 多 op 公共父目录 (不含末段); 对齐官方 RA 会话落在父路径
local function _ops_common_parent(ops)
	if type(ops) ~= "table" or #ops == 0 then
		return ""
	end

	local function parent_of(p)
		p = _rel_clean(p)
		local dir = string.match(p, "^(.*)/[^/]+$")
		return dir or ""
	end

	local common = parent_of(ops[1].path)
	for i = 2, #ops do
		local p = parent_of(ops[i].path)
		while common ~= "" do
			if p == common or string.sub(p, 1, #common + 1) == common .. "/" then
				break
			end
			common = string.match(common, "^(.*)/[^/]+$") or ""
		end
	end

	return common
end


local function _ops_strip_parent(ops, parent)
	if parent == nil or parent == "" then
		return ops
	end

	local prefix = parent .. "/"
	local out = {}
	for i = 1, #ops do
		local o = ops[i]
		local n = {}
		for k, v in pairs(o) do
			n[k] = v
		end
		local path = _rel_clean(o.path)
		if string.sub(path, 1, #prefix) == prefix then
			n.path = string.sub(path, #prefix + 1)
		end
		if type(o.copyfrom_path) == "string" and o.copyfrom_path ~= "" then
			local cf = _rel_clean(o.copyfrom_path)
			if string.sub(cf, 1, #prefix) == prefix then
				n.copyfrom_path = string.sub(cf, #prefix + 1)
			end
		end
		out[#out + 1] = n
	end

	return out
end


local function _do_merge(req, res, cfg, repo, rest, user, pass, body)
	local kind, txn = _match_special(rest)
	-- HTTPv2: MERGE 常打在仓根, body 内 D:href 指向 !svn/txn/NAME
	if kind ~= "txn" then
		txn = string.match(body or "", "/!svn/txn/([^</\"%s]+)")
		if not txn then
			_send(res, 501, "MERGE needs !svn/txn or source href")
			return
		end
	end

	local store = _txn_store(cfg)
	local t = _txn_get(cfg, txn, repo)
	if not t then
		_dbg("merge txn=%s not found", tostring(txn))
		_send(res, 404, "txn not found")
		return
	end

	_dbg("merge txn=%s ops=%d", tostring(txn), #t.ops)
	if #t.ops == 0 then
		-- 空事务不可回 200 空体: serf 解析 merge-response 会报 E175009 非法 XML
		store[txn] = nil
		_send_dav_error(res, 409, "svn:commit-failed", "empty transaction: no changes")
		return
	end

	-- M4: spool 的 body 在提交前读回内存 (txn 期间仅占磁盘)
	for i = 1, #t.ops do
		local mop, merr = _op_materialize(t.ops[i])
		if not mop then
			_txn_spool_free(t)
			store[txn] = nil
			_send_ra_error(res, nil, "svn:ra-error", merr, 0, "")
			return
		end
	end

	-- 官方客户端把 RA 会话落在父目录, add-file 只用相对名; 避免 open-dir 畸形
	-- copyfrom 落在父目录之外时勿 reparent (否则 copy URL 指错; Bug.16)
	local parent = _ops_common_parent(t.ops)
	if parent ~= "" then
		for i = 1, #t.ops do
			local cf = t.ops[i].copyfrom_path
			if type(cf) == "string" and cf ~= "" then
				local cfc = _rel_clean(cf)
				if cfc ~= parent and string.sub(cfc, 1, #parent + 1) ~= (parent .. "/") then
					parent = ""
					break
				end
			end
		end
	end

	local open_repo = repo
	local commit_ops = t.ops
	if parent ~= "" then
		open_repo = repo .. "/" .. parent
		commit_ops = _ops_strip_parent(t.ops, parent)
	end

	-- 独立会话 commit, 避免与读路径共用 socket 交错
	local ra, err = ksvnra.open({
		hostport = cfg.svnserve or "127.0.0.1:3690",
		user = user,
		pass = pass,
		repo = open_repo,
	})
	if not ra then
		_send_ra_fail(res, cfg, open_repo, err)
		return
	end

	local base_y = ra:youngest()
	local new_rev, cerr = ra:commit({ log = t.log or "", ops = commit_ops })
	-- 错误码/消息须在 close 前捕获, 否则 C 会话释放后 last_error 为空
	local ecode, emsg = nil, nil
	if not new_rev then
		ecode, emsg = _ra_err(ra)
	end
	if ra.close then
		ra:close()
	end
	if not new_rev then
		-- editor 收尾未解析到 rev 时, 另开会话看 FS 是否已前进
		local ra2 = ksvnra.open({
			hostport = cfg.svnserve or "127.0.0.1:3690",
			user = user,
			pass = pass,
			repo = repo,
		})
		if ra2 then
			local y2 = ra2:youngest()
			if ra2.close then
				ra2:close()
			end
			if type(y2) == "number" and type(base_y) == "number" and y2 > base_y then
				new_rev = y2
				cerr = nil
			end
		end
	end
	_txn_spool_free(t)
	store[txn] = nil
	cfg._ra_cache = {}

	if not new_rev then
		-- Bug.26/27: commit 失败时 _ra_err(ra) 常返回空(ra:close 先调或 ra_serf 不填充);
		-- 实际错误详情在 cerr 里. 把 cerr 合入 err 参数便于 _send_ra_error 关键词匹配
		local combined_err = tostring(cerr or "")
		if combined_err == "" then
			combined_err = tostring(emsg or "")
		end
		-- MERGE 失败留痕: 错误正文/码与事务规模, 便于定位 svnserve 侧原因
		if type(log) == "table" and type(log.write) == "function" then
			local nops = 0

			if type(commit_ops) == "table" then
				nops = #commit_ops
			end
			log.write("error", "svn_https merge failed", {
				module = "svn_https",
				repo = repo,
				txn = txn,
				ops = nops,
				ecode = ecode or 0,
				emsg = combined_err,
				cerr = tostring(cerr or ""),
			})
		end
		-- 传 combined_err 作 err 参数, 让 _send_ra_error 关键词匹配生效
		_send_ra_error(res, nil, "svn:commit-failed", tostring(cerr or "commit failed"), ecode or 0, combined_err)
		return
	end

	-- ra_serf: 仅当 resourcetype=baseline 时把 version-name 记为 commit rev
	local vcc = "/svn/" .. repo .. "/!svn/vcc/default"
	local body = table.concat({
		'<?xml version="1.0" encoding="utf-8"?>\n',
		'<D:merge-response xmlns:D="DAV:">\n',
		"<D:updated-set>\n<D:response>\n",
		"<D:href>", _xml_escape(vcc), "</D:href>\n",
		"<D:propstat>\n<D:prop>\n",
		"<D:resourcetype><D:baseline/></D:resourcetype>\n",
		"<D:version-name>", tostring(new_rev), "</D:version-name>\n",
		"</D:prop>\n<D:status>HTTP/1.1 200 OK</D:status>\n</D:propstat>\n",
		"</D:response>\n</D:updated-set>\n</D:merge-response>\n",
	})

	_send(res, 200, body, {
		{ "Content-Type", 'text/xml; charset="utf-8"' },
		{ "SVN-Youngest-Rev", tostring(new_rev) },
		{ "Server", "alua-svn-https" },
	})
end


local function _txn_append_copy(req, res, cfg, repo, rest, user, pass, also_delete_src)
	local dest = _header_get(req.headers, "destination")
	local txn_dst, rel_dst = _parse_destination_txr(dest, repo)
	if not txn_dst then
		_send(res, 400, "bad Destination")
		return
	end

	local t = _txn_get(cfg, txn_dst, repo)
	if not t then
		_send(res, 404, "txn not found")
		return
	end

	if rel_dst == "" then
		_send(res, 403, "cannot COPY txn root")
		return
	end

	local copyfrom_rev = nil
	local copyfrom_path = nil
	local rev_s, rel_rvr = string.match(rest, "^/!svn/rvr/(%d+)/?(.*)$")
	local txn_src = nil
	local rel_src = nil

	if rev_s then
		copyfrom_rev = tonumber(rev_s)
		copyfrom_path = _rel_clean(rel_rvr)
	elseif not string.match(rest, "^/!svn/") then
		copyfrom_path = _rel_clean(rest)
		local ver = _header_get(req.headers, "x-svn-version-name")
		copyfrom_rev = tonumber(ver)
		if not copyfrom_rev then
			local ra, err = _ra(cfg, repo, user, pass)
			if not ra then
				_send_ra_fail(res, cfg, repo, err)
				return
			end
			copyfrom_rev = ra:youngest()
		end
	else
		txn_src, rel_src = string.match(rest, "^/!svn/txr/([^/]+)/?(.*)$")
		if not txn_src or txn_src ~= txn_dst then
			_send(res, 501, "COPY source not supported")
			return
		end

		copyfrom_path = _rel_clean(rel_src)
		local ver = _header_get(req.headers, "x-svn-version-name")
		copyfrom_rev = tonumber(ver)
		if not copyfrom_rev then
			local ra, err = _ra(cfg, repo, user, pass)
			if not ra then
				_send_ra_fail(res, cfg, repo, err)
				return
			end
			copyfrom_rev = ra:youngest()
		end
	end

	if not copyfrom_rev or copyfrom_path == "" then
		_send(res, 400, "bad copy source")
		return
	end

	local vis, pop = _txn_path_state(t, copyfrom_path)
	if vis == "put" then
		local nop = { op = "put", path = rel_dst, add = true }
		if type(pop.spool_path) == "string" then
			nop.spool_path = pop.spool_path
			nop.spool_size = pop.spool_size
			nop.spool_svndiff = pop.spool_svndiff
		elseif pop.svndiff then
			nop.svndiff = pop.svndiff
		else
			nop.data = pop.data or ""
		end
		t.ops[#t.ops + 1] = nop
	elseif vis == "copy" then
		t.ops[#t.ops + 1] = {
			op = "copy",
			path = rel_dst,
			copyfrom_path = pop.copyfrom_path,
			copyfrom_rev = pop.copyfrom_rev,
			kind = pop.kind or "file",
		}
	elseif vis == "copydir" then
		t.ops[#t.ops + 1] = {
			op = "copy",
			path = rel_dst,
			copyfrom_path = pop.copyfrom_path,
			copyfrom_rev = pop.copyfrom_rev,
			kind = "dir",
		}
	elseif vis == "mkdir" then
		if _txn_has_children(t, copyfrom_path) then
			_txn_replicate_prefix(t, copyfrom_path, rel_dst)
		else
			t.ops[#t.ops + 1] = { op = "mkdir", path = rel_dst }
		end
	elseif vis == nil then
		-- move commit: 先 DELETE 源再 COPY 自 !svn/rvr/公开 URL; txn 内已删不等于 peg 不存在
		if not rev_s and txn_src then
			_send(res, 404, "copy source deleted in txn")
			return
		end
		vis = "repos"
	end

	if vis == "repos" then
		local ra, err = _ra(cfg, repo, user, pass)
		if not ra then
			_send_ra_fail(res, cfg, repo, err)
			return
		end

		local kind = _node_kind_at(ra, copyfrom_path, copyfrom_rev)
		t.ops[#t.ops + 1] = {
			op = "copy",
			path = rel_dst,
			copyfrom_path = copyfrom_path,
			copyfrom_rev = copyfrom_rev,
			kind = kind,
		}
	end

	if also_delete_src then
		local src_rel = copyfrom_path
		local del_rev = copyfrom_rev
		local txn_src, rel_txr = string.match(rest, "^/!svn/txr/([^/]+)/?(.*)$")

		if txn_src then
			src_rel = _rel_clean(rel_txr)
			local ver = _header_get(req.headers, "x-svn-version-name")
			del_rev = tonumber(ver) or del_rev
		end

		if src_rel ~= "" and src_rel ~= rel_dst then
			local op = { op = "delete", path = src_rel }
			if del_rev then
				op.rev = del_rev
			end
			-- MOVE: 与分步 DELETE+COPY 一致, delete 须在 copy 之前
			local last_op = table.remove(t.ops)
			t.ops[#t.ops + 1] = op
			t.ops[#t.ops + 1] = last_op
		end
	end

	_send(res, 201, "", { { "Server", "alua-svn-https" } })
end


local function _do_copy(req, res, cfg, repo, rest, user, pass)
	_txn_append_copy(req, res, cfg, repo, rest, user, pass, false)
end


local function _do_move(req, res, cfg, repo, rest, user, pass)
	_txn_append_copy(req, res, cfg, repo, rest, user, pass, true)
end


local function _do_lock(req, res, cfg, repo, rest, user, pass, body)
	local rel = _rel_clean(rest)
	local existing = _lock_get(cfg, repo, rel)
	local token
	local owner = user or "user"
	local lock_sec = 3600
	local tmo = _header_get(req.headers, "timeout")
	if type(tmo) == "string" then
		local sec = tonumber(string.match(tmo, "Second%-(%d+)"))
		if sec then
			lock_sec = sec
		end
	end

	if type(existing) == "table" and type(existing.token) == "string" and existing.token ~= "" then
		token = existing.token
		owner = existing.owner or owner
	else
		token = string.format("opaquelocktoken:%s-%s-%d",
			repo, rel ~= "" and string.gsub(rel, "/", "_") or "root", os.time())
		_lock_set(cfg, repo, rel, {
			token = token,
			owner = owner,
			created = os.time(),
			expires = os.time() + lock_sec,
		})
	end

	local body_xml = table.concat({
		'<?xml version="1.0" encoding="utf-8"?>\n',
		'<D:prop xmlns:D="DAV:"><D:lockdiscovery><D:activelock>\n',
		"<D:locktype><D:write/></D:locktype>\n",
		"<D:lockscope><D:exclusive/></D:lockscope>\n",
		"<D:depth>0</D:depth>\n",
		"<D:creator-displayname>", _xml_escape(owner), "</D:creator-displayname>\n",
		"<D:owner><D:href>", _xml_escape(owner), "</D:href></D:owner>\n",
		"<D:timeout>Second-", tostring(lock_sec), "</D:timeout>\n",
		"<D:locktoken><D:href>", _xml_escape(token), "</D:href></D:locktoken>\n",
		"</D:activelock></D:lockdiscovery></D:prop>\n",
	})
	_send(res, 200, body_xml, {
		{ "Lock-Token", "<" .. token .. ">" },
		{ "X-SVN-Lock-Owner", owner },
		{ "Content-Type", 'text/xml; charset="utf-8"' },
		{ "Server", "alua-svn-https" },
	})
end


local function _do_unlock(req, res, cfg, repo, rest, user, pass, body)
	local rel = _rel_clean(rest)
	local info = _lock_get(cfg, repo, rel)
	local hdr = _header_get(req.headers, "lock-token") or ""
	local token = string.match(hdr, "<([^>]+)>") or string.match(hdr, "(opaquelocktoken:[%w%-%._]+)") or hdr

	if not info then
		_send(res, 409, "not locked", { { "Content-Type", "text/plain" }, { "Server", "alua-svn-https" } })
		return
	end

	if token ~= "" and info.token ~= token then
		_send(res, 409, "lock token mismatch", { { "Content-Type", "text/plain" }, { "Server", "alua-svn-https" } })
		return
	end

	_lock_del(cfg, repo, rel)
	_send(res, 204, "", { { "Server", "alua-svn-https" } })
end


-- @brief ra 会话互斥键: 同仓/同用户共享一条 ra 会话 (P1-5)
local function _ra_key(repo, user)
	return tostring(repo or "") .. "\0" .. tostring(user or "")
end


-- @brief 会写 cfg 共享表 (txn / 锁) 的方法 (M4)
local WRITE_METHODS = {
	POST = true,
	PUT = true,
	DELETE = true,
	PROPPATCH = true,
	MKCOL = true,
	COPY = true,
	MOVE = true,
	MERGE = true,
	LOCK = true,
	UNLOCK = true,
}


-- @brief 仓级写互斥键 (txn / 锁表按仓串行; 与 ra 会话键区分)
-- @param [in] repo[string]
-- @return key[string]
local function _repo_write_key(repo)
	return "w\0" .. tostring(repo or "")
end


-- @brief 获取会话互斥 (协程让出等待; P1-5)
local function _session_lock(cfg, key)
	local mtx = cfg._mutex
	if type(mtx) ~= "table" then
		mtx = {}
		cfg._mutex = mtx
	end

	while mtx[key] do
		if type(kco) == "table" and type(kco.co_sleep) == "function" then
			kco.co_sleep(20)
		else
			break
		end
	end

	mtx[key] = true
end


-- @brief 释放会话互斥
local function _session_unlock(cfg, key)
	if type(cfg._mutex) == "table" then
		cfg._mutex[key] = nil
	end
end


-- @brief 按方法分派 (P1-5: 由 M.handle 持会话锁调用)
local function _dispatch(method, req, res, cfg, repo, rest, user, pass, body)
	if method == "OPTIONS" then
		_do_options(req, res, cfg, repo, user, pass)
	elseif method == "PROPFIND" then
		_do_propfind(req, res, cfg, repo, rest, user, pass)
	elseif method == "REPORT" then
		_do_report(req, res, cfg, repo, rest, user, pass, body)
	elseif method == "GET" then
		_do_get(req, res, cfg, repo, rest, user, pass)
	elseif method == "HEAD" then
		_do_head(req, res, cfg, repo, rest, user, pass)
	elseif method == "POST" then
		_do_post(req, res, cfg, repo, rest, user, pass, body)
	elseif method == "PROPPATCH" then
		_do_proppatch(req, res, cfg, repo, rest, user, pass, body)
	elseif method == "MKCOL" then
		_do_mkcol(req, res, cfg, repo, rest, user, pass)
	elseif method == "PUT" then
		_do_put(req, res, cfg, repo, rest, user, pass, body)
	elseif method == "DELETE" then
		_do_delete(req, res, cfg, repo, rest, user, pass)
	elseif method == "COPY" then
		_do_copy(req, res, cfg, repo, rest, user, pass)
	elseif method == "MOVE" then
		_do_move(req, res, cfg, repo, rest, user, pass)
	elseif method == "LOCK" then
		_do_lock(req, res, cfg, repo, rest, user, pass, body)
	elseif method == "UNLOCK" then
		_do_unlock(req, res, cfg, repo, rest, user, pass, body)
	elseif method == "MERGE" then
		_do_merge(req, res, cfg, repo, rest, user, pass, body)
	elseif method == "CHECKOUT" or method == "MKACTIVITY" or method == "BASELINE-CONTROL"
		or method == "LABEL" or method == "UPDATE" then
		-- P0-1: 经典 DeltaV 写未实现, 回显式不支持 (不再广告 CHECKOUT / version-control)
		_send_dav_error(res, 501, "svn:unsupported-feature", "classic DeltaV not implemented; use HTTPv2")
	else
		_send_dav_error(res, 501, "svn:unsupported-feature", "method not implemented: " .. method)
	end
end


-- @brief 处理一条 DAV 请求
-- @param [in] req[table]
-- @param [in] res[table]
-- @param [in] cfg[table] { svnserve }
M.handle = function (req, res, cfg)
	cfg = cfg or {}
	_txn_purge(cfg)

	local method = string.upper(tostring(req.method or "GET"))
	local path = req.path or "/"
	if os.getenv("ALUA_SVN_DAV_DEBUG") == "1" then
		local dest = _header_get(req.headers, "destination") or ""
		local preview = string.sub(tostring(req.body or ""), 1, 160)
		preview = string.gsub(preview, "[\r\n]", " ")
		io.stderr:write("[dav] ", method, " ", path, " dest=", dest, " body=", preview, "\n")
		local f = io.open("/tmp/svn_dav_http.log", "a")
		if f then
			f:write(method, " ", path, " dest=", dest, " body=", preview, "\n")
			f:close()
		end
	end
	local parsed = { _parse_path(path) }
	local repo, rest = parsed[1], parsed[2]

	if not repo then
		_send(res, 404, "not found")
		return
	end

	local user, pass = _need_auth(req, res)
	if not user then
		return
	end

	local body = req.body or ""
	if type(body) ~= "string" then
		body = ""
	end

	-- P1-7: 最小 access log (方法/路径/仓/用户)
	if type(log) == "table" and type(log.write) == "function" then
		log.write("info", "svn_https access", {
			module = "svn_https",
			method = method,
			path = path,
			repo = repo,
			user = user,
		})
	end

	-- P1-5: 同仓/同用户串行 (ra 会话为共享 socket, 不可并发复用)
	local key = _ra_key(repo, user)
	_session_lock(cfg, key)

	-- M4: 写方法再加仓级互斥 (txn / 锁表为共享可变表)
	local wkey = nil
	if WRITE_METHODS[method] then
		wkey = _repo_write_key(repo)
		_session_lock(cfg, wkey)
	end

	local ok, derr = pcall(_dispatch, method, req, res, cfg, repo, rest, user, pass, body)
	if wkey ~= nil then
		_session_unlock(cfg, wkey)
	end
	_session_unlock(cfg, key)

	if not ok and os.getenv("ALUA_SVN_DAV_DEBUG") == "1" then
		io.stderr:write("[dav] dispatch error: ", tostring(derr), "\n")
	end
	if not ok and not res._sent then
		-- Bug.26/27: pcall 捕获的 Lua error 也尝试按 SVN_ERR_TAGS 映射
		local http = 500
		if type(derr) == "string" then
			local m = string.match(derr, "svn:-(%d+)") or string.match(derr, "code[=:](%d+)")
			if m then
				local c = tonumber(m)
				local _RA_CODE_HTTP = {
					[160013] = 409, [160020] = 409, [160006] = 404,
					[170003] = 403, [195012] = 409,
				}
				http = _RA_CODE_HTTP[c] or 500
			end
		end
		if os.getenv("ALUA_SVN_DAV_DEBUG") == "1" then
			io.stderr:write(string.format("[dav] dispatch pcall fallback http=%d derr=%s\n", http, tostring(derr)))
		end
		_send_dav_error(res, http, "svn:ra-error", tostring(derr or "handler error"))
	end
end


return M

-- end
