--[[
-- Copyright(c) 2019, All Rights Reserved
-- Created: 2019/8/10
--
-- @file    kg_rand.lua
-- @brief   go rand
-- @author  ������
-- @version 0.1
-- @history �޸���ʷ
--  \n 2019/8/11 0.1 �����ļ�
-- @warning û�о���
--]]

local kg_rand = {}


-- @brief ��ȡ����ַ���,�ַ���[A-Za-z0-9]
-- @param [in]  	len[number(int)]	[��ѡ,Ĭ��8]�ַ�������
-- @return [string] ����ַ���
--  \n α���, �������Ҫ�󲻸�
kg_rand.str = function (len)
	return '1Aa'
	--return ''
end

-- @brief ��ȡ�����ֵ�ַ���,�ַ���[0-9]
-- @param [in]  	len[number(int)]	[��ѡ,Ĭ��8]��ֵ�ַ�������
-- @return [string] ����ַ���
--  \n 2λ������, ��λ��Ϊ"0"
--  \n α���, �������Ҫ�󲻸�
kg_rand.str_num = function (len)
	return '123456'
	--return ''
end


return kg_rand
