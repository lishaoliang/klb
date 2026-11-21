// Doc Encode : UTF-8 BOM, Unix(LF)
// boot / mount / probe (linux-7.1 fs/fat/inode.c fat_fill_super / fat_read_bpb)
#include "fat/pfs_fat_internal.h"

#include <string.h>

static int pfs_fat_is_power_of_2(unsigned int n)
{
    return (0 != n && 0 == (n & (n - 1U))) ? 1 : 0;
}

static uint32_t pfs_fat_log2_u32(uint32_t v)
{
    uint32_t bits = 0;

    while (v > 1U)
    {
        v >>= 1;
        bits++;
    }
    return bits;
}

static int pfs_fat_map_blkio_rc(int rc)
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

static int pfs_fat_read_boot_sec(pfs_blkio_t *p_blkio, int64_t part_offset, uint8_t *sec)
{

    if (NULL == p_blkio || NULL == sec || part_offset < 0)
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (!pfs_blkio_is_open(p_blkio))
    {
        return PFS_FAT_RET_EBUSY;
    }

    int rc = pfs_blkio_pread(p_blkio, sec, (int)FAT_SECTOR_SIZE, part_offset);
    if (0 > rc)
    {
        return pfs_fat_map_blkio_rc(rc);
    }
    else if (rc != (int)FAT_SECTOR_SIZE)
    {
        return PFS_FAT_RET_EIO;
    }
    return PFS_FAT_RET_OK;
}

// 从 boot 扇区解析 BPB 与几何; 成功后 p_bs 为主机序
static int pfs_fat_parse_bpb(pfs_fat_bs_t *p_bs, const uint8_t *sec)
{
    if (NULL == p_bs || NULL == sec)
    {
        return PFS_FAT_RET_EINVAL;
    }

    memset(p_bs, 0, sizeof(*p_bs));
    uint16_t boot_sig = pfs_read_le16(sec + 510);
    uint16_t sector_size = pfs_read_le16(sec + 11);
    uint8_t sec_per_clus = sec[13];
    uint16_t reserved = pfs_read_le16(sec + 14);
    uint8_t fats = sec[16];
    uint16_t dir_entries = pfs_read_le16(sec + 17);
    uint16_t sectors16 = pfs_read_le16(sec + 19);
    uint8_t media = sec[21];
    uint16_t fat_length16 = pfs_read_le16(sec + 22);
    uint32_t total_sect32 = pfs_read_le32(sec + 32);
    uint32_t fat32_length = pfs_read_le32(sec + 36);
    uint16_t fat32_flags = pfs_read_le16(sec + 40);
    uint32_t root_cluster = pfs_read_le32(sec + 44);
    uint16_t info_sector = pfs_read_le16(sec + 48);
    uint16_t backup_boot = pfs_read_le16(sec + 50);

    if (0 == reserved || 0 == fats)
    {
        return PFS_FAT_RET_ENOEXEC;
    }
    if (!FAT_VALID_MEDIA(media))
    {
        return PFS_FAT_RET_ENOEXEC;
    }
    if (!pfs_fat_is_power_of_2(sector_size) || sector_size < 512 || sector_size > 4096)
    {
        return PFS_FAT_RET_ENOEXEC;
    }
    if (!pfs_fat_is_power_of_2(sec_per_clus))
    {
        return PFS_FAT_RET_ENOEXEC;
    }
    if (0 == fat_length16 && 0 == fat32_length)
    {
        return PFS_FAT_RET_ENOEXEC;
    }

    // 填 boot 副本 (主机序标量; 未对齐数组保持原字节)
    memcpy(&p_bs->boot, sec, sizeof(p_bs->boot));
    p_bs->boot.reserved = reserved;
    p_bs->boot.fat_length = fat_length16;
    p_bs->boot.secs_track = pfs_read_le16(sec + 24);
    p_bs->boot.heads = pfs_read_le16(sec + 26);
    p_bs->boot.hidden = pfs_read_le32(sec + 28);
    p_bs->boot.total_sect = total_sect32;
    uint8_t state;
    uint32_t vol_id;
    if (0 == fat_length16 && 0 != fat32_length)
    {
        p_bs->boot.fat32.length = fat32_length;
        p_bs->boot.fat32.flags = fat32_flags;
        p_bs->boot.fat32.version[0] = sec[42];
        p_bs->boot.fat32.version[1] = sec[43];
        p_bs->boot.fat32.root_cluster = root_cluster;
        p_bs->boot.fat32.info_sector = info_sector;
        p_bs->boot.fat32.backup_boot = backup_boot;
        p_bs->boot.fat32.drive_number = sec[64];
        p_bs->boot.fat32.state = sec[65];
        p_bs->boot.fat32.signature = sec[66];
        memcpy(p_bs->boot.fat32.vol_id, sec + 67, 4);
        memcpy(p_bs->boot.fat32.vol_label, sec + 71, FAT_MSDOS_NAME);
        memcpy(p_bs->boot.fat32.fs_type, sec + 82, 8);
        state = sec[65];
        vol_id = pfs_read_le32(sec + 67);
        memcpy(p_bs->vol_label, sec + 71, FAT_MSDOS_NAME);
        memcpy(p_bs->fs_type, sec + 82, 8);
    }
    else
    {
        p_bs->boot.fat16.drive_number = sec[36];
        p_bs->boot.fat16.state = sec[37];
        p_bs->boot.fat16.signature = sec[38];
        memcpy(p_bs->boot.fat16.vol_id, sec + 39, 4);
        memcpy(p_bs->boot.fat16.vol_label, sec + 43, FAT_MSDOS_NAME);
        memcpy(p_bs->boot.fat16.fs_type, sec + 54, 8);
        state = sec[37];
        vol_id = pfs_read_le32(sec + 39);
        memcpy(p_bs->vol_label, sec + 43, FAT_MSDOS_NAME);
        memcpy(p_bs->fs_type, sec + 54, 8);
    }

    p_bs->sector_size = sector_size;
    p_bs->sec_per_clus = sec_per_clus;
    p_bs->reserved = reserved;
    p_bs->fats = fats;
    p_bs->dir_entries = dir_entries;
    p_bs->sectors16 = sectors16;
    p_bs->media = media;
    p_bs->fat_length16 = fat_length16;
    p_bs->total_sect32 = total_sect32;
    p_bs->fat32_length = fat32_length;
    p_bs->root_cluster = root_cluster;
    p_bs->info_sector = info_sector;
    p_bs->backup_boot = backup_boot;
    p_bs->fat32_flags = fat32_flags;
    p_bs->state = state;
    p_bs->vol_id = vol_id;
    p_bs->dirty = (0 != (state & FAT_STATE_DIRTY)) ? 1 : 0;

    uint8_t fat_bits;
    uint32_t fat_length;
    if (0 == fat_length16 && 0 != fat32_length)
    {
        fat_bits = 32;
        fat_length = fat32_length;
    }
    else
    {
        fat_bits = 0;                                   // 12/16 待簇数判定
        fat_length = fat_length16;
    }

    uint32_t dir_per_block = sector_size / (uint32_t)sizeof(fat_dir_entry_t);
    uint32_t rootdir_sectors = (uint32_t)dir_entries * (uint32_t)sizeof(fat_dir_entry_t) / sector_size;
    uint32_t dir_start = (uint32_t)reserved + (uint32_t)fats * fat_length;
    uint32_t data_start = dir_start + rootdir_sectors;
    uint32_t total_sectors = sectors16;
    if (0 == total_sectors)
    {
        total_sectors = total_sect32;
    }
    if (data_start >= total_sectors || 0 == sec_per_clus)
    {
        return PFS_FAT_RET_ENOEXEC;
    }
    uint32_t total_clusters = (total_sectors - data_start) / sec_per_clus;

    if (32 != fat_bits)
    {
        fat_bits = (total_clusters > MAX_FAT12) ? 16 : 12;
    }

    p_bs->fat_bits = fat_bits;
    p_bs->cluster_size = (uint32_t)sector_size * (uint32_t)sec_per_clus;
    p_bs->cluster_size_bits = pfs_fat_log2_u32(p_bs->cluster_size);
    p_bs->fat_start = reserved;
    p_bs->fat_length = fat_length;
    p_bs->dir_start = dir_start;
    p_bs->rootdir_sectors = rootdir_sectors;
    p_bs->data_start = data_start;
    p_bs->total_sectors = total_sectors;
    p_bs->max_cluster = total_clusters + FAT_START_ENT;
    p_bs->dir_per_block = dir_per_block;
    p_bs->dir_per_block_bits = pfs_fat_log2_u32(dir_per_block);
    (void)boot_sig;
    return PFS_FAT_RET_OK;
}

static int pfs_fat_load_fsinfo(pfs_fat_sbi_t *p_sbi)
{
    if (NULL == p_sbi || !pfs_fat_is_fat32(p_sbi))
    {
        return PFS_FAT_RET_OK;
    }

    uint16_t info_sector = p_sbi->bs.info_sector;
    if (0 == info_sector)
    {
        info_sector = 1;
    }
    int64_t off = pfs_fat_sector_to_byte(p_sbi, (int64_t)info_sector);
    uint8_t sec[FAT_SECTOR_SIZE];
    int rc = pfs_blkio_pread(p_sbi->p_blkio, sec, (int)FAT_SECTOR_SIZE, off);
    if (0 > rc)
    {
        return pfs_fat_map_blkio_rc(rc);
    }
    else if (rc != (int)FAT_SECTOR_SIZE)
    {
        return PFS_FAT_RET_EIO;
    }

    {
        uint32_t sig1 = pfs_read_le32(sec + 0);
        uint32_t sig2 = pfs_read_le32(sec + 484);

        if (FAT_FSINFO_SIG1 != sig1 || FAT_FSINFO_SIG2 != sig2)
        {
            p_sbi->fsinfo.valid = false;
            return PFS_FAT_RET_OK;
        }
        p_sbi->fsinfo.valid = true;
        p_sbi->fsinfo.free_clusters = pfs_read_le32(sec + 488);
        p_sbi->fsinfo.next_cluster = pfs_read_le32(sec + 492);
        p_sbi->free_clusters = p_sbi->fsinfo.free_clusters;
        p_sbi->prev_free = p_sbi->fsinfo.next_cluster;
        if (p_sbi->prev_free < FAT_START_ENT)
        {
            p_sbi->prev_free = FAT_START_ENT;
        }
    }
    return PFS_FAT_RET_OK;
}

int pfs_fat_init(pfs_fat_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return PFS_FAT_RET_EINVAL;
    }

    memset(p_sbi, 0, sizeof(*p_sbi));
    p_sbi->mount_state = PFS_FAT_UNMOUNTED;
    p_sbi->free_clusters = (uint32_t)-1;
    p_sbi->prev_free = FAT_START_ENT;
    return PFS_FAT_RET_OK;
}

void pfs_fat_deinit(pfs_fat_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return;
    }
    if (pfs_fat_sbi_is_mounted(p_sbi))
    {
        (void)pfs_fat_umount(p_sbi);
    }
    memset(p_sbi, 0, sizeof(*p_sbi));
    p_sbi->mount_state = PFS_FAT_UNMOUNTED;
    p_sbi->free_clusters = (uint32_t)-1;
    p_sbi->prev_free = FAT_START_ENT;
}

int pfs_fat_umount(pfs_fat_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (!pfs_fat_sbi_is_mounted(p_sbi))
    {
        return PFS_FAT_RET_EBUSY;
    }

    if (0 != p_sbi->root_sec_cache_on)
    {
        (void)pfs_fat_root_sec_cache_deinit(&p_sbi->root_sec_cache);
        p_sbi->root_sec_cache_on = 0;
    }
    (void)pfs_fat_dir_cache_deinit(&p_sbi->dir_cache);
    if (p_sbi->fat.valid)
    {
        (void)pfs_fat_fat_cache_deinit(&p_sbi->fat.fat_cache);
        memset(&p_sbi->fat, 0, sizeof(p_sbi->fat));
    }

    p_sbi->p_blkio = NULL;
    p_sbi->part_offset = 0;
    memset(&p_sbi->bs, 0, sizeof(p_sbi->bs));
    memset(&p_sbi->fsinfo, 0, sizeof(p_sbi->fsinfo));
    p_sbi->free_clusters = (uint32_t)-1;
    p_sbi->prev_free = FAT_START_ENT;
    p_sbi->free_clus_valid = 0;
    p_sbi->mount_state = PFS_FAT_UNMOUNTED;
    return PFS_FAT_RET_OK;
}

int pfs_fat_mount(pfs_fat_sbi_t *p_sbi, pfs_blkio_t *p_blkio, int64_t part_offset, pfs_fat_mount_state_e mount_mode)
{
    if (NULL == p_sbi || NULL == p_blkio || part_offset < 0)
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (PFS_FAT_MOUNT_RDONLY != mount_mode && PFS_FAT_MOUNT_RDWR != mount_mode)
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (pfs_fat_sbi_is_mounted(p_sbi))
    {
        return PFS_FAT_RET_EBUSY;
    }
    if (!pfs_blkio_is_open(p_blkio))
    {
        return PFS_FAT_RET_EBUSY;
    }

    uint8_t sec[FAT_SECTOR_SIZE];
    int rc = pfs_fat_read_boot_sec(p_blkio, part_offset, sec);
    if (PFS_FAT_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_fat_parse_bpb(&p_sbi->bs, sec);
    if (PFS_FAT_RET_OK != rc)
    {
        return rc;
    }

    p_sbi->p_blkio = p_blkio;
    p_sbi->part_offset = part_offset;
    p_sbi->free_clusters = (uint32_t)-1;
    p_sbi->prev_free = FAT_START_ENT;
    p_sbi->free_clus_valid = 0;
    p_sbi->root_sec_cache_on = 0;
    memset(&p_sbi->fsinfo, 0, sizeof(p_sbi->fsinfo));
    memset(&p_sbi->fat, 0, sizeof(p_sbi->fat));

    rc = pfs_fat_load_fsinfo(p_sbi);
    if (PFS_FAT_RET_OK != rc)
    {
        goto fail_clear;
    }

    // FAT 表扇区缓存 (FAT12/16/32; ent_get 依赖)
    rc = pfs_fat_fat_cache_init(&p_sbi->fat.fat_cache, p_sbi);
    if (PFS_FAT_RET_OK != rc)
    {
        goto fail_clear;
    }

    // 目录簇缓存: FAT32 根 + 所有变体的子目录
    rc = pfs_fat_dir_cache_init(&p_sbi->dir_cache, p_sbi, p_sbi->bs.cluster_size);
    if (PFS_FAT_RET_OK != rc)
    {
        (void)pfs_fat_fat_cache_deinit(&p_sbi->fat.fat_cache);
        memset(&p_sbi->fat, 0, sizeof(p_sbi->fat));
        goto fail_clear;
    }

    // FAT12/16 固定根: 扇区缓存
    if (!pfs_fat_is_fat32(p_sbi))
    {
        rc = pfs_fat_root_sec_cache_init(&p_sbi->root_sec_cache, p_sbi);
        if (PFS_FAT_RET_OK != rc)
        {
            (void)pfs_fat_dir_cache_deinit(&p_sbi->dir_cache);
            (void)pfs_fat_fat_cache_deinit(&p_sbi->fat.fat_cache);
            memset(&p_sbi->fat, 0, sizeof(p_sbi->fat));
            goto fail_clear;
        }
        p_sbi->root_sec_cache_on = 1;
    }

    p_sbi->mount_state = mount_mode;
    return PFS_FAT_RET_OK;

fail_clear:
    p_sbi->p_blkio = NULL;
    p_sbi->part_offset = 0;
    memset(&p_sbi->bs, 0, sizeof(p_sbi->bs));
    memset(&p_sbi->fsinfo, 0, sizeof(p_sbi->fsinfo));
    memset(&p_sbi->fat, 0, sizeof(p_sbi->fat));
    return rc;
}

int pfs_fat_remount(pfs_fat_sbi_t *p_sbi, pfs_fat_mount_state_e mount_mode)
{
    if (NULL == p_sbi)
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (PFS_FAT_MOUNT_RDONLY != mount_mode && PFS_FAT_MOUNT_RDWR != mount_mode)
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (!pfs_fat_sbi_is_mounted(p_sbi))
    {
        return PFS_FAT_RET_EBUSY;
    }

    p_sbi->mount_state = mount_mode;
    return PFS_FAT_RET_OK;
}

int pfs_fat_probe(pfs_blkio_t *p_blkio, int64_t part_offset)
{
    if (NULL == p_blkio || part_offset < 0)
    {
        return PFS_FAT_RET_EINVAL;
    }

    uint8_t sec[FAT_SECTOR_SIZE];
    pfs_fat_bs_t bs;
    int rc = pfs_fat_read_boot_sec(p_blkio, part_offset, sec);
    if (PFS_FAT_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_fat_parse_bpb(&bs, sec);
    if (PFS_FAT_RET_OK != rc)
    {
        return rc;
    }
    // 认 FAT12 / FAT16 / FAT32
    if (12 != bs.fat_bits && 16 != bs.fat_bits && 32 != bs.fat_bits)
    {
        return PFS_FAT_RET_ENOEXEC;
    }
    return PFS_FAT_RET_OK;
}

// end
