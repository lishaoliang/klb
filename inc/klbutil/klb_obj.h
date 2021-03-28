///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/05/19
//
/// @file    klb_obj.h
/// @brief   基础obj定义
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
///  \n 2019/05/19 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_OBJ_H__
#define __KLB_OBJ_H__

#include "klb_type.h"
#include "klbutil/klb_hash.h"

#if defined(__cplusplus)
extern "C" {
#endif


#pragma pack(4)

/// @brief 控制操作函数
/// @param [in] *p_obj      klb_obj_t* 对象
/// @param [in] lparam      整型参数lparam
/// @param [in] wparam      整型参数wparam
/// @param [in out] *p_param 指针型参数: 具体用途由调用双方约定
/// @return int 0.成功; 非0.失败
typedef int(*klb_obj_ctrl_cb)(void* p_obj, int lparam, int wparam, void* p_lparam);


/// @brief 销毁对象函数
/// @param [in] *p_obj  待销毁的对象
/// @return 无
typedef void(*klb_obj_destroy_cb)(void* p_obj);


/// @struct klb_obj_t
/// @brief  klb基础对象
/// @note 注意32, 64位系统对齐
typedef struct klb_obj_t_
{
    uint32_t                module;         ///< 模块名称(32位哈希值): 函数klb_hash32()
    uint32_t                name;           ///< 对象名称(32位哈希值): 函数klb_hash32()

    klb_obj_destroy_cb      cb_destroy;     ///< 销毁对象函数
    klb_obj_ctrl_cb         cb_ctrl;        ///< 控制操作函数

    char                    obj[1];         ///< 可变长,附加对象
}klb_obj_t;

#pragma pack()


/// @def   KLB_OBJ_DESTROY
/// @brief 销毁基础对象
#define KLB_OBJ_DESTROY(KLB_OBJ_)   {if(KLB_OBJ_){klb_obj_destroy_cb cb_destroy=((klb_obj_t*)(KLB_OBJ_))->cb_destroy;cb_destroy(KLB_OBJ_);(KLB_OBJ_)=NULL;}}


/// @def   klb_obj_hash
/// @brief obj hash
#define klb_obj_hash                klb_hash32


#ifdef __cplusplus
}
#endif

#endif // __KLB_OBJ_H__
//end
