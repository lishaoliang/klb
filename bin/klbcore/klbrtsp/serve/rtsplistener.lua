--[[
-- Copyright (c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   rtsplistener.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  RTSP服务 监听
-- @history 修改历史
--		[2025-10] 添加基础监听
--]]
local krtsp = require("krtsp")
local rtspserver = require("klbcore.klbrtsp.serve.rtspserver")


--------------------------------------------------------------------------------------------
-- 前置定义
local E = {}


--------------------------------------------------------------------------------------------
-- 内部实现


--------------------------------------------------------------------------------------------
-- rtsplisten 对外接口

local rtsplisten = {}

-- @brief 关闭监听
function rtsplisten:close()
	if self._listen then
		self._listen:close()
		self._listen = nil
	end
end

-- @brief 是否已成功 open
-- @return ok[boolean]
function rtsplisten:is_open()
	return nil ~= self._listen
end

-- @brief 开始监听端口
-- @param [in]      port[number(int)]	端口
-- @param [in]      opts[table]			[可选] host/family/max_connect/reuseport/nodelay/keepalive
-- @return ok[boolean]					true 成功; 失败为 nil
-- @return code[number]					失败时 klb_netcode_e
function rtsplisten:open(port, opts)
	-- 关闭
	self:close()

	-- 开启监听
	local listen, code = krtsp.listen(port, opts)
	if not listen then
		self._last_code = code
		return nil, code
	end

	self._listen = listen
	self._last_code = 0
	return true
end

function rtsplisten:port()
	if not self._listen or not self._listen.port then
		return 0
	end
	return self._listen:port()
end

function rtsplisten:local_addr()
	if not self._listen or not self._listen.local_addr then
		return nil, nil
	end
	return self._listen:local_addr()
end

function rtsplisten:native_fd()
	if not self._listen or not self._listen.native_fd then
		return nil
	end
	return self._listen:native_fd()
end

-- @brief 组装 new_serve 的 TLS 选项(cert/key[/tls])
local BuildServeOpts = function (cfg)
	if 'table' ~= type(cfg) then
		return nil
	end

	local opts = nil
	if 'string' == type(cfg.cert) and 'string' == type(cfg.key) then
		opts = {
			cert = cfg.cert,
			key = cfg.key,
		}
		if nil ~= cfg.tls then
			opts.tls = cfg.tls
		end
	elseif true == cfg.tls then
		opts = { tls = true }
	end

	return opts
end

-- @brief 接收 新连接
-- @return [table]	服务连接模块; 无连接或退出时为 nil
-- @return [string|number]	成功为 "ok"; 失败/退出为 C/new_serve code 或 "exit"
-- @note 须在 kco 协程内; rtsps 须在 cfg 带 cert/key(透传给 krtsp.new_serve)
-- @note 契约形状与 C 侧 krtsp listen:co_accept 对齐: 均回 (对象, "ok") / (nil, code)
function rtsplisten:co_accept()
	if not self._listen then
		return nil
	end

	-- C/Lua 协程里面, 暂未找到直接 返回userdata的方法
	-- 这里 先返回指针, 再通过接口函数新建
	local ptr, msg = self._listen:co_accept()

	-- close/退出: C 侧可能推 lightuserdata(NULL)+"exit"; NULL lightuserdata 不是 Lua nil
	if 'exit' == msg then
		return nil, msg
	end

	local conn, code = krtsp.new_serve(ptr, BuildServeOpts(self._cfg))
	if not conn then
		return nil, code
	end

	return rtspserver.new(conn, self._cfg), 'ok'
end


--------------------------------------------------------------------------------------------
-- rtsplistener

local rtsplistener = {}


-- @brief 新建一个监听模块
rtsplistener.new = function (cfg)
	local obj = {
		_listen = nil,				-- C/C++ 提供的监听模块
		_cfg = cfg,					-- 会话: h264/user/password/realm; TLS: cert/key[/tls]
		_last_code = 0,				-- 最近一次 open 失败码
	}

	setmetatable(obj, {
		__index = rtsplisten,
		__tostring = function(self)
			return self._listen
		end
	})

	return obj
end


return rtsplistener
