--[[
-- Copyright(c) 2022, LGPL All Rights Reserved
-- @file   kkpa.lua
-- @brief  C kkpa, package
--   \n require("kkpa")
--   \n C导出文件: ./klb/src_c/klua/klua_base/klua_kpackage.c
--   \n 打包/解包文件
-- @version 0.1
--]]
local kkpa = {}



-- @brief 以写入方式打包文件
-- @param [in] path[string]			路径: eg. '/aaa/bbb'
-- @return kkpa对象
function kkpa.open_w(path)
	local kpa = {}
	
	-- @brief 关闭
	-- @return 无
	-- @note 显示关闭, 可提前释放非Lua相关的资源(内存, 文件句柄等)
	--		不显示关闭, 则需要等待gc才释放	
	kpa:close = function ()
		
	end

	-- @brief 按K:V方式写入数据
	-- @param [in]	k[string]		key
	-- @param [in]	v[string]		value
	-- @return [number(int)] 0.成功; 非0.错误码
	kpa:write = function (k, v)
		return 0
	end

	-- @brief 按K:File方式写入数据
	-- @param [in]	k[string]		key
	-- @param [in]	path[string]	写入文件路径
	-- @return [number(int)] 0.成功; 非0.错误码
	kpa:write_file = function (k, path)
		return 0
	end

	return kpa
end


-- @brief 以读取方式打包文件
-- @param [in] path[string]			路径: eg. '/aaa/bbb'
-- @return kkpa对象
function kkpa.open_r()
	local kpa = {}
	
	-- @brief 关闭
	-- @return 无
	-- @note 显示关闭, 可提前释放非Lua相关的资源(内存, 文件句柄等)
	--		不显示关闭, 则需要等待gc才释放
	kpa:close = function ()
		
	end
	
	-- @brief 读取包中的文件数目
	-- @return [number(int)] 文件数目
	kpa:size = function ()
		return 0
	end

	-- @brief 按序号读取包中的数据
	-- @param [in]	idx[number(int)]	序号, 范围[1, kpa:size()]
	-- @return [nil,string] key
	--         [nil,string] value
	kpa:read = function (idx)
		local k = '' -- or nil
		local v = '' -- or nil
		return k, v
	end
	
	return kpa
end


return kkpa

