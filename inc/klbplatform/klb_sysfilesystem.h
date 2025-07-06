#ifndef __KLB_SYSFILESYSTEM_H__
#define __KLB_SYSFILESYSTEM_H__


#include "klb_type.h"
#include "klbthird/sds.h"


#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 获取当前 主执行程序 的路径
/// @return sds 当前 主执行程序 的路径
KLB_API sds klb_get_base_path();


/// @brief 获取当前 应用 的配置文档路径
/// @return sds 路径
KLB_API sds klb_get_pref_path(const char* p_org, const char* p_app);


#if defined(__cplusplus)
}
#endif

#endif // __KLB_SYSFILESYSTEM_H__
//end
