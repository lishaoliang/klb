///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, MIT License
//  Created: 2019/05/26
//
/// @file    klb_buf.h
/// @brief   简易缓存
/// @author  李绍良
/// @version 0.1
/// @history 修改历史
///  \n 2019/05/26 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_BUF_H__
#define __KLB_BUF_H__

#include "klb_type.h"
#include "obj/klb_obj.h"

#if defined(__cplusplus)
extern "C" {
#endif

#pragma pack(4)


/// @struct klb_buf_t
/// @brief  简易缓存
typedef struct klb_buf_t_
{
    klb_obj_t   obj;            ///< 基础对象

    struct
    {
        char*   p_buf;          ///< 缓存指针
        int     buf_len;        ///< 缓存大小

        int     start;          ///< 有效数据起始位置
        int     end;            ///< 有效数据末尾
    };
}klb_buf_t;

#pragma pack()


#ifdef __cplusplus
}
#endif

#endif // __KLB_BUF_H__
//end
