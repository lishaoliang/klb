local kd_demo = {}


-- @brief hello
-- @return  [string]	×Ö·û´®
--  \n 		[boolean]	true;false
kd_demo.hello = function ()
	return '', true
end

-- @brief test
-- @param [in]  	a1[string]		×Ö·û´®
-- @param [in]  	a2[string]		×Ö·û´®
-- @return  [string]	×Ö·û´®
--  \n 		[string]	×Ö·û´®
kd_demo.test = function (a1, a2)
	return a1, a2
end


return kd_demo
