--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   alua_boot.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua 可执行程序入口 (仅加载 klbagent)
-- @stability stable
-- @impl mature
-- @attention
--   \n @stability stable: AI 禁止擅自添加/删除/变更本文件对外接口 (含导出名, 参数, 返回与已文档含义); 须用户点名
--   \n @impl mature: 实现倾向冻结; 重构须同等谨慎, 不得破坏本文件导出签名与可观察语义
-- @note   由 alua 强制 dofile; --serve|-s = 服务侧 (klbagent.entry_serve); 其余 = 客户端 CLI (klbagent.entry_cli); 默认 CLI
--   \n 所有入口一律走 klbenv.co_main_loop: CLI=co_task; serve/包/worker=自调或 run_host_loop
-- @history 修改历史
--  \n [2026-09] 创建文件
--  \n [2026-11] G 仅持 klbagent; cli/serve 挂在 klbagent.cli / klbagent.serve
--  \n [2026-11] CLI 全协程化; 禁 CLI 侧 ssid.new_serve
--]]
local kenv = require("kenv")
local klbagent = require("klbagent")


-- @brief 参数内是否含帮助标记
-- @param [in] args[table]
-- @return has[boolean]
local function _has_help_flag(args)
	for i = 1, #args do
		local a = args[i]
		if a == "-h" or a == "--help" or a == "help" then
			return true
		end
	end

	return false
end


-- step1. 取位置参数: get_args = prog + entry + user...
local packed = table.pack(kenv.get_args())
local user = {}
for i = 3, packed.n do
	user[#user + 1] = packed[i]
end

-- step2. 模式判定: --serve|-s = 服务侧 (唯一入口); 其余全是客户端 CLI (剥掉标记本身)
--   serve [port] [profile] 已废弃 (客户端); serve yes|no 与 serve -h 仍走 CLI
local is_serve = false
if user[1] == "--serve" or user[1] == "-s" then
	is_serve = true
	table.remove(user, 1)
end

-- step3. 交给对应入口; 服务侧带 -h 交 CLI 打 serve 帮助 (不静默起监听)
local ok, code
if is_serve and _has_help_flag(user) then
	ok, code = klbagent.entry_cli("serve", "-h")
elseif is_serve then
	ok, code = klbagent.entry_serve(table.unpack(user))
else
	ok, code = klbagent.entry_cli(table.unpack(user))
end

-- step4. 进程退出码 (CLI 常驻经 co_task/co_util; serve 侧自 klbenv.co_main_loop)
local exit_code = code or 2
os.exit(exit_code)
