--[[
-- Copyright(c) 2022, LGPL All Rights Reserved
-- @file   klist.lua
-- @brief  C klist
--   \n require("klist")
--   \n C导出文件: ./klb/src_c/klua/klua_multithread/klua_klist.c
--   \n 进程内部按链表格式共享的数据
--   \n 数据只能被取出一次, 放入的数据按klb_obj_t组织 
--   \n 适合重量级数据, 例如socket/媒体流等等
--   \n 适用于 A线程生产数据, B线程消费数据, 此类简易夸线程的 生产-消费 模型使用
-- @version 0.1
--]]

local klist = {}



-- @brief 放入C数据到尾部
-- @param [in] path[string]			路径: eg. '/aaa/bbb'
-- @param [in] cobj[lightuserdata]	C指针: klb_obj_t*
-- @return 无
-- @note 先进先出
klist.push = function (path, cobj)
	return
end


-- @brief 取出顶部C数据
-- @param [in] path[string]			路径: eg. '/aaa/bbb'
-- @return [lightuserdata] C指针(klb_obj_t*)
-- @note 先进先出
klist.pop = function (path)
	-- return cobj
	return 
end


-- @brief 创建一个klist对象
-- @param [in] path[string]			路径: eg. '/aaa/bbb'
-- @return klist对象
-- @note 先进先出
--  按命名路径来访问, 理论上创建的klist对象数目无限制
klist.new = function (path)
	local o

	-- @brief 放入C数据到尾部
	-- @param [in] cobj[lightuserdata]	C指针: klb_obj_t*
	-- @return 无
	o.push = function (cobj)
		return
	end
	
	-- @brief 取出头部C数据
	-- @return [lightuserdata] C指针(klb_obj_t*)
	o.pop = function ()
		-- return cobj
		return 
	end
	
	-- @brief 获取队列中数据的数目
	-- @return [number(int)] 数目
	o.size = function ()
		return 0
	end
	
	-- @brief 清空所有数据
	-- @return 无
	o.clear = function ()
		return
	end
	
	return o
end
