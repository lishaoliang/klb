--[[
-- Copyright(c) 2019, All Rights Reserved
-- Created: 2019/8/10
--
-- @file    kg_rand.lua
-- @brief   go rand
-- @author  李绍良
-- @version 0.1
-- @history 修改历史
--  \n 2019/8/11 0.1 创建文件
-- @warning 没有警告
--]]

local kg_rand = {}


-- @brief 获取随机字符串,字符集[A-Za-z0-9]
-- @param [in]  	len[number(int)]	[可选,默认8]字符串长度
-- @return [string] 随机字符串
--  \n 伪随机, 对随机性要求不高
kg_rand.str = function (len)
	return '1Aa'
	--return ''
end

-- @brief 获取随机数值字符串,字符集[0-9]
-- @param [in]  	len[number(int)]	[可选,默认8]数值字符串长度
-- @return [string] 随机字符串
--  \n 2位数以上, 首位不为"0"
--  \n 伪随机, 对随机性要求不高
kg_rand.str_num = function (len)
	return '123456'
	--return ''
end


return kg_rand
