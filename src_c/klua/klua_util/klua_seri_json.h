#ifndef __KLUA_SERIALIZE_JSON_H__
#define __KLUA_SERIALIZE_JSON_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include <lua.h>
#include "klbthird/cJSON.h"


cJSON* luaseri_json_pack(lua_State *L, int base_idx);
int luaseri_json_unpack(lua_State *L, int base_idx, cJSON* p_array);


#ifdef __cplusplus
}
#endif

#endif
