

local kmcache = require("kmcache")
local ksys = require("ksys")




kmcache.set('1', 1, 'abc', {a=1, b=2})
kmcache.set('2', 2, 'efg', true, false, 3.14)
kmcache.set('3', 3, 'erd', false, 3.1415926)

kmcache.set('1', 1, 'egeg', 123456, {a=1,b=2})

print('kmcache.size', kmcache.size())


print('--> 1', kmcache.get('1'))
print('--> 2', kmcache.get('2'))
print('--> 3', kmcache.get('3'))


kmcache.clear()

print('kmcache.size', kmcache.size())


print('====>>>> ksys.exit()')
ksys.exit()
