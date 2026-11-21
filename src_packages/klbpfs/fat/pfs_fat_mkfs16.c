// Doc Encode : UTF-8 BOM, Unix(LF)
// 制作 FAT16 卷 (用户态 mkfs; 布局对齐 mkfs.fat / ChaN FatFs f_mkfs FAT16 路径)
#include "fat/pfs_fat_internal.h"

#include <stdlib.h>
#include <string.h>

#define PFS_FAT_MKFS16_RESERVED         1U
#define PFS_FAT_MKFS16_NUM_FATS         2U
#define PFS_FAT_MKFS16_ROOT_ENTRIES     512U
#define PFS_FAT_MKFS16_MEDIA            0xF8U
#define PFS_FAT_MKFS16_MIN_SECTORS      128U

// FatFs f_mkfs FAT AU 边界表 (卷大小单位: 4K 扇区)
static const uint16_t g_pfs_fat_mkfs16_cst[] = { 1, 4, 16, 64, 256, 512, 0 };

static uint32_t pfs_fat_mkfs16_root_sectors(void)
{
    return (PFS_FAT_MKFS16_ROOT_ENTRIES * (uint32_t)sizeof(fat_dir_entry_t)) /
            PFS_FAT_MKFS_SECTOR_SIZE;
}

static uint32_t pfs_fat_mkfs16_pick_sec_per_clus(uint32_t total_sectors)
{
    uint32_t n = total_sectors / 0x1000U;
    uint32_t pau = 1U;
    for (int i = 0; 0 != g_pfs_fat_mkfs16_cst[i] && g_pfs_fat_mkfs16_cst[i] <= n; i++)
    {
        pau <<= 1;
    }
    if (pau > 128U)
    {
        pau = 128U;
    }
    return pau;
}

// 计算 FAT16 几何; 输出主机序; 保证 MAX_FAT12 < 簇数 <= MAX_FAT16
static int pfs_fat_mkfs16_calc_layout(
        uint32_t total_sectors,
        uint32_t *p_sec_per_clus,
        uint32_t *p_reserved,
        uint32_t *p_fat_length,
        uint32_t *p_root_sectors,
        uint32_t *p_data_start,
        uint32_t *p_num_clusters)
{
    if (NULL == p_sec_per_clus || NULL == p_reserved || NULL == p_fat_length ||
            NULL == p_root_sectors || NULL == p_data_start || NULL == p_num_clusters)
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (total_sectors < PFS_FAT_MKFS16_MIN_SECTORS)
    {
        return PFS_FAT_RET_EINVAL;
    }

    uint32_t sec_per_clus = pfs_fat_mkfs16_pick_sec_per_clus(total_sectors);
    uint32_t reserved = PFS_FAT_MKFS16_RESERVED;
    uint32_t root_sectors = pfs_fat_mkfs16_root_sectors();
    uint32_t fat_length = 1U;

    for (int i = 0; i < 96; i++)
    {
        uint32_t data_start = reserved + fat_length * PFS_FAT_MKFS16_NUM_FATS + root_sectors;
        if (data_start >= total_sectors)
        {
            return PFS_FAT_RET_EINVAL;
        }
        uint32_t num_clusters = (total_sectors - data_start) / sec_per_clus;
        if (num_clusters < 2U)
        {
            return PFS_FAT_RET_EINVAL;
        }
        if (num_clusters > MAX_FAT16)
        {
            // 簇过多, 加大每簇扇区数
            if (sec_per_clus >= 128U)
            {
                return PFS_FAT_RET_EINVAL;
            }
            sec_per_clus <<= 1;
            fat_length = 1U;
            continue;
        }
        if (num_clusters <= MAX_FAT12)
        {
            // 簇过少会落成 FAT12, 减小每簇扇区数以增多簇
            if (sec_per_clus <= 1U)
            {
                return PFS_FAT_RET_EINVAL;
            }
            sec_per_clus >>= 1;
            fat_length = 1U;
            continue;
        }
        // FAT 字节: 簇数*2+4, 对齐 FatFs
        uint32_t fat_bytes = num_clusters * 2U + 4U;
        uint32_t need_fat = (fat_bytes + PFS_FAT_MKFS_SECTOR_SIZE - 1U) / PFS_FAT_MKFS_SECTOR_SIZE;
        if (fat_length >= need_fat)
        {
            *p_sec_per_clus = sec_per_clus;
            *p_reserved = reserved;
            *p_fat_length = fat_length;
            *p_root_sectors = root_sectors;
            *p_data_start = data_start;
            *p_num_clusters = num_clusters;
            return PFS_FAT_RET_OK;
        }
        fat_length = need_fat;
    }
    return PFS_FAT_RET_EINVAL;
}

static void pfs_fat_mkfs16_pack_boot(
        uint8_t *sec,
        uint32_t total_sectors,
        uint32_t sec_per_clus,
        uint32_t reserved,
        uint32_t fat_length,
        uint32_t vol_id,
        uint32_t hidden_sectors)
{
    memset(sec, 0, PFS_FAT_MKFS_SECTOR_SIZE);
    sec[0] = 0xEB;
    sec[1] = 0xFE;
    sec[2] = 0x90;
    memcpy(sec + 3, "MSDOS5.0", 8);
    pfs_write_le16(sec + 11, (uint16_t)PFS_FAT_MKFS_SECTOR_SIZE);
    sec[13] = (uint8_t)sec_per_clus;
    pfs_write_le16(sec + 14, (uint16_t)reserved);
    sec[16] = (uint8_t)PFS_FAT_MKFS16_NUM_FATS;
    pfs_write_le16(sec + 17, (uint16_t)PFS_FAT_MKFS16_ROOT_ENTRIES);
    if (total_sectors < 0x10000U)
    {
        pfs_write_le16(sec + 19, (uint16_t)total_sectors);
        pfs_write_le32(sec + 32, 0);
    }
    else
    {
        pfs_write_le16(sec + 19, 0);
        pfs_write_le32(sec + 32, total_sectors);
    }
    sec[21] = (uint8_t)PFS_FAT_MKFS16_MEDIA;
    pfs_write_le16(sec + 22, (uint16_t)fat_length);
    pfs_write_le16(sec + 24, 63);
    pfs_write_le16(sec + 26, 255);
    pfs_write_le32(sec + 28, hidden_sectors);
    // FAT12/16 扩展 BPB @36
    sec[36] = 0x80;
    sec[37] = 0;
    sec[38] = (uint8_t)FAT_EXT_BOOT_SIG;
    pfs_write_le32(sec + 39, vol_id);
    memcpy(sec + 43, "NO NAME    ", 11);
    memcpy(sec + 54, "FAT16   ", 8);
    pfs_write_le16(sec + 510, (uint16_t)FAT_BOOT_SIGNATURE);
}

static int pfs_fat_mkfs16_write_fat(
        pfs_blkio_t *p_blkio,
        int64_t part_offset,
        uint32_t fat_start,
        uint32_t fat_length)
{
    uint8_t *p_sec = (uint8_t *)malloc(PFS_FAT_MKFS_SECTOR_SIZE);
    if (NULL == p_sec)
    {
        return PFS_FAT_RET_ENOMEM;
    }

    for (uint32_t fat_i = 0; fat_i < PFS_FAT_MKFS16_NUM_FATS; fat_i++)
    {
        uint32_t base = fat_start + fat_i * fat_length;

        for (uint32_t sec_i = 0; sec_i < fat_length; sec_i++)
        {
            memset(p_sec, 0, PFS_FAT_MKFS_SECTOR_SIZE);
            if (0 == sec_i)
            {
                // FAT[0]=media|EOC; FAT[1]=EOC (对齐 FatFs: 0xFFFFFFF8)
                pfs_write_le32(p_sec + 0, 0xFFFFFFF8U);
            }
            int rc = pfs_fat_mkfs_write_sector(p_blkio, part_offset, base + sec_i, p_sec);
            if (PFS_FAT_RET_OK != rc)
            {
                free(p_sec);
                return rc;
            }
        }
    }

    free(p_sec);
    return PFS_FAT_RET_OK;
}

int pfs_fat_mkfs16(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes)
{
    const uint32_t sector_size = PFS_FAT_MKFS_SECTOR_SIZE;

    if (NULL == p_blkio || 0 > part_offset)
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (!pfs_blkio_is_open(p_blkio))
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (0 != (part_offset % (int64_t)sector_size))
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (vol_bytes < (uint64_t)sector_size || 0U != (vol_bytes % sector_size))
    {
        return PFS_FAT_RET_EINVAL;
    }

    uint32_t total_sectors = (uint32_t)(vol_bytes / sector_size);
    if ((uint64_t)total_sectors != vol_bytes / sector_size)
    {
        return PFS_FAT_RET_EINVAL;
    }

    uint32_t sec_per_clus = 0;
    uint32_t reserved = 0;
    uint32_t fat_length = 0;
    uint32_t root_sectors = 0;
    uint32_t data_start = 0;
    uint32_t num_clusters = 0;
    int rc = pfs_fat_mkfs16_calc_layout(
            total_sectors,
            &sec_per_clus,
            &reserved,
            &fat_length,
            &root_sectors,
            &data_start,
            &num_clusters);
    if (PFS_FAT_RET_OK != rc)
    {
        return rc;
    }
    (void)num_clusters;
    (void)data_start;

    uint32_t hidden_sectors = (uint32_t)((uint64_t)part_offset / (uint64_t)sector_size);
    uint32_t vol_id = pfs_fat_mkfs_rand_serial();
    uint8_t boot[PFS_FAT_MKFS_SECTOR_SIZE];

    pfs_fat_mkfs16_pack_boot(
            boot,
            total_sectors,
            sec_per_clus,
            reserved,
            fat_length,
            vol_id,
            hidden_sectors);

    rc = pfs_fat_mkfs_write_sector(p_blkio, part_offset, 0, boot);
    if (PFS_FAT_RET_OK == rc)
    {
        rc = pfs_fat_mkfs16_write_fat(p_blkio, part_offset, reserved, fat_length);
    }
    if (PFS_FAT_RET_OK == rc)
    {
        rc = pfs_fat_mkfs_zero_sectors(
                p_blkio,
                part_offset,
                reserved + fat_length * PFS_FAT_MKFS16_NUM_FATS,
                root_sectors);
    }

    if (PFS_FAT_RET_OK == rc)
    {
        int sync_rc = pfs_blkio_fsync(p_blkio);
        if (sync_rc < 0)
        {
            rc = pfs_fat_mkfs_map_blkio_rc(sync_rc);
        }
    }
    return rc;
}

// end
