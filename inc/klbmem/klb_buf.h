///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_buf.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   基础缓存
/// @version 0.2
/// @history 修改历史
///   \n [2019]    添加基础缓存
///   \n [2025-10] 调整buf结构; 添加内存池代理
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_BUF_H__
#define __KLB_BUF_H__

#include "klb_type.h"
#include <stdlib.h>

#if defined(__cplusplus)
extern "C" {
#endif

#pragma pack(4)


/// @struct klb_buf_type_e
/// @brief  缓存类型
typedef enum klb_buf_type_e_
{
    KLB_BUF_NORMAL      = 0,        ///< 常规缓存; 直接使用 malloc/free 分配释放
    KLB_BUF_EXTRA       = 1,        ///< 常规缓存; klb_buf_t + [data]分配在一起

    KLB_BUF_ATOM        = 10,       ///< 带原子变量(引用计数)的缓存
    KLB_BUF_FIX_POOL    = 11,       ///< 固定大小内存池分配: klb_fpool_t

    KLB_BUF_MAX         = 0x80,             ///< 内部 buffer 类型最大值
    KLB_BUF_USER        = KLB_BUF_MAX + 1,  ///< 用户扩展 buffer 类型
}klb_buf_type_e;


/// @struct klb_buf_format_e
/// @brief  数据类型与组织格式
typedef enum klb_buf_format_e_
{
    KLB_BUF_FMT_NORMAL  = 0,        ///< 常规非媒体组包格式
    KLB_BUF_FMT_SLICE   = 1,        ///< 媒体分片格式
    KLB_BUF_FMT_FRAME   = 2,        ///< 媒体完整帧

    KLB_BUF_FMT_MAX     = 0x7,      ///< max
}klb_buf_format_e;


/// @struct klb_buf_t
/// @brief  缓存
typedef struct klb_buf_t_
{
    // 缓存 基本信息
    struct
    {
        char*               p_buf;      ///< 缓存指针
        int                 buf_len;    ///< 缓存大小

        int                 start;      ///< 有效数据起始位置
        int                 end;        ///< 有效数据末尾

        struct klb_buf_t_*  p_next;     ///< 下一个节点
    };

    // 扩展buf结构
    struct
    {
        uint8_t             type;       ///< 缓存类型: klb_buf_type_e

        uint8_t             resv1;
        uint16_t            resv2;
    };

    union
    {
        uint32_t            udata32;    ///< 用户数据: user data 32
        struct
        {
            int32_t         format : 3; ///< 数据类型与组织格式: klb_buf_format_e
            int32_t         vtype : 8;  ///< 媒体数据帧类型: klb_mnp_vtype_e, format=KLB_BUF_FMT_SLICE/KLB_BUF_FMT_FRAME时有效
            int32_t         udata : 16; ///< 用户数据: user data
        };
    };

    union
    {
        uint64_t            udata64;    ///< 用户数据: user data 64
        struct
        {
            uint32_t        udata1;    ///< 用户数据: user data 1
            uint32_t        udata2;    ///< 用户数据: user data 2
        };
    };
}klb_buf_t;

#pragma pack()


/// @brief 初始化缓存: 类型KLB_BUF_EXTRA
/// @param [in] buf_len         目标缓存长度
/// @param [in] zero            是否初始化
/// @return klb_buf_t* p_buf
/// @note 直接通过 free 释放
KLB_API klb_buf_t* klb_buf_malloc(int buf_len, bool zero);


/// @brief 写数据
/// @return int 写入的数据长度
KLB_API int klb_buf_write(klb_buf_t* p_buf, const char* p_data, int data_len);


/// @brief 缓存数据长度
KLB_API int klb_buf_data_len(klb_buf_t* p_buf);


/// @brief 将有效数据拼接在一起
/// @param [in]  *p_head        缓存头指针
/// @return klb_buf_t* KLB_BUF_EXTRA类型缓存; [klb_buf_t][data...]
KLB_API klb_buf_t* klb_buf_join(klb_buf_t* p_head);


/// @brief 引用加一
/// @param [in]  *p_buf         目标缓存
/// @return int 
KLB_API int klb_buf_ref(klb_buf_t* p_buf);


/// @brief 引用减一
/// @param [in]  *p_buf         目标缓存
/// @return int 
KLB_API int klb_buf_unref(klb_buf_t* p_buf);


/// @brief 引用加一: 当前节点及后续节点
/// @param [in]  *p_buf         目标缓存
/// @return int 
KLB_API int klb_buf_ref_next(klb_buf_t* p_buf);


/// @brief 引用减一: 当前节点及后续节点
/// @param [in]  *p_buf         目标缓存
/// @return int 
KLB_API int klb_buf_unref_next(klb_buf_t* p_buf);


//////////////////////////////////////////////////////////////////////////


/// @brief 缓存池获取缓存的 函数指针
typedef klb_buf_t*(*klb_buf_malloc_cb)(void* p_pool, size_t size);


/// @struct klb_bufagent_t
/// @brief  内存池代理
typedef struct klb_bufagent_t_
{
    klb_buf_malloc_cb   cb_malloc;          ///< 申请内存
    void*               p_pool;             ///< 内存池指针
}klb_bufagent_t;


/// @brief 使用代理 申请内存
/// @return klb_buf_t* 内存指针
KLB_API klb_buf_t* klb_bufagent_malloc(klb_bufagent_t* p_bufagent, size_t size);



#ifdef __cplusplus
}
#endif

#endif // __KLB_BUF_H__
//end
