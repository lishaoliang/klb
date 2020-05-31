--[[
-- Copyright(c) 2020, LGPL All Rights Reserved
-- @brief   ģ���豸Ӳ��
--   \n kg simulate
-- @author  ������
-- @version 0.1
--]]

local kg_simulate = {}


-- @brief ��ʼ��ģ��
-- @param [in] cfg[string]	   json��ʽ���ò���
-- @return [boolean] true.�ɹ�; false.ʧ��
--   \n [string] error  �����ַ���
kg_simulate.init = function (cfg)
	return true, ''
	--return false, 'error'
end


-- @brief �˳�ģ��
-- @return ��
kg_simulate.quit = function ()

end


-- @brief ����ģ��
-- @return [boolean] true.�ɹ�; false.ʧ��
--   \n [string] error  �����ַ���
kg_simulate.start = function ()
	return true, ''
	--return false, 'error'
end


-- @brief ֹͣģ��
-- @return ��
kg_simulate.stop = function ()

end


-- @brief ����ý�����ݽ��մ�����
-- @param [in] receiver[userdata(lightuserdata,function)]    ���ݽ��մ�����(C����)
-- @return ��
-- @note ����C����ԭ��: 
-- @brief �����ص���������
-- @param [in] chnn        ͨ��
-- @param [in] sidx        �����: klb_mnp_sidx_e
-- @param [in] *p_stream   ��������
-- @return int 0.�ɹ�; ��0.ʧ��
-- typedef int(*klb_mnp_stream_cb)(int chnn, int sidx, klb_mnp_stream_t* p_stream);
-- @see klb_mnp_stream.h
kg_simulate.set_stream_receiver = function (receiver)

end


-- @brief ��������: ���õ�
-- @param [in] cmd[string]      ��������
-- @param [in] chnn[number]     ͨ��
-- @param [in] sidx[number]     �����
-- @param [in] lparam[string]   ����1
-- @param [in] wparam[string]   ����2
-- @return [number] 0.�ɹ�; ��0.������
--   \n [string] error  �����ַ���
kg_simulate.command = function (cmd, chnn, sidx, lparam, wparam)
	return 0, ''
end

return kg_simulate
