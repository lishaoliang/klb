--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbagent 包入口 (AI Agent CLI 契约库)
-- @stability stable
-- @impl mature
-- @attention
--   \n @stability stable: AI 禁止擅自添加/删除/变更本文件对外接口 (含导出名, 参数, 返回与已文档含义); 须用户点名
--   \n @impl mature: 实现倾向冻结; 重构须同等谨慎, 不得破坏本文件导出签名与可观察语义
-- @note   与 klbcore 并列; 可 require("klbcore.*"); 由 alua 加载; 非 lua test/demo
--   \n 根大分类: core/ code/ tools/ docs/ packages/; 错误码统一 require("klbagent.code")
--   \n 框架模块 require("klbagent.core.*")
-- @history 修改历史
--  \n [2026-09] 创建文件; 子目录分模块 (cli/envelope/... 为 */init.lua)
--  \n [2026-11] entry_serve / entry_cli 成功后挂 klbagent.serve / klbagent.cli
--  \n [2026-11] 包服务成功亦挂 klbagent.serve = packages.<name>
--]]
local version = require("klbagent.version")


local klbagent = {}


-- 未登记码的统一名 (与 klbagent.code 同值)
local UNKNOWN = "alua.UNKNOWN"


--------------------------------------------------------------------------------------------
-- 包入口


-- 脚本线版本: base.YYYYMM; 与 VERSION / alua -v 一致
klbagent.VERSION = version.string
klbagent.version = version


-- @brief Agent CLI 入口
-- @param [in]      ...					透传给 cli.entry (list/categories/call)
-- @return ok[boolean]
-- @return code[number]
-- @note 成功后挂 klbagent.cli; G 持 klbagent 即可防 GC
--   \n 整段 CLI 在 kco 内跑 (co_task)
--   \n 禁 CLI 调 ssid.new_serve; 常驻子命令只 fork 业务协程, 由 co_task 泵至 kenv.exit
klbagent.entry_cli = function (...)
	local co_task = require("klbagent.core.cli.co_task")

	return co_task.run(function (...)
		local cli = require("klbagent.core.cli")
		local ok, code = cli.entry(...)
		if not ok then
			return false, code
		end

		-- 挂载 cli; 供 alua_boot 经 G.klbagent 持有防 GC
		klbagent.cli = cli
		return true, code
	end, ...)
end


-- @brief Agent serve 侧入口 (alua --serve); 分辨 core.serve vs packages 注册服务
-- @param [in]      ...					首参=已安装包名 → packages.serve; 否则 core.serve (port/profile)
-- @return ok[boolean]
-- @return code[number]
-- @note 例: alua --serve → core.serve; alua --serve 12817; alua --serve proxy serve …
--   \n 成功后挂 klbagent.serve: core 为 serve 表; 包服务为 packages.<name> 模块表
klbagent.entry_serve = function (...)
	local pkg_serve = require("klbagent.packages.serve")
	local name, rest = pkg_serve.resolve(...)
	if name ~= nil then
		local ok, code = pkg_serve.entry(name, rest)
		if not ok then
			return false, code
		end

		-- 挂载包模块 (第三返回是 message 非 hold); 经 G.klbagent 防 GC
		klbagent.serve = require("klbagent.packages." .. name)
		return true, code
	end

	local serve = require("klbagent.core.serve")
	local ok, code = serve.entry(...)
	if not ok then
		return false, code
	end

	-- 挂载 serve (含 worker); 供 alua_boot 经 G.klbagent 持有防 GC
	klbagent.serve = serve
	return true, code
end


-- @brief 码名 (整个 klbagent 统一查码入口)
-- @param [in] code[number]				码值
-- @return name[string]					alua.<通用码> / alua.core.<域>.<码名> / alua.packages.<包名>.<码名> / alua.UNKNOWN
-- @stability stable
-- @note 转交 klbagent.code.strerror; 未登记为 alua.UNKNOWN
--   \n eg. klbagent.strerror(0) -> "alua.OK"
--   \n     klbagent.strerror(69846) -> "alua.core.cli.BAD_REQUEST"
--   \n     klbagent.strerror(69746) -> "alua.core.serve.BAD_ARGV"
--   \n     klbagent.strerror(9999) -> "alua.UNKNOWN"
klbagent.strerror = function (code)
	return require("klbagent.code").strerror(code)
end


return klbagent
