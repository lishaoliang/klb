// Doc Encode : UTF-8 BOM, Unix(LF)
// boot / mount / probe (linux-7.1 fs/exfat/super.c)
#include "exfat/pfs_exfat_internal.h"

#include <string.h>
#include <stdbool.h>

// on-disk boot 多字节字段为小端; 读入后转主机序供校验与解析
static void pfs_exfat_boot_le_to_cpu(exfat_boot_sector_t *p_boot)
{
    if (NULL == p_boot)
    {
        return;
    }

    p_boot->partition_offset = pfs_le64_to_cpu(p_boot->partition_offset);
    p_boot->vol_length = pfs_le64_to_cpu(p_boot->vol_length);
    p_boot->fat_offset = pfs_le32_to_cpu(p_boot->fat_offset);
    p_boot->fat_length = pfs_le32_to_cpu(p_boot->fat_length);
    p_boot->clu_offset = pfs_le32_to_cpu(p_boot->clu_offset);
    p_boot->clu_count = pfs_le32_to_cpu(p_boot->clu_count);
    p_boot->root_cluster = pfs_le32_to_cpu(p_boot->root_cluster);
    p_boot->vol_serial = pfs_le32_to_cpu(p_boot->vol_serial);
    p_boot->vol_flags = pfs_le16_to_cpu(p_boot->vol_flags);
    p_boot->signature = pfs_le16_to_cpu(p_boot->signature);
}

static bool pfs_exfat_boot_is_valid(const exfat_boot_sector_t *p_boot)
{
    if (NULL == p_boot)
    {
        return false;
    }
    if (0 != memcmp(p_boot->fs_name, STR_EXFAT, EXFAT_BOOTSEC_FS_NAME_LEN))
    {
        return false;
    }
    if (EXFAT_BOOT_SIGNATURE != p_boot->signature)
    {
        return false;
    }
    if (p_boot->sect_size_bits < EXFAT_MIN_SECT_SIZE_BITS ||
            p_boot->sect_size_bits > EXFAT_MAX_SECT_SIZE_BITS)
    {
        return false;
    }
    if (0 == p_boot->num_fats || p_boot->num_fats > 2)
    {
        return false;
    }
    if (p_boot->root_cluster < EXFAT_FIRST_CLUSTER)
    {
        return false;
    }
    if (p_boot->sect_per_clus_bits > EXFAT_MAX_SECT_PER_CLUS_BITS(p_boot))
    {
        return false;
    }
    return true;
}

static int pfs_exfat_parse_boot(pfs_exfat_bs_t *p_bs, const exfat_boot_sector_t *p_boot)
{
    if (NULL == p_bs || NULL == p_boot)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (!pfs_exfat_boot_is_valid(p_boot))
    {
        return PFS_EXFAT_RET_ENOEXEC;
    }

    p_bs->boot = *p_boot;
    p_bs->sectors_per_clu = 1U << p_boot->sect_per_clus_bits;
    p_bs->sectors_per_clu_bits = p_boot->sect_per_clus_bits;
    p_bs->cluster_size_bits = p_boot->sect_per_clus_bits + p_boot->sect_size_bits;
    p_bs->cluster_size = 1U << p_bs->cluster_size_bits;
    p_bs->num_FAT_sectors = p_boot->fat_length;
    p_bs->FAT1_start_sector = p_boot->fat_offset;
    p_bs->FAT2_start_sector = p_boot->fat_offset;
    if (2 == p_boot->num_fats)
    {
        p_bs->FAT2_start_sector += p_bs->num_FAT_sectors;
    }
    p_bs->data_start_sector = p_boot->clu_offset;
    p_bs->num_sectors = p_boot->vol_length;
    p_bs->num_clusters = p_boot->clu_count + EXFAT_RESERVED_CLUSTERS;
    p_bs->root_dir_clu = p_boot->root_cluster;
    p_bs->dentries_per_clu = 1U << (p_bs->cluster_size_bits - EXFAT_DENTRY_SIZE_BITS);
    p_bs->vol_flags = p_boot->vol_flags;
    p_bs->vol_flags_persistent = p_bs->vol_flags & (EXFAT_VOLUME_DIRTY | EXFAT_MEDIA_FAILURE);
    return PFS_EXFAT_RET_OK;
}

static int pfs_exfat_read_boot(pfs_blkio_t *p_blkio, int64_t part_offset, exfat_boot_sector_t *p_boot)
{

    if (NULL == p_blkio || NULL == p_boot)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (!pfs_blkio_is_open(p_blkio))
    {
        return PFS_EXFAT_RET_EBUSY;
    }

    int rc = pfs_blkio_pread(p_blkio, p_boot, (int)sizeof(*p_boot), part_offset);
    if (0 > rc)
    {
        return PFS_EXFAT_RET_EIO;
    }
    else if (rc != (int)sizeof(*p_boot))
    {
        return PFS_EXFAT_RET_EIO;
    }

    pfs_exfat_boot_le_to_cpu(p_boot);
    return PFS_EXFAT_RET_OK;
}

// 扫描根目录系统项 (BITMAP / UPCASE / VOLUME); 对齐 balloc load_bitmap + nls create_upcase 的查找段
// 成功时须已找到位图项; upcase/卷标可选
static int pfs_exfat_parse_root_entries(pfs_exfat_sbi_t *p_sbi)
{
    if (NULL == p_sbi || NULL == p_sbi->p_blkio)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    const pfs_exfat_bs_t *p_bs = PFS_EXFAT_SBI_BS(p_sbi);
    if (0 == p_bs->cluster_size || 0 == p_bs->dentries_per_clu)
    {
        return PFS_EXFAT_RET_EBADMSG;
    }
    if (!pfs_exfat_is_valid_cluster(p_sbi, p_bs->root_dir_clu))
    {
        return PFS_EXFAT_RET_EBADMSG;
    }

    uint32_t need_map_size = ((EXFAT_DATA_CLUSTER_COUNT(p_bs) - 1U) / 8U) + 1U;
    uint32_t clu = p_bs->root_dir_clu;
    uint32_t visited = 0;
    while (clu != EXFAT_EOF_CLUSTER)
    {
        if (!pfs_exfat_is_valid_cluster(p_sbi, clu))
        {
            return PFS_EXFAT_RET_EBADMSG;
        }
        if (visited >= p_bs->num_clusters)
        {
            // 防环
            return PFS_EXFAT_RET_EBADMSG;
        }
        visited++;

        pfs_cache_cluster_slot_t *p_slot = pfs_exfat_dir_cache_get(&p_sbi->dir_cache, clu);
        if (NULL == p_slot || NULL == p_slot->p_data)
        {
            return PFS_EXFAT_RET_EIO;
        }

        const exfat_dentry_t *p_ents = (const exfat_dentry_t *)p_slot->p_data;
        for (uint32_t i = 0; i < p_bs->dentries_per_clu; i++)
        {
            unsigned int type = pfs_exfat_get_entry_type(&p_ents[i]);

            if (PFS_EXFAT_TYPE_UNUSED == type)
            {
                goto scan_done;
            }

            if (!p_sbi->bitmap.valid && PFS_EXFAT_TYPE_BITMAP == type
                    && 0x0 == p_ents[i].dentry.bitmap.flags)
            {
                uint32_t start_clu = pfs_le32_to_cpu(p_ents[i].dentry.bitmap.start_clu);
                uint64_t map_size = pfs_le64_to_cpu(p_ents[i].dentry.bitmap.size);

                if (!pfs_exfat_is_valid_cluster(p_sbi, start_clu))
                {
                    return PFS_EXFAT_RET_EBADMSG;
                }
                // 对齐内核: 位图偏小则失败; 偏大可接受
                if ((uint64_t)need_map_size > map_size)
                {
                    return PFS_EXFAT_RET_EBADMSG;
                }
                p_sbi->bitmap.start_clu = start_clu;
                p_sbi->bitmap.size = map_size;
                p_sbi->bitmap.valid = true;
            }
            else if (!p_sbi->upcase.valid && PFS_EXFAT_TYPE_UPCASE == type)
            {
                uint32_t start_clu = pfs_le32_to_cpu(p_ents[i].dentry.upcase.start_clu);
                uint64_t up_size = pfs_le64_to_cpu(p_ents[i].dentry.upcase.size);

                if (!pfs_exfat_is_valid_cluster(p_sbi, start_clu) || 0 == up_size)
                {
                    return PFS_EXFAT_RET_EBADMSG;
                }
                p_sbi->upcase.start_clu = start_clu;
                p_sbi->upcase.size = up_size;
                p_sbi->upcase.checksum = pfs_le32_to_cpu(p_ents[i].dentry.upcase.checksum);
                p_sbi->upcase.valid = true;
            }
            else if (!p_sbi->vol_label.valid && PFS_EXFAT_TYPE_VOLUME == type)
            {
                uint8_t n = p_ents[i].dentry.volume_label.char_count;

                if (n > EXFAT_VOLUME_LABEL_LEN)
                {
                    return PFS_EXFAT_RET_EBADMSG;
                }
                for (int j = 0; j < n; j++)
                {
                    p_sbi->vol_label.name[j] = pfs_le16_to_cpu(
                            p_ents[i].dentry.volume_label.volume_label[j]);
                }
                p_sbi->vol_label.len = (int)n;
                p_sbi->vol_label.valid = true;
            }

            // 三项齐则提前结束
            if (p_sbi->bitmap.valid && p_sbi->upcase.valid && p_sbi->vol_label.valid)
            {
                goto scan_done;
            }
        }

        int rc = pfs_exfat_ent_get(p_sbi, clu, &clu);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }
    }

scan_done:
    if (!p_sbi->bitmap.valid)
    {
        return PFS_EXFAT_RET_EBADMSG;
    }
    return PFS_EXFAT_RET_OK;
}

// mount 失败/卸载时释放已分配缓存 (upcase / bmp / fat / dir)
static void pfs_exfat_mount_release_caches(pfs_exfat_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return;
    }
    pfs_exfat_free_upcase_table(p_sbi);
    (void)pfs_exfat_bmp_cache_deinit(&p_sbi->bitmap.bmp_cache);
    (void)pfs_exfat_fat_cache_deinit(&p_sbi->fat.fat_cache);
    (void)pfs_exfat_dir_cache_deinit(&p_sbi->dir_cache);
}

int pfs_exfat_init(pfs_exfat_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    memset(p_sbi, 0, sizeof(*p_sbi));
    p_sbi->mount_state = PFS_EXFAT_UNMOUNTED;
    return PFS_EXFAT_RET_OK;
}

void pfs_exfat_deinit(pfs_exfat_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return;
    }
    if (pfs_exfat_sbi_is_mounted(p_sbi))
    {
        pfs_exfat_umount(p_sbi);
    }
    memset(p_sbi, 0, sizeof(*p_sbi));
    p_sbi->mount_state = PFS_EXFAT_UNMOUNTED;
}

int pfs_exfat_mount(pfs_exfat_sbi_t *p_sbi, pfs_blkio_t *p_blkio, int64_t part_offset, pfs_exfat_mount_state_e mount_mode)
{
    if (NULL == p_sbi || NULL == p_blkio)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (0 > part_offset)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (PFS_EXFAT_MOUNT_RDONLY != mount_mode && PFS_EXFAT_MOUNT_RDWR != mount_mode)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (pfs_exfat_sbi_is_mounted(p_sbi))
    {
        return PFS_EXFAT_RET_EBUSY;
    }

    exfat_boot_sector_t boot;
    int rc = pfs_exfat_read_boot(p_blkio, part_offset, &boot);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }

    memset(p_sbi, 0, sizeof(*p_sbi));
    p_sbi->p_blkio = p_blkio;
    p_sbi->part_offset = part_offset;
    p_sbi->mount_state = PFS_EXFAT_UNMOUNTED;

    rc = pfs_exfat_parse_boot(&p_sbi->bs, &boot);
    if (PFS_EXFAT_RET_OK != rc)
    {
        memset(p_sbi, 0, sizeof(*p_sbi));
        p_sbi->mount_state = PFS_EXFAT_UNMOUNTED;
        return rc;
    }

    // 三池: 仅分配槽内存, 不预读 FAT/bitmap/目录全量; 扫根时 dir_cache_get 按需读目录簇
    rc = pfs_exfat_dir_cache_init(&p_sbi->dir_cache, p_sbi, p_sbi->bs.cluster_size);
    if (PFS_EXFAT_RET_OK != rc)
    {
        memset(p_sbi, 0, sizeof(*p_sbi));
        p_sbi->mount_state = PFS_EXFAT_UNMOUNTED;
        return rc;
    }

    rc = pfs_exfat_fat_cache_init(&p_sbi->fat.fat_cache, p_sbi);
    if (PFS_EXFAT_RET_OK != rc)
    {
        pfs_exfat_mount_release_caches(p_sbi);
        memset(p_sbi, 0, sizeof(*p_sbi));
        p_sbi->mount_state = PFS_EXFAT_UNMOUNTED;
        return rc;
    }

    // bitmap 元数据在 parse_root 填; 此处只建缓存池
    rc = pfs_exfat_bmp_cache_init(&p_sbi->bitmap.bmp_cache, p_sbi, p_sbi->bs.cluster_size);
    if (PFS_EXFAT_RET_OK != rc)
    {
        pfs_exfat_mount_release_caches(p_sbi);
        memset(p_sbi, 0, sizeof(*p_sbi));
        p_sbi->mount_state = PFS_EXFAT_UNMOUNTED;
        return rc;
    }

    rc = pfs_exfat_parse_root_entries(p_sbi);
    if (PFS_EXFAT_RET_OK != rc)
    {
        pfs_exfat_mount_release_caches(p_sbi);
        memset(p_sbi, 0, sizeof(*p_sbi));
        p_sbi->mount_state = PFS_EXFAT_UNMOUNTED;
        return rc;
    }

    // upcase: 分配固定表并展开加载; 此后只读至 umount
    rc = pfs_exfat_create_upcase_table(p_sbi);
    if (PFS_EXFAT_RET_OK != rc)
    {
        pfs_exfat_mount_release_caches(p_sbi);
        memset(p_sbi, 0, sizeof(*p_sbi));
        p_sbi->mount_state = PFS_EXFAT_UNMOUNTED;
        return rc;
    }

    p_sbi->mount_state = mount_mode;
    return PFS_EXFAT_RET_OK;
}

int pfs_exfat_remount(pfs_exfat_sbi_t *p_sbi, pfs_exfat_mount_state_e mount_mode)
{
    if (NULL == p_sbi)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (PFS_EXFAT_MOUNT_RDONLY != mount_mode && PFS_EXFAT_MOUNT_RDWR != mount_mode)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (!pfs_exfat_sbi_is_mounted(p_sbi))
    {
        return PFS_EXFAT_RET_EBUSY;
    }

    p_sbi->mount_state = mount_mode;
    return PFS_EXFAT_RET_OK;
}

int pfs_exfat_umount(pfs_exfat_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (!pfs_exfat_sbi_is_mounted(p_sbi))
    {
        return PFS_EXFAT_RET_EBUSY;
    }

    pfs_exfat_mount_release_caches(p_sbi);

    p_sbi->p_blkio = NULL;
    p_sbi->part_offset = 0;
    memset(&p_sbi->bs, 0, sizeof(p_sbi->bs));
    memset(&p_sbi->bitmap, 0, sizeof(p_sbi->bitmap));
    memset(&p_sbi->fat, 0, sizeof(p_sbi->fat));
    memset(&p_sbi->upcase, 0, sizeof(p_sbi->upcase));
    memset(&p_sbi->vol_label, 0, sizeof(p_sbi->vol_label));
    p_sbi->mount_state = PFS_EXFAT_UNMOUNTED;
    return PFS_EXFAT_RET_OK;
}

int pfs_exfat_probe(pfs_blkio_t *p_blkio, int64_t part_offset)
{
    if (NULL == p_blkio || 0 > part_offset)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    exfat_boot_sector_t boot;
    int rc = pfs_exfat_read_boot(p_blkio, part_offset, &boot);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }
    if (!pfs_exfat_boot_is_valid(&boot))
    {
        return PFS_EXFAT_RET_ENOEXEC;
    }
    return PFS_EXFAT_RET_OK;
}

// end
