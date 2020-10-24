--[[
-- Copyright (c) 2019, LGPL All Rights Reserved
-- @file  demo.lua
-- @author  李绍良
--]]


-- @file 不要引用此文件
assert(false)

-- @note Lua内置类型名称:
-- \n number
-- \n string
-- \n boolean
-- \n function
-- \n table
-- \n userdata(lightuserdata)


-- \n number(int)


--[[
-- Copyright(c) 2019, LGPL All Rights Reserved
-- @file    xxx.lua
-- @brief   文件头注释模板
-- @author  xxx
-- @version 0.1
-- @history 修改历史
--  \n 2019/8/10 0.1 创建文件
-- @warning 没有警告
--]]

-- @module	 name
-- @brief   模块定义
-- @export  标明模块导出变量, 可以被外部访问
-- @variable 标明模块运行时内置变量, 不可被外部访问
-- @note 定义形如同 "M.xxx = xxx",一律为导出变量,函数或其他
--  \n 定义形如 "local xxx = xxx",一律为内部数据
--  \n 定义形如 "local M = {a = 'aa',b = false} ... return M", 一律 M为导出, a,b为内部数据


-- @name   M.name
-- @export 模块导出变量


-- @brief 函数描述
-- @param [in]      xxx[string]	输入参数
-- @param [out]     xxx[table]	输出参数
-- @param [in,out]  xxx[table]	输入输出参数
-- @param [in]      x_xx[function] 如果有回调函数, 必须标明在see中标明原型
-- @return [nil, table] [boolean] 几个返回值集合
-- @note 注意事项,参见xxx
-- @see [string][table] = x_xx(string, string, table)


-- @global 全局变量,仅支持 G, setup, init, quit
--  \n 第二行描述
-- @note 注意事项,参见GetQueryError()
-- @see GetQueryError()


-- @variable 局部变量描述
--  \n 第二行描述
-- @note 注意事项,参见GetQueryError()
-- @see GetQueryError()


-- @def   name
-- @brief 常量定义, 无特殊情况不可修改

