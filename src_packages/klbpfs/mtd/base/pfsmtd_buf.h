// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfsmtd_buf.h
/// @brief   简易对齐缓冲
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    仅分配/释放内存; 不含块设备 I/O
///  \n 同步自 portfs base/pfs_buf.*; 返回值数值对齐 pfs_ret_e (0 / -12 ENOMEM / -16 EBUSY / -22 EINVAL)
///////////////////////////////////////////////////////////////////////////
#ifndef _PFSMTD_BUF_H_
#define _PFSMTD_BUF_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// @def   PFSMTD_BUF_ALIGN_4K
/// @brief 数据区对齐粒度 [单位:字节]
#define PFSMTD_BUF_ALIGN_4K                 4096u

/// @brief 简易对齐缓冲 (有效区 [start, end) 相对 p_data)
typedef struct _pfsmtd_buf_t {
    int                 buf_len;                          ///< [单位:字节] 缓冲容量
    int                 start;                            ///< [单位:字节] 有效数据起始偏移 (相对 p_data)
    int                 end;                              ///< [单位:字节] 有效数据结束偏移 (相对 p_data, 不含)
    int                 size;                             ///< [单位:字节] 有效数据大小
    uint8_t             *p_data;                          ///< 对齐数据区
} pfsmtd_buf_t;

/// @brief 分配对齐数据区
/// @param [in, out] *p_buf    缓冲对象
/// @param [in] buf_len        请求容量; 向上取整到 PFSMTD_BUF_ALIGN_4K
/// @return int 0; -EINVAL / -EBUSY / -ENOMEM
int pfsmtd_buf_init(pfsmtd_buf_t *p_buf, int buf_len);

/// @brief 释放数据区
/// @param [in, out] *p_buf    缓冲对象
/// @return int 0; -EINVAL
int pfsmtd_buf_deinit(pfsmtd_buf_t *p_buf);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFSMTD_BUF_H_

// end
