--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   const.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  ssid 共享常量 (base)
-- @stability internal
-- @impl evolving
-- @note   require("klbagent.core.ssid.base.const"); 非对外契约面
--   \n 原始 token 仅 ssid.serve 维护; LPC/XPC 客户端方法面分区见 LPC_METHODS / XPC_METHODS
-- @history 修改历史
--  \n [2026-11] 创建文件; 自 core.ssid 门面抽出
--  \n [2026-11] 增 LPC/XPC 方法名分区
--  \n [2026-11] C1 proof 前缀 / mid TTL / 多 token 槽 / list_tokens
--]]


local M = {}


-- LPC / XPC 模块名: XPC 缓冲上限见 klbxpc.NAME_BUF(108); 本包仍用短名常量;
--   \n unix 路径由 xpcpath 拼 /tmp/alua.ssid.xpc.unix.pxxxx, 非本常量
M.MODULE_NAME = "alua.ssid"

-- ALUA_SSID = digest12 + salt4 ([0-9a-f])
M.SSID_LEN = 16
M.DIGEST_LEN = 12
M.SALT_LEN = 4

-- 将过期提前续期窗口
M.RENEW_MARGIN_MS = 30 * 60 * 1000

-- C1 mid 缓存 TTL (服务端内存; 与日常临时票窗对齐)
M.MID_TTL_MS = 6 * 60 * 60 * 1000

-- 临时会话 token 文本前缀 (auth.issue); 无此前缀且非 proof 视为原始形态
M.TEMP_TOKEN_PREFIX = "t."

-- C1 证明票前缀 (ssid+raw 派生; 非 raw; 验过可再签发会话票)
M.PROOF_TOKEN_PREFIX = "p."

-- 默认 token 槽名 (ssid → 多 token)
M.DEFAULT_TOKEN_SLOT = "default"

-- 同进程 LPC 可注册的 RPC 方法 (可比 XPC 多 begin_ctx / verify_token / list_tokens)
M.LPC_METHODS = {
	"whoami", "ensure", "get_token", "renew", "destroy", "status",
	"register", "resolve", "verify_token", "begin_ctx", "list_tokens", "gc",
}

-- 跨进程 XPC 可注册的 RPC 方法 (无明文 reveal; 验票仅临时/证明票; gc 无密钥)
M.XPC_METHODS = {
	"whoami", "ensure", "get_token", "renew", "destroy", "status",
	"register", "resolve", "verify_token", "list_tokens", "gc",
}


return M

-- end
