--[[
-- Copyright(c) 2019, All Rights Reserved
-- Created: 2019/8/10
--
-- @file    kg_thread.lua
-- @brief   go thread
-- @author  ������
-- @version 0.1
-- @history �޸���ʷ
--  \n 2019/8/11 0.1 �����ļ�
-- @warning û�о���
--]]

local kg_thread = {}


-- @brief �����߳�
-- @param [in]  	name[string]	�߳�����; ""��ʾ�������߳�
-- @param [in]		loader[string]	�������: require(loader)
-- @return [boolean] true.�ɹ�; false.ʧ��
kg_thread.create = function (name, loader)
	return true
	--return false
end


-- @brief �����߳�
-- @param [in]  	name[string]	�߳�����
-- @return ��
kg_thread.destroy = function (name)
	
end


-- @brief ��ĳ�߳�Post��Ϣ
-- @param [in]  	name[string]	�߳�����
-- @param [in]  	msg[string]		��Ϣ
-- @param [in]  	lparam[string]	����1
-- @param [in]  	wparam[string]	����2
-- @param [in]  	ptr[string]		ָ�����: void*
-- @return [boolean] true.�ɹ�; false.ʧ��
kg_thread.post = function (name, msg, lparam, wparam, ptr)
	return true
	--return false
end


return kg_thread
