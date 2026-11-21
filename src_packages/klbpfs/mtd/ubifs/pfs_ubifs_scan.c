// Doc Encode : UTF-8 BOM, Unix(LF)
// ubifs main 区扫描重建缓存 + append 写节点 (无完整 journal/TNC)
#include "ubifs/pfs_ubifs_internal.h"
#include "base/pfsmtd_mem.h"

#include <string.h>

static int pfs_ubifs_scan_one_leb(pfs_ubifs_sbi_t *p_sbi, int lnum, uint8_t *buf)
{
    int offs = 0;
    int last_used = 0;

    while (offs + PFS_UBIFS_CH_SZ <= p_sbi->leb_size)
    {
        int rc = pfs_ubifs_leb_read(p_sbi, lnum, offs, buf, PFS_UBIFS_CH_SZ);
        if (PFS_OK != rc)
        {
            return rc;
        }
        uint32_t magic = pfs_ubifs_le32(buf);
        if (0xFFFFFFFFu == magic || 0 == magic)
        {
            break;
        }
        if (PFS_UBIFS_NODE_MAGIC != magic)
        {
            offs = PFS_UBIFS_ALIGN8(offs + 8);
            continue;
        }
        uint32_t nlen = pfs_ubifs_le32(buf + 16);
        if (PFS_UBIFS_CH_SZ > (int)nlen || offs + (int)nlen > p_sbi->leb_size)
        {
            break;
        }
        rc = pfs_ubifs_leb_read(p_sbi, lnum, offs, buf, (int)nlen);
        if (PFS_OK != rc)
        {
            return rc;
        }
        rc = pfs_ubifs_check_node(buf, (int)nlen);
        if (PFS_OK != rc)
        {
            offs = PFS_UBIFS_ALIGN8(offs + 8);
            continue;
        }

        uint8_t ntype = buf[20];
        uint64_t sqnum = pfs_ubifs_le64(buf + 8);
        if (sqnum > p_sbi->max_sqnum)
        {
            p_sbi->max_sqnum = sqnum;
        }

        if (PFS_UBIFS_INO_NODE == ntype && (int)nlen >= PFS_UBIFS_INO_NODE_SZ)
        {
            pfs_ubifs_key_t key;
            pfs_ubifs_key_read_flash(&key, buf + PFS_UBIFS_OFF_KEY);
            uint64_t inum = key.u[0];
            uint64_t size = pfs_ubifs_le64(buf + PFS_UBIFS_INO_OFF_SIZE);
            uint32_t nlink = pfs_ubifs_le32(buf + PFS_UBIFS_INO_OFF_NLINK);
            uint32_t mode = pfs_ubifs_le32(buf + PFS_UBIFS_INO_OFF_MODE);
            rc = pfs_ubifs_ic_upsert(p_sbi, inum, mode, size, nlink, sqnum);
            if (PFS_OK != rc)
            {
                return rc;
            }
            if (inum > p_sbi->highest_inum)
            {
                p_sbi->highest_inum = inum;
            }
        }
        else if (PFS_UBIFS_DENT_NODE == ntype && (int)nlen >= PFS_UBIFS_DENT_NODE_SZ)
        {
            pfs_ubifs_key_t key;
            pfs_ubifs_key_read_flash(&key, buf + PFS_UBIFS_OFF_KEY);
            uint64_t inum = pfs_ubifs_le64(buf + PFS_UBIFS_DENT_OFF_INUM);
            uint8_t dtype = buf[PFS_UBIFS_DENT_OFF_TYPE];
            uint16_t nlen_name = pfs_ubifs_le16(buf + PFS_UBIFS_DENT_OFF_NLEN);
            if (0 < nlen_name && nlen_name <= PFS_UBIFS_MAX_NLEN &&
                PFS_UBIFS_DENT_OFF_NAME + nlen_name <= (int)nlen)
            {
                rc = pfs_ubifs_dc_upsert(p_sbi, key.u[0], inum, dtype,
                        (const char *)(buf + PFS_UBIFS_DENT_OFF_NAME), (int)nlen_name, sqnum);
                if (PFS_OK != rc)
                {
                    return rc;
                }
            }
        }
        else if (PFS_UBIFS_DATA_NODE == ntype && (int)nlen >= PFS_UBIFS_DATA_NODE_SZ)
        {
            pfs_ubifs_key_t key;
            pfs_ubifs_key_read_flash(&key, buf + PFS_UBIFS_OFF_KEY);
            uint64_t inum = key.u[0];
            uint32_t block = key.u[1] & PFS_UBIFS_S_KEY_BLOCK_MASK;
            uint64_t size = pfs_ubifs_le32(buf + PFS_UBIFS_DATA_OFF_SIZE);
            rc = pfs_ubifs_dref_upsert(p_sbi, inum, block, (uint32_t)size, lnum, offs, PFS_UBIFS_DATA_OFF_DATA, sqnum);
            if (PFS_OK != rc)
            {
                return rc;
            }
        }

        last_used = PFS_UBIFS_ALIGN8(offs + (int)nlen);
        offs = last_used;
    }

    if (lnum == p_sbi->write_lnum && last_used > p_sbi->write_offs)
    {
        p_sbi->write_offs = last_used;
    }
    if (0 < last_used && (lnum > p_sbi->write_lnum || (lnum == p_sbi->write_lnum && last_used >= p_sbi->write_offs)))
    {
        p_sbi->write_lnum = lnum;
        p_sbi->write_offs = last_used;
    }
    return PFS_OK;
}

int pfs_ubifs_scan_rebuild(pfs_ubifs_sbi_t *p_sbi)
{
    if (NULL == p_sbi || NULL == p_sbi->p_ics)
    {
        return PFS_EINVAL;
    }
    memset(p_sbi->p_ics, 0, sizeof(pfs_ubifs_ic_t) * PFS_UBIFS_CACHE_INO_MAX);
    memset(p_sbi->p_dcs, 0, sizeof(pfs_ubifs_dc_t) * PFS_UBIFS_CACHE_DENT_MAX);
    memset(p_sbi->p_drefs, 0, sizeof(pfs_ubifs_dref_t) * PFS_UBIFS_CACHE_DATA_MAX);
    p_sbi->ic_cnt = 0;
    p_sbi->dc_cnt = 0;
    p_sbi->dref_cnt = 0;
    p_sbi->max_sqnum = 0;
    p_sbi->write_lnum = p_sbi->main_first;
    p_sbi->write_offs = 0;

    uint8_t *buf = (uint8_t *)pfsmtd_malloc((size_t)p_sbi->leb_size);
    if (NULL == buf)
    {
        return PFS_ENOMEM;
    }

    int end = p_sbi->main_first + p_sbi->main_lebs;
    for (int lnum = p_sbi->main_first; lnum < end; lnum++)
    {
        int rc = pfs_ubifs_scan_one_leb(p_sbi, lnum, buf);
        if (PFS_OK != rc)
        {
            pfsmtd_free(buf);
            return rc;
        }
    }
    pfsmtd_free(buf);

    // 保证根存在 (旧空盘 mkfs 无 INO 时合成)
    if (NULL == pfs_ubifs_ic_find(p_sbi, PFS_UBIFS_ROOT_INO))
    {
        int rc = pfs_ubifs_ic_upsert(p_sbi, PFS_UBIFS_ROOT_INO, PFS_UBIFS_S_IFDIR | 0755, 0, 2, 1);
        if (PFS_OK != rc)
        {
            return rc;
        }
    }
    if (p_sbi->highest_inum < PFS_UBIFS_FIRST_INO)
    {
        p_sbi->highest_inum = PFS_UBIFS_FIRST_INO;
    }
    return PFS_OK;
}

int pfs_ubifs_append_node(pfs_ubifs_sbi_t *p_sbi, void *node, int len)
{
    if (NULL == p_sbi || NULL == node || PFS_UBIFS_CH_SZ > len)
    {
        return PFS_EINVAL;
    }
    if (PFS_UBIFS_MOUNT_RDWR != p_sbi->mount_state)
    {
        return PFS_EPERM;
    }

    int alen = PFS_UBIFS_ALIGN8(len);
    int end = p_sbi->main_first + p_sbi->main_lebs;
    if (p_sbi->write_offs + alen > p_sbi->leb_size)
    {
        if (p_sbi->write_lnum + 1 >= end)
        {
            return PFS_ENOSPC;
        }
        p_sbi->write_lnum++;
        p_sbi->write_offs = 0;
    }

    p_sbi->max_sqnum++;
    {
        uint8_t ntype = ((uint8_t *)node)[20];
        pfs_ubifs_prepare_node(node, len, ntype, p_sbi->max_sqnum);
    }

    int rc = pfs_ubifs_write_node(p_sbi, p_sbi->write_lnum, p_sbi->write_offs, node, len);
    if (PFS_OK != rc)
    {
        return rc;
    }
    p_sbi->write_offs += alen;
    return PFS_OK;
}

int pfs_ubifs_flush_mst(pfs_ubifs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return PFS_EINVAL;
    }

    uint8_t *mst = (uint8_t *)pfsmtd_mallocz(PFS_UBIFS_MST_NODE_SZ);
    if (NULL == mst)
    {
        return PFS_ENOMEM;
    }
    pfs_ubifs_put_le64(mst + 24, p_sbi->highest_inum);
    pfs_ubifs_put_le64(mst + 32, 0);
    pfs_ubifs_put_le32(mst + 40, PFS_UBIFS_MST_NO_ORPHS | PFS_UBIFS_MST_DIRTY);
    pfs_ubifs_put_le32(mst + 44, PFS_UBIFS_LOG_LNUM);
    pfs_ubifs_put_le32(mst + 48, (uint32_t)p_sbi->main_first);
    pfs_ubifs_put_le32(mst + 52, 0);
    pfs_ubifs_put_le32(mst + 56, PFS_UBIFS_INO_NODE_SZ);
    pfs_ubifs_put_le32(mst + 164, (uint32_t)p_sbi->leb_cnt);
    p_sbi->max_sqnum++;
    pfs_ubifs_prepare_node(mst, PFS_UBIFS_MST_NODE_SZ, PFS_UBIFS_MST_NODE, p_sbi->max_sqnum);
    int rc = pfs_ubifs_write_node(p_sbi, PFS_UBIFS_MST_LNUM, 0, mst, PFS_UBIFS_MST_NODE_SZ);
    if (PFS_OK == rc)
    {
        rc = pfs_ubifs_write_node(p_sbi, PFS_UBIFS_MST_LNUM + 1, 0, mst, PFS_UBIFS_MST_NODE_SZ);
    }
    pfsmtd_free(mst);
    return rc;
}

// end
