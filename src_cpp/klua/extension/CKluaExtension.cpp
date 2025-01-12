// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/extension/klua_extension.h"
#include "klua/extension/CKluaExGui.hpp"


//////////////////////////////////////////////////////////////////////////


#if defined(__cplusplus)
extern "C" {
#endif

/// @brief (内部)注册标准 CPP扩展
/// @return int 0.成功; 非0. 失败
int klua_register_extension_std_cpp(klua_env_t* p_env)
{
    // cpp gui
    klb::CKluaExGui::RegisterGuiEx(p_env);

    return 0;
}

int klua_using_cpp_gui(klua_env_t* p_env)
{
    // init cpp gui
    klb::CKluaExGui::GetGuiEx(p_env);

    return 0;
}

#ifdef __cplusplus
}
#endif
