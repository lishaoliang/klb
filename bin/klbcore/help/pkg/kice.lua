--[[
-- Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   kice.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  C kice, ICE agent
--   \n require("kice")
--   \n C导出文件: ./klb/src_packages/klbice/ice_klua/klbice_klua.c
--   \n 依赖当前 klua_env 的 netmulti; 无 netmulti 时 create 返回 nil, INVAL
-- @version 0.1
--]]

local kice = {}


-- @brief 创建 ICE agent
-- @param [in] role[number(int)]	[可选] 0.controlled(默认); 1.controlling
-- @return agent[userdata]	agent 对象
-- @return nil, code[number(int)]	失败; 描述串用 knetcode.strerror(code)
-- @note 对象方法: close / destroy / local_ufrag / local_pwd / local_count
--   \n           local_candidate / set_remote_credentials / add_remote_candidate
--   \n           gather / start / state / send / recv
kice.create = function (role)
	local agent = {}

	-- @brief 销毁 agent (幂等)
	-- @return 无
	-- @note destroy 为 close 别名; __gc / __close 亦调此
	agent.close = function (self)
		return
	end

	-- @brief 本端 ufrag
	-- @return [string]
	-- @return nil, code[number(int)]	失败
	agent.local_ufrag = function (self)
		return ''
	end

	-- @brief 本端 pwd
	-- @return [string]
	-- @return nil, code[number(int)]	失败
	agent.local_pwd = function (self)
		return ''
	end

	-- @brief 本地候选数
	-- @return [number(int)]
	-- @return nil, code[number(int)]	失败
	agent.local_count = function (self)
		return 0
	end

	-- @brief 取指定下标的本地候选行
	-- @param [in] index[number(int)]	下标 (从 0 起)
	-- @return [string]	candidate 行
	-- @return nil, code[number(int)]	失败
	agent.local_candidate = function (self, index)
		return ''
	end

	-- @brief 设置远端凭据
	-- @param [in] ufrag[string]	远端 ufrag
	-- @param [in] pwd[string]		远端 pwd
	-- @return [boolean] 成功
	-- @return nil, code[number(int)]	失败
	agent.set_remote_credentials = function (self, ufrag, pwd)
		return true
	end

	-- @brief 添加远端候选
	-- @param [in] candidate[string]	candidate 行
	-- @return [boolean] 成功
	-- @return nil, code[number(int)]	失败
	agent.add_remote_candidate = function (self, candidate)
		return true
	end

	-- @brief 收集本地候选
	-- @return [boolean] 成功
	-- @return nil, code[number(int)]	失败
	agent.gather = function (self)
		return true
	end

	-- @brief 启动连通性检查
	-- @return [boolean] 成功
	-- @return nil, code[number(int)]	失败
	agent.start = function (self)
		return true
	end

	-- @brief 当前状态
	-- @return state[number(int)]	0.new / 1.gathering / 2.checking / 3.connected
	--                           \n 4.failed / 5.closed
	-- @return nil, code[number(int)]	失败
	agent.state = function (self)
		return 0
	end

	-- @brief 发送应用数据 (走已选中的候选对)
	-- @param [in] data[string]	应用数据
	-- @return [boolean] 成功
	-- @return nil, code[number(int)]	失败
	agent.send = function (self, data)
		return true
	end

	-- @brief 接收应用数据 (非阻塞轮询)
	-- @param [in] cap[number(int)]	[可选] 最大字节数; 默认 2048; 越界回落 2048
	-- @return [string]	数据
	-- @return nil		暂无数据
	-- @return nil, code[number(int)]	失败
	agent.recv = function (self, cap)
		return nil
	end

	return agent
end


return kice
