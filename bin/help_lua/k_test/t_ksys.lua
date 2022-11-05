

local ksys = require("ksys")


-- step 1
local s1 = ksys.pack_string(1, 'abc', 3.1415926, true, {a=1,b=2})
print('s1', ksys.unpack(s1))


-- step 2
local s2 = ksys.pack_json(1, 'abc', 3.1415926, true, {a=1,b=2})
print('s2', s2)
print('s2 unpack json', ksys.unpack_json(s2))


-- step 3
local s3 = ksys.pack_string(ksys.get_arg())
print('s3', ksys.unpack(s3))


-- end
print('====>>>> ksys.exit()')
ksys.exit()
