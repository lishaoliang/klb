--[[
-- Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   kstore.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  C kstore, 数据仓路径与 ID 规则
--   \n require("kstore")
--   \n C导出文件: ./klb/src_packages/klbstore/store_klua/klbstore_klua.c
--   \n 仅做 ID 校验与路径推导; 不读写文件
-- @version 0.1
--]]

local kstore = {}


-- @brief 是否可用 (编译期已启用本包)
-- @return [boolean] 恒为 true
kstore.available = function ()
	return true
end


-- @brief 校验 ID 是否合法
-- @param [in] id[string]	数据 ID
-- @return [boolean] true.合法; false.非法
kstore.valid_id = function (id)
	return false
end


-- @brief ID 转仓储相对路径
-- @param [in] id[string]	数据 ID
-- @return [string]	相对路径
-- @return nil, code[number(int)]	失败; code 见 klbcode.base
-- @note 路径缓冲上限 512 (KLB_STORE_PATH_MAX), 超长按 E_ARG
kstore.rel_path = function (id)
	return ''
end


-- @brief ID 转仓储绝对路径
-- @param [in] root[string]	数据根目录 (无尾斜杠)
-- @param [in] id[string]		数据 ID
-- @return [string]	绝对路径
-- @return nil, code[number(int)]	失败; code 见 klbcode.base
-- @note root 长度 >= 512 直接 base.INVAL
kstore.abs_path = function (root, id)
	return ''
end


-- 错误码真源: require("klbcore.klbcode").base (同 C klb_basecode_e)
-- 常用: OK/INVAL/NOENT/IO/NOSPC/BUSY/NOTIMPL/ACCES/RANGE


return kstore
