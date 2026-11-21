--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   basecode.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  脚本层基础错码册 (明面导出 C klb_basecode_e 短键)
-- @stability stable
-- @impl mature
-- @attention
--   \n @stability stable: AI 禁止擅自添加/删除/变更本文件对外接口 (含导出名, 参数, 返回与已文档含义); 须用户点名
--   \n @impl mature: 实现倾向冻结; 重构须同等谨慎, 不得破坏本文件导出签名与可观察语义
-- @note   真源: klb/inc/klbbase/klb_basecode.h; 数值须与 C 枚举保持一致
--   \n 只做常量的集中导出, 不依赖 C 模块; 描述串用 kbasecode.strerror(code)
--   \n 与 alua core/code.lua 无关: 本表为 klbbase C 码镜像, 非 alua 业务域码
--   \n require("klbcore.klbcode.basecode"); 门面 require("klbcore.klbcode")
-- @history 修改历史
--]]


local basecode = {}


-- 私有段区间 [0x10000, 0x10000+1999] (与 netcode [0x10000+2000, +3999] 分区间)
basecode.PRIVATE_BASE		= 0x10000		-- 私有码基址
basecode.PRIVATE_MAX		= 0x10000 + 1999	-- 私有码上限 (含)

-- 成功
basecode.OK					= 0				-- 正常

-- 负段: -errno (与 netcode 负段同名同值; 另含 NOSPC)
basecode.ERR				= -1			-- 泛化失败 (EPERM)
basecode.NOENT				= -2			-- 不存在 (ENOENT)
basecode.INTR				= -4			-- 被信号中断 (EINTR)
basecode.IO					= -5			-- I/O 失败 (EIO)
basecode.BADF				= -9			-- 坏的 fd (EBADF)
basecode.AGAIN				= -11			-- 需重试 (EAGAIN)
basecode.NOMEM				= -12			-- 资源不足 (ENOMEM)
basecode.ACCES				= -13			-- 权限不足 (EACCES)
basecode.BUSY				= -16			-- 忙/不可用 (EBUSY)
basecode.EXIST				= -17			-- 已存在 (EEXIST)
basecode.INVAL				= -22			-- 参数非法 (EINVAL)
basecode.NOSPC				= -28			-- 空间不足 (ENOSPC)
basecode.PIPE				= -32			-- 写对端已关 (EPIPE)
basecode.MSGSIZE			= -90			-- 报文过大 (EMSGSIZE)
basecode.NOTSUP				= -95			-- 不支持 (EOPNOTSUPP)
basecode.AFNOSUPPORT		= -97			-- 地址族不支持 (EAFNOSUPPORT)
basecode.ADDRINUSE			= -98			-- 地址已占用 (EADDRINUSE)
basecode.ADDRNOTAVAIL		= -99			-- 地址不可用 (EADDRNOTAVAIL)
basecode.NETUNREACH			= -101			-- 网络不可达 (ENETUNREACH)
basecode.CONNABORTED		= -103			-- 连接中止 (ECONNABORTED)
basecode.CONNRESET			= -104			-- 连接复位 (ECONNRESET)
basecode.NOBUFS				= -105			-- 无缓冲空间 (ENOBUFS)
basecode.ISCONN				= -106			-- 已连接 (EISCONN)
basecode.NOTCONN			= -107			-- 未连接 (ENOTCONN)
basecode.TIMEDOUT			= -110			-- 操作超时 (ETIMEDOUT)
basecode.CONNREFUSED		= -111			-- 连接被拒绝 (ECONNREFUSED)
basecode.HOSTUNREACH		= -113			-- 主机不可达 (EHOSTUNREACH)
basecode.ALREADY			= -114			-- 操作已在进行 (EALREADY)
basecode.INPROGRESS			= -115			-- 操作进行中 (EINPROGRESS)
basecode.ECANCELED			= -125			-- 操作被取消 (ECANCELED)

-- A 通用 [BASE+0x00, BASE+0x1F]
basecode.STATE				= 65536		-- 状态非法
basecode.RANGE				= 65537		-- 区间/范围非法
basecode.OVERFLOW			= 65538		-- 超限/溢出
basecode.CANCELED			= 65539		-- 本地取消/中止操作
basecode.LIMIT				= 65540		-- 限额/配额
basecode.NOTIMPL			= 65541		-- 未实现

-- B string [BASE+0x20, BASE+0x3F]
basecode.STRING_TRUNC		= 65568		-- 字符串截断
basecode.STRING_EMPTY		= 65569		-- 空串/缺内容

-- C package [BASE+0x40, BASE+0x5F]
basecode.PACKAGE_BAD		= 65600		-- 包格式/键非法
basecode.PACKAGE_NOENT		= 65601		-- 包内条目不存在
basecode.PACKAGE_IO			= 65602		-- 包读写失败

-- D mnp/smp [BASE+0x60, BASE+0x7F]
basecode.MNP_PROTO			= 65632		-- MNP/SMP 协议错误
basecode.MNP_STATE			= 65633		-- MNP/SMP 状态非法
basecode.MNP_PEER			= 65634		-- MNP/SMP 对端错误

-- E codec/crypto [BASE+0x80, BASE+0x9F]
basecode.CODEC_PROTO		= 65664		-- 编解码协议/布局错误
basecode.CODEC_NOSUP		= 65665		-- 编解码不支持
basecode.CRYPTO_FAIL		= 65666		-- 加解密失败


-- 码值 → 短键名反查 (区间/段边界非错码; STATE 与 PRIVATE_BASE 同值, 故跳过边界键)
local NAME = {}

local SKIP_NAME =
{
	PRIVATE_BASE	= true,
	PRIVATE_MAX		= true,
}

for k, v in pairs(basecode) do
	if "number" == type(v) and not SKIP_NAME[k] then
		NAME[v] = k
	end
end


-- @brief 错误码转短键名
-- @param [in]  code[number]	错误码
-- @return [string]			短键名; 未知为 "basecode-<n>"
-- @stability stable
basecode.str = function (code)
	local s = NAME[code]
	if nil ~= s then
		return s
	end
	return "basecode-" .. tostring(code)
end


return basecode

-- end
