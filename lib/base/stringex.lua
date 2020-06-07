local string = require("string")

local stringex = {}


-- 去除前后空白
stringex.trim = function (s)
	return (string.gsub(s, '^%s*(.-)%s*$', '%1'))
end

-- 忽略大小写匹配
stringex.cmp_ignore_case = function (s1, s2)
	return (string.lower(s1) == string.lower(s2))
end

-- 忽略大小写查找
stringex.find_ignore_case = function (s1, s2)
	local low_s1 = string.lower(s1)
	local low_s2 = string.lower(s2)
	
	if nil ~= string.find(low_s1, low_s2) then
		return true
	end

	return false
end

return stringex
