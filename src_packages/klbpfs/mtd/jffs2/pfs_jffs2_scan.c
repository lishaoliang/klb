// Doc Encode : UTF-8 BOM, Unix(LF)
// jffs2 介质扫描 (对照 fs/jffs2/scan.c 子集; 全镜像线性扫, 不依赖 erase_size)
#include "jffs2/pfs_jffs2_internal.h"
#include "base/pfsmtd_mem.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

static const uint32_t g_pfs_jffs2_crc32_table[256] =
{
    0x00000000u, 0x77073096u, 0xee0e612cu, 0x990951bau, 0x076dc419u,
    0x706af48fu, 0xe963a535u, 0x9e6495a3u, 0x0edb8832u, 0x79dcb8a4u,
    0xe0d5e91eu, 0x97d2d988u, 0x09b64c2bu, 0x7eb17cbdu, 0xe7b82d07u,
    0x90bf1d91u, 0x1db71064u, 0x6ab020f2u, 0xf3b97148u, 0x84be41deu,
    0x1adad47du, 0x6ddde4ebu, 0xf4d4b551u, 0x83d385c7u, 0x136c9856u,
    0x646ba8c0u, 0xfd62f97au, 0x8a65c9ecu, 0x14015c4fu, 0x63066cd9u,
    0xfa0f3d63u, 0x8d080df5u, 0x3b6e20c8u, 0x4c69105eu, 0xd56041e4u,
    0xa2677172u, 0x3c03e4d1u, 0x4b04d447u, 0xd20d85fdu, 0xa50ab56bu,
    0x35b5a8fau, 0x42b2986cu, 0xdbbbc9d6u, 0xacbcf940u, 0x32d86ce3u,
    0x45df5c75u, 0xdcd60dcfu, 0xabd13d59u, 0x26d930acu, 0x51de003au,
    0xc8d75180u, 0xbfd06116u, 0x21b4f4b5u, 0x56b3c423u, 0xcfba9599u,
    0xb8bda50fu, 0x2802b89eu, 0x5f058808u, 0xc60cd9b2u, 0xb10be924u,
    0x2f6f7c87u, 0x58684c11u, 0xc1611dabu, 0xb6662d3du, 0x76dc4190u,
    0x01db7106u, 0x98d220bcu, 0xefd5102au, 0x71b18589u, 0x06b6b51fu,
    0x9fbfe4a5u, 0xe8b8d433u, 0x7807c9a2u, 0x0f00f934u, 0x9609a88eu,
    0xe10e9818u, 0x7f6a0dbbu, 0x086d3d2du, 0x91646c97u, 0xe6635c01u,
    0x6b6b51f4u, 0x1c6c6162u, 0x856530d8u, 0xf262004eu, 0x6c0695edu,
    0x1b01a57bu, 0x8208f4c1u, 0xf50fc457u, 0x65b0d9c6u, 0x12b7e950u,
    0x8bbeb8eau, 0xfcb9887cu, 0x62dd1ddfu, 0x15da2d49u, 0x8cd37cf3u,
    0xfbd44c65u, 0x4db26158u, 0x3ab551ceu, 0xa3bc0074u, 0xd4bb30e2u,
    0x4adfa541u, 0x3dd895d7u, 0xa4d1c46du, 0xd3d6f4fbu, 0x4369e96au,
    0x346ed9fcu, 0xad678846u, 0xda60b8d0u, 0x44042d73u, 0x33031de5u,
    0xaa0a4c5fu, 0xdd0d7cc9u, 0x5005713cu, 0x270241aau, 0xbe0b1010u,
    0xc90c2086u, 0x5768b525u, 0x206f85b3u, 0xb966d409u, 0xce61e49fu,
    0x5edef90eu, 0x29d9c998u, 0xb0d09822u, 0xc7d7a8b4u, 0x59b33d17u,
    0x2eb40d81u, 0xb7bd5c3bu, 0xc0ba6cadu, 0xedb88320u, 0x9abfb3b6u,
    0x03b6e20cu, 0x74b1d29au, 0xead54739u, 0x9dd277afu, 0x04db2615u,
    0x73dc1683u, 0xe3630b12u, 0x94643b84u, 0x0d6d6a3eu, 0x7a6a5aa8u,
    0xe40ecf0bu, 0x9309ff9du, 0x0a00ae27u, 0x7d079eb1u, 0xf00f9344u,
    0x8708a3d2u, 0x1e01f268u, 0x6906c2feu, 0xf762575du, 0x806567cbu,
    0x196c3671u, 0x6e6b06e7u, 0xfed41b76u, 0x89d32be0u, 0x10da7a5au,
    0x67dd4accu, 0xf9b9df6fu, 0x8ebeeff9u, 0x17b7be43u, 0x60b08ed5u,
    0xd6d6a3e8u, 0xa1d1937eu, 0x38d8c2c4u, 0x4fdff252u, 0xd1bb67f1u,
    0xa6bc5767u, 0x3fb506ddu, 0x48b2364bu, 0xd80d2bdau, 0xaf0a1b4cu,
    0x36034af6u, 0x41047a60u, 0xdf60efc3u, 0xa867df55u, 0x316e8eefu,
    0x4669be79u, 0xcb61b38cu, 0xbc66831au, 0x256fd2a0u, 0x5268e236u,
    0xcc0c7795u, 0xbb0b4703u, 0x220216b9u, 0x5505262fu, 0xc5ba3bbeu,
    0xb2bd0b28u, 0x2bb45a92u, 0x5cb36a04u, 0xc2d7ffa7u, 0xb5d0cf31u,
    0x2cd99e8bu, 0x5bdeae1du, 0x9b64c2b0u, 0xec63f226u, 0x756aa39cu,
    0x026d930au, 0x9c0906a9u, 0xeb0e363fu, 0x72076785u, 0x05005713u,
    0x95bf4a82u, 0xe2b87a14u, 0x7bb12baeu, 0x0cb61b38u, 0x92d28e9bu,
    0xe5d5be0du, 0x7cdcefb7u, 0x0bdbdf21u, 0x86d3d2d4u, 0xf1d4e242u,
    0x68ddb3f8u, 0x1fda836eu, 0x81be16cdu, 0xf6b9265bu, 0x6fb077e1u,
    0x18b74777u, 0x88085ae6u, 0xff0f6a70u, 0x66063bcau, 0x11010b5cu,
    0x8f659effu, 0xf862ae69u, 0x616bffd3u, 0x166ccf45u, 0xa00ae278u,
    0xd70dd2eeu, 0x4e048354u, 0x3903b3c2u, 0xa7672661u, 0xd06016f7u,
    0x4969474du, 0x3e6e77dbu, 0xaed16a4au, 0xd9d65adcu, 0x40df0b66u,
    0x37d83bf0u, 0xa9bcae53u, 0xdebb9ec5u, 0x47b2cf7fu, 0x30b5ffe9u,
    0xbdbdf21cu, 0xcabac28au, 0x53b39330u, 0x24b4a3a6u, 0xbad03605u,
    0xcdd70693u, 0x54de5729u, 0x23d967bfu, 0xb3667a2eu, 0xc4614ab8u,
    0x5d681b02u, 0x2a6f2b94u, 0xb40bbe37u, 0xc30c8ea1u, 0x5a05df1bu,
    0x2d02ef8du
};

uint32_t pfs_jffs2_crc32(const void *buf, size_t len)
{
    if (NULL == buf || 0 == len)
    {
        return 0;
    }

    const uint8_t *p = (const uint8_t *)buf;
    uint32_t crc = 0;
    for (size_t i = 0; i < len; ++i)
    {
        crc = g_pfs_jffs2_crc32_table[(crc ^ p[i]) & 0xffu] ^ (crc >> 8);
    }
    return crc;
}

#define PFS_JFFS2_SCAN_RD_BUF_SZ        (256 * 1024)

typedef struct pfs_jffs2_scan_rd_t {
    pfs_jffs2_sbi_t    *p_sbi;
    uint8_t            *buf;
    int64_t             buf_ofs;
    int                 buf_len;
} pfs_jffs2_scan_rd_t;

static int pfs_jffs2_scan_rd_init(pfs_jffs2_scan_rd_t *p_rd, pfs_jffs2_sbi_t *p_sbi)
{
    if (NULL == p_rd || NULL == p_sbi)
    {
        return PFS_EINVAL;
    }
    memset(p_rd, 0, sizeof(*p_rd));
    p_rd->p_sbi = p_sbi;
    p_rd->buf = (uint8_t *)pfsmtd_malloc((size_t)PFS_JFFS2_SCAN_RD_BUF_SZ);
    if (NULL == p_rd->buf)
    {
        return PFS_ENOMEM;
    }
    return PFS_OK;
}

static void pfs_jffs2_scan_rd_done(pfs_jffs2_scan_rd_t *p_rd)
{
    if (NULL == p_rd)
    {
        return;
    }
    if (NULL != p_rd->buf)
    {
        pfsmtd_free(p_rd->buf);
        p_rd->buf = NULL;
    }
    p_rd->buf_ofs = 0;
    p_rd->buf_len = 0;
    p_rd->p_sbi = NULL;
}

static int pfs_jffs2_scan_rd_load(pfs_jffs2_scan_rd_t *p_rd, int64_t rel_ofs, int need_len)
{
    if (NULL == p_rd || NULL == p_rd->p_sbi || NULL == p_rd->buf || rel_ofs < 0 || need_len <= 0)
    {
        return PFS_EINVAL;
    }

    int64_t flash_size;

    flash_size = p_rd->p_sbi->flash_size;
    if (rel_ofs >= flash_size)
    {
        return PFS_EIO;
    }
    if (need_len > PFS_JFFS2_SCAN_RD_BUF_SZ)
    {
        return PFS_EINVAL;
    }
    if (rel_ofs >= p_rd->buf_ofs && rel_ofs + (int64_t)need_len <= p_rd->buf_ofs + (int64_t)p_rd->buf_len)
    {
        return PFS_OK;
    }

    int64_t remain;
    int want;
    int n;
    int64_t abs_off;

    remain = flash_size - rel_ofs;
    want = PFS_JFFS2_SCAN_RD_BUF_SZ;
    if ((int64_t)want > remain)
    {
        want = (int)remain;
    }
    if (want < need_len)
    {
        return PFS_EIO;
    }

    abs_off = p_rd->p_sbi->part_offset + rel_ofs;
    n = pfs_blkio_pread(p_rd->p_sbi->p_blkio, p_rd->buf, want, abs_off);
    if (n < need_len)
    {
        if (n < 0)
        {
            return pfs_jffs2_map_blkio_rc(n);
        }
        return PFS_EIO;
    }
    p_rd->buf_ofs = rel_ofs;
    p_rd->buf_len = n;
    return PFS_OK;
}

static int pfs_jffs2_scan_rd_copy(pfs_jffs2_scan_rd_t *p_rd, void *dst, int len, int64_t rel_ofs)
{
    if (NULL == p_rd || NULL == dst || len <= 0 || rel_ofs < 0)
    {
        return PFS_EINVAL;
    }

    uint8_t *p_dst;
    int left;
    int bi;
    int n;

    p_dst = (uint8_t *)dst;
    left = len;
    while (left > 0)
    {
        int rc = pfs_jffs2_scan_rd_load(p_rd, rel_ofs, left);
        if (PFS_OK != rc)
        {
            return rc;
        }
        bi = (int)(rel_ofs - p_rd->buf_ofs);
        n = p_rd->buf_len - bi;
        if (n <= 0)
        {
            return PFS_EIO;
        }
        if (n > left)
        {
            n = left;
        }
        memcpy(p_dst, p_rd->buf + bi, (size_t)n);
        p_dst += n;
        rel_ofs += (int64_t)n;
        left -= n;
    }
    return PFS_OK;
}

static int pfs_jffs2_scan_rd_peek_hdr(pfs_jffs2_scan_rd_t *p_rd, int64_t rel_ofs, pfs_jffs2_unknown_node_t *p_hdr)
{
    if (NULL == p_rd || NULL == p_hdr)
    {
        return PFS_EINVAL;
    }

    int bi;

    int rc = pfs_jffs2_scan_rd_load(p_rd, rel_ofs, (int)sizeof(*p_hdr));
    if (PFS_OK != rc)
    {
        return rc;
    }
    bi = (int)(rel_ofs - p_rd->buf_ofs);
    if (bi < 0 || bi + (int)sizeof(*p_hdr) > p_rd->buf_len)
    {
        return PFS_EIO;
    }
    memcpy(p_hdr, p_rd->buf + bi, sizeof(*p_hdr));
    return PFS_OK;
}

static int pfs_jffs2_scan_rd_advance_empty(pfs_jffs2_scan_rd_t *p_rd, int64_t *p_ofs, int64_t flash_size)
{
    if (NULL == p_rd || NULL == p_ofs)
    {
        return PFS_EINVAL;
    }

    int64_t ofs;
    int bi;
    uint16_t magic;
    uint16_t nodetype;

    ofs = *p_ofs;
    for (;;)
    {
        if (ofs + 4 > flash_size)
        {
            *p_ofs = flash_size;
            return PFS_OK;
        }
        int rc = pfs_jffs2_scan_rd_load(p_rd, ofs, 4);
        if (PFS_OK != rc)
        {
            return rc;
        }
        bi = (int)(ofs - p_rd->buf_ofs);
        while (bi + 4 <= p_rd->buf_len && ofs + 4 <= flash_size)
        {
            magic = pfs_jffs2_le16(p_rd->buf + bi);
            nodetype = pfs_jffs2_le16(p_rd->buf + bi + 2);
            if (PFS_JFFS2_EMPTY_BITMASK != magic && 0xffffu != nodetype
                    && PFS_JFFS2_DIRTY_BITMASK != magic)
            {
                *p_ofs = ofs;
                return PFS_OK;
            }
            bi += 4;
            ofs += 4;
        }
        if (ofs >= flash_size)
        {
            break;
        }
        if (p_rd->buf_len < PFS_JFFS2_SCAN_RD_BUF_SZ)
        {
            break;
        }
        ofs = p_rd->buf_ofs + (int64_t)p_rd->buf_len;
    }
    *p_ofs = ofs;
    return PFS_OK;
}

static int pfs_jffs2_hdr_crc_ok(const pfs_jffs2_unknown_node_t *p_node)
{
    pfs_jffs2_unknown_node_t tmp;
    uint32_t crc;

    tmp = *p_node;
    tmp.nodetype = (uint16_t)(tmp.nodetype | PFS_JFFS2_NODE_ACCURATE);
    crc = pfs_jffs2_crc32(&tmp, sizeof(tmp) - 4);
    return (crc == p_node->hdr_crc) ? 1 : 0;
}

void pfs_jffs2_scan_free(pfs_jffs2_sbi_t *p_sbi)
{
    pfs_jffs2_ic_t *p_ic;
    pfs_jffs2_ic_t *p_ic_n;
    pfs_jffs2_dnode_t *p_dn;
    pfs_jffs2_dnode_t *p_dn_n;
    pfs_jffs2_fd_t *p_fd;
    pfs_jffs2_fd_t *p_fd_n;

    if (NULL == p_sbi)
    {
        return;
    }
    p_ic = p_sbi->ic_list;
    p_sbi->ic_list = NULL;
    while (NULL != p_ic)
    {
        p_ic_n = p_ic->next;
        p_dn = p_ic->nodes;
        while (NULL != p_dn)
        {
            p_dn_n = p_dn->next;
            pfsmtd_free(p_dn);
            p_dn = p_dn_n;
        }
        p_fd = p_ic->dents;
        while (NULL != p_fd)
        {
            p_fd_n = p_fd->next;
            pfsmtd_free(p_fd);
            p_fd = p_fd_n;
        }
        pfsmtd_free(p_ic);
        p_ic = p_ic_n;
    }
}

pfs_jffs2_ic_t *pfs_jffs2_ic_find(pfs_jffs2_sbi_t *p_sbi, uint32_t ino)
{
    pfs_jffs2_ic_t *p_ic;

    if (NULL == p_sbi || 0 == ino)
    {
        return NULL;
    }
    for (p_ic = p_sbi->ic_list; NULL != p_ic; p_ic = p_ic->next)
    {
        if (p_ic->ino == ino)
        {
            return p_ic;
        }
    }
    return NULL;
}

bool pfs_jffs2_ic_is_dir(const pfs_jffs2_ic_t *p_ic)
{
    if (NULL == p_ic)
    {
        return false;
    }
    if (0 != (p_ic->mode & PFS_JFFS2_S_IFMT))
    {
        return (PFS_JFFS2_S_IFDIR == (p_ic->mode & PFS_JFFS2_S_IFMT));
    }
    // 无 inode 节点时仅有 dents 的父目录仍当目录
    return (NULL != p_ic->dents);
}

static pfs_jffs2_ic_t *pfs_jffs2_ic_get(pfs_jffs2_sbi_t *p_sbi, uint32_t ino)
{
    pfs_jffs2_ic_t *p_ic;

    p_ic = pfs_jffs2_ic_find(p_sbi, ino);
    if (NULL != p_ic)
    {
        return p_ic;
    }
    p_ic = (pfs_jffs2_ic_t *)pfsmtd_mallocz(sizeof(*p_ic));
    if (NULL == p_ic)
    {
        return NULL;
    }
    p_ic->ino = ino;
    p_ic->next = p_sbi->ic_list;
    p_sbi->ic_list = p_ic;
    return p_ic;
}

static int pfs_jffs2_add_inode_node(pfs_jffs2_sbi_t *p_sbi, uint32_t flash_ofs, const pfs_jffs2_raw_inode_t *p_ri)
{
    uint32_t crc;

    crc = pfs_jffs2_crc32(p_ri, sizeof(*p_ri) - 8);
    if (crc != p_ri->node_crc)
    {
        return PFS_OK; // 坏节点跳过
    }

    pfs_jffs2_ic_t *p_ic;
    pfs_jffs2_dnode_t *p_dn;

    p_ic = pfs_jffs2_ic_get(p_sbi, p_ri->ino);
    if (NULL == p_ic)
    {
        return PFS_ENOMEM;
    }

    if (p_ri->version >= p_ic->highest_version)
    {
        p_ic->highest_version = p_ri->version;
        p_ic->mode = p_ri->mode;
        p_ic->isize = p_ri->isize;
    }

    if (0 == p_ri->dsize && 0 == p_ri->csize)
    {
        return PFS_OK; // 仅元数据/截断节点
    }

    p_dn = (pfs_jffs2_dnode_t *)pfsmtd_mallocz(sizeof(*p_dn));
    if (NULL == p_dn)
    {
        return PFS_ENOMEM;
    }
    p_dn->flash_ofs = flash_ofs;
    p_dn->version = p_ri->version;
    p_dn->offset = p_ri->offset;
    p_dn->csize = p_ri->csize;
    p_dn->dsize = p_ri->dsize;
    p_dn->compr = p_ri->compr;
    p_dn->next = p_ic->nodes;
    p_ic->nodes = p_dn;
    return PFS_OK;
}

static int pfs_jffs2_add_dirent_node(pfs_jffs2_sbi_t *p_sbi, const pfs_jffs2_raw_dirent_t *p_rd, const char *name)
{
    uint32_t crc;
    int nsize;

    crc = pfs_jffs2_crc32(p_rd, sizeof(*p_rd) - 8);
    if (crc != p_rd->node_crc)
    {
        return PFS_OK;
    }
    nsize = (int)p_rd->nsize;
    if (nsize <= 0 || nsize > PFS_JFFS2_MAX_NAME_LEN)
    {
        return PFS_OK;
    }
    crc = pfs_jffs2_crc32(name, (size_t)nsize);
    if (crc != p_rd->name_crc)
    {
        return PFS_OK;
    }

    pfs_jffs2_ic_t *p_pic;
    pfs_jffs2_fd_t *p_fd;
    pfs_jffs2_fd_t *p_prev;

    p_pic = pfs_jffs2_ic_get(p_sbi, p_rd->pino);
    if (NULL == p_pic)
    {
        return PFS_ENOMEM;
    }

    p_prev = NULL;
    for (p_fd = p_pic->dents; NULL != p_fd; p_prev = p_fd, p_fd = p_fd->next)
    {
        if (p_fd->nsize == (uint8_t)nsize && 0 == memcmp(p_fd->name, name, (size_t)nsize))
        {
            if (p_rd->version < p_fd->version)
            {
                return PFS_OK;
            }
            // 同名高 version 覆盖
            p_fd->version = p_rd->version;
            p_fd->ino = p_rd->ino;
            p_fd->type = p_rd->type;
            return PFS_OK;
        }
    }

    p_fd = (pfs_jffs2_fd_t *)pfsmtd_mallocz(sizeof(*p_fd));
    if (NULL == p_fd)
    {
        return PFS_ENOMEM;
    }
    p_fd->version = p_rd->version;
    p_fd->ino = p_rd->ino;
    p_fd->type = p_rd->type;
    p_fd->nsize = (uint8_t)nsize;
    memcpy(p_fd->name, name, (size_t)nsize);
    p_fd->name[nsize] = '\0';
    p_fd->next = NULL;
    if (NULL == p_prev)
    {
        p_pic->dents = p_fd;
    }
    else
    {
        p_prev->next = p_fd;
    }
    return PFS_OK;
}

static int pfs_jffs2_ensure_root(pfs_jffs2_sbi_t *p_sbi)
{
    pfs_jffs2_ic_t *p_root;

    p_root = pfs_jffs2_ic_get(p_sbi, PFS_JFFS2_ROOT_INO);
    if (NULL == p_root)
    {
        return PFS_ENOMEM;
    }
    if (0 == (p_root->mode & PFS_JFFS2_S_IFMT))
    {
        p_root->mode = PFS_JFFS2_S_IFDIR | 0755;
    }
    return PFS_OK;
}

int pfs_jffs2_probe_magic(pfs_blkio_t *p_blkio, int64_t part_offset, int64_t flash_size)
{
    if (NULL == p_blkio || part_offset < 0 || flash_size < (int64_t)sizeof(pfs_jffs2_unknown_node_t))
    {
        return PFS_ENOEXEC;
    }

    uint8_t buf[4096];
    int64_t ofs;
    int64_t end;
    int n;
    pfs_jffs2_unknown_node_t node;

    end = flash_size;
    if (end > (int64_t)(1024 * 1024))
    {
        end = (int64_t)(1024 * 1024); // 探测只扫前 1MiB
    }

    for (ofs = 0; ofs + (int64_t)sizeof(node) <= end; )
    {
        n = (int)(end - ofs);
        if (n > (int)sizeof(buf))
        {
            n = (int)sizeof(buf);
        }
        n = pfs_blkio_pread(p_blkio, buf, n, part_offset + ofs);
        if (n < (int)sizeof(node))
        {
            if (n < 0)
            {
                return pfs_jffs2_map_blkio_rc(n);
            }
            break;
        }
        for (int i = 0; i + (int)sizeof(node) <= n; i += 2)
        {
            memcpy(&node, buf + i, sizeof(node));
            if (PFS_JFFS2_CIGAM_BITMASK == node.magic)
            {
                return PFS_ENOEXEC;
            }
            if (PFS_JFFS2_MAGIC_BITMASK == node.magic && pfs_jffs2_hdr_crc_ok(&node))
            {
                if (node.totlen >= sizeof(node) && node.totlen < (uint32_t)(16 * 1024 * 1024))
                {
                    return PFS_OK;
                }
            }
        }
        // 重叠保留半窗, 避免跨块漏检
        if (n <= (int)sizeof(node))
        {
            break;
        }
        ofs += (int64_t)(n - (int)sizeof(node) + 2);
        ofs &= ~(int64_t)1;
    }
    return PFS_ENOEXEC;
}

int pfs_jffs2_scan_medium(pfs_jffs2_sbi_t *p_sbi)
{
    if (NULL == p_sbi || NULL == p_sbi->p_blkio || p_sbi->flash_size < (int64_t)sizeof(pfs_jffs2_unknown_node_t))
    {
        return PFS_EINVAL;
    }

    pfs_jffs2_scan_rd_t rd;

    int rc = pfs_jffs2_scan_rd_init(&rd, p_sbi);
    if (PFS_OK != rc)
    {
        return rc;
    }

    pfs_jffs2_scan_free(p_sbi);

    int64_t flash_size = p_sbi->flash_size;
    int64_t ofs = 0;
    int64_t used_end = 0;
    uint32_t max_ino = PFS_JFFS2_ROOT_INO;

    while (ofs + (int64_t)sizeof(pfs_jffs2_unknown_node_t) <= flash_size)
    {
        pfs_jffs2_unknown_node_t hdr;

        rc = pfs_jffs2_scan_rd_peek_hdr(&rd, ofs, &hdr);
        if (PFS_OK != rc)
        {
            pfs_jffs2_scan_rd_done(&rd);
            return rc;
        }

        if (PFS_JFFS2_EMPTY_BITMASK == hdr.magic || 0xffffu == hdr.nodetype
                || PFS_JFFS2_DIRTY_BITMASK == hdr.magic)
        {
            rc = pfs_jffs2_scan_rd_advance_empty(&rd, &ofs, flash_size);
            if (PFS_OK != rc)
            {
                pfs_jffs2_scan_rd_done(&rd);
                return rc;
            }
            continue;
        }
        if (PFS_JFFS2_CIGAM_BITMASK == hdr.magic)
        {
            pfs_jffs2_scan_rd_done(&rd);
            return PFS_ENOEXEC;
        }
        if (PFS_JFFS2_MAGIC_BITMASK != hdr.magic)
        {
            ofs += 4;
            continue;
        }
        if (!pfs_jffs2_hdr_crc_ok(&hdr))
        {
            ofs += 4;
            continue;
        }

        uint32_t totlen = hdr.totlen;

        if (totlen < sizeof(hdr) || (int64_t)totlen > flash_size - ofs)
        {
            ofs += 4;
            continue;
        }

        uint32_t step = PFS_JFFS2_PAD(totlen);
        uint16_t nodetype = hdr.nodetype;

        if (0 == (nodetype & PFS_JFFS2_NODE_ACCURATE))
        {
            ofs += (int64_t)step;
            if (ofs > used_end)
            {
                used_end = ofs;
            }
            continue;
        }

        if (PFS_JFFS2_NODETYPE_INODE == nodetype)
        {
            pfs_jffs2_raw_inode_t ri;

            if (totlen < sizeof(ri))
            {
                ofs += (int64_t)step;
                continue;
            }
            rc = pfs_jffs2_scan_rd_copy(&rd, &ri, (int)sizeof(ri), ofs);
            if (PFS_OK != rc)
            {
                pfs_jffs2_scan_rd_done(&rd);
                return rc;
            }
            rc = pfs_jffs2_add_inode_node(p_sbi, (uint32_t)ofs, &ri);
            if (PFS_OK != rc)
            {
                pfs_jffs2_scan_rd_done(&rd);
                return rc;
            }
            if (ri.ino > max_ino)
            {
                max_ino = ri.ino;
            }
            ofs += (int64_t)step;
            if (ofs > used_end)
            {
                used_end = ofs;
            }
            continue;
        }

        if (PFS_JFFS2_NODETYPE_DIRENT == nodetype)
        {
            pfs_jffs2_raw_dirent_t rdent;
            uint8_t name[PFS_JFFS2_MAX_NAME_LEN];

            if (totlen < sizeof(rdent))
            {
                ofs += (int64_t)step;
                continue;
            }
            rc = pfs_jffs2_scan_rd_copy(&rd, &rdent, (int)sizeof(rdent), ofs);
            if (PFS_OK != rc)
            {
                pfs_jffs2_scan_rd_done(&rd);
                return rc;
            }
            if (rdent.nsize > PFS_JFFS2_MAX_NAME_LEN || sizeof(rdent) + (uint32_t)rdent.nsize > totlen)
            {
                ofs += (int64_t)step;
                continue;
            }
            if (rdent.nsize > 0)
            {
                rc = pfs_jffs2_scan_rd_copy(&rd, name, (int)rdent.nsize, ofs + (int64_t)sizeof(rdent));
                if (PFS_OK != rc)
                {
                    pfs_jffs2_scan_rd_done(&rd);
                    return rc;
                }
            }
            rc = pfs_jffs2_add_dirent_node(p_sbi, &rdent, (const char *)name);
            if (PFS_OK != rc)
            {
                pfs_jffs2_scan_rd_done(&rd);
                return rc;
            }
            if (rdent.ino > max_ino)
            {
                max_ino = rdent.ino;
            }
            if (rdent.pino > max_ino)
            {
                max_ino = rdent.pino;
            }
            ofs += (int64_t)step;
            if (ofs > used_end)
            {
                used_end = ofs;
            }
            continue;
        }

        if (PFS_JFFS2_NODETYPE_CLEANMARKER == nodetype
                || PFS_JFFS2_NODETYPE_PADDING == nodetype
                || PFS_JFFS2_NODETYPE_SUMMARY == nodetype)
        {
            ofs += (int64_t)step;
            if (ofs > used_end)
            {
                used_end = ofs;
            }
            continue;
        }

        // 未知: 按 totlen 跳过
        ofs += (int64_t)step;
        if (ofs > used_end)
        {
            used_end = ofs;
        }
    }

    p_sbi->free_ofs = used_end;
    p_sbi->next_ino = max_ino + 1;
    if (p_sbi->next_ino < 2)
    {
        p_sbi->next_ino = 2;
    }
    rc = pfs_jffs2_ensure_root(p_sbi);
    pfs_jffs2_scan_rd_done(&rd);
    return rc;
}

static void pfs_jffs2_fill_hdr_crc(pfs_jffs2_unknown_node_t *p_node)
{
    pfs_jffs2_unknown_node_t tmp;
    uint32_t crc;

    tmp = *p_node;
    tmp.nodetype = (uint16_t)(tmp.nodetype | PFS_JFFS2_NODE_ACCURATE);
    crc = pfs_jffs2_crc32(&tmp, sizeof(tmp) - 4);
    p_node->hdr_crc = crc;
}

int pfs_jffs2_append_raw(pfs_jffs2_sbi_t *p_sbi, const void *buf, uint32_t len)
{
    if (NULL == p_sbi || NULL == p_sbi->p_blkio || NULL == buf || 0 == len)
    {
        return PFS_EINVAL;
    }
    if (PFS_JFFS2_MOUNT_RDWR != p_sbi->mount_state)
    {
        return PFS_EPERM;
    }

    uint32_t pad;
    uint32_t rem;
    uint8_t ff[4];
    int n;
    int64_t abs_off;
    int64_t eb_start;
    int64_t eb_end;

    pad = PFS_JFFS2_PAD(len);
    if (p_sbi->erase_size > 0)
    {
        eb_start = (p_sbi->free_ofs / (int64_t)p_sbi->erase_size) * (int64_t)p_sbi->erase_size;
        eb_end = eb_start + (int64_t)p_sbi->erase_size;
        if (p_sbi->free_ofs + (int64_t)pad > eb_end)
        {
            while (p_sbi->free_ofs < eb_end)
            {
                rem = (uint32_t)(eb_end - p_sbi->free_ofs);
                if (rem > sizeof(ff))
                {
                    rem = (uint32_t)sizeof(ff);
                }
                memset(ff, 0xff, sizeof(ff));
                abs_off = p_sbi->part_offset + p_sbi->free_ofs;
                n = pfs_blkio_pwrite(p_sbi->p_blkio, ff, (int)rem, abs_off);
                if (n != (int)rem)
                {
                    if (n < 0)
                    {
                        return pfs_jffs2_map_blkio_rc(n);
                    }
                    return PFS_EIO;
                }
                p_sbi->free_ofs += (int64_t)rem;
            }
        }
    }
    if (p_sbi->free_ofs < 0 || p_sbi->free_ofs + (int64_t)pad > p_sbi->flash_size)
    {
        return PFS_ENOSPC;
    }
    abs_off = p_sbi->part_offset + p_sbi->free_ofs;
    n = pfs_blkio_pwrite(p_sbi->p_blkio, buf, (int)len, abs_off);
    if (n != (int)len)
    {
        if (n < 0)
        {
            return pfs_jffs2_map_blkio_rc(n);
        }
        return PFS_EIO;
    }
    rem = pad - len;
    if (rem > 0)
    {
        ff[0] = 0xff;
        ff[1] = 0xff;
        ff[2] = 0xff;
        ff[3] = 0xff;
        n = pfs_blkio_pwrite(p_sbi->p_blkio, ff, (int)rem, abs_off + (int64_t)len);
        if (n != (int)rem)
        {
            if (n < 0)
            {
                return pfs_jffs2_map_blkio_rc(n);
            }
            return PFS_EIO;
        }
    }
    p_sbi->free_ofs += (int64_t)pad;
    return PFS_OK;
}

int pfs_jffs2_emit_inode(pfs_jffs2_sbi_t *p_sbi, uint32_t ino, uint32_t version, uint32_t mode, uint32_t isize, uint32_t offset, uint32_t dsize, const void *data, uint8_t compr)
{
    pfs_jffs2_raw_inode_t ri;
    uint8_t *buf;
    uint32_t csize;
    uint32_t totlen;
    uint32_t flash_ofs;

    if (NULL == p_sbi || 0 == ino)
    {
        return PFS_EINVAL;
    }
    if (PFS_JFFS2_COMPR_NONE == compr)
    {
        csize = dsize;
        if (dsize > 0 && NULL == data)
        {
            return PFS_EINVAL;
        }
    }
    else if (PFS_JFFS2_COMPR_ZERO == compr)
    {
        csize = 0;
        data = NULL;
    }
    else
    {
        return PFS_EOPNOTSUPP;
    }
    if (csize > PFS_JFFS2_WRITE_CHUNK_MAX)
    {
        return PFS_EINVAL;
    }

    totlen = (uint32_t)sizeof(ri) + csize;
    buf = (uint8_t *)pfsmtd_mallocz((size_t)totlen);
    if (NULL == buf)
    {
        return PFS_ENOMEM;
    }


    memset(&ri, 0, sizeof(ri));
    ri.magic = PFS_JFFS2_MAGIC_BITMASK;
    ri.nodetype = PFS_JFFS2_NODETYPE_INODE;
    ri.totlen = totlen;
    ri.ino = ino;
    ri.version = version;
    ri.mode = mode;
    ri.isize = isize;
    ri.offset = offset;
    ri.csize = csize;
    ri.dsize = dsize;
    ri.compr = compr;
    pfs_jffs2_fill_hdr_crc((pfs_jffs2_unknown_node_t *)&ri);
    if (csize > 0)
    {
        memcpy(buf + sizeof(ri), data, (size_t)csize);
        ri.data_crc = pfs_jffs2_crc32(buf + sizeof(ri), (size_t)csize);
    }
    else
    {
        ri.data_crc = 0;
    }
    ri.node_crc = pfs_jffs2_crc32(&ri, sizeof(ri) - 8);
    memcpy(buf, &ri, sizeof(ri));

    flash_ofs = (uint32_t)p_sbi->free_ofs;
    int rc = pfs_jffs2_append_raw(p_sbi, buf, totlen);
    if (PFS_OK == rc)
    {
        rc = pfs_jffs2_add_inode_node(p_sbi, flash_ofs, &ri);
    }
    pfsmtd_free(buf);
    return rc;
}

int pfs_jffs2_emit_dirent(pfs_jffs2_sbi_t *p_sbi, uint32_t pino, uint32_t version, uint32_t ino, uint8_t type, const char *name, int nlen)
{
    pfs_jffs2_raw_dirent_t rd;
    uint8_t *buf;
    uint32_t totlen;

    if (NULL == p_sbi || NULL == name || nlen <= 0 || nlen > PFS_JFFS2_MAX_NAME_LEN || 0 == pino)
    {
        return PFS_EINVAL;
    }

    totlen = (uint32_t)sizeof(rd) + (uint32_t)nlen;
    buf = (uint8_t *)pfsmtd_mallocz((size_t)totlen);
    if (NULL == buf)
    {
        return PFS_ENOMEM;
    }


    memset(&rd, 0, sizeof(rd));
    rd.magic = PFS_JFFS2_MAGIC_BITMASK;
    rd.nodetype = PFS_JFFS2_NODETYPE_DIRENT;
    rd.totlen = totlen;
    rd.pino = pino;
    rd.version = version;
    rd.ino = ino;
    rd.nsize = (uint8_t)nlen;
    rd.type = type;
    pfs_jffs2_fill_hdr_crc((pfs_jffs2_unknown_node_t *)&rd);
    rd.name_crc = pfs_jffs2_crc32(name, (size_t)nlen);
    rd.node_crc = pfs_jffs2_crc32(&rd, sizeof(rd) - 8);
    memcpy(buf, &rd, sizeof(rd));
    memcpy(buf + sizeof(rd), name, (size_t)nlen);

    int rc = pfs_jffs2_append_raw(p_sbi, buf, totlen);
    if (PFS_OK == rc)
    {
        rc = pfs_jffs2_add_dirent_node(p_sbi, &rd, name);
    }
    pfsmtd_free(buf);
    return rc;
}

int pfs_jffs2_sync_media(pfs_jffs2_sbi_t *p_sbi)
{
    if (NULL == p_sbi || NULL == p_sbi->p_blkio)
    {
        return PFS_EINVAL;
    }


    int rc = pfs_blkio_fsync(p_sbi->p_blkio);
    if (rc < 0)
    {
        return pfs_jffs2_map_blkio_rc(rc);
    }
    return PFS_OK;
}

// end
