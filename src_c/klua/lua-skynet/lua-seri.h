#ifndef LUA_SERIALIZE_H
#define LUA_SERIALIZE_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <lua.h>

int luaseri_pack(lua_State *L);
int luaseri_unpack(lua_State *L);


//////////////////////////////////

int luaseri_pack_from(lua_State *L, int idx);
int luaseri_unpack_by_buffer(lua_State *L, int base_idx, char* p_data, int data_size);


#ifdef __cplusplus
}
#endif

#endif
