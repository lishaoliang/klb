local string = require("string")

local stringex = {}


-- ȥ��ǰ��հ�
stringex.trim = function (s)
	return (string.gsub(s, '^%s*(.-)%s*$', '%1'))
end

-- ���Դ�Сдƥ��
stringex.cmp_ignore_case = function (s1, s2)
	return (string.lower(s1) == string.lower(s2))
end

-- ���Դ�Сд����
stringex.find_ignore_case = function (s1, s2)
	local low_s1 = string.lower(s1)
	local low_s2 = string.lower(s2)
	
	if nil ~= string.find(low_s1, low_s2) then
		return true
	end

	return false
end

return stringex
