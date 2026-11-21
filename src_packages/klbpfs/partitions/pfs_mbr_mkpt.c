// Doc Encode : UTF-8 BOM, Unix(LF)
// 做 MBR 分区表; 支持 count 1..4 均分数据区
#include "pfs.h"
#include "pfs_part.h"
#include "partitions/pfs_part_internal.h"
#include "partitions/pfs_part_mbr_raw.h"
#include "pfs_endian.h"
#include "base/pfs_mem.h"

#include <string.h>

// 单分区起始尽量 1MiB 对齐 (相对盘首)
#define PFS_PART_MBR_ALIGN_BYTES        (1024u * 1024u)

static int pfs_mbr_mkpt_map_blkio_rc(int rc)
{
    if (rc >= 0)
    {
        return PFS_OK;
    }
    if (PFS_BLKIO_RET_PARAM == rc || PFS_BLKIO_RET_STATE == rc)
    {
        return PFS_EINVAL;
    }
    return PFS_EIO;
}

static int pfs_mbr_mkpt_map_xfer_rc(int rc, int expect)
{
    if (rc == expect)
    {
        return PFS_OK;
    }
    else if (rc >= 0)
    {
        return PFS_EIO;
    }
    return pfs_mbr_mkpt_map_blkio_rc(rc);
}

// fs_type: PFS_NONE -> sys_ind 0; PFS_FAT12/16/32/EXFAT/NTFS -> 对应 MBR sys_ind; 其它 EINVAL
static int pfs_mbr_mkpt_sys_ind_from_fs(pfs_fs_type_e fs_type, uint8_t *p_sys_ind)
{
    if (NULL == p_sys_ind)
    {
        return PFS_EINVAL;
    }
    switch (fs_type)
    {
    case PFS_NONE:
        *p_sys_ind = PFS_PART_MBR_SYS_EMPTY;
        return PFS_OK;
    case PFS_FAT12:
        *p_sys_ind = PFS_PART_MBR_SYS_FAT12;
        return PFS_OK;
    case PFS_FAT16:
        *p_sys_ind = PFS_PART_MBR_SYS_FAT16_LBA;
        return PFS_OK;
    case PFS_FAT32:
        *p_sys_ind = PFS_PART_MBR_SYS_FAT32_LBA;
        return PFS_OK;
    case PFS_EXFAT:
    case PFS_NTFS:
        *p_sys_ind = PFS_PART_MBR_SYS_NTFS_EXFAT;
        return PFS_OK;
    default:
        return PFS_EINVAL;
    }
}

static void pfs_mbr_mkpt_set_chs_lba_only(pfs_part_mbr_ent_t *p_ent)
{
    // 超出 CHS 可表达范围时常用 FE/FF/FF, 以 LBA 字段为准
    p_ent->start_head = 0xFEu;
    p_ent->start_sector = 0xFFu;
    p_ent->start_cyl = 0xFFu;
    p_ent->end_head = 0xFEu;
    p_ent->end_sector = 0xFFu;
    p_ent->end_cyl = 0xFFu;
}

// count 1..4 均分数据区; 起止由实现按 1MiB 对齐自算
int pfs_mbr_mkpt(pfs_blkio_t *p_blkio, uint64_t total_bytes, const pfs_mkpt_table_t *p_in, pfs_part_table_t *p_out)
{
    if (NULL == p_blkio || NULL == p_in || NULL == p_out)
    {
        return PFS_EINVAL;
    }
    if (!pfs_blkio_is_open(p_blkio))
    {
        return PFS_EINVAL;
    }
    if (p_in->count < 1 || p_in->count > PFS_PART_ENTRY_MAX)
    {
        return PFS_EINVAL;
    }
    if (p_in->count > PFS_PART_MBR_PRIMARY_COUNT)
    {
        return PFS_EOPNOTSUPP;
    }

    for (int i = 0; i < p_in->count; i++)
    {
        pfs_fs_type_e fs_type = p_in->entries[i].fs_type;
        if (0 == p_in->entries[i].mbr_sys_ind)
        {
            uint8_t sys_ind;
            int rc = pfs_mbr_mkpt_sys_ind_from_fs(fs_type, &sys_ind);
            if (PFS_OK != rc)
            {
                return rc;
            }
        }
    }

    pfs_part_mbr_t mbr;
    if (sizeof(mbr) != (size_t)PFS_PART_MBR_SECTOR_SIZE)
    {
        return PFS_EINVAL;
    }

    int sector_size = pfs_blkio_logical_sector_size(p_blkio);
    if (sector_size < PFS_PART_MBR_SECTOR_SIZE ||
            sector_size > 4096 ||
            (sector_size & (sector_size - 1)) != 0)
    {
        return PFS_EINVAL;
    }

    uint8_t sec_buf[4096];

    uint64_t disk_bytes = total_bytes;
    if (0 == disk_bytes)
    {
        int64_t dev_bytes = pfs_blkio_get_size(p_blkio);
        if (dev_bytes <= 0)
        {
            return PFS_EINVAL;
        }
        disk_bytes = (uint64_t)dev_bytes;
    }
    else
    {
        int64_t dev_bytes = pfs_blkio_get_size(p_blkio);
        if (dev_bytes > 0 && disk_bytes > (uint64_t)dev_bytes)
        {
            return PFS_EINVAL;
        }
    }

    if (0 != (disk_bytes % (uint64_t)sector_size))
    {
        return PFS_EINVAL;
    }

    uint64_t total_sectors = disk_bytes / (uint64_t)sector_size;
    // MBR start_lba/size_lba 为 uint32; 超出须走 GPT
    if (0 == total_sectors || total_sectors > 0xFFFFFFFFu)
    {
        return PFS_EINVAL;
    }

    uint64_t align_lba = (uint64_t)PFS_PART_MBR_ALIGN_BYTES / (uint64_t)sector_size;
    if (0 == align_lba)
    {
        align_lba = 1;
    }
    uint64_t first_usable;
    if (total_sectors > align_lba)
    {
        first_usable = align_lba;
    }
    else if (total_sectors > 1u)
    {
        first_usable = 1;
    }
    else
    {
        return PFS_EINVAL;
    }
    uint64_t last_usable = total_sectors - 1ULL;

    uint64_t part_starts[PFS_PART_ENTRY_MAX];
    uint64_t part_sizes[PFS_PART_ENTRY_MAX];
    int part_count = p_in->count;
    int rc = pfs_part_mkpt_split_even(first_usable, last_usable, align_lba, part_count,
            part_starts, part_sizes);
    if (PFS_PART_RET_OK != rc)
    {
        return rc;
    }

    for (int i = 0; i < part_count; i++)
    {
        if (part_starts[i] > 0xFFFFFFFFu || part_sizes[i] > 0xFFFFFFFFu ||
                part_starts[i] + part_sizes[i] > 0xFFFFFFFFu)
        {
            return PFS_EINVAL;
        }
    }

    pfs_fs_type_e fs_type = p_in->entries[0].fs_type;
    memset(&mbr, 0, sizeof(mbr));
    uint32_t disk_sig = (uint32_t)total_sectors ^ ((uint32_t)fs_type << 16) ^ 0x50465331u;
    if (0 == disk_sig)
    {
        disk_sig = 0x50465331u;
    }
    mbr.disk_signature = pfs_cpu_to_le32(disk_sig);
    mbr.signature = pfs_cpu_to_le16((uint16_t)PFS_PART_MBR_SIGNATURE);

    for (int i = 0; i < part_count; i++)
    {
        fs_type = p_in->entries[i].fs_type;
        uint8_t sys_ind = p_in->entries[i].mbr_sys_ind;
        if (0 == sys_ind)
        {
            rc = pfs_mbr_mkpt_sys_ind_from_fs(fs_type, &sys_ind);
            if (PFS_OK != rc)
            {
                return rc;
            }
        }
        bool bootable = p_in->entries[i].bootable;

        pfs_part_mbr_ent_t *p_ent = &mbr.partition[i];
        p_ent->boot_ind = bootable ? 0x80u : 0u;
        p_ent->sys_ind = sys_ind;
        pfs_mbr_mkpt_set_chs_lba_only(p_ent);
        p_ent->start_lba = pfs_cpu_to_le32((uint32_t)part_starts[i]);
        p_ent->size_lba = pfs_cpu_to_le32((uint32_t)part_sizes[i]);
    }

    if (sector_size == PFS_PART_MBR_SECTOR_SIZE)
    {
        rc = pfs_blkio_pwrite(p_blkio, &mbr, PFS_PART_MBR_SECTOR_SIZE, 0);
        rc = pfs_mbr_mkpt_map_xfer_rc(rc, PFS_PART_MBR_SECTOR_SIZE);
        if (PFS_OK != rc)
        {
            return rc;
        }
    }
    else
    {
        // 逻辑扇区 >512: RMW 首扇区, 仅覆盖前 512 字节 MBR
        rc = pfs_blkio_pread(p_blkio, sec_buf, sector_size, 0);
        rc = pfs_mbr_mkpt_map_xfer_rc(rc, sector_size);
        if (PFS_OK != rc)
        {
            return rc;
        }
        memcpy(sec_buf, &mbr, sizeof(mbr));
        rc = pfs_blkio_pwrite(p_blkio, sec_buf, sector_size, 0);
        rc = pfs_mbr_mkpt_map_xfer_rc(rc, sector_size);
        if (PFS_OK != rc)
        {
            return rc;
        }
    }

    rc = pfs_blkio_fsync(p_blkio);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return pfs_mbr_mkpt_map_blkio_rc(rc);
    }

    memset(p_out, 0, sizeof(*p_out));
    p_out->scheme = PFS_PART_SCHEME_MBR;
    p_out->count = part_count;
    for (int i = 0; i < part_count; i++)
    {
        pfs_fs_type_e fs_type = p_in->entries[i].fs_type;
        uint8_t sys_ind = p_in->entries[i].mbr_sys_ind;
        if (0 == sys_ind)
        {
            rc = pfs_mbr_mkpt_sys_ind_from_fs(fs_type, &sys_ind);
            if (PFS_OK != rc)
            {
                return rc;
            }
        }
        p_out->entries[i].start_lba = part_starts[i];
        p_out->entries[i].size_lba = part_sizes[i];
        p_out->entries[i].bootable = p_in->entries[i].bootable;
        p_out->entries[i].mbr_sys_ind = sys_ind;
    }
    return PFS_OK;
}

// end
