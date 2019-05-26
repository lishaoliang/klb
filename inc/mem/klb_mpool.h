///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, MIT License
//  Created: 2019/05/26
//
/// @file    klb_mpool.h
/// @brief   内存池
/// @author  李绍良
/// @version 0.1
/// @history 修改历史
///  \n 2019/05/26 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_MPOOL_H__
#define __KLB_MPOOL_H__

#include "klb_type.h"
#include "obj/klb_obj.h"

#if defined(__cplusplus)
extern "C" {
#endif

#pragma pack(4)

/// @struct klb_mpbuf_t
/// @brief  内存池缓存(mem pool buf)
typedef struct klb_mpbuf_t_
{
    klb_obj_t       obj;            ///< 基础对象

    struct
    {
        char*       p_buf;          ///< 缓存指针
        int         buf_len;        ///< 缓存大小

        int         start;          ///< 有效数据起始位置
        int         end;            ///< 有效数据末尾
    };

    struct
    {
        uint32_t    x;
        uint32_t    y;
        uint32_t    z;
        uint32_t    a;
    };
}klb_mpbuf_t;

#pragma pack()


#ifdef __cplusplus
}
#endif

#endif // __KLB_MPOOL_H__
//end
