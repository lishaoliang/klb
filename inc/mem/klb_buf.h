///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/05/26
//
/// @file    klb_buf.h
/// @brief   简易缓存
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
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
/// @brief  缓存类型
typedef enum klb_buf_type_e_
{
    KLB_BUF_NORMAL      = 0,    ///< 常规缓存
}klb_buf_type_e;


/// @struct klb_buf_t
/// @brief  简易缓存
typedef struct klb_buf_t_
{
    char*   p_buf;          ///< 缓存指针
    int     buf_len;        ///< 缓存大小

    int     start;          ///< 有效数据起始位置
    int     end;            ///< 有效数据末尾

    int     type;           ///< 缓存类型: klb_buf_type_e

    struct klb_buf_t_* p_next;  ///< 下一个节点

    char    extra[];        ///< 附加数据
}klb_buf_t;

#pragma pack()


/// @brief 初始化缓存
/// @param [in] *p_buf        简易缓存
/// @param [in] buf_len       目标缓存长度
/// @param [in] *p_data       按数据源来初始化缓存; NULL表示不初始化
/// @param [in] data_len      数据源长度
/// @return klb_buf_t* p_buf
/// @note 只申请 p_buf->p_buf 的内存
klb_buf_t* klb_buf_init(klb_buf_t* p_buf, int buf_len, const char* p_data, int data_len);


/// @brief 退出缓存
/// @param [in] *p_buf        简易缓存
/// @return 无
/// @note 只释放 p_buf->p_buf 的内存
void klb_buf_quit(klb_buf_t* p_buf);


/// @def   KLB_BUF_INIT
/// @brief 初始化缓存
#define KLB_BUF_INIT(P_BUF_, BUF_LEN_, P_DATA_, DATA_LEN_)  klb_buf_init(P_BUF_, BUF_LEN_, P_DATA_, DATA_LEN_)


/// @def   KLB_BUF_QUIT
/// @brief 退出缓存
#define KLB_BUF_QUIT(P_BUF_)                                klb_buf_quit(P_BUF_)


#ifdef __cplusplus
}
#endif

#endif // __KLB_BUF_H__
//end
