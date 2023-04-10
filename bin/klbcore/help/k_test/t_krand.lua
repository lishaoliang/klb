
local krand = require("krand")
local ksys = require("ksys")



print('krand.rand()', krand.rand())
print('krand.rand(100)', krand.rand(100))
print('krand.rand(1000)', krand.rand(1000))


print('krand.rand_string()', krand.rand_string())
print('krand.rand_string(8)', krand.rand_string(8))
print('krand.rand_string(16)', krand.rand_string(16))


print('krand.rand_integer()', krand.rand_integer())
print('krand.rand_integer(8)', krand.rand_integer(8))
print('krand.rand_integer(16)', krand.rand_integer(16))

for i = 1, 8 do
	print(krand.rand_string(i))
end

for i = 8, 1, -1 do
	print(krand.rand_string(i))
end


print('====>>>> ksys.exit()')
ksys.exit()
