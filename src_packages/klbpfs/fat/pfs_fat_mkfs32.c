// Doc Encode : UTF-8 BOM, Unix(LF)
// 制作 FAT32 卷 (用户态 mkfs; 布局对齐 mkfs.fat / ChaN FatFs f_mkfs FAT32 路径)
#include "fat/pfs_fat_internal.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#define PFS_FAT_MKFS_RESERVED           32U
#define PFS_FAT_MKFS_INFO_SECTOR        1U
#define PFS_FAT_MKFS_BACKUP_BOOT        6U
#define PFS_FAT_MKFS_NUM_FATS           2U
#define PFS_FAT_MKFS_ROOT_CLUSTER       2U
#define PFS_FAT_MKFS_MEDIA              0xF8U
#define PFS_FAT_MKFS_MIN_SECTORS        128U

// FatFs f_mkfs FAT32 AU 边界表 (卷大小单位: 128K 扇区)
static const uint16_t g_pfs_fat_mkfs_cst32[] = { 1, 2, 4, 8, 16, 32, 0 };

uint32_t pfs_fat_mkfs_rand_serial(void)
{
    static bool seeded = false;

    if (!seeded)
    {
        srand((unsigned int)time(NULL));
        seeded = true;
    }
    uint32_t sig = ((uint32_t)rand() << 16) ^ (uint32_t)rand();
    if (0 == sig)
    {
        sig = (uint32_t)rand() | 0x80000000U;
    }
    return sig;
}

static uint32_t pfs_fat_mkfs_pick_sec_per_clus(uint32_t total_sectors)
{
    // 对齐 FatFs: n = sz_vol / 0x20000; pau 从 cst32 表翻倍
    uint32_t n = total_sectors / 0x20000U;
    uint32_t pau = 1U;
    for (int i = 0; 0 != g_pfs_fat_mkfs_cst32[i] && g_pfs_fat_mkfs_cst32[i] <= n; i++)
    {
        pau <<= 1;
    }
    if (pau > 128U)
    {
        pau = 128U;
    }
    return pau;
}

int pfs_fat_mkfs_map_blkio_rc(int rc)
{
    if (rc >= 0)
    {
        return PFS_FAT_RET_OK;
    }
    if (PFS_BLKIO_RET_PARAM == rc || PFS_BLKIO_RET_STATE == rc)
    {
        return PFS_FAT_RET_EINVAL;
    }
    return PFS_FAT_RET_EIO;
}

static int pfs_fat_mkfs_pwrite(
        pfs_blkio_t *p_blkio,
        int64_t part_offset,
        int64_t vol_byte_off,
        const void *buf,
        int size)
{

    if (NULL == p_blkio || NULL == buf || size <= 0 || 0 > vol_byte_off)
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (0 > part_offset)
    {
        return PFS_FAT_RET_EINVAL;
    }
    int rc = pfs_blkio_pwrite(p_blkio, buf, size, part_offset + vol_byte_off);
    if (0 > rc)
    {
        return pfs_fat_mkfs_map_blkio_rc(rc);
    }
    else if (rc != size)
    {
        return PFS_FAT_RET_EIO;
    }
    return PFS_FAT_RET_OK;
}

int pfs_fat_mkfs_write_sector(
        pfs_blkio_t *p_blkio,
        int64_t part_offset,
        uint32_t sector,
        const uint8_t *sec)
{
    return pfs_fat_mkfs_pwrite(
            p_blkio,
            part_offset,
            (int64_t)sector * (int64_t)PFS_FAT_MKFS_SECTOR_SIZE,
            sec,
            (int)PFS_FAT_MKFS_SECTOR_SIZE);
}

int pfs_fat_mkfs_zero_sectors(
        pfs_blkio_t *p_blkio,
        int64_t part_offset,
        uint32_t start_sector,
        uint32_t count)
{
    if (0 == count)
    {
        return PFS_FAT_RET_OK;
    }

    uint8_t sec[PFS_FAT_MKFS_SECTOR_SIZE];
    memset(sec, 0, sizeof(sec));
    for (uint32_t i = 0; i < count; i++)
    {
        int rc = pfs_fat_mkfs_write_sector(p_blkio, part_offset, start_sector + i, sec);
        if (PFS_FAT_RET_OK != rc)
        {
            return rc;
        }
    }
    return PFS_FAT_RET_OK;
}

// 计算 FAT32 几何; 输出主机序布局参数
static int pfs_fat_mkfs_calc_layout(
        uint32_t total_sectors,
        uint32_t *p_sec_per_clus,
        uint32_t *p_reserved,
        uint32_t *p_fat_length,
        uint32_t *p_data_start,
        uint32_t *p_num_clusters)
{
    if (NULL == p_sec_per_clus || NULL == p_reserved || NULL == p_fat_length ||
            NULL == p_data_start || NULL == p_num_clusters)
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (total_sectors < PFS_FAT_MKFS_MIN_SECTORS)
    {
        return PFS_FAT_RET_EINVAL;
    }

    uint32_t sec_per_clus = pfs_fat_mkfs_pick_sec_per_clus(total_sectors);
    uint32_t reserved = PFS_FAT_MKFS_RESERVED;
    uint32_t fat_length = 1U;

    for (int i = 0; i < 64; i++)
    {
        uint32_t data_start = reserved + fat_length * PFS_FAT_MKFS_NUM_FATS;
        if (data_start >= total_sectors)
        {
            return PFS_FAT_RET_EINVAL;
        }
        uint32_t num_clusters = (total_sectors - data_start) / sec_per_clus;
        if (num_clusters < 2U)
        {
            return PFS_FAT_RET_EINVAL;
        }
        if (num_clusters > MAX_FAT32)
        {
            // 簇过大, 加大每簇扇区数重试
            if (sec_per_clus >= 128U)
            {
                return PFS_FAT_RET_EINVAL;
            }
            sec_per_clus <<= 1;
            fat_length = 1U;
            continue;
        }
        // FAT 扇区数: (簇数+2) 项 * 4 字节, 对齐 FatFs (n_clst*4+8)
        uint32_t need_fat = (uint32_t)(((uint64_t)num_clusters * 4U + 8U + PFS_FAT_MKFS_SECTOR_SIZE - 1U) /
                              PFS_FAT_MKFS_SECTOR_SIZE);
        if (fat_length >= need_fat)
        {
            *p_sec_per_clus = sec_per_clus;
            *p_reserved = reserved;
            *p_fat_length = fat_length;
            *p_data_start = data_start;
            *p_num_clusters = num_clusters;
            return PFS_FAT_RET_OK;
        }
        fat_length = need_fat;
    }
    return PFS_FAT_RET_EINVAL;
}

static void pfs_fat_mkfs_pack_boot(
        uint8_t *sec,
        uint32_t total_sectors,
        uint32_t sec_per_clus,
        uint32_t reserved,
        uint32_t fat_length,
        uint32_t root_cluster,
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
    sec[16] = (uint8_t)PFS_FAT_MKFS_NUM_FATS;
    pfs_write_le16(sec + 17, 0);                        // root entries = 0 (FAT32)
    pfs_write_le16(sec + 19, 0);                        // totsec16 = 0
    sec[21] = (uint8_t)PFS_FAT_MKFS_MEDIA;
    pfs_write_le16(sec + 22, 0);                        // fat_length16 = 0 (FAT32)
    pfs_write_le16(sec + 24, 63);                       // secs_track
    pfs_write_le16(sec + 26, 255);                      // heads
    pfs_write_le32(sec + 28, hidden_sectors);
    pfs_write_le32(sec + 32, total_sectors);
    // FAT32 扩展 BPB @36
    pfs_write_le32(sec + 36, fat_length);
    pfs_write_le16(sec + 40, 0);                        // flags
    sec[42] = 0;                                        // version major
    sec[43] = 0;                                        // version minor
    pfs_write_le32(sec + 44, root_cluster);
    pfs_write_le16(sec + 48, (uint16_t)PFS_FAT_MKFS_INFO_SECTOR);
    pfs_write_le16(sec + 50, (uint16_t)PFS_FAT_MKFS_BACKUP_BOOT);
    sec[64] = 0x80;                                     // drive_number
    sec[65] = 0;                                        // state
    sec[66] = (uint8_t)FAT_EXT_BOOT_SIG;
    pfs_write_le32(sec + 67, vol_id);
    memcpy(sec + 71, "NO NAME    ", 11);
    memcpy(sec + 82, "FAT32   ", 8);
    pfs_write_le16(sec + 510, (uint16_t)FAT_BOOT_SIGNATURE);
}

static void pfs_fat_mkfs_pack_fsinfo(uint8_t *sec, uint32_t free_clusters, uint32_t next_cluster)
{
    memset(sec, 0, PFS_FAT_MKFS_SECTOR_SIZE);
    pfs_write_le32(sec + 0, FAT_FSINFO_SIG1);
    pfs_write_le32(sec + 484, FAT_FSINFO_SIG2);
    pfs_write_le32(sec + 488, free_clusters);
    pfs_write_le32(sec + 492, next_cluster);
    pfs_write_le16(sec + 510, (uint16_t)FAT_BOOT_SIGNATURE);
}

static int pfs_fat_mkfs_write_fat(
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

    for (uint32_t fat_i = 0; fat_i < PFS_FAT_MKFS_NUM_FATS; fat_i++)
    {
        uint32_t base = fat_start + fat_i * fat_length;

        for (uint32_t sec_i = 0; sec_i < fat_length; sec_i++)
        {
            memset(p_sec, 0, PFS_FAT_MKFS_SECTOR_SIZE);
            if (0 == sec_i)
            {
                // FAT[0]=media|EOC 风格; FAT[1]=EOC; FAT[2]=根目录 EOC (对齐 FatFs)
                pfs_write_le32(p_sec + 0, 0xFFFFFFF8U);
                pfs_write_le32(p_sec + 4, 0xFFFFFFFFU);
                pfs_write_le32(p_sec + 8, (uint32_t)EOF_FAT32);
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

int pfs_fat_mkfs32(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes)
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
    uint32_t data_start = 0;
    uint32_t num_clusters = 0;
    int rc = pfs_fat_mkfs_calc_layout(
            total_sectors,
            &sec_per_clus,
            &reserved,
            &fat_length,
            &data_start,
            &num_clusters);
    if (PFS_FAT_RET_OK != rc)
    {
        return rc;
    }

    uint32_t hidden_sectors = (uint32_t)((uint64_t)part_offset / (uint64_t)sector_size);
    uint32_t vol_id = pfs_fat_mkfs_rand_serial();
    uint32_t free_clusters = num_clusters - 1U;         // 根目录占用 1 簇
    uint8_t boot[PFS_FAT_MKFS_SECTOR_SIZE];
    uint8_t fsinfo[PFS_FAT_MKFS_SECTOR_SIZE];

    pfs_fat_mkfs_pack_boot(
            boot,
            total_sectors,
            sec_per_clus,
            reserved,
            fat_length,
            PFS_FAT_MKFS_ROOT_CLUSTER,
            vol_id,
            hidden_sectors);
    pfs_fat_mkfs_pack_fsinfo(fsinfo, free_clusters, PFS_FAT_MKFS_ROOT_CLUSTER);

    // 主 boot + FSINFO
    rc = pfs_fat_mkfs_write_sector(p_blkio, part_offset, 0, boot);
    if (PFS_FAT_RET_OK == rc)
    {
        rc = pfs_fat_mkfs_write_sector(p_blkio, part_offset, PFS_FAT_MKFS_INFO_SECTOR, fsinfo);
    }
    // 保留区中间清零 (2..5)
    if (PFS_FAT_RET_OK == rc)
    {
        rc = pfs_fat_mkfs_zero_sectors(p_blkio, part_offset, 2, PFS_FAT_MKFS_BACKUP_BOOT - 2U);
    }
    // 备份 boot + FSINFO (sector 6/7)
    if (PFS_FAT_RET_OK == rc)
    {
        rc = pfs_fat_mkfs_write_sector(p_blkio, part_offset, PFS_FAT_MKFS_BACKUP_BOOT, boot);
    }
    if (PFS_FAT_RET_OK == rc)
    {
        rc = pfs_fat_mkfs_write_sector(
                p_blkio, part_offset, PFS_FAT_MKFS_BACKUP_BOOT + 1U, fsinfo);
    }
    // 其余保留扇区清零 (8..reserved-1)
    if (PFS_FAT_RET_OK == rc && reserved > PFS_FAT_MKFS_BACKUP_BOOT + 2U)
    {
        rc = pfs_fat_mkfs_zero_sectors(
                p_blkio,
                part_offset,
                PFS_FAT_MKFS_BACKUP_BOOT + 2U,
                reserved - (PFS_FAT_MKFS_BACKUP_BOOT + 2U));
    }
    // FAT1/FAT2
    if (PFS_FAT_RET_OK == rc)
    {
        rc = pfs_fat_mkfs_write_fat(p_blkio, part_offset, reserved, fat_length);
    }
    // 根目录簇清零
    if (PFS_FAT_RET_OK == rc)
    {
        rc = pfs_fat_mkfs_zero_sectors(p_blkio, part_offset, data_start, sec_per_clus);
    }

    if (PFS_FAT_RET_OK == rc)
    {
        int sync_rc = pfs_blkio_fsync(p_blkio);
        if (0 > sync_rc)
        {
            rc = pfs_fat_mkfs_map_blkio_rc(sync_rc);
        }
    }
    return rc;
}

// end
