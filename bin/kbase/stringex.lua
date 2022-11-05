local string = require("string")

local stringex = {}

stringex.url_encode = function (s)
	s = string.gsub(s, '([^%w%.%- ])', function(c) return string.format('%%%02X', string.byte(c)) end)
    return string.gsub(s, ' ', '+')
end

stringex.url_decode = function (s)
	s = string.gsub(s, '%%(%x%x)', function(h) return string.char(tonumber(h, 16)) end)
	return s
end

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
