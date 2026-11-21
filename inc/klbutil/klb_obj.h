///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_obj.h
/// @brief   轻量可传递对象信封(跨线程交接等)
///  \n 实例含身份标签 + 销毁回调 + 可变长载荷; 供 klist 等传递重量级内容
/// @version 0.4
/// @history 修改历史
///  \n 2019 0.1 创建文件
///  \n 2026 简化klb_obj_t结构
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_OBJ_H__
#define __KLB_OBJ_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


///
typedef struct klb_obj_t_ klb_obj_t;


/// @brief 销毁对象函数
/// @param [in] *p_obj  待销毁的对象
/// @return 无
typedef void(*klb_obj_destroy_cb)(klb_obj_t* p_obj);


/// @struct klb_obj_t
/// @brief  轻量对象信封
/// @note 跨线程交接时 cb_destroy 必须非空; 载荷放在 extra[]
///  \n p_module/p_name 供调查/日志; 须指向静态或长寿命字符串, 可为 NULL
typedef struct klb_obj_t_
{
    const char*             p_module;       ///< 模块名(调试/身份, 可 NULL)
    const char*             p_name;         ///< 名称(调试/身份, 可 NULL)

    klb_obj_destroy_cb      cb_destroy;     ///< 销毁回调(必填)

    char                    extra[];        ///< 可变长载荷
}klb_obj_t;


/// @def   KLB_OBJ_DESTROY
/// @brief 销毁基础对象
#define KLB_OBJ_DESTROY(KLB_OBJ_)   {if(KLB_OBJ_){klb_obj_destroy_cb cb_destroy=((klb_obj_t*)(KLB_OBJ_))->cb_destroy;cb_destroy(KLB_OBJ_);(KLB_OBJ_)=NULL;}}


#ifdef __cplusplus
}
#endif

#endif // __KLB_OBJ_H__

// end
