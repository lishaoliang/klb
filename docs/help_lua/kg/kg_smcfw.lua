--[[
-- Copyright(c) 2020, LGPL All Rights Reserved
-- @brief   ��ý��ͻ���
--   \n kg stream media client framework
-- @author  ������
-- @version 0.1
--]]

local kg_smcfw = {}


-- @brief ��ʼ��ģ��
-- @param [in] cfg[string]	   json��ʽ���ò���
-- @return [boolean] true.�ɹ�; false.ʧ��
--   \n [string] error  �����ַ���
kg_smcfw.init = function (cfg)
	return true, ''
	--return false, 'error'
end


-- @brief �˳�ģ��
-- @return ��
kg_smcfw.quit = function ()

end


-- @brief ����ģ��: ���������ű�
-- @param [in] name[string]	   �߳�����
-- @param [in] entry[string]   ������ڽű�; eg.'xxx.lua'
-- @param [in] preload[string] Ԥ���ؿ⺯������; �����̴߳�����ʹ����ЩԤ���ؿ�
-- @param [in] loader[string]  Lua�ű����غ���; ���Զ��庯������Lua�ű�; Ĭ��ʹ��fopen���ļ�����
-- @return [boolean] true.�ɹ�; false.ʧ��
--   \n [string] error  �����ַ���
-- @note see kg_thread.create
kg_smcfw.start = function (name, entry, preload, loader)
	return true, ''
	--return false, 'error'
end


-- @brief ֹͣģ��
-- @return ��
kg_smcfw.stop = function ()

end


-- @brief ���������Ӽ�������: ��������, ����ʧ��
-- @param [in] newconn[function]    ������
-- @return ��
-- @note ���պ���ԭ��: 
-- local newconn = function (name, protocol, ok, err)
--    return 0
-- end
-- @param [in] name[string]	    ��������
-- @param [in] protocol[string] Э������
-- @param [in] ok[boolean]      true.�ɹ�; false.ʧ��
-- @param [in] err[string]      �����ַ���
-- @return 0
kg_smcfw.set_newconn = function (newconn)

end


-- @brief ����txt���ݽ��մ�����
-- @param [in] receiver[function]    ���ݽ��մ�����
-- @return ��
-- @note ���պ���ԭ��: 
-- local receiver = function (name, protocol, sequence, uid, extra, txt)
--    return 0
-- end
-- @param [in] name[string]	    ��������
-- @param [in] protocol[string] Э������
-- @param [in] sequence[number] ���к�: [0,2^32)
-- @param [in] uid[number]      UID: [0,2^32)
-- @param [in] extra[string]    ��������(��ͷ����)
-- @param [in] txt[string]      ��ʽ�ı�����
-- @return 0
kg_smcfw.set_receiver = function (receiver)

end


-- @brief ���������쳣�ȶϿ�������
-- @param [in] disconnect[function]   �쳣�ȶϿ�������
-- @return ��
-- @note ������ԭ��: 
-- local disconnect = function (name, protocol, err)
--    return 0
-- end
-- @param [in] name[string]	    ��������
-- @param [in] protocol[string] Э������
-- @param [in] err[string]      �����ַ���
-- @return 0
kg_smcfw.set_disconnect = function (disconnect)

end


-- @brief ��������
-- @param [in] name[string]	    ��������
-- @param [in] protocol[string] Э������
-- @param [in] network[string]  ��������: 'tcp'
-- @param [in] address[string]  �����ַ: ':8000'
-- @return ��
kg_smcfw.dial = function (name, protocol, network, address)

end


-- @brief �����ı�����
-- @param [in] name[string]	    ��������
-- @param [in] protocol[string] Э������
-- @param [in] sequence[number] ���к�: [0,2^32)
-- @param [in] uid[number]      UID: [0,2^32)
-- @param [in] extra[string]    ��������(��ͷ����)
-- @param [in] txt[string]      ��ʽ�ı�����
-- @return [boolean] true.�ɹ�; false.ʧ��
kg_smcfw.send_txt = function (name, protocol, sequence, uid, extra, txt)
	return true
	--return false
end

return kg_smcfw
