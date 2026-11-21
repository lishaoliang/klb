// Doc Encode : UTF-8 BOM, Unix(LF)
// ISO 9660 super: probe/mount (只读); PVD + Joliet SVD + Rock 探测
#include "isofs/pfs_isofs_internal.h"
#include "base/pfs_mem.h"

#include <string.h>

int pfs_isofs_map_blkio_rc(int rc)
{
    if (rc >= 0)
    {
        return PFS_ISOFS_RET_OK;
    }
    if (PFS_ISOFS_RET_EINVAL == rc || PFS_ISOFS_RET_ENOMEM == rc ||
            PFS_ISOFS_RET_EIO == rc || PFS_ISOFS_RET_EBUSY == rc)
    {
        return rc;
    }
    return PFS_ISOFS_RET_EIO;
}

void pfs_isofs_inode_from_rec(const pfs_iso_directory_record_t *p_rec,
        pfs_isofs_inode_t *p_out)
{
    memset(p_out, 0, sizeof(*p_out));
    p_out->flags = pfs_isonum_711(p_rec->flags);
    p_out->extent = pfs_isofs_data_lba(p_rec);
    p_out->size = pfs_isonum_733(p_rec->size);
    p_out->n_sections = 1;
    p_out->sec[0].extent = p_out->extent;
    p_out->sec[0].size = p_out->size;
    if (0 != (p_out->flags & PFS_ISO_FILE_DIRECTORY))
    {
        p_out->type = PFS_ISOFS_TYPE_DIR;
    }
    else
    {
        p_out->type = PFS_ISOFS_TYPE_FILE;
    }
}

static int pfs_isofs_fill_root_from_rec(const uint8_t *root_rec34, pfs_isofs_inode_t *p_root)
{
    const pfs_iso_directory_record_t *p_rec;

    p_rec = (const pfs_iso_directory_record_t *)root_rec34;
    if (pfs_isonum_711(p_rec->length) < 34)
    {
        return PFS_ISOFS_RET_EBADMSG;
    }
    pfs_isofs_inode_from_rec(p_rec, p_root);
    if (PFS_ISOFS_TYPE_DIR != p_root->type || 0 == p_root->extent)
    {
        return PFS_ISOFS_RET_EBADMSG;
    }
    return PFS_ISOFS_RET_OK;
}

int pfs_isofs_read_volumes(pfs_blkio_t *p_blkio, int64_t part_offset,
        uint32_t *p_lbs, uint32_t *p_vol_blocks,
        pfs_isofs_inode_t *p_root_pri, pfs_isofs_inode_t *p_root_joliet,
        int *p_joliet_level)
{
    if (NULL == p_blkio || part_offset < 0 || NULL == p_lbs || NULL == p_vol_blocks ||
            NULL == p_root_pri || NULL == p_root_joliet || NULL == p_joliet_level)
    {
        return PFS_ISOFS_RET_EINVAL;
    }

    uint8_t *buf = (uint8_t *)pfs_malloc((size_t)PFS_ISOFS_BLOCK_SIZE);
    if (NULL == buf)
    {
        return PFS_ISOFS_RET_ENOMEM;
    }

    int found_pri = 0;
    int joliet_level = 0;
    uint8_t joliet_root[34];
    memset(joliet_root, 0, sizeof(joliet_root));
    memset(p_root_joliet, 0, sizeof(*p_root_joliet));

    int rc;
    for (int i = 16; i < 100; i++)
    {
        int64_t off = part_offset + ((int64_t)i * (int64_t)PFS_ISOFS_BLOCK_SIZE);
        rc = pfs_blkio_pread(p_blkio, buf, PFS_ISOFS_BLOCK_SIZE, off);
        if (rc != PFS_ISOFS_BLOCK_SIZE)
        {
            pfs_free(buf);
            if (rc < 0)
            {
                return pfs_isofs_map_blkio_rc(rc);
            }
            return PFS_ISOFS_RET_EIO;
        }

        if (0 != memcmp(buf + 1, PFS_ISO_STANDARD_ID, PFS_ISO_STANDARD_ID_LEN))
        {
            continue;
        }
        if (PFS_ISO_VD_END == buf[0])
        {
            break;
        }
        if (PFS_ISO_VD_PRIMARY == buf[0] && 0 == found_pri)
        {
            pfs_iso_primary_descriptor_t *p_pri = (pfs_iso_primary_descriptor_t *)buf;
            uint32_t lbs = (uint32_t)pfs_isonum_723(p_pri->logical_block_size);
            uint32_t vol_blocks = pfs_isonum_733(p_pri->volume_space_size);
            if (0 == lbs || (lbs & (lbs - 1u)) != 0 || lbs > 8192u)
            {
                pfs_free(buf);
                return PFS_ISOFS_RET_EBADMSG;
            }
            rc = pfs_isofs_fill_root_from_rec(p_pri->root_directory_record, p_root_pri);
            if (PFS_ISOFS_RET_OK != rc)
            {
                pfs_free(buf);
                return rc;
            }
            *p_lbs = lbs;
            *p_vol_blocks = vol_blocks;
            found_pri = 1;
            // 继续扫 Joliet SVD
            continue;
        }
        if (PFS_ISO_VD_SUPPLEMENTARY == buf[0])
        {
            pfs_iso_supplementary_descriptor_t *p_sec = (pfs_iso_supplementary_descriptor_t *)buf;
            if (0x25 == p_sec->escape[0] && 0x2f == p_sec->escape[1])
            {
                int lvl = 0;
                if (0x40 == p_sec->escape[2])
                {
                    lvl = 1;
                }
                else if (0x43 == p_sec->escape[2])
                {
                    lvl = 2;
                }
                else if (0x45 == p_sec->escape[2])
                {
                    lvl = 3;
                }
                if (lvl > 0 && lvl >= joliet_level)
                {
                    joliet_level = lvl;
                    memcpy(joliet_root, p_sec->root_directory_record, 34);
                }
            }
        }
    }

    pfs_free(buf);
    if (0 == found_pri)
    {
        return PFS_ISOFS_RET_ENOEXEC;
    }
    *p_joliet_level = joliet_level;
    if (joliet_level > 0)
    {
        rc = pfs_isofs_fill_root_from_rec(joliet_root, p_root_joliet);
        if (PFS_ISOFS_RET_OK != rc)
        {
            *p_joliet_level = 0;
            memset(p_root_joliet, 0, sizeof(*p_root_joliet));
        }
    }
    return PFS_ISOFS_RET_OK;
}

int pfs_isofs_init(pfs_isofs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return PFS_ISOFS_RET_EINVAL;
    }
    memset(p_sbi, 0, sizeof(*p_sbi));
    p_sbi->mount_state = PFS_ISOFS_UNMOUNTED;
    p_sbi->rock_offset = -1;
    return PFS_ISOFS_RET_OK;
}

void pfs_isofs_deinit(pfs_isofs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return;
    }
    (void)pfs_isofs_umount(p_sbi);
    memset(p_sbi, 0, sizeof(*p_sbi));
}

int pfs_isofs_probe(pfs_blkio_t *p_blkio, int64_t part_offset)
{
    uint32_t lbs;
    uint32_t vol_blocks;
    pfs_isofs_inode_t root_pri;
    pfs_isofs_inode_t root_joliet;
    int joliet_level;

    return pfs_isofs_read_volumes(p_blkio, part_offset, &lbs, &vol_blocks,
            &root_pri, &root_joliet, &joliet_level);
}

int pfs_isofs_mount(pfs_isofs_sbi_t *p_sbi, pfs_blkio_t *p_blkio, int64_t part_offset,
        pfs_isofs_mount_state_e mode)
{
    uint32_t lbs;
    uint32_t vol_blocks;
    pfs_isofs_inode_t root_pri;
    pfs_isofs_inode_t root_joliet;
    int joliet_level;

    if (NULL == p_sbi || NULL == p_blkio || part_offset < 0)
    {
        return PFS_ISOFS_RET_EINVAL;
    }
    if (PFS_ISOFS_UNMOUNTED != p_sbi->mount_state)
    {
        return PFS_ISOFS_RET_EBUSY;
    }
    if (PFS_ISOFS_MOUNT_RDONLY != mode)
    {
        return PFS_ISOFS_RET_EPERM;
    }

    int rc = pfs_isofs_read_volumes(p_blkio, part_offset, &lbs, &vol_blocks,
            &root_pri, &root_joliet, &joliet_level);
    if (PFS_ISOFS_RET_OK != rc)
    {
        return rc;
    }

    p_sbi->p_blkio = p_blkio;
    p_sbi->part_offset = part_offset;
    p_sbi->logical_block_size = lbs;
    p_sbi->volume_space_size = vol_blocks;
    p_sbi->joliet_level = 0;
    p_sbi->rock = 0;
    p_sbi->rock_offset = -1;
    p_sbi->root = root_pri;
    p_sbi->mount_state = PFS_ISOFS_MOUNT_RDONLY;

    // 先在 Primary 根上探测 Rock Ridge
    rc = pfs_isofs_rock_detect(p_sbi, &root_pri);
    if (rc < 0)
    {
        p_sbi->mount_state = PFS_ISOFS_UNMOUNTED;
        return rc;
    }

    // RR 优先; 否则有 Joliet 则切 Joliet 根
    if (0 == p_sbi->rock && joliet_level > 0 && 0 != root_joliet.extent)
    {
        p_sbi->joliet_level = joliet_level;
        p_sbi->root = root_joliet;
    }

    return PFS_ISOFS_RET_OK;
}

int pfs_isofs_remount(pfs_isofs_sbi_t *p_sbi, pfs_isofs_mount_state_e mode)
{
    if (NULL == p_sbi)
    {
        return PFS_ISOFS_RET_EINVAL;
    }
    if (PFS_ISOFS_UNMOUNTED == p_sbi->mount_state)
    {
        return PFS_ISOFS_RET_EBUSY;
    }
    if (PFS_ISOFS_MOUNT_RDONLY == mode)
    {
        p_sbi->mount_state = PFS_ISOFS_MOUNT_RDONLY;
        return PFS_ISOFS_RET_OK;
    }
    return PFS_ISOFS_RET_EPERM;
}

int pfs_isofs_umount(pfs_isofs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return PFS_ISOFS_RET_EINVAL;
    }
    p_sbi->p_blkio = NULL;
    p_sbi->part_offset = 0;
    p_sbi->logical_block_size = 0;
    p_sbi->volume_space_size = 0;
    p_sbi->joliet_level = 0;
    p_sbi->rock = 0;
    p_sbi->rock_offset = -1;
    memset(&p_sbi->root, 0, sizeof(p_sbi->root));
    p_sbi->mount_state = PFS_ISOFS_UNMOUNTED;
    return PFS_ISOFS_RET_OK;
}

int pfs_isofs_sbi_is_mounted(const pfs_isofs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return 0;
    }
    return (PFS_ISOFS_UNMOUNTED != p_sbi->mount_state) ? 1 : 0;
}

int pfs_isofs_sbi_can_write(const pfs_isofs_sbi_t *p_sbi)
{
    (void)p_sbi;
    return 0;
}

// end
