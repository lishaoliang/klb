// Doc Encode : UTF-8 BOM, Unix(LF)
// 制作 exFAT 卷 (用户态 mkfs; 布局对齐常见 mkfs.exfat / linux exFAT 几何)
#include "exfat/pfs_exfat_internal.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PFS_EXFAT_MKFS_BOOT_REGION_SECTORS  12U
#define PFS_EXFAT_MKFS_BACKUP_BOOT_SECTOR   12U
#define PFS_EXFAT_MKFS_FAT_OFFSET           128U
#define PFS_EXFAT_MKFS_BITMAP_CLUSTER       2U
// 根目录系统项: Volume + Bitmap + Upcase (规范允许 0~1 个 Volume GUID; 不写 GUID 以免 null GUID/checksum 被 chkdsk 判坏)
#define PFS_EXFAT_MKFS_ROOT_DENTRY_CNT      3U

static uint32_t pfs_exfat_mkfs_chksum32(const uint8_t *data, int len, uint32_t chksum, int type)
{

    if (NULL == data || len <= 0)
    {
        return chksum;
    }
    for (int i = 0; i < len; i++)
    {
        if (EXFAT_CS_BOOT_SECTOR == type && (106 == i || 107 == i || 112 == i))
        {
            continue;
        }
        chksum = ((chksum << 31) | (chksum >> 1)) + data[i];
    }
    return chksum;
}

static uint32_t pfs_exfat_mkfs_upcase_checksum(void)
{
    return pfs_exfat_mkfs_chksum32(
            (const uint8_t *)g_pfs_exfat_def_upcase,
            PFS_EXFAT_NUM_UPCASE * (int)sizeof(uint16_t),
            0,
            EXFAT_CS_DEFAULT);
}

static uint32_t pfs_exfat_mkfs_rand_serial(void)
{
    static int seeded = 0;

    if (0 == seeded)
    {
        srand((unsigned int)time(NULL));
        seeded = 1;
    }
    uint32_t sig = ((uint32_t)rand() << 16) ^ (uint32_t)rand();
    if (0 == sig)
    {
        sig = (uint32_t)rand() | 0x80000000U;
    }
    return sig;
}

static uint8_t pfs_exfat_mkfs_pick_sect_per_clus_bits(uint64_t vol_length)
{
    if (vol_length >= ((uint64_t)32 * 1024 * 1024 * 1024) / 512)
    {
        return 8;
    }
    if (vol_length >= ((uint64_t)4 * 1024 * 1024 * 1024) / 512)
    {
        return 7;
    }
    return 3;
}

static uint32_t pfs_exfat_mkfs_clu_count_for_bytes(uint32_t bytes, uint32_t cluster_size)
{
    if (0 == bytes || 0 == cluster_size)
    {
        return 0;
    }
    return (bytes + cluster_size - 1U) / cluster_size;
}

static int pfs_exfat_mkfs_calc_layout(
        uint64_t vol_length,
        uint8_t sect_size_bits,
        uint8_t sect_per_clus_bits,
        uint8_t num_fats,
        uint32_t fat_offset,
        uint32_t *p_fat_length,
        uint32_t *p_clu_offset,
        uint32_t *p_clu_count)
{
    if (NULL == p_fat_length || NULL == p_clu_offset || NULL == p_clu_count)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    uint32_t sector_size = 1U << sect_size_bits;
    uint32_t sect_per_clus = 1U << sect_per_clus_bits;
    uint32_t fat_length = 8;

    if (vol_length <= (uint64_t)fat_offset + (uint64_t)sect_per_clus)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    for (int i = 0; i < 64; i++)
    {
        uint32_t clu_offset = fat_offset + fat_length * num_fats;
        uint32_t pad = (sect_per_clus - (clu_offset % sect_per_clus)) % sect_per_clus;
        clu_offset += pad;
        if ((uint64_t)clu_offset >= vol_length)
        {
            return PFS_EXFAT_RET_EINVAL;
        }
        uint32_t clu_count = (uint32_t)((vol_length - clu_offset) / sect_per_clus);
        uint32_t num_clusters = clu_count + EXFAT_RESERVED_CLUSTERS;
        uint32_t need_fat = (uint32_t)(((uint64_t)num_clusters * 4U + sector_size - 1U) / sector_size);
        if (fat_length >= need_fat)
        {
            *p_fat_length = fat_length;
            *p_clu_offset = clu_offset;
            *p_clu_count = clu_count;
            return PFS_EXFAT_RET_OK;
        }
        fat_length = need_fat;
    }
    return PFS_EXFAT_RET_EINVAL;
}

static int64_t pfs_exfat_mkfs_cluster_lba(uint32_t clu_offset, uint32_t sect_per_clus, uint32_t cluster)
{
    return (int64_t)clu_offset + (int64_t)(cluster - EXFAT_FIRST_CLUSTER) * (int64_t)sect_per_clus;
}

static int pfs_exfat_mkfs_map_blkio_rc(int rc)
{
    if (rc >= 0)
    {
        return PFS_EXFAT_RET_OK;
    }
    if (PFS_BLKIO_RET_PARAM == rc || PFS_BLKIO_RET_STATE == rc)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    return PFS_EXFAT_RET_EIO;
}

// vol_byte_off: 相对卷首的字节偏移
static int pfs_exfat_mkfs_pwrite(
        pfs_blkio_t *p_blkio,
        int64_t part_offset,
        int64_t vol_byte_off,
        const void *buf,
        int size)
{

    if (NULL == p_blkio || NULL == buf || 0 >= size || 0 > vol_byte_off)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (0 > part_offset)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    int rc = pfs_blkio_pwrite(p_blkio, buf, size, part_offset + vol_byte_off);
    if (0 > rc)
    {
        return pfs_exfat_mkfs_map_blkio_rc(rc);
    }
    else if (rc != size)
    {
        return PFS_EXFAT_RET_EIO;
    }
    return PFS_EXFAT_RET_OK;
}

static void pfs_exfat_mkfs_pack_boot(uint8_t *sec, const exfat_boot_sector_t *p_host)
{
    exfat_boot_sector_t le;

    memset(sec, 0, 512);
    le = *p_host;
    le.partition_offset = pfs_cpu_to_le64(p_host->partition_offset);
    le.vol_length = pfs_cpu_to_le64(p_host->vol_length);
    le.fat_offset = pfs_cpu_to_le32(p_host->fat_offset);
    le.fat_length = pfs_cpu_to_le32(p_host->fat_length);
    le.clu_offset = pfs_cpu_to_le32(p_host->clu_offset);
    le.clu_count = pfs_cpu_to_le32(p_host->clu_count);
    le.root_cluster = pfs_cpu_to_le32(p_host->root_cluster);
    le.vol_serial = pfs_cpu_to_le32(p_host->vol_serial);
    le.vol_flags = pfs_cpu_to_le16(p_host->vol_flags);
    le.signature = pfs_cpu_to_le16(p_host->signature);
    memcpy(sec, &le, sizeof(le));
}

static int pfs_exfat_mkfs_write_boot_region_one(
        pfs_blkio_t *p_blkio,
        int64_t part_offset,
        int base_sector,
        const exfat_boot_sector_t *p_bs)
{
    uint8_t sec[512];

    if (NULL == p_bs || 0 > base_sector)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    int64_t base_off = (int64_t)base_sector * 512;
    pfs_exfat_mkfs_pack_boot(sec, p_bs);
    int rc = pfs_exfat_mkfs_pwrite(p_blkio, part_offset, base_off, sec, (int)sizeof(sec));
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }
    uint32_t chksum = pfs_exfat_mkfs_chksum32(sec, (int)sizeof(sec), 0, EXFAT_CS_BOOT_SECTOR);
    for (int sn = 1; sn <= 8; sn++)
    {
        memset(sec, 0, sizeof(sec));
        sec[510] = 0x55;
        sec[511] = 0xAA;
        rc = pfs_exfat_mkfs_pwrite(
                p_blkio, part_offset, base_off + (int64_t)sn * 512, sec, (int)sizeof(sec));
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }
        chksum = pfs_exfat_mkfs_chksum32(sec, (int)sizeof(sec), chksum, EXFAT_CS_DEFAULT);
    }
    memset(sec, 0, sizeof(sec));
    rc = pfs_exfat_mkfs_pwrite(p_blkio, part_offset, base_off + 9LL * 512, sec, (int)sizeof(sec));
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }
    chksum = pfs_exfat_mkfs_chksum32(sec, (int)sizeof(sec), chksum, EXFAT_CS_DEFAULT);
    rc = pfs_exfat_mkfs_pwrite(p_blkio, part_offset, base_off + 10LL * 512, sec, (int)sizeof(sec));
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }
    chksum = pfs_exfat_mkfs_chksum32(sec, (int)sizeof(sec), chksum, EXFAT_CS_DEFAULT);
    memset(sec, 0, sizeof(sec));
    for (int i = 0; i < (int)sizeof(sec); i += 4)
    {
        sec[i] = (uint8_t)(chksum & 0xFF);
        sec[i + 1] = (uint8_t)((chksum >> 8) & 0xFF);
        sec[i + 2] = (uint8_t)((chksum >> 16) & 0xFF);
        sec[i + 3] = (uint8_t)((chksum >> 24) & 0xFF);
    }
    return pfs_exfat_mkfs_pwrite(p_blkio, part_offset, base_off + 11LL * 512, sec, (int)sizeof(sec));
}

static int pfs_exfat_mkfs_write_boot_region(
        pfs_blkio_t *p_blkio,
        int64_t part_offset,
        const exfat_boot_sector_t *p_bs)
{

    int rc = pfs_exfat_mkfs_write_boot_region_one(p_blkio, part_offset, 0, p_bs);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }
    return pfs_exfat_mkfs_write_boot_region_one(
            p_blkio, part_offset, (int)PFS_EXFAT_MKFS_BACKUP_BOOT_SECTOR, p_bs);
}

static int pfs_exfat_mkfs_zero_sectors(
        pfs_blkio_t *p_blkio,
        int64_t part_offset,
        uint32_t start_sector,
        uint32_t end_sector,
        uint32_t sector_size)
{
    uint8_t sec[512];

    if (sector_size > sizeof(sec) || start_sector >= end_sector)
    {
        return PFS_EXFAT_RET_OK;
    }
    memset(sec, 0, sizeof(sec));
    for (uint32_t sn = start_sector; sn < end_sector; sn++)
    {
        int rc = pfs_exfat_mkfs_pwrite(
                p_blkio,
                part_offset,
                (int64_t)sn * (int64_t)sector_size,
                sec,
                (int)sector_size);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }
    }
    return PFS_EXFAT_RET_OK;
}

static void pfs_exfat_mkfs_fat_put_u32(uint8_t *p_fat, uint32_t cluster, uint32_t value)
{
    pfs_write_le32(p_fat + (size_t)cluster * 4U, value);
}

static void pfs_exfat_mkfs_fat_put_chain(uint8_t *p_fat, uint32_t start_clu, uint32_t clu_cnt)
{

    if (NULL == p_fat || 0 == clu_cnt)
    {
        return;
    }
    for (uint32_t clu = start_clu; clu < start_clu + clu_cnt - 1U; clu++)
    {
        pfs_exfat_mkfs_fat_put_u32(p_fat, clu, clu + 1U);
    }
    pfs_exfat_mkfs_fat_put_u32(p_fat, start_clu + clu_cnt - 1U, EXFAT_EOF_CLUSTER);
}

static int pfs_exfat_mkfs_write_fat_table(
        pfs_blkio_t *p_blkio,
        int64_t part_offset,
        uint32_t fat_offset,
        uint32_t fat_length,
        uint32_t sector_size,
        uint32_t bitmap_cluster,
        uint32_t bitmap_clu_cnt,
        uint32_t upcase_cluster,
        uint32_t upcase_clu_cnt,
        uint32_t root_cluster,
        uint32_t root_clu_cnt)
{
    if (0 == fat_length || 0 == sector_size ||
            0 == bitmap_clu_cnt || 0 == upcase_clu_cnt || 0 == root_clu_cnt)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    uint32_t fat_bytes = fat_length * sector_size;
    uint8_t *p_fat = (uint8_t *)calloc(1, fat_bytes);
    if (NULL == p_fat)
    {
        return PFS_EXFAT_RET_ENOMEM;
    }
    pfs_exfat_mkfs_fat_put_u32(p_fat, 0, 0xFFFFFFF8U);
    pfs_exfat_mkfs_fat_put_u32(p_fat, 1, 0xFFFFFFFFU);
    pfs_exfat_mkfs_fat_put_chain(p_fat, bitmap_cluster, bitmap_clu_cnt);
    pfs_exfat_mkfs_fat_put_chain(p_fat, upcase_cluster, upcase_clu_cnt);
    pfs_exfat_mkfs_fat_put_chain(p_fat, root_cluster, root_clu_cnt);
    for (uint32_t sn = 0; sn < fat_length; sn++)
    {
        int rc = pfs_exfat_mkfs_pwrite(
                p_blkio,
                part_offset,
                ((int64_t)fat_offset + (int64_t)sn) * (int64_t)sector_size,
                p_fat + (size_t)sn * sector_size,
                (int)sector_size);
        if (PFS_EXFAT_RET_OK != rc)
        {
            free(p_fat);
            return rc;
        }
    }
    free(p_fat);
    return PFS_EXFAT_RET_OK;
}

static void pfs_exfat_mkfs_bitmap_set_range(
        uint8_t *buf,
        uint32_t bitmap_size,
        uint32_t start_clu,
        uint32_t clu_cnt)
{

    for (uint32_t clu = start_clu; clu < start_clu + clu_cnt; clu++)
    {
        uint32_t bit = clu - EXFAT_RESERVED_CLUSTERS;
        if (bit / 8U < bitmap_size)
        {
            buf[bit / 8U] |= (uint8_t)(1U << (bit % 8U));
        }
    }
}

static int pfs_exfat_mkfs_write_cluster(
        pfs_blkio_t *p_blkio,
        int64_t part_offset,
        int64_t lba,
        uint32_t sector_size,
        uint32_t sect_per_clus,
        const void *data,
        uint32_t data_len)
{
    uint32_t cluster_size = sector_size * sect_per_clus;

    if (0 == sector_size || 0 == sect_per_clus || data_len > cluster_size)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    uint8_t *p_buf = (uint8_t *)calloc(1, cluster_size);
    if (NULL == p_buf)
    {
        return PFS_EXFAT_RET_ENOMEM;
    }
    if (NULL != data && data_len > 0)
    {
        memcpy(p_buf, data, data_len);
    }
    int rc = pfs_exfat_mkfs_pwrite(
            p_blkio, part_offset, lba * (int64_t)sector_size, p_buf, (int)cluster_size);
    free(p_buf);
    return rc;
}

static int pfs_exfat_mkfs_write_upcase_cluster(
        pfs_blkio_t *p_blkio,
        int64_t part_offset,
        int64_t lba,
        uint32_t sector_size,
        uint32_t sect_per_clus)
{
    uint32_t upcase_bytes = (uint32_t)PFS_EXFAT_NUM_UPCASE * (uint32_t)sizeof(uint16_t);
    uint32_t cluster_size = sector_size * sect_per_clus;

    if (0 == sector_size || 0 == sect_per_clus || 0 == cluster_size)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    uint32_t clu_cnt = pfs_exfat_mkfs_clu_count_for_bytes(upcase_bytes, cluster_size);
    if (0 == clu_cnt)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    uint8_t *p_le = (uint8_t *)malloc(upcase_bytes);
    if (NULL == p_le)
    {
        return PFS_EXFAT_RET_ENOMEM;
    }
    for (uint32_t i = 0; i < (uint32_t)PFS_EXFAT_NUM_UPCASE; i++)
    {
        pfs_write_le16(p_le + (size_t)i * 2U, g_pfs_exfat_def_upcase[i]);
    }

    for (uint32_t clu = 0; clu < clu_cnt; clu++)
    {
        uint32_t byte_off = clu * cluster_size;
        uint32_t chunk = upcase_bytes - byte_off;

        if (chunk > cluster_size)
        {
            chunk = cluster_size;
        }
        int rc = pfs_exfat_mkfs_write_cluster(
                p_blkio,
                part_offset,
                lba + (int64_t)clu * (int64_t)sect_per_clus,
                sector_size,
                sect_per_clus,
                p_le + byte_off,
                chunk);
        if (PFS_EXFAT_RET_OK != rc)
        {
            free(p_le);
            return rc;
        }
    }

    free(p_le);
    return PFS_EXFAT_RET_OK;
}

static int pfs_exfat_mkfs_write_root_dir(
        pfs_blkio_t *p_blkio,
        int64_t part_offset,
        int64_t root_lba,
        uint32_t sector_size,
        uint32_t sect_per_clus,
        uint32_t bitmap_cluster,
        uint32_t bitmap_size,
        uint32_t upcase_cluster,
        uint32_t upcase_size,
        uint32_t upcase_chksum)
{
    uint32_t cluster_size = sector_size * sect_per_clus;
    uint32_t dentries_len = PFS_EXFAT_MKFS_ROOT_DENTRY_CNT * EXFAT_DENTRY_SIZE;

    if (0 == sector_size || 0 == sect_per_clus || cluster_size < dentries_len)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    uint8_t *p_buf = (uint8_t *)calloc(1, cluster_size);
    if (NULL == p_buf)
    {
        return PFS_EXFAT_RET_ENOMEM;
    }
    exfat_dentry_t *p_vol = (exfat_dentry_t *)p_buf;
    exfat_dentry_t *p_bitmap = (exfat_dentry_t *)(p_buf + EXFAT_DENTRY_SIZE);
    exfat_dentry_t *p_upcase = (exfat_dentry_t *)(p_buf + 2 * EXFAT_DENTRY_SIZE);

    p_vol->type = EXFAT_VOLUME;
    p_vol->dentry.volume_label.char_count = 0;

    p_bitmap->type = EXFAT_BITMAP;
    p_bitmap->dentry.bitmap.flags = 0;
    p_bitmap->dentry.bitmap.start_clu = pfs_cpu_to_le32(bitmap_cluster);
    p_bitmap->dentry.bitmap.size = pfs_cpu_to_le64((uint64_t)bitmap_size);

    p_upcase->type = EXFAT_UPCASE;
    p_upcase->dentry.upcase.checksum = pfs_cpu_to_le32(upcase_chksum);
    p_upcase->dentry.upcase.start_clu = pfs_cpu_to_le32(upcase_cluster);
    p_upcase->dentry.upcase.size = pfs_cpu_to_le64((uint64_t)upcase_size);

    int rc = pfs_exfat_mkfs_pwrite(
            p_blkio, part_offset, root_lba * (int64_t)sector_size, p_buf, (int)cluster_size);
    free(p_buf);
    return rc;
}

int pfs_exfat_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes)
{
    const uint8_t sect_size_bits = 9;
    const uint8_t num_fats = 1;
    const uint32_t sector_size = 1U << sect_size_bits;
    const uint32_t fat_offset = PFS_EXFAT_MKFS_FAT_OFFSET;
    const uint32_t bitmap_cluster = PFS_EXFAT_MKFS_BITMAP_CLUSTER;
    const uint32_t upcase_size = (uint32_t)PFS_EXFAT_NUM_UPCASE * (uint32_t)sizeof(uint16_t);

    if (NULL == p_blkio || 0 > part_offset)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (!pfs_blkio_is_open(p_blkio))
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (0 != (part_offset % (int64_t)sector_size))
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (vol_bytes < (uint64_t)sector_size || 0U != (vol_bytes % sector_size))
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    uint64_t vol_length = vol_bytes / sector_size;
    uint8_t sect_per_clus_bits = pfs_exfat_mkfs_pick_sect_per_clus_bits(vol_length);
    uint32_t sect_per_clus = 1U << sect_per_clus_bits;
    uint32_t fat_length = 0;
    uint32_t clu_offset = 0;
    uint32_t clu_count = 0;
    int rc = pfs_exfat_mkfs_calc_layout(
            vol_length,
            sect_size_bits,
            sect_per_clus_bits,
            num_fats,
            fat_offset,
            &fat_length,
            &clu_offset,
            &clu_count);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }

    uint32_t cluster_size = sector_size * sect_per_clus;
    uint32_t bitmap_size = (clu_count + 7U) / 8U;
    uint32_t bitmap_clu_cnt = pfs_exfat_mkfs_clu_count_for_bytes(bitmap_size, cluster_size);
    uint32_t upcase_clu_cnt = pfs_exfat_mkfs_clu_count_for_bytes(upcase_size, cluster_size);
    uint32_t root_clu_cnt = pfs_exfat_mkfs_clu_count_for_bytes(
            PFS_EXFAT_MKFS_ROOT_DENTRY_CNT * EXFAT_DENTRY_SIZE, cluster_size);
    if (0 == bitmap_clu_cnt || 0 == upcase_clu_cnt || 0 == root_clu_cnt)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    uint32_t upcase_cluster = bitmap_cluster + bitmap_clu_cnt;
    uint32_t root_cluster = upcase_cluster + upcase_clu_cnt;
    if (root_cluster + root_clu_cnt - 1U > clu_count + 1U)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (bitmap_size > bitmap_clu_cnt * cluster_size || upcase_size > upcase_clu_cnt * cluster_size)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    uint32_t upcase_chksum = pfs_exfat_mkfs_upcase_checksum();
    if (PFS_EXFAT_UPCASE_CHECKSUM != upcase_chksum)
    {
        return PFS_EXFAT_RET_EBADMSG;
    }

    uint8_t *p_bitmap = (uint8_t *)malloc(bitmap_size);
    if (NULL == p_bitmap)
    {
        return PFS_EXFAT_RET_ENOMEM;
    }
    memset(p_bitmap, 0, bitmap_size);
    pfs_exfat_mkfs_bitmap_set_range(p_bitmap, bitmap_size, bitmap_cluster, bitmap_clu_cnt);
    pfs_exfat_mkfs_bitmap_set_range(p_bitmap, bitmap_size, upcase_cluster, upcase_clu_cnt);
    pfs_exfat_mkfs_bitmap_set_range(p_bitmap, bitmap_size, root_cluster, root_clu_cnt);

    int64_t bitmap_lba = pfs_exfat_mkfs_cluster_lba(clu_offset, sect_per_clus, bitmap_cluster);
    int64_t upcase_lba = pfs_exfat_mkfs_cluster_lba(clu_offset, sect_per_clus, upcase_cluster);
    int64_t root_lba = pfs_exfat_mkfs_cluster_lba(clu_offset, sect_per_clus, root_cluster);

    exfat_boot_sector_t bs;
    memset(&bs, 0, sizeof(bs));
    bs.jmp_boot[0] = 0xEB;
    bs.jmp_boot[1] = 0x76;
    bs.jmp_boot[2] = 0x90;
    memcpy(bs.fs_name, STR_EXFAT, EXFAT_BOOTSEC_FS_NAME_LEN);
    bs.partition_offset = (uint64_t)part_offset / (uint64_t)sector_size;
    bs.vol_length = vol_length;
    bs.fat_offset = fat_offset;
    bs.fat_length = fat_length;
    bs.clu_offset = clu_offset;
    bs.clu_count = clu_count;
    bs.root_cluster = root_cluster;
    bs.vol_serial = pfs_exfat_mkfs_rand_serial();
    bs.fs_revision[0] = 0;
    bs.fs_revision[1] = 1;
    bs.vol_flags = 0;
    bs.sect_size_bits = sect_size_bits;
    bs.sect_per_clus_bits = sect_per_clus_bits;
    bs.num_fats = num_fats;
    bs.drv_sel = 0x80;
    bs.percent_in_use = 0;
    bs.signature = EXFAT_BOOT_SIGNATURE;

    rc = pfs_exfat_mkfs_write_boot_region(p_blkio, part_offset, &bs);
    if (PFS_EXFAT_RET_OK == rc && fat_offset > PFS_EXFAT_MKFS_BOOT_REGION_SECTORS * 2U)
    {
        rc = pfs_exfat_mkfs_zero_sectors(
                p_blkio,
                part_offset,
                PFS_EXFAT_MKFS_BOOT_REGION_SECTORS * 2U,
                fat_offset,
                sector_size);
    }
    if (PFS_EXFAT_RET_OK == rc)
    {
        rc = pfs_exfat_mkfs_write_fat_table(
                p_blkio,
                part_offset,
                fat_offset,
                fat_length,
                sector_size,
                bitmap_cluster,
                bitmap_clu_cnt,
                upcase_cluster,
                upcase_clu_cnt,
                root_cluster,
                root_clu_cnt);
    }
    if (PFS_EXFAT_RET_OK == rc)
    {
        rc = pfs_exfat_mkfs_write_cluster(
                p_blkio,
                part_offset,
                bitmap_lba,
                sector_size,
                sect_per_clus,
                p_bitmap,
                bitmap_size);
    }
    if (PFS_EXFAT_RET_OK == rc)
    {
        rc = pfs_exfat_mkfs_write_upcase_cluster(
                p_blkio, part_offset, upcase_lba, sector_size, sect_per_clus);
    }
    if (PFS_EXFAT_RET_OK == rc)
    {
        rc = pfs_exfat_mkfs_write_root_dir(
                p_blkio,
                part_offset,
                root_lba,
                sector_size,
                sect_per_clus,
                bitmap_cluster,
                bitmap_size,
                upcase_cluster,
                upcase_size,
                upcase_chksum);
    }

    free(p_bitmap);
    if (PFS_EXFAT_RET_OK == rc)
    {
        int sync_rc = pfs_blkio_fsync(p_blkio);
        if (sync_rc < 0)
        {
            rc = pfs_exfat_mkfs_map_blkio_rc(sync_rc);
        }
    }
    return rc;
}

// end
