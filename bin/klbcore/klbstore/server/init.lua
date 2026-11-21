--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
--
-- @file   init.lua
-- @brief  klbstore HTTP 服务面 (klbweb 路由挂载)
-- @stability experimental
-- @impl evolving
-- @note   P0 路由: POST /store/v1/upload · POST /store/v1/delete · GET /store/v1/stat · GET /store/v1/storage
--   \n 下载: /store/blob/<file_id> 由 klbweb static 承担 (Range / 206 / ETag / 304), 本层不回文件体
--   \n 请求体上限: cfg.max_body 写入 app._cfg.max_body 与 max_body_paths["/store/v1/upload"]
--]]


local cjson = require("cjson")

local klbcode = require("klbcore.klbcode")
local base = klbcode.base
local id = require("klbcore.klbstore.id")
local storage = require("klbcore.klbstore.storage")
local index = require("klbcore.klbstore.index")


local mod = {}


mod.PREFIX = "/store/v1"
mod.BLOB_PREFIX = "/store/blob"


local HTTP_OF = {
	[base.INVAL] = 400,
	[base.ACCES] = 401,
	[base.NOENT] = 404,
	[base.BUSY] = 409,
	[base.RANGE] = 416,
	[base.NOSPC] = 507,
}


local function _http_of(code)
	return HTTP_OF[code] or 500
end


local function _reply(res, obj, code)
	res:co_json(obj, _http_of(code))
end


local function _urldecode(s)
	s = string.gsub(s or "", "+", " ")
	return (string.gsub(s, "%%(%x%x)", function (hex)
		return string.char(tonumber(hex, 16))
	end))
end


local function _query(req)
	local out = {}
	local qs = req.query_string or ""

	for pair in string.gmatch(qs, "[^&]+") do
		local k, v = string.match(pair, "^([^=]*)=(.*)$")
		if k == nil then
			k = pair
			v = ""
		end
		out[_urldecode(k)] = _urldecode(v)
	end

	return out
end


local function _json_body(req)
	if type(req.body) ~= "string" or req.body == "" then
		return nil
	end

	local ok, obj = pcall(cjson.decode, req.body)
	if not ok or type(obj) ~= "table" then
		return nil
	end

	return obj
end


local function _handle_upload(st, req, res)
	local q = _query(req)
	local group = q.group
	local ext = q.ext
	local overwrite = (q.overwrite == "1" or q.overwrite == "true")
	local body = req.body or ""

	local file_id, code = id.new_id(group, ext)
	if file_id == nil then
		_reply(res, { ok = false, error = "id", code = code }, code)
		return
	end

	local ok, code_p = storage.put(st, file_id, body, { overwrite = overwrite })
	if not ok then
		_reply(res, { ok = false, error = "put", code = code_p }, code_p)
		return
	end

	_reply(res, { ok = true, file_id = file_id, size = #body }, klbcode.OK)
end


local function _handle_delete(st, req, res)
	local obj = _json_body(req)
	local q = _query(req)
	local file_id = nil

	if type(obj) == "table" then
		file_id = obj.file_id
	end
	if type(file_id) ~= "string" and type(q.file_id) == "string" then
		file_id = q.file_id
	end

	if type(file_id) ~= "string" or file_id == "" then
		_reply(res, { ok = false, error = "file_id", code = base.INVAL }, base.INVAL)
		return
	end

	local ok, code = storage.del(st, file_id)
	if not ok then
		_reply(res, { ok = false, error = "del", code = code }, code)
		return
	end

	_reply(res, { ok = true, file_id = file_id }, klbcode.OK)
end


local function _handle_stat(st, req, res)
	local q = _query(req)
	local file_id = q.file_id

	if type(file_id) ~= "string" or file_id == "" then
		_reply(res, { ok = false, error = "file_id", code = base.INVAL }, base.INVAL)
		return
	end

	local info, code = storage.stat(st, file_id)
	if info == nil then
		_reply(res, { ok = false, error = "stat", code = code }, code)
		return
	end

	_reply(res, {
		ok = true,
		file_id = file_id,
		size = info.size,
		mtime = info.mtime,
		url = mod.BLOB_PREFIX .. "/" .. file_id,
	}, klbcode.OK)
end


local function _handle_storage(idx, req, res)
	local q = _query(req)
	local group = q.group

	local info, code = index.stat(idx, group)
	if info == nil then
		_reply(res, { ok = false, error = "stat", code = code }, code)
		return
	end

	_reply(res, {
		ok = true,
		group = group,
		count = info.count,
		bytes = info.bytes,
		backend = index.backend,
	}, klbcode.OK)
end


-- @brief 挂载 klbstore 路由与静态下载前缀
-- @param [in] app[table]			klbweb app
-- @param [in] cfg[table]			{ root, db_path, group, max_body }
-- @return [table] 服务对象 { storage, index }; 失败 nil, code
-- @stability experimental
mod.mount = function (app, cfg)
	if type(app) ~= "table" or type(cfg) ~= "table" then
		return nil, base.INVAL
	end

	if type(cfg.root) ~= "string" or cfg.root == "" then
		return nil, base.INVAL
	end

	local db_path = cfg.db_path or (cfg.root .. "/.store.db")
	local idx, code = index.open(db_path)
	if idx == nil then
		return nil, code
	end

	local st, code_s = storage.new({
		root = cfg.root,
		group = cfg.group,
		index = idx,
	})
	if st == nil then
		index.close(idx)
		return nil, code_s
	end

	if type(cfg.max_body) == "number" and cfg.max_body > 0 and type(app._cfg) == "table" then
		app._cfg.max_body = cfg.max_body
		if type(app._cfg.max_body_paths) ~= "table" then
			app._cfg.max_body_paths = {}
		end
		app._cfg.max_body_paths[mod.PREFIX .. "/upload"] = cfg.max_body
	end

	app:post(mod.PREFIX .. "/upload", function (req, res)
		_handle_upload(st, req, res)
	end)

	app:post(mod.PREFIX .. "/delete", function (req, res)
		_handle_delete(st, req, res)
	end)

	app:get(mod.PREFIX .. "/stat", function (req, res)
		_handle_stat(st, req, res)
	end)

	app:get(mod.PREFIX .. "/storage", function (req, res)
		_handle_storage(idx, req, res)
	end)

	app:static(mod.BLOB_PREFIX, st.root, { listing = false })

	return { storage = st, index = idx, prefix = mod.PREFIX }
end


return mod

-- end
