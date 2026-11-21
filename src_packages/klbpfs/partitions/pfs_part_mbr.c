// Doc Encode : UTF-8 BOM, Unix(LF)
// MBR 探测; 布局对齐 linux-7.1 msdos_partition / legacy_mbr
#include "partitions/pfs_part_internal.h"
#include "partitions/pfs_part_mbr_raw.h"
#include "pfs_endian.h"

#include <string.h>
#include <stdbool.h>

static bool pfs_part_mbr_is_extended(uint8_t sys_ind)
{
    return (PFS_PART_MBR_SYS_EXTENDED == sys_ind ||
            PFS_PART_MBR_SYS_WIN98_EXT == sys_ind ||
            PFS_PART_MBR_SYS_LINUX_EXT == sys_ind);
}

static int pfs_part_mbr_map_blkio_rc(int rc)
{
    if (rc >= 0)
    {
        return PFS_PART_RET_OK;
    }
    else if (PFS_BLKIO_RET_PARAM == rc || PFS_BLKIO_RET_STATE == rc)
    {
        return PFS_PART_RET_EINVAL;
    }
    return PFS_PART_RET_EIO;
}

// expect>0 时要求恰好传输 expect 字节; 短读/短写为 EIO
static int pfs_part_mbr_map_xfer_rc(int rc, int expect)
{
    if (rc == expect)
    {
        return PFS_PART_RET_OK;
    }
    else if (rc >= 0)
    {
        return PFS_PART_RET_EIO;
    }
    return pfs_part_mbr_map_blkio_rc(rc);
}

int pfs_part_mbr_probe(pfs_blkio_t *p_blkio, pfs_part_table_t *p_out)
{
    if (NULL == p_blkio || NULL == p_out)
    {
        return PFS_PART_RET_EINVAL;
    }
    if (!pfs_blkio_is_open(p_blkio))
    {
        return PFS_PART_RET_EINVAL;
    }

    memset(p_out, 0, sizeof(*p_out));
    p_out->scheme = PFS_PART_SCHEME_NONE;

    if (sizeof(pfs_part_mbr_t) != (size_t)PFS_PART_MBR_SECTOR_SIZE)
    {
        return PFS_PART_RET_EINVAL;
    }

    int sector_size = pfs_blkio_logical_sector_size(p_blkio);
    if (sector_size < PFS_PART_MBR_SECTOR_SIZE ||
            sector_size > 4096 ||
            (sector_size & (sector_size - 1)) != 0)
    {
        return PFS_PART_RET_EINVAL;
    }

    // 读首扇区; MBR 固定取前 512 字节 (与 mbr_mkpt 一致, LBA 按逻辑扇区计)
    pfs_part_mbr_t mbr;
    int rc;
    if (sector_size == PFS_PART_MBR_SECTOR_SIZE)
    {
        rc = pfs_blkio_pread(p_blkio, &mbr, PFS_PART_MBR_SECTOR_SIZE, 0);
        rc = pfs_part_mbr_map_xfer_rc(rc, PFS_PART_MBR_SECTOR_SIZE);
    }
    else
    {
        uint8_t sec_buf[4096];

        rc = pfs_blkio_pread(p_blkio, sec_buf, sector_size, 0);
        rc = pfs_part_mbr_map_xfer_rc(rc, sector_size);
        if (PFS_PART_RET_OK == rc)
        {
            memcpy(&mbr, sec_buf, sizeof(mbr));
        }
    }
    if (PFS_PART_RET_OK != rc)
    {
        return rc;
    }

    uint16_t sig = pfs_le16_to_cpu(mbr.signature);
    if (PFS_PART_MBR_SIGNATURE != sig)
    {
        return PFS_PART_RET_ENOEXEC;
    }

    // boot_ind 须为 0 或 0x80; 否则可能是无分区表的 FAT 引导扇 (对齐 linux msdos_partition)
    for (int i = 0; i < PFS_PART_MBR_PRIMARY_COUNT; i++)
    {
        uint8_t boot_ind = mbr.partition[i].boot_ind;

        if (0 != boot_ind && 0x80u != boot_ind)
        {
            return PFS_PART_RET_ENOEXEC;
        }
    }

    // Protective MBR (0xEE) 交给 GPT 探测; 本函数视为非 DOS/MBR scheme
    for (int i = 0; i < PFS_PART_MBR_PRIMARY_COUNT; i++)
    {
        if (PFS_PART_MBR_OSTYPE_EFI_GPT == mbr.partition[i].sys_ind)
        {
            return PFS_PART_RET_ENOEXEC;
        }
    }

    int count = 0;
    for (int i = 0; i < PFS_PART_MBR_PRIMARY_COUNT; i++)
    {
        pfs_part_mbr_ent_t *p_ent = &mbr.partition[i];
        uint32_t start_lba = pfs_le32_to_cpu(p_ent->start_lba);
        uint32_t size_lba = pfs_le32_to_cpu(p_ent->size_lba);

        if (PFS_PART_MBR_SYS_EMPTY == p_ent->sys_ind)
        {
            continue;
        }
        if (0 == size_lba)
        {
            continue;
        }
        // 扩展分区: 主表项跳过; 逻辑分区解析未落地
        if (pfs_part_mbr_is_extended(p_ent->sys_ind))
        {
            continue;
        }
        if (count >= PFS_PART_ENTRY_MAX)
        {
            return PFS_PART_RET_EBADMSG;
        }
        p_out->entries[count].start_lba = (uint64_t)start_lba;
        p_out->entries[count].size_lba = (uint64_t)size_lba;
        p_out->entries[count].bootable = (0x80u == p_ent->boot_ind);
        p_out->entries[count].mbr_sys_ind = p_ent->sys_ind;
        count++;
    }

    p_out->scheme = PFS_PART_SCHEME_MBR;
    p_out->count = count;
    return PFS_PART_RET_OK;
}

// end
