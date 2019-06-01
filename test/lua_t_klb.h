#ifndef __LUA_T_KLB_H__
#define __LUA_T_KLB_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif

#include "lua.h"


/// @brief klb test函数
/// @param [in] *L  lua状态
/// @return int 1
KLB_API int luaopen_t_klb(lua_State* L);


#ifdef __cplusplus
}
#endif

#endif // __LUA_T_KLB_H__
//end
