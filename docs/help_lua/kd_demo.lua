local kd_demo = {}


-- @brief hello
-- @return  [string]	�ַ���
--  \n 		[boolean]	true;false
kd_demo.hello = function ()
	return '', true
end

-- @brief test
-- @param [in]  	a1[string]		�ַ���
-- @param [in]  	a2[string]		�ַ���
-- @return  [string]	�ַ���
--  \n 		[string]	�ַ���
kd_demo.test = function (a1, a2)
	return a1, a2
end


return kd_demo
