///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_obj.h
/// @brief   基础obj定义
/// @version 0.1
/// @history 修改历史
///  \n 2019 0.1 创建文件
///  \n 2022 定义绑定obj的操作函数等
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_OBJ_H__
#define __KLB_OBJ_H__

#include "klb_type.h"
#include "klbutil/klb_hash.h"
#include "klbutil/klb_map.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// 
typedef struct klb_obj_t_ klb_obj_t;


/// @brief 销毁对象函数
/// @param [in] *p_obj  待销毁的对象
/// @return 无
typedef void(*klb_obj_destroy_cb)(klb_obj_t* p_obj);


/// @brief 控制操作函数
/// @param [in] *p_obj      klb_obj_t* 对象
/// @param [in] *p_in_map   输入参数集
/// @param [in] *p_out_map  输出参数集
/// @return int 0.成功; 非0.失败
typedef int(*klb_obj_ctrl_cb)(klb_obj_t* p_obj, const klb_map_t* p_in_map, klb_map_t* p_out_map);


/// @brief 从输入map参数, 转换成本obj的结构体参数
/// @param [in] *p_obj      klb_obj_t* 对象
/// @param [in] *p_in_map   输入参数集
/// @return int 0.成功; 非0.失败
typedef int(*klb_obj_set_cb)(klb_obj_t* p_obj, const klb_map_t* p_in_map);


/// @brief 导出成map格式
/// @param [in] *p_obj      klb_obj_t* 对象
/// @param [in] *p_out_map  输出参数集
/// @return int 0.成功; 非0.失败
typedef int(*klb_obj_get_cb)(klb_obj_t* p_obj, klb_map_t* p_out_map);


#pragma pack(4)


typedef struct klb_obj_ops_t_
{
    const char* p_module;                   ///< 模块名
    const char* p_name;                     ///< 名称
    int         size;                       ///< 大小

    klb_obj_destroy_cb      cb_destroy;     ///< 销毁
    klb_obj_ctrl_cb         cb_ctrl;        ///< 常规操作

    klb_obj_set_cb          cb_set;         ///< 从输入map转换成本obj(struct)
    klb_obj_get_cb          cb_get;         ///< 导出成map格式
}klb_obj_ops_t;


/// @struct klb_obj_t
/// @brief  klb基础object
/// @note 若需要抽象化处理, 才使用object
typedef struct klb_obj_t_
{
    uint32_t                module;         ///< 模块名称(32位哈希值): 函数klb_hash32()
    uint32_t                name;           ///< 对象名称(32位哈希值): 函数klb_hash32()

    char*                   p_udata;        ///< user data
    char*                   p_ex;           ///< 扩展指针

    klb_obj_ops_t           ops;            ///< 操作函数表

    char                    ex[];           ///< 可变长,附加对象
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
