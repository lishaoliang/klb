#ifndef __KLUA_H__
#define __KLUA_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"



int lua_main(int argc, char **argv);


#ifdef __cplusplus
}
#endif

#endif // __KLUA_H__
//end
