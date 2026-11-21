// Doc Encode : UTF-8 BOM, Unix(LF)
// jffs2 格式化 mkfs (写根目录 inode) / mkvol (mkfs + 灌 src_dir)
#include "jffs2/pfs_jffs2_internal.h"
#include "base/pfsmtd_dirwalk.h"
#include "base/pfsmtd_mem.h"

#include <stdio.h>
#include <string.h>

#ifndef _WIN32
#include <sys/stat.h>
#include <stdbool.h>
#endif

static void pfs_jffs2_mkfs_fill_hdr_crc(pfs_jffs2_unknown_node_t *p_node)
{
    pfs_jffs2_unknown_node_t tmp = *p_node;
    tmp.nodetype = (uint16_t)(tmp.nodetype | PFS_JFFS2_NODE_ACCURATE);
    uint32_t crc = pfs_jffs2_crc32(&tmp, sizeof(tmp) - 4);
    p_node->hdr_crc = crc;
}

int pfs_jffs2_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes)
{
    if (NULL == p_blkio || part_offset < 0)
    {
        return PFS_EINVAL;
    }

    int64_t sz = pfs_blkio_get_size(p_blkio);
    if (sz < 0)
    {
        return pfs_jffs2_map_blkio_rc((int)sz);
    }
    if (part_offset >= sz)
    {
        return PFS_EINVAL;
    }
    int64_t flash_size = sz - part_offset;
    if (flash_size < (int64_t)sizeof(pfs_jffs2_raw_inode_t))
    {
        return PFS_ENOSPC;
    }

    int64_t fill = flash_size;
    if (0 != vol_bytes && (int64_t)vol_bytes < fill)
    {
        fill = (int64_t)vol_bytes;
    }
    if (fill < (int64_t)sizeof(pfs_jffs2_raw_inode_t))
    {
        fill = (int64_t)sizeof(pfs_jffs2_raw_inode_t);
    }

    uint8_t chunk[4096];
    memset(chunk, 0xff, sizeof(chunk));
    for (int64_t ofs = 0; ofs < fill; )
    {
        int w = (int)(fill - ofs);
        if (w > (int)sizeof(chunk))
        {
            w = (int)sizeof(chunk);
        }
        int n = pfs_blkio_pwrite(p_blkio, chunk, w, part_offset + ofs);
        if (n != w)
        {
            if (0 > n)
            {
                return pfs_jffs2_map_blkio_rc(n);
            }
            return PFS_EIO;
        }
        ofs += w;
    }

    pfs_jffs2_raw_inode_t ri;
    memset(&ri, 0, sizeof(ri));
    ri.magic = PFS_JFFS2_MAGIC_BITMASK;
    ri.nodetype = PFS_JFFS2_NODETYPE_INODE;
    ri.totlen = (uint32_t)sizeof(ri);
    ri.ino = PFS_JFFS2_ROOT_INO;
    ri.version = 1;
    ri.mode = PFS_JFFS2_S_IFDIR | 0755;
    ri.isize = 0;
    pfs_jffs2_mkfs_fill_hdr_crc((pfs_jffs2_unknown_node_t *)&ri);
    ri.data_crc = 0;
    ri.node_crc = pfs_jffs2_crc32(&ri, sizeof(ri) - 8);

    int n = pfs_blkio_pwrite(p_blkio, &ri, (int)sizeof(ri), part_offset);
    if (n != (int)sizeof(ri))
    {
        if (0 > n)
        {
            return pfs_jffs2_map_blkio_rc(n);
        }
        return PFS_EIO;
    }
    n = pfs_blkio_fsync(p_blkio);
    if (0 > n)
    {
        return pfs_jffs2_map_blkio_rc(n);
    }
    return PFS_OK;
}

typedef struct _pfs_jffs2_mkvol_ctx_t {
    pfs_jffs2_sbi_t        *p_sbi;
} pfs_jffs2_mkvol_ctx_t;

static int pfs_jffs2_mkvol_peek_elf(const char *host_path, int *p_is_elf)
{
    if (NULL == p_is_elf)
    {
        return PFS_EINVAL;
    }
    *p_is_elf = 0;
    if (NULL == host_path || '\0' == host_path[0])
    {
        return PFS_EINVAL;
    }
    FILE *fp = fopen(host_path, "rb");
    if (NULL == fp)
    {
        return PFS_EIO;
    }
    unsigned char b[4];
    if (4 == fread(b, 1, 4, fp))
    {
        if (0x7fu == b[0] && 'E' == b[1] && 'L' == b[2] && 'F' == b[3])
        {
            *p_is_elf = 1;
        }
    }
    fclose(fp);
    return PFS_OK;
}

static uint32_t pfs_jffs2_mkvol_host_perm(const char *host_path)
{
    uint32_t perm = 0644u;
    int is_elf = 0;

#ifndef _WIN32
    {
        struct stat st;

        if (0 == stat(host_path, &st) && S_ISREG(st.st_mode))
        {
            perm = (uint32_t)(st.st_mode & 0777u);
        }
    }
#endif
    if (0 == perm)
    {
        perm = 0644u;
    }
    if (0 == (perm & 0111u))
    {
        if (PFS_OK == pfs_jffs2_mkvol_peek_elf(host_path, &is_elf) && 0 != is_elf)
        {
            perm = (perm & 0666u) | 0111u;
        }
        const char *dot = strrchr(host_path, '.');
        if (NULL != dot && 0 == strcmp(dot, ".so"))
        {
            perm = (perm & 0666u) | 0111u;
        }
    }
    return perm;
}

static int pfs_jffs2_mkvol_on_entry(void *ctx, const char *vfs_rel, const char *host_path, bool is_dir)
{
    pfs_jffs2_mkvol_ctx_t *p_ctx = (pfs_jffs2_mkvol_ctx_t *)ctx;
    if (is_dir)
    {
        return pfs_jffs2_mkdir(p_ctx->p_sbi, vfs_rel, 0755);
    }

    uint32_t perm = pfs_jffs2_mkvol_host_perm(host_path);
    uint32_t ino;
    int rc = pfs_jffs2_creat(p_ctx->p_sbi, vfs_rel, perm, &ino);
    if (PFS_OK != rc)
    {
        return rc;
    }
    uint8_t *data;
    uint32_t size;
    rc = pfsmtd_host_read_file(host_path, &data, &size);
    if (PFS_OK != rc)
    {
        return rc;
    }
    pfs_jffs2_file_meta_t fm;
    rc = pfs_jffs2_file_open(p_ctx->p_sbi, vfs_rel, PFS_O_RDWR, &fm);
    if (PFS_OK != rc)
    {
        pfsmtd_free(data);
        return rc;
    }
    int64_t pos = 0;
    while ((uint32_t)pos < size)
    {
        int chunk = (int)(size - (uint32_t)pos);
        if (chunk > PFS_JFFS2_WRITE_CHUNK_MAX)
        {
            chunk = PFS_JFFS2_WRITE_CHUNK_MAX;
        }
        int put;
        rc = pfs_jffs2_file_write(p_ctx->p_sbi, &fm, pos, data + pos, chunk, &put);
        if (PFS_OK != rc || put != chunk)
        {
            pfs_jffs2_file_close(p_ctx->p_sbi, &fm);
            pfsmtd_free(data);
            return (PFS_OK != rc) ? rc : PFS_EIO;
        }
        pos += chunk;
    }
    pfs_jffs2_file_close(p_ctx->p_sbi, &fm);
    pfsmtd_free(data);
    return PFS_OK;
}

int pfs_jffs2_mkvol_ex(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes, const char *src_dir, const char *volume_id, uint32_t erase_size)
{
    (void)volume_id;
    if (NULL == src_dir || '\0' == src_dir[0])
    {
        return PFS_EINVAL;
    }
    if (erase_size > 0 && 0u != (erase_size & (erase_size - 1u)))
    {
        return PFS_EINVAL;
    }

    int rc = pfs_jffs2_mkfs(p_blkio, part_offset, vol_bytes);
    if (PFS_OK != rc)
    {
        return rc;
    }
    pfs_jffs2_sbi_t sbi;
    pfs_jffs2_mkvol_ctx_t ctx;
    rc = pfs_jffs2_init(&sbi);
    if (PFS_OK != rc)
    {
        return rc;
    }
    sbi.erase_size = erase_size;
    rc = pfs_jffs2_mount(&sbi, p_blkio, part_offset, PFS_MNT_RDWR);
    if (PFS_OK != rc)
    {
        pfs_jffs2_deinit(&sbi);
        return rc;
    }
    ctx.p_sbi = &sbi;
    rc = pfsmtd_dirwalk(src_dir, pfs_jffs2_mkvol_on_entry, &ctx);
    (void)pfs_jffs2_umount(&sbi);
    pfs_jffs2_deinit(&sbi);
    return rc;
}

int pfs_jffs2_mkvol(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes, const char *src_dir, const char *volume_id)
{
    return pfs_jffs2_mkvol_ex(p_blkio, part_offset, vol_bytes, src_dir, volume_id, 0);
}

// end
