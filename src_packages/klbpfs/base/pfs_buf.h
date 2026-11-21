// Doc Encode : UTF-8 BOM, Unix(LF)

///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_buf.h
/// @brief   缓冲块
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_BUF_H_
#define _PFS_BUF_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// 简易对齐缓冲 (仅分配/释放内存; 不含块设备 I/O)
/// 返回值数值对齐 pfs_ret_e (0 / -12 ENOMEM / -16 EBUSY / -22 EINVAL)

#define PFS_BUF_ALIGN_4K                    4096u     ///< [单位:字节] 数据区对齐粒度

typedef struct _pfs_buf_t {
    int                 buf_len;                          ///< [单位:字节] 缓冲容量
    int                 start;                            ///< [单位:字节] 有效数据起始偏移 (相对 p_data)
    int                 end;                              ///< [单位:字节] 有效数据结束偏移 (相对 p_data, 不含)
    int                 size;                             ///< [单位:字节] 有效数据大小
    uint8_t             *p_data;                          ///< 对齐数据区
} pfs_buf_t;

/// 分配对齐数据区; buf_len 向上取整到 PFS_BUF_ALIGN_4K; 成功 0; -EINVAL / -EBUSY / -ENOMEM
int pfs_buf_init(pfs_buf_t *p_buf, int buf_len);

/// 释放数据区; 成功 0; -EINVAL
int pfs_buf_deinit(pfs_buf_t *p_buf);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_BUF_H_

// end
