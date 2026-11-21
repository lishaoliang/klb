// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_klua_blkio.h
/// @brief   kpfs 一次性 blkio 会话 (制盘/探测)
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    不登记 pfs_klua_ex 盘符表
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_KLUA_BLKIO_H_
#define _PFS_KLUA_BLKIO_H_

#include "pfs_blkio.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct pfs_klua_blkio_sess_t {
    pfs_blkio_t     blkio;
    int             open;
} pfs_klua_blkio_sess_t;

void pfs_klua_blkio_sess_init(pfs_klua_blkio_sess_t *p_sess);

void pfs_klua_blkio_sess_done(pfs_klua_blkio_sess_t *p_sess);

/// @return PFS_OK / PFS_EIO 等
int pfs_klua_blkio_sess_open_rdonly(pfs_klua_blkio_sess_t *p_sess, const char *path);

/// @return PFS_OK / PFS_EIO 等
int pfs_klua_blkio_sess_open_rdwr(pfs_klua_blkio_sess_t *p_sess, const char *path);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_KLUA_BLKIO_H_

// end
