--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua-tunnel PSK 会话 (N3-2; SHA256-CTR + HMAC 标签 AEAD; 非 TLS)
-- @note   PSK=显式 key 或 SHA256(user:pass); 会话键=SHA256(psk|nc|ns)
--  \n       SHA256 优先 ktunnel → kcrypto → 纯 Lua; 密封仍 ktunnel/Lua EtM
--  \n       Lua 密封: seq(4)+ct+tag(16); CSPRNG 优先 /dev/urandom
--]]


local kproxy_io = require("klbcore.klbproxy.io")
local frame = require("klbcore.klbtunnel.frame")


local M = {}


local function _load_wire()
	local ok, mod = pcall(require, "ktunnel")
	if ok and type(mod) == "table" and type(mod.sha256) == "function" then
		return mod
	end
	return nil
end


local function _load_kcrypto()
	local ok, mod = pcall(require, "kcrypto")
	if ok and type(mod) == "table" and type(mod.sha256) == "function" then
		return mod
	end
	return nil
end


local W = _load_wire()
local K = _load_kcrypto()


M.FLAG_CRYPTO = (W and W.FLAG_CRYPTO) or 0x01
M.NONCE_LEN = (W and W.NONCE_LEN) or 16
M.TAG_LEN = 16
M.T_DATA = (W and W.T_DATA) or 0x20


------------------------------------------------------------------
-- SHA256 (compact; public domain style; last resort when no C)
------------------------------------------------------------------

local function _u32(x)
	return x % 0x100000000
end


local function _ror(x, n)
	n = n % 32
	return _u32(math.floor(x / (2 ^ n)) + (x * (2 ^ (32 - n))))
end


local function _sha256(msg)
	local k = {
		0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
		0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
		0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
		0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
		0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
		0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
		0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
		0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
	}

	local h0, h1, h2, h3 = 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a
	local h4, h5, h6, h7 = 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19

	local len = #msg
	local bitlen = len * 8
	msg = msg .. string.char(0x80)
	while (#msg % 64) ~= 56 do
		msg = msg .. string.char(0)
	end

	-- big-endian 64-bit length (high 32 always 0 for our sizes)
	local function be32(n)
		n = _u32(n)
		return string.char(
			math.floor(n / 0x1000000) % 256,
			math.floor(n / 0x10000) % 256,
			math.floor(n / 0x100) % 256,
			n % 256
		)
	end

	msg = msg .. be32(0) .. be32(bitlen)

	for i = 1, #msg, 64 do
		local chunk = string.sub(msg, i, i + 63)
		local w = {}
		for j = 0, 15 do
			local b1, b2, b3, b4 = string.byte(chunk, j * 4 + 1, j * 4 + 4)
			w[j] = b1 * 0x1000000 + b2 * 0x10000 + b3 * 0x100 + b4
		end
		for j = 16, 63 do
			local v15, v2 = w[j - 15], w[j - 2]
			local s0 = _u32(_ror(v15, 7) ~ _ror(v15, 18) ~ math.floor(v15 / 8))
			local s1 = _u32(_ror(v2, 17) ~ _ror(v2, 19) ~ math.floor(v2 / 32))
			w[j] = _u32(w[j - 16] + s0 + w[j - 7] + s1)
		end

		local a, b, c, d = h0, h1, h2, h3
		local e, f, g, h = h4, h5, h6, h7
		for j = 0, 63 do
			local S1 = _u32(_ror(e, 6) ~ _ror(e, 11) ~ _ror(e, 25))
			local ch = _u32((e & f) ~ ((~e) & g))
			local t1 = _u32(h + S1 + ch + k[j + 1] + w[j])
			local S0 = _u32(_ror(a, 2) ~ _ror(a, 13) ~ _ror(a, 22))
			local maj = _u32((a & b) ~ (a & c) ~ (b & c))
			local t2 = _u32(S0 + maj)
			h = g
			g = f
			f = e
			e = _u32(d + t1)
			d = c
			c = b
			b = a
			a = _u32(t1 + t2)
		end

		h0 = _u32(h0 + a)
		h1 = _u32(h1 + b)
		h2 = _u32(h2 + c)
		h3 = _u32(h3 + d)
		h4 = _u32(h4 + e)
		h5 = _u32(h5 + f)
		h6 = _u32(h6 + g)
		h7 = _u32(h7 + h)
	end

	local out = {}
	local hs = { h0, h1, h2, h3, h4, h5, h6, h7 }
	for i = 1, 8 do
		out[#out + 1] = be32(hs[i])
	end
	return table.concat(out)
end


M.sha256 = function (msg)
	if W ~= nil then
		return W.sha256(msg or "")
	end

	if K ~= nil then
		return K.sha256(msg or "")
	end

	return _sha256(msg or "")
end


local function _byte(n)
	return string.char((tonumber(n) or 0) % 256)
end


local function _u16be(n)
	n = tonumber(n) or 0
	return string.char(math.floor(n / 256) % 256, n % 256)
end


local function _u32be(n)
	n = tonumber(n) or 0
	n = n % 0x100000000
	return string.char(
		math.floor(n / 0x1000000) % 256,
		math.floor(n / 0x10000) % 256,
		math.floor(n / 0x100) % 256,
		n % 256
	)
end


-- @brief 系统 CSPRNG (/dev/urandom); 失败回 nil
local function _csprng_bytes(n)
	n = tonumber(n) or 0
	if n <= 0 then
		return nil
	end

	local ok_f, f = pcall(io.open, "/dev/urandom", "rb")
	if ok_f and f ~= nil then
		local ok_r, data = pcall(function ()
			return f:read(n)
		end)
		pcall(function ()
			f:close()
		end)
		if ok_r and type(data) == "string" and #data == n then
			return data
		end
	end

	return nil
end


local function _rand_nonce(n)
	n = n or M.NONCE_LEN
	if W ~= nil and type(W.random_nonce) == "function" then
		local out = W.random_nonce(n)
		if type(out) == "string" and #out == n then
			return out
		end
	end

	local ur = _csprng_bytes(n)
	if type(ur) == "string" and #ur == n then
		return ur
	end

	-- 弱回退: 仅当无系统熵源时
	local parts = {}
	local seed = tostring(os.time()) .. tostring(os.clock()) .. tostring({})
	local dig = M.sha256(seed)
	while #table.concat(parts) < n do
		dig = M.sha256(dig .. tostring(#parts))
		parts[#parts + 1] = dig
	end
	return string.sub(table.concat(parts), 1, n)
end


M.random_nonce = _rand_nonce
M.csprng_bytes = _csprng_bytes


-- @brief 新建重放窗口
-- @param [in]  window[number|nil]		窗口大小 (默认 64)
-- @return [table]						窗口状态
M.new_replay = function (window)
	window = tonumber(window) or 64
	if 8 > window then
		window = 8
	end

	return { window = window, max = nil, seen = {} }
end


-- @brief 校验并记录序号
-- @param [in]  st[table]					`new_replay` 结果
-- @param [in]  seq[number]				帧序号
-- @return accepted[boolean]				false 表示重放或过旧
M.seq_accept = function (st, seq)
	if "table" ~= type(st) then
		return false
	end

	seq = tonumber(seq)
	if nil == seq then
		return false
	end

	seq = math.floor(seq) % 0x100000000

	local win = st.window
	if nil == st.max then
		st.max = seq
		st.seen[seq % win] = seq
		return true
	end

	if seq > st.max then
		local diff = seq - st.max
		if diff >= win then
			st.seen = {}
		else
			for i = 1, diff do
				st.seen[(st.max + i) % win] = nil
			end
		end

		st.max = seq
		st.seen[seq % win] = seq
		return true
	end

	if win <= st.max - seq then
		return false
	end

	if st.seen[seq % win] == seq then
		return false
	end

	st.seen[seq % win] = seq
	return true
end


-- @brief 密封标签 (EtM; 密钥派生 MAC 键)
local function _aead_tag(key, seq, ct)
	local mac_key = M.sha256("alua-tunnel-aead-mac|" .. tostring(key))
	local dig = M.sha256(mac_key .. "|" .. _u32be(seq) .. "|" .. (ct or ""))
	return string.sub(dig, 1, M.TAG_LEN)
end


-- @brief 从 opts/auth 取 PSK; 无则 nil (明文模式)
M.psk_from_opts = function (opts)
	if type(opts) ~= "table" then
		return nil
	end

	if type(opts.key) == "string" and opts.key ~= "" then
		return opts.key
	end

	if type(opts.auth) == "table" then
		if type(opts.auth.key) == "string" and opts.auth.key ~= "" then
			return opts.auth.key
		end
		local user = tostring(opts.auth.user or "")
		local pass = tostring(opts.auth.pass or "")
		local token = opts.auth.token
		if type(token) == "string" and token ~= "" then
			return M.sha256("alua-tunnel-psk|token|" .. token)
		end
		if user ~= "" or pass ~= "" then
			return M.sha256("alua-tunnel-psk|user|" .. user .. "|" .. pass)
		end
	end

	return nil
end


-- @brief 会话键
M.derive_session_key = function (psk, nonce_c, nonce_s)
	if W ~= nil and type(W.derive_session_key) == "function" then
		return W.derive_session_key(tostring(psk), tostring(nonce_c), tostring(nonce_s))
	end
	return M.sha256("alua-tunnel-sess|" .. tostring(psk) .. "|" .. tostring(nonce_c) .. "|" .. tostring(nonce_s))
end


local function _keystream_block(key, seq, block_i)
	return M.sha256(key .. "|" .. _u32be(seq) .. "|" .. _u32be(block_i))
end


local function _xor_stream(key, seq, plain)
	local out = {}
	local i = 1
	local bi = 0
	while i <= #plain do
		local ks = _keystream_block(key, seq, bi)
		bi = bi + 1
		for j = 1, #ks do
			if i > #plain then
				break
			end
			out[#out + 1] = string.char(string.byte(plain, i) ~ string.byte(ks, j))
			i = i + 1
		end
	end
	return table.concat(out)
end


-- @brief 密封 AEAD: seq(4) + ciphertext + tag(16)
-- @note 仅当 ktunnel 声明 aead 时走 C; 否则统一 Lua EtM (与旧纯 CTR 不互通)
M.seal = function (key, seq, plain)
	if W ~= nil and W.aead == true and type(W.seal) == "function" then
		local out = W.seal(key, seq, plain or "")
		if type(out) == "string" then
			return out
		end
	end

	plain = plain or ""
	local ct = _xor_stream(key, seq, plain)
	local tag = _aead_tag(key, seq, ct)
	return _u32be(seq) .. ct .. tag
end


M.open = function (key, blob)
	if W ~= nil and W.aead == true and type(W.open) == "function" then
		local plain, seq_or_err = W.open(key, blob)
		if type(plain) == "string" then
			return plain, seq_or_err
		end
		return nil, seq_or_err or "open fail"
	end

	if type(blob) ~= "string" or #blob < 4 + M.TAG_LEN then
		return nil, "short seal"
	end
	local seq = string.byte(blob, 1) * 0x1000000
		+ string.byte(blob, 2) * 0x10000
		+ string.byte(blob, 3) * 0x100
		+ string.byte(blob, 4)
	local body = string.sub(blob, 5)
	local ct = string.sub(body, 1, #body - M.TAG_LEN)
	local tag = string.sub(body, #body - M.TAG_LEN + 1)
	local expect = _aead_tag(key, seq, ct)
	if tag ~= expect then
		return nil, "aead tag fail"
	end
	return _xor_stream(key, seq, ct), seq
end


-- @brief 编码带 crypto 的 HELLO payload
M.encode_hello_crypto = function (token, nonce_c)
	if W ~= nil and type(W.encode_hello_crypto) == "function" then
		local payload, nc = W.encode_hello_crypto(token or "", nonce_c)
		if type(payload) == "string" then
			return payload, nc
		end
		return nil, nc or "encode fail"
	end

	token = token or ""
	nonce_c = nonce_c or _rand_nonce(M.NONCE_LEN)
	if #nonce_c ~= M.NONCE_LEN then
		return nil, "bad nonce"
	end
	return _byte(M.FLAG_CRYPTO) .. _u16be(#token) .. token .. nonce_c, nonce_c
end


-- @brief 解析 HELLO payload
-- @return [table] `{ crypto, token, nonce_c }`
M.parse_hello_payload = function (payload)
	if W ~= nil and type(W.parse_hello_payload) == "function" then
		local t, err = W.parse_hello_payload(payload or "")
		if type(t) == "table" then
			return t
		end
		return nil, err or "short hello crypto"
	end

	payload = payload or ""
	if #payload >= 1 + 2 + M.NONCE_LEN and string.byte(payload, 1) == M.FLAG_CRYPTO then
		local tlen = string.byte(payload, 2) * 256 + string.byte(payload, 3)
		if #payload < 3 + tlen + M.NONCE_LEN then
			return nil, "short hello crypto"
		end
		return {
			crypto = true,
			token = string.sub(payload, 4, 3 + tlen),
			nonce_c = string.sub(payload, 4 + tlen, 3 + tlen + M.NONCE_LEN),
		}
	end

	return { crypto = false, token = payload, nonce_c = nil }
end


-- @brief HELLO_OK crypto payload: flags + nonce_s
M.encode_hello_ok_crypto = function (nonce_s)
	if W ~= nil and type(W.encode_hello_ok_crypto) == "function" then
		local payload, ns = W.encode_hello_ok_crypto(nonce_s)
		if type(payload) == "string" then
			return payload, ns
		end
		return nil, ns or "encode fail"
	end

	nonce_s = nonce_s or _rand_nonce(M.NONCE_LEN)
	if #nonce_s ~= M.NONCE_LEN then
		return nil, "bad nonce"
	end
	return _byte(M.FLAG_CRYPTO) .. nonce_s, nonce_s
end


-- @brief 解析 HELLO_OK payload
M.parse_hello_ok_payload = function (payload)
	if W ~= nil and type(W.parse_hello_ok_payload) == "function" then
		return W.parse_hello_ok_payload(payload or "")
	end

	payload = payload or ""
	if #payload >= 1 + M.NONCE_LEN and string.byte(payload, 1) == M.FLAG_CRYPTO then
		return {
			crypto = true,
			nonce_s = string.sub(payload, 2, 1 + M.NONCE_LEN),
		}
	end

	return { crypto = false, nonce_s = nil }
end


local function _fatal_recv_err(err)
	if err == nil or err == "timeout" then
		return false
	end
	return true
end


-- @brief 在 framed 端与 raw 端之间中继 (代理侧; 需 tcp:recv 支持 timeout)
-- @note client 侧为 DATA 帧密封; upstream 为裸 TCP
-- @param [in]  opts[table|nil]	`{ coop[boolean] }` 协作式 yield (入口并发)
M.relay_framed_raw = function (framed, raw, key, opts)
	if framed == nil or raw == nil or type(key) ~= "string" then
		return false, "bad args"
	end

	local coop = false
	if type(opts) == "table" and opts.coop == true then
		coop = true
	end

	local kco = nil
	if coop then
		local ok_c, mod = pcall(require, "kco")
		if not ok_c or type(mod) ~= "table" or type(mod.co_sleep) ~= "function" then
			return false, "kco.co_sleep unavailable"
		end
		kco = mod
	end

	local seq_s = 0
	local fbuf = ""
	local replay = M.new_replay()
	local poll_ms = coop and 0 or 40
	local idle_rounds = 0
	local busy_rounds = 0
	local idle_max = coop and 10000 or 2500

	while true do
		local progressed = false

		local chunk_f, err_f = kproxy_io.recv(framed, 4096, poll_ms)
		if type(chunk_f) == "string" and #chunk_f > 0 then
			fbuf = fbuf .. chunk_f
			progressed = true
			idle_rounds = 0
		elseif _fatal_recv_err(err_f) then
			break
		end

		while true do
			local ok_f, msg, rest = frame.try_decode(fbuf)
			if not ok_f then
				if msg ~= "short" then
					return false, tostring(msg or "frame fail")
				end
				break
			end

			fbuf = rest or ""
			if msg.type ~= M.T_DATA and msg.type ~= frame.T_DATA then
				return false, "unexpected frame"
			end

			local plain, seq_o = M.open(key, msg.payload)
			if plain == nil then
				return false, seq_o or "open fail"
			end

			if not M.seq_accept(replay, seq_o) then
				return false, "replay"
			end

			if #plain > 0 then
				local ok_s = kproxy_io.send(raw, plain)
				if not ok_s then
					return false, "raw send fail"
				end
			end
			progressed = true
		end

		local chunk_r, err_r = kproxy_io.recv(raw, 4096, poll_ms)
		if type(chunk_r) == "string" and #chunk_r > 0 then
			local sealed = M.seal(key, seq_s, chunk_r)
			seq_s = (seq_s + 1) % 0x100000000
			local wire, err_e = frame.encode(frame.T_DATA, sealed)
			if wire == nil then
				return false, err_e or "encode fail"
			end
			local ok_s = kproxy_io.send(framed, wire)
			if not ok_s then
				return false, "framed send fail"
			end
			progressed = true
			idle_rounds = 0
		elseif _fatal_recv_err(err_r) then
			break
		end

		if progressed then
			idle_rounds = 0
			if coop then
				busy_rounds = busy_rounds + 1
				if busy_rounds >= 8 then
					busy_rounds = 0
					kco.co_sleep(1)
				end
			end
		else
			busy_rounds = 0
			idle_rounds = idle_rounds + 1
			-- 两端持续 timeout: 保持轮询; 避免无 timeout 支持时忙等
			if idle_rounds > idle_max then
				break
			end

			if coop then
				kco.co_sleep(10)
			end
		end
	end

	return true
end


-- @brief 包装 tcp 为密封 DATA 收发 (客户端 hop 出口)
-- @note conn.tcp 指向自身, 供 chain 多跳复用密封流
M.wrap_conn = function (tcp, key)
	local seq_s = 0
	local rbuf = ""
	local data_leftover = ""
	local replay = M.new_replay()
	local conn = {
		kind = "alua-tunnel-crypto",
		key = key,
		_raw = tcp,
	}

	conn.close = function ()
		if conn._raw ~= nil and type(conn._raw.close) == "function" then
			pcall(function ()
				conn._raw:close()
			end)
		end
		conn._raw = nil
		conn.tcp = nil
		data_leftover = ""
		rbuf = ""
	end

	conn.send = function (_, data)
		if conn._raw == nil then
			return false, "closed"
		end
		data = data or ""
		local sealed = M.seal(key, seq_s, data)
		seq_s = (seq_s + 1) % 0x100000000
		local wire, err = frame.encode(frame.T_DATA, sealed)
		if wire == nil then
			return false, err or "encode fail"
		end
		return kproxy_io.send(conn._raw, wire)
	end

	conn.recv = function (_, max_n, timeout_ms)
		if conn._raw == nil then
			return nil, "closed"
		end

		if #data_leftover > 0 then
			local out = data_leftover
			if max_n ~= nil and #out > max_n then
				data_leftover = string.sub(out, max_n + 1)
				return string.sub(out, 1, max_n)
			end
			data_leftover = ""
			return out
		end

		while true do
			local ok_f, msg, rest = frame.try_decode(rbuf)
			if ok_f then
				rbuf = rest or ""
				if msg.type ~= M.T_DATA and msg.type ~= frame.T_DATA then
					return nil, "unexpected frame"
				end
				local plain, seq_o = M.open(key, msg.payload)
				if plain == nil then
					return nil, seq_o
				end

				if not M.seq_accept(replay, seq_o) then
					return nil, "replay"
				end
				if max_n ~= nil and #plain > max_n then
					data_leftover = string.sub(plain, max_n + 1)
					return string.sub(plain, 1, max_n)
				end
				return plain
			end

			if msg ~= "short" then
				return nil, msg
			end

			local chunk, err_r = kproxy_io.recv(conn._raw, 4096, timeout_ms)
			if type(chunk) == "string" and #chunk > 0 then
				rbuf = rbuf .. chunk
			else
				return nil, err_r or "eof"
			end
		end
	end

	-- chain 复用密封流: tcp 字段指向包装本身
	conn.tcp = conn
	return conn
end


return M

-- end
