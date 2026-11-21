--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   netcode.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  脚本层网络错码册 (明面导出 C klb_netcode_e 短键)
-- @note   真源: klb/inc/klbnet/klb_netcode.h; 数值须与 C 枚举保持一致
--   \n 只做常量的集中导出, 不依赖 C 模块; 描述串仍用 knetcode.strerror(code)
--   \n socket 段 [0, 64) 为 klb_socket_status_e 别名, 与 C 同名短键一并导出
--   \n 另含 HTTP status [100,599] / WS close [1000,4999] 协议码别名
--   \n 段序按数值: 负段 / socket / HTTP / WS / 私有(0x10000+2000..)
--   \n require("klbcore.klbcode.netcode"); 门面 require("klbcore.klbcode")
-- @stability experimental
-- @impl evolving
-- @history 修改历史
--]]


local netcode = {}

-- 私有段区间
netcode.PRIVATE_BASE		= 0x10000 + 2000	-- 私有码基址
netcode.PRIVATE_MAX			= 0x10000 + 2000 + 1999	-- 私有码上限 (含)

-- 负段: -errno
netcode.ERR					= -1			-- 泛化失败 (EPERM)
netcode.NOENT				= -2			-- 不存在 (ENOENT)
netcode.INTR				= -4			-- 被信号中断 (EINTR)
netcode.IO					= -5			-- I/O 失败 (EIO)
netcode.BADF				= -9			-- 坏的 fd (EBADF)
netcode.AGAIN				= -11			-- 需重试 (EAGAIN)
netcode.NOMEM				= -12			-- 资源不足 (ENOMEM)
netcode.ACCES				= -13			-- 权限不足 (EACCES)
netcode.BUSY				= -16			-- 忙/不可用 (EBUSY)
netcode.EXIST				= -17			-- 已存在 (EEXIST)
netcode.INVAL				= -22			-- 参数非法 (EINVAL)
netcode.PIPE				= -32			-- 写对端已关 (EPIPE)
netcode.MSGSIZE				= -90			-- 报文过大 (EMSGSIZE)
netcode.NOTSUP				= -95			-- 不支持 (EOPNOTSUPP)
netcode.AFNOSUPPORT			= -97			-- 地址族不支持 (EAFNOSUPPORT)
netcode.ADDRINUSE			= -98			-- 地址已占用 (EADDRINUSE)
netcode.ADDRNOTAVAIL		= -99			-- 地址不可用 (EADDRNOTAVAIL)
netcode.NETUNREACH			= -101			-- 网络不可达 (ENETUNREACH)
netcode.CONNABORTED			= -103			-- 连接中止 (ECONNABORTED)
netcode.CONNRESET			= -104			-- 连接复位 (ECONNRESET)
netcode.NOBUFS				= -105			-- 无缓冲空间 (ENOBUFS)
netcode.ISCONN				= -106			-- 已连接 (EISCONN)
netcode.NOTCONN				= -107			-- 未连接 (ENOTCONN)
netcode.TIMEDOUT			= -110			-- 操作超时 (ETIMEDOUT)
netcode.CONNREFUSED			= -111			-- 连接被拒绝 (ECONNREFUSED)
netcode.HOSTUNREACH			= -113			-- 主机不可达 (EHOSTUNREACH)
netcode.ALREADY				= -114			-- 操作已在进行 (EALREADY)
netcode.INPROGRESS			= -115			-- 操作进行中 (EINPROGRESS)
netcode.ECANCELED			= -125			-- 操作被取消 (ECANCELED)

-- socket 段 [0, 64) (别名, 数值同 klb_socket_status_e)
netcode.OK					= 0				-- 正常
netcode.DISCONNECT			= 1				-- 连接断开(读写socket错误引发)
netcode.TIMEOUT				= 2				-- 超时
netcode.CLOSED				= 3				-- 连接已关闭
netcode.ERR_PROTOCOL		= 4				-- 协议错误
netcode.ERR_RESET			= 5				-- 对端复位 (RST 等)
netcode.ERR_REFUSED			= 6				-- 连接被拒绝
netcode.ERR_UNREACH			= 7				-- 网络/主机不可达
netcode.ERR_ABORT			= 8				-- 连接中止 (ECONNABORTED 等)
netcode.ERR_PIPE			= 9				-- 写时对端已关 (EPIPE 等)
netcode.ERR_INVAL			= 10			-- 参数或状态非法
netcode.ERR_ACCESS			= 11			-- 权限/访问被拒
netcode.ERR_MAX				= 12			-- 错误最大值 (含以上 ERR_*)
netcode.CLOSEING			= 62			-- 主动关闭
netcode.CONNECT				= 63			-- 连接成功
netcode.SOCKET_MAX			= 64			-- socket 段上限: 2^6

-- H HTTP status [100, 599]
netcode.HTTP_CONTINUE			= 100			-- Continue
netcode.HTTP_SWITCHING_PROTOCOLS			= 101			-- Switching Protocols
netcode.HTTP_PROCESSING			= 102			-- Processing
netcode.HTTP_OK			= 200			-- OK
netcode.HTTP_CREATED			= 201			-- Created
netcode.HTTP_ACCEPTED			= 202			-- Accepted
netcode.HTTP_NON_AUTHORITATIVE_INFORMATION			= 203			-- Non-Authoritative Information
netcode.HTTP_NO_CONTENT			= 204			-- No Content
netcode.HTTP_RESET_CONTENT			= 205			-- Reset Content
netcode.HTTP_PARTIAL_CONTENT			= 206			-- Partial Content
netcode.HTTP_MULTI_STATUS			= 207			-- Multi-Status
netcode.HTTP_ALREADY_REPORTED			= 208			-- Already Reported
netcode.HTTP_IM_USED			= 226			-- IM Used
netcode.HTTP_MULTIPLE_CHOICES			= 300			-- Multiple Choices
netcode.HTTP_MOVED_PERMANENTLY			= 301			-- Moved Permanently
netcode.HTTP_FOUND			= 302			-- Found
netcode.HTTP_SEE_OTHER			= 303			-- See Other
netcode.HTTP_NOT_MODIFIED			= 304			-- Not Modified
netcode.HTTP_USE_PROXY			= 305			-- Use Proxy
netcode.HTTP_TEMPORARY_REDIRECT			= 307			-- Temporary Redirect
netcode.HTTP_PERMANENT_REDIRECT			= 308			-- Permanent Redirect
netcode.HTTP_BAD_REQUEST			= 400			-- Bad Request
netcode.HTTP_UNAUTHORIZED			= 401			-- Unauthorized
netcode.HTTP_PAYMENT_REQUIRED			= 402			-- Payment Required
netcode.HTTP_FORBIDDEN			= 403			-- Forbidden
netcode.HTTP_NOT_FOUND			= 404			-- Not Found
netcode.HTTP_METHOD_NOT_ALLOWED			= 405			-- Method Not Allowed
netcode.HTTP_NOT_ACCEPTABLE			= 406			-- Not Acceptable
netcode.HTTP_PROXY_AUTHENTICATION_REQUIRED			= 407			-- Proxy Authentication Required
netcode.HTTP_REQUEST_TIMEOUT			= 408			-- Request Timeout
netcode.HTTP_CONFLICT			= 409			-- Conflict
netcode.HTTP_GONE			= 410			-- Gone
netcode.HTTP_LENGTH_REQUIRED			= 411			-- Length Required
netcode.HTTP_PRECONDITION_FAILED			= 412			-- Precondition Failed
netcode.HTTP_PAYLOAD_TOO_LARGE			= 413			-- Payload Too Large
netcode.HTTP_URI_TOO_LONG			= 414			-- URI Too Long
netcode.HTTP_UNSUPPORTED_MEDIA_TYPE			= 415			-- Unsupported Media Type
netcode.HTTP_RANGE_NOT_SATISFIABLE			= 416			-- Range Not Satisfiable
netcode.HTTP_EXPECTATION_FAILED			= 417			-- Expectation Failed
netcode.HTTP_MISDIRECTED_REQUEST			= 421			-- Misdirected Request
netcode.HTTP_UNPROCESSABLE_ENTITY			= 422			-- Unprocessable Entity
netcode.HTTP_LOCKED			= 423			-- Locked
netcode.HTTP_FAILED_DEPENDENCY			= 424			-- Failed Dependency
netcode.HTTP_UPGRADE_REQUIRED			= 426			-- Upgrade Required
netcode.HTTP_PRECONDITION_REQUIRED			= 428			-- Precondition Required
netcode.HTTP_TOO_MANY_REQUESTS			= 429			-- Too Many Requests
netcode.HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE			= 431			-- Request Header Fields Too Large
netcode.HTTP_UNAVAILABLE_FOR_LEGAL_REASONS			= 451			-- Unavailable For Legal Reasons
netcode.HTTP_INTERNAL_SERVER_ERROR			= 500			-- Internal Server Error
netcode.HTTP_NOT_IMPLEMENTED			= 501			-- Not Implemented
netcode.HTTP_BAD_GATEWAY			= 502			-- Bad Gateway
netcode.HTTP_SERVICE_UNAVAILABLE			= 503			-- Service Unavailable
netcode.HTTP_GATEWAY_TIMEOUT			= 504			-- Gateway Timeout
netcode.HTTP_VERSION_NOT_SUPPORTED			= 505			-- HTTP Version Not Supported
netcode.HTTP_VARIANT_ALSO_NEGOTIATES			= 506			-- Variant Also Negotiates
netcode.HTTP_INSUFFICIENT_STORAGE			= 507			-- Insufficient Storage
netcode.HTTP_LOOP_DETECTED			= 508			-- Loop Detected
netcode.HTTP_NOT_EXTENDED			= 510			-- Not Extended
netcode.HTTP_NETWORK_AUTHENTICATION_REQUIRED			= 511			-- Network Authentication Required

-- W WebSocket close [1000, 4999]
netcode.WS_NORMAL			= 1000			-- Normal Closure
netcode.WS_GOING_AWAY			= 1001			-- Going Away
netcode.WS_PROTO			= 1002			-- Protocol Error
netcode.WS_UNSUPPORTED			= 1003			-- Unsupported Data
netcode.WS_NOSTATUS			= 1005			-- No Status Rcvd (保留)
netcode.WS_ABNORMAL			= 1006			-- Abnormal Closure (保留)
netcode.WS_UTF8			= 1007			-- Invalid frame payload data
netcode.WS_POLICY			= 1008			-- Policy Violation
netcode.WS_TOOBIG			= 1009			-- Message Too Big
netcode.WS_MANDATORY_EXT			= 1010			-- Mandatory Ext.
netcode.WS_INTERNAL			= 1011			-- Internal Server Error
netcode.WS_SERVICE_RESTART			= 1012			-- Service Restart
netcode.WS_TRY_AGAIN			= 1013			-- Try Again Later
netcode.WS_BAD_GATEWAY			= 1014			-- Bad Gateway
netcode.WS_TLS_HANDSHAKE			= 1015			-- TLS handshake (保留)

-- A 流控 [BASE+0, BASE+31]
netcode.WBUF_EMPTY			= 67536		-- 写缓存空
netcode.RBUF_FULL			= 67537		-- 读缓存满
netcode.WBUF_FULL			= 67538		-- 写缓存满

-- B 连接 [BASE+32, BASE+63]
netcode.CONNECT_FAIL		= 67568		-- 建连失败 (泛化)
netcode.CONNECT_REFUSED		= 67569		-- 对端拒绝连接
netcode.DNS_FAIL			= 67570		-- 主机名解析失败
netcode.CONNECT_TIMEOUT		= 67571		-- 建连超时
netcode.CONNECT_HOST		= 67572		-- 主机可达性失败 (非 DNS)

-- C TLS [BASE+64, BASE+95]
netcode.TLS_HANDSHAKE		= 67600		-- TLS 握手失败
netcode.TLS_CERT			= 67601		-- TLS 证书校验失败
netcode.TLS_WANT_IO			= 67602		-- TLS 非阻塞握手未完成
netcode.TLS_ALERT			= 67603		-- TLS 对端 alert 关连
netcode.TLS_VERIFY			= 67604		-- TLS 主机名/SNI 校验失败

-- D listen / mux [BASE+96, BASE+127]
netcode.BIND_FAIL			= 67632		-- bind 失败
netcode.LISTEN_FAIL			= 67633		-- listen 失败
netcode.ACCEPT_FAIL			= 67634		-- accept 失败
netcode.QUEUE_OVERFLOW		= 67635		-- 分发/挂起队列溢出
netcode.PROBE_FAIL			= 67636		-- 协议嗅探失败
netcode.ADDR_IN_USE			= 67637		-- 地址已占用 (EADDRINUSE)
netcode.ADDR_NOT_AVAIL		= 67638		-- 地址不可用 (EADDRNOTAVAIL)

-- E 协议传输 [BASE+128, BASE+159]
netcode.PROTO_PARSE			= 67664		-- 协议帧解析错误
netcode.PROTO_TOO_LARGE		= 67665		-- 协议帧/体过大
netcode.PROTO_UNEXPECTED	= 67666		-- 非预期帧/状态机错误
netcode.PROTO_VERSION		= 67667		-- 协议版本不支持

-- G 会话 / 限额 [BASE+160, BASE+191]
netcode.CANCELED			= 67696		-- 本地取消/中止操作
netcode.LIMIT				= 67697		-- 连接数/速率等限额

-- 码值 → 短键名反查 (区间/段边界非错码; WBUF_EMPTY 与 PRIVATE_BASE 同值, 故跳过边界键)
local NAME = {}

local SKIP_NAME =
{
	PRIVATE_BASE	= true,
	PRIVATE_MAX		= true,
	ERR_MAX			= true,
	SOCKET_MAX		= true,
}

for k, v in pairs(netcode) do
	if "number" == type(v) and not SKIP_NAME[k] then
		NAME[v] = k
	end
end


-- @brief 错误码转短键名
-- @param [in]  code[number]	错误码
-- @return [string]			短键名; 未知为 "netcode-<n>"
netcode.str = function (code)
	local s = NAME[code]
	if nil ~= s then
		return s
	end
	return "netcode-" .. tostring(code)
end


return netcode

-- end
