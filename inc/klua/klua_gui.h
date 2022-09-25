#ifndef __KLUA_GUI_H__
#define __KLUA_GUI_H__

#include "klb_type.h"
#include "klua/klua_env.h"
#include "klbgui/klb_gui.h"

#if defined(__cplusplus)
extern "C" {
#endif


KLB_API klb_gui_t* klua_gui_get(klua_env_t* p_env);
KLB_API klb_gui_t* klua_gui_get_by_L(lua_State* L);


#ifdef __cplusplus
}
#endif

#endif // __KLUA_GUI_H__
//end
