#ifndef LUA_SERIALIZE_H
#define LUA_SERIALIZE_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <lua.h>

int luaseri_pack(lua_State *L);
int luaseri_unpack(lua_State *L);

#ifdef __cplusplus
}
#endif

#endif
