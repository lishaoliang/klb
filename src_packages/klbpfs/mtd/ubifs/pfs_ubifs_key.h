// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_ubifs_key.h
/// @brief   ubifs 索引 key
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    对照 fs/ubifs/key.h; simple + R5
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_UBIFS_KEY_H_
#define _PFS_UBIFS_KEY_H_

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PFS_UBIFS_INO_KEY               0
#define PFS_UBIFS_DATA_KEY              1
#define PFS_UBIFS_DENT_KEY              2
#define PFS_UBIFS_S_KEY_BLOCK_BITS      29
#define PFS_UBIFS_S_KEY_BLOCK_MASK      0x1FFFFFFFu
#define PFS_UBIFS_S_KEY_HASH_MASK       0x1FFFFFFFu

/// @brief ubifs 索引 key (simple 格式)
typedef struct _pfs_ubifs_key_t {
    uint32_t                u[4];
} pfs_ubifs_key_t;

/// @brief 掩码 hash 并避开保留值
static inline uint32_t pfs_ubifs_key_mask_hash(uint32_t hash)
{
    hash &= PFS_UBIFS_S_KEY_HASH_MASK;
    if (hash <= 2u)
    {
        hash += 3u;
    }
    return hash;
}

/// @brief R5 字符串 hash
static inline uint32_t pfs_ubifs_key_r5_hash(const char *s, int len)
{
    uint32_t a = 0;
    const signed char *str = (const signed char *)s;

    while (len-- > 0)
    {
        a += (uint32_t)(*str << 4);
        a += (uint32_t)((unsigned char)(*str) >> 4);
        a *= 11u;
        str++;
    }
    return pfs_ubifs_key_mask_hash(a);
}

/// @brief 初始化 inode key
static inline void pfs_ubifs_ino_key_init(pfs_ubifs_key_t *key, uint64_t inum)
{
    memset(key, 0, sizeof(*key));
    key->u[0] = (uint32_t)inum;
    key->u[1] = (uint32_t)PFS_UBIFS_INO_KEY << PFS_UBIFS_S_KEY_BLOCK_BITS;
}

/// @brief 初始化 data key
static inline void pfs_ubifs_data_key_init(pfs_ubifs_key_t *key, uint64_t inum, uint32_t block)
{
    memset(key, 0, sizeof(*key));
    key->u[0] = (uint32_t)inum;
    key->u[1] = ((uint32_t)PFS_UBIFS_DATA_KEY << PFS_UBIFS_S_KEY_BLOCK_BITS) | (block & PFS_UBIFS_S_KEY_BLOCK_MASK);
}

/// @brief 初始化 dirent key
static inline void pfs_ubifs_dent_key_init(pfs_ubifs_key_t *key, uint64_t parent, const char *name, int nlen)
{
    memset(key, 0, sizeof(*key));
    key->u[0] = (uint32_t)parent;
    key->u[1] = ((uint32_t)PFS_UBIFS_DENT_KEY << PFS_UBIFS_S_KEY_BLOCK_BITS) | pfs_ubifs_key_r5_hash(name, nlen);
}

/// @brief 将 key 序列化到 flash 缓冲区
static inline void pfs_ubifs_key_write_flash(uint8_t *dst, const pfs_ubifs_key_t *key)
{
    dst[0] = (uint8_t)(key->u[0] & 0xffu);
    dst[1] = (uint8_t)((key->u[0] >> 8) & 0xffu);
    dst[2] = (uint8_t)((key->u[0] >> 16) & 0xffu);
    dst[3] = (uint8_t)((key->u[0] >> 24) & 0xffu);
    dst[4] = (uint8_t)(key->u[1] & 0xffu);
    dst[5] = (uint8_t)((key->u[1] >> 8) & 0xffu);
    dst[6] = (uint8_t)((key->u[1] >> 16) & 0xffu);
    dst[7] = (uint8_t)((key->u[1] >> 24) & 0xffu);
    memset(dst + 8, 0, 8);
}

/// @brief 从 flash 缓冲区反序列化 key
static inline void pfs_ubifs_key_read_flash(pfs_ubifs_key_t *key, const uint8_t *src)
{
    memset(key, 0, sizeof(*key));
    key->u[0] = (uint32_t)src[0] | ((uint32_t)src[1] << 8) | ((uint32_t)src[2] << 16) | ((uint32_t)src[3] << 24);
    key->u[1] = (uint32_t)src[4] | ((uint32_t)src[5] << 8) | ((uint32_t)src[6] << 16) | ((uint32_t)src[7] << 24);
}

#ifdef __cplusplus
}
#endif

#endif // _PFS_UBIFS_KEY_H_

// end
