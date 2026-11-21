// Doc Encode : UTF-8 BOM, Unix(LF)
// boot / mount / probe (linux-7.1 fs/ntfs/super.c 子集: OEM + 几何 + $MFT 引导)
#include "ntfs/pfs_ntfs_internal.h"

#include <stdbool.h>

static uint32_t pfs_ntfs_log2_u32(uint32_t v)
{
    uint32_t bits = 0;

    while (v > 1U)
    {
        v >>= 1;
        bits++;
    }
    return bits;
}

static bool pfs_ntfs_boot_is_valid(const ntfs_boot_sector_t *p_boot)
{
    if (NULL == p_boot)
    {
        return false;
    }
    if (NTFS_OEM_ID != p_boot->oem_id)
    {
        return false;
    }
    if (0xAA55 != pfs_le16_to_cpu(p_boot->end_of_sector_marker))
    {
        return false;
    }
    uint16_t bps = pfs_le16_to_cpu(p_boot->bpb.bytes_per_sector);
    if (512 != bps && 1024 != bps && 2048 != bps && 4096 != bps)
    {
        return false;
    }
    uint8_t spc = p_boot->bpb.sectors_per_cluster;
    if (0 == spc)
    {
        return false;
    }
    // 正值须为 2 的幂; 或编码为 0xF0.. 大簇 (本实现 mkfs 仅用正值 1..128)
    if (spc <= 128U)
    {
        if (0 != (spc & (spc - 1U)))
        {
            return false;
        }
    }
    uint64_t sectors = pfs_le64_to_cpu(p_boot->number_of_sectors);
    if (sectors < 2048ULL)
    {
        return false;
    }
    uint64_t mft_lcn = pfs_le64_to_cpu(p_boot->mft_lcn);
    uint64_t mirr_lcn = pfs_le64_to_cpu(p_boot->mftmirr_lcn);
    if (mft_lcn >= sectors || mirr_lcn >= sectors)
    {
        return false;
    }
    int8_t cpmft = p_boot->clusters_per_mft_record;
    if (0 == cpmft)
    {
        return false;
    }
    if (0 == p_boot->clusters_per_index_record)
    {
        return false;
    }
    return true;
}

// 解码 boot.clusters_per_*_record -> 字节大小 (对齐 linux parse_ntfs_boot_sector)
// 正值: 簇数倍数 (按 2 的幂移位); 负值: -log2(字节数)
static int pfs_ntfs_decode_record_size(int8_t clusters_per_record, uint32_t cluster_size, uint32_t *p_out)
{
    if (NULL == p_out || 0U == cluster_size)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (0 == clusters_per_record)
    {
        return PFS_NTFS_RET_ENOEXEC;
    }
    uint32_t sz;
    if (clusters_per_record > 0)
    {
        uint32_t c = (uint32_t)clusters_per_record;
        uint32_t c_bits = pfs_ntfs_log2_u32(c);

        if (0U != (c & (c - 1U)))
        {
            return PFS_NTFS_RET_ENOEXEC;
        }
        if (c_bits >= 31U)
        {
            return PFS_NTFS_RET_ENOEXEC;
        }
        sz = cluster_size << c_bits;
    }
    else
    {
        int shift = -(int)clusters_per_record;

        if (shift <= 0 || shift >= 31)
        {
            return PFS_NTFS_RET_ENOEXEC;
        }
        sz = 1U << (unsigned)shift;
    }
    if (0U == sz || 0U != (sz & (sz - 1U)))
    {
        return PFS_NTFS_RET_ENOEXEC;
    }
    *p_out = sz;
    return PFS_NTFS_RET_OK;
}

static int pfs_ntfs_parse_boot(pfs_ntfs_bs_t *p_bs, const ntfs_boot_sector_t *p_boot_le)
{
    if (NULL == p_bs || NULL == p_boot_le)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (!pfs_ntfs_boot_is_valid(p_boot_le))
    {
        return PFS_NTFS_RET_ENOEXEC;
    }

    memset(p_bs, 0, sizeof(*p_bs));
    p_bs->boot = *p_boot_le;
    uint32_t sector_size = pfs_le16_to_cpu(p_boot_le->bpb.bytes_per_sector);
    uint8_t spc = p_boot_le->bpb.sectors_per_cluster;
    if (spc > 128U)
    {
        // 编码为 256-n 扇区/簇 (少见); 本路径按 mkfs 正值处理失败
        return PFS_NTFS_RET_ENOEXEC;
    }
    uint32_t sectors_per_cluster = (uint32_t)spc;
    uint32_t sectors_per_cluster_bits = pfs_ntfs_log2_u32(sectors_per_cluster);
    uint32_t cluster_size = sector_size << sectors_per_cluster_bits;
    if (0U == cluster_size || 0U != (cluster_size & (cluster_size - 1U)))
    {
        return PFS_NTFS_RET_ENOEXEC;
    }
    if (cluster_size < sector_size)
    {
        return PFS_NTFS_RET_ENOEXEC;
    }

    uint32_t mft_record_size;
    uint32_t index_record_size;
    int rc = pfs_ntfs_decode_record_size(p_boot_le->clusters_per_mft_record, cluster_size, &mft_record_size);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_ntfs_decode_record_size(p_boot_le->clusters_per_index_record, cluster_size, &index_record_size);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    if (index_record_size < sector_size)
    {
        return PFS_NTFS_RET_ENOEXEC;
    }

    uint64_t num_sectors = pfs_le64_to_cpu(p_boot_le->number_of_sectors);
    uint64_t num_clusters = num_sectors >> sectors_per_cluster_bits;
    if (0ULL == num_clusters || num_clusters >= (1ULL << 32))
    {
        // 对齐内核: 暂不支持 64-bit 簇号空间
        return PFS_NTFS_RET_ENOEXEC;
    }
    uint64_t mft_lcn = pfs_le64_to_cpu(p_boot_le->mft_lcn);
    uint64_t mft_mirr_lcn = pfs_le64_to_cpu(p_boot_le->mftmirr_lcn);
    if (mft_lcn >= num_clusters || mft_mirr_lcn >= num_clusters)
    {
        return PFS_NTFS_RET_ENOEXEC;
    }

    p_bs->num_sectors = num_sectors;
    p_bs->cluster_size = cluster_size;
    p_bs->cluster_size_bits = pfs_ntfs_log2_u32(cluster_size);
    p_bs->mft_lcn = mft_lcn;
    p_bs->mft_mirr_lcn = mft_mirr_lcn;

    p_bs->sector_size = sector_size;
    p_bs->sector_size_bits = pfs_ntfs_log2_u32(sector_size);
    p_bs->cluster_size_mask = cluster_size - 1U;
    p_bs->sectors_per_cluster = sectors_per_cluster;
    p_bs->mft_record_size = mft_record_size;
    p_bs->mft_record_size_mask = mft_record_size - 1U;
    p_bs->mft_record_size_bits = pfs_ntfs_log2_u32(mft_record_size);
    p_bs->index_record_size = index_record_size;
    p_bs->index_record_size_mask = index_record_size - 1U;
    p_bs->index_record_size_bits = pfs_ntfs_log2_u32(index_record_size);
    p_bs->num_clusters = num_clusters;
    p_bs->serial_no = pfs_le64_to_cpu(p_boot_le->volume_serial_number);
    // cluster<=4*mft_record: 镜像 4 条; 否则一簇能放下的记录数
    if (cluster_size <= (4U << p_bs->mft_record_size_bits))
    {
        p_bs->mft_mirr_size = 4U;
    }
    else
    {
        p_bs->mft_mirr_size = cluster_size >> p_bs->mft_record_size_bits;
    }
    return PFS_NTFS_RET_OK;
}

static int pfs_ntfs_read_boot(pfs_blkio_t *p_blkio, int64_t part_offset, ntfs_boot_sector_t *p_boot)
{

    if (NULL == p_blkio || NULL == p_boot || 0 > part_offset)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (!pfs_blkio_is_open(p_blkio))
    {
        return PFS_NTFS_RET_EBUSY;
    }

    int rc = pfs_blkio_pread(p_blkio, p_boot, (int)sizeof(*p_boot), part_offset);
    if (0 > rc)
    {
        return pfs_ntfs_map_blkio_rc(rc);
    }
    else if (rc != (int)sizeof(*p_boot))
    {
        return PFS_NTFS_RET_EIO;
    }
    return PFS_NTFS_RET_OK;
}

int pfs_ntfs_init(pfs_ntfs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    memset(p_sbi, 0, sizeof(*p_sbi));
    p_sbi->mount_state = PFS_NTFS_UNMOUNTED;
    p_sbi->mft.runlist.hint = -1;
    p_sbi->root.data_runlist.hint = -1;
    p_sbi->root.i30.alloc_runlist.hint = -1;
    p_sbi->lcn_bmp.runlist.hint = -1;
    p_sbi->lcn_bmp.free_clusters = -1;
    p_sbi->lcn_bmp.hint_lcn = -1;
    p_sbi->lcn_bmp.mft_zone_start = -1;
    p_sbi->lcn_bmp.mft_zone_end = -1;
    p_sbi->lcn_bmp.mft_zone_pos = -1;
    p_sbi->lcn_bmp.data_zone_pos = -1;
    p_sbi->mft_bmp.runlist.hint = -1;
    p_sbi->mft_bmp.free_mft_records = -1;
    return PFS_NTFS_RET_OK;
}

void pfs_ntfs_deinit(pfs_ntfs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return;
    }
    if (pfs_ntfs_sbi_is_mounted(p_sbi))
    {
        (void)pfs_ntfs_umount(p_sbi);
    }
    pfs_ntfs_mft_reset(&p_sbi->mft);
    pfs_ntfs_upcase_reset(&p_sbi->upcase);
    pfs_ntfs_inode_reset(&p_sbi->root);
    pfs_ntfs_lcn_bitmap_reset(&p_sbi->lcn_bmp);
    pfs_ntfs_mft_bitmap_reset(&p_sbi->mft_bmp);
    (void)pfs_cache_cluster_deinit(&p_sbi->cache);
    memset(p_sbi, 0, sizeof(*p_sbi));
    p_sbi->mount_state = PFS_NTFS_UNMOUNTED;
}

int pfs_ntfs_mount(pfs_ntfs_sbi_t *p_sbi, pfs_blkio_t *p_blkio, int64_t part_offset, pfs_ntfs_mount_state_e mount_mode)
{
    if (NULL == p_sbi || NULL == p_blkio)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (0 > part_offset)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (PFS_NTFS_MOUNT_RDONLY != mount_mode && PFS_NTFS_MOUNT_RDWR != mount_mode)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (pfs_ntfs_sbi_is_mounted(p_sbi))
    {
        return PFS_NTFS_RET_EBUSY;
    }
    if (!pfs_blkio_is_open(p_blkio))
    {
        return PFS_NTFS_RET_EBUSY;
    }

    ntfs_boot_sector_t boot;
    pfs_ntfs_bs_t bs;
    int rc = pfs_ntfs_read_boot(p_blkio, part_offset, &boot);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_ntfs_parse_boot(&bs, &boot);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }

    // 先填几何与 blkio, 再引导 $MFT (失败则回滚, 保持 UNMOUNTED)
    p_sbi->bs = bs;
    p_sbi->p_blkio = p_blkio;
    p_sbi->part_offset = part_offset;
    pfs_ntfs_mft_reset(&p_sbi->mft);
    pfs_ntfs_upcase_reset(&p_sbi->upcase);
    pfs_ntfs_inode_reset(&p_sbi->root);
    pfs_ntfs_lcn_bitmap_reset(&p_sbi->lcn_bmp);
    pfs_ntfs_mft_bitmap_reset(&p_sbi->mft_bmp);

    rc = pfs_ntfs_mft_load_bootstrap(p_sbi);
    if (PFS_NTFS_RET_OK != rc)
    {
        p_sbi->p_blkio = NULL;
        p_sbi->part_offset = 0;
        memset(&p_sbi->bs, 0, sizeof(p_sbi->bs));
        pfs_ntfs_mft_reset(&p_sbi->mft);
        pfs_ntfs_upcase_reset(&p_sbi->upcase);
        pfs_ntfs_inode_reset(&p_sbi->root);
        pfs_ntfs_lcn_bitmap_reset(&p_sbi->lcn_bmp);
        pfs_ntfs_mft_bitmap_reset(&p_sbi->mft_bmp);
        p_sbi->mount_state = PFS_NTFS_UNMOUNTED;
        return rc;
    }

    rc = pfs_cache_cluster_init(&p_sbi->cache, p_blkio, p_sbi->bs.cluster_size);
    if (0 != rc)
    {
        pfs_ntfs_mft_reset(&p_sbi->mft);
        pfs_ntfs_upcase_reset(&p_sbi->upcase);
        pfs_ntfs_inode_reset(&p_sbi->root);
        pfs_ntfs_lcn_bitmap_reset(&p_sbi->lcn_bmp);
        pfs_ntfs_mft_bitmap_reset(&p_sbi->mft_bmp);
        p_sbi->p_blkio = NULL;
        p_sbi->part_offset = 0;
        memset(&p_sbi->bs, 0, sizeof(p_sbi->bs));
        p_sbi->mount_state = PFS_NTFS_UNMOUNTED;
        if (-12 == rc)
        {
            return PFS_NTFS_RET_ENOMEM;
        }
        if (-22 == rc || -16 == rc)
        {
            return PFS_NTFS_RET_EINVAL;
        }
        return PFS_NTFS_RET_EIO;
    }

    rc = pfs_ntfs_upcase_load(p_sbi);
    if (PFS_NTFS_RET_OK != rc)
    {
        (void)pfs_cache_cluster_deinit(&p_sbi->cache);
        pfs_ntfs_mft_reset(&p_sbi->mft);
        pfs_ntfs_upcase_reset(&p_sbi->upcase);
        pfs_ntfs_inode_reset(&p_sbi->root);
        pfs_ntfs_lcn_bitmap_reset(&p_sbi->lcn_bmp);
        pfs_ntfs_mft_bitmap_reset(&p_sbi->mft_bmp);
        p_sbi->p_blkio = NULL;
        p_sbi->part_offset = 0;
        memset(&p_sbi->bs, 0, sizeof(p_sbi->bs));
        p_sbi->mount_state = PFS_NTFS_UNMOUNTED;
        return rc;
    }

    rc = pfs_ntfs_inode_load(p_sbi, NTFS_FILE_root, &p_sbi->root);
    if (PFS_NTFS_RET_OK != rc)
    {
        (void)pfs_cache_cluster_deinit(&p_sbi->cache);
        pfs_ntfs_mft_reset(&p_sbi->mft);
        pfs_ntfs_upcase_reset(&p_sbi->upcase);
        pfs_ntfs_inode_reset(&p_sbi->root);
        pfs_ntfs_lcn_bitmap_reset(&p_sbi->lcn_bmp);
        pfs_ntfs_mft_bitmap_reset(&p_sbi->mft_bmp);
        p_sbi->p_blkio = NULL;
        p_sbi->part_offset = 0;
        memset(&p_sbi->bs, 0, sizeof(p_sbi->bs));
        p_sbi->mount_state = PFS_NTFS_UNMOUNTED;
        return rc;
    }

    if (PFS_NTFS_MOUNT_RDWR == mount_mode)
    {
        rc = pfs_ntfs_lcn_bitmap_load(p_sbi);
        if (PFS_NTFS_RET_OK != rc)
        {
            (void)pfs_cache_cluster_deinit(&p_sbi->cache);
            pfs_ntfs_mft_reset(&p_sbi->mft);
            pfs_ntfs_upcase_reset(&p_sbi->upcase);
            pfs_ntfs_inode_reset(&p_sbi->root);
            pfs_ntfs_lcn_bitmap_reset(&p_sbi->lcn_bmp);
            pfs_ntfs_mft_bitmap_reset(&p_sbi->mft_bmp);
            p_sbi->p_blkio = NULL;
            p_sbi->part_offset = 0;
            memset(&p_sbi->bs, 0, sizeof(p_sbi->bs));
            p_sbi->mount_state = PFS_NTFS_UNMOUNTED;
            return rc;
        }
        rc = pfs_ntfs_mft_bitmap_load(p_sbi);
        if (PFS_NTFS_RET_OK != rc)
        {
            (void)pfs_cache_cluster_deinit(&p_sbi->cache);
            pfs_ntfs_mft_reset(&p_sbi->mft);
            pfs_ntfs_upcase_reset(&p_sbi->upcase);
            pfs_ntfs_inode_reset(&p_sbi->root);
            pfs_ntfs_lcn_bitmap_reset(&p_sbi->lcn_bmp);
            pfs_ntfs_mft_bitmap_reset(&p_sbi->mft_bmp);
            p_sbi->p_blkio = NULL;
            p_sbi->part_offset = 0;
            memset(&p_sbi->bs, 0, sizeof(p_sbi->bs));
            p_sbi->mount_state = PFS_NTFS_UNMOUNTED;
            return rc;
        }
    }

    p_sbi->mount_state = mount_mode;
    return PFS_NTFS_RET_OK;
}

int pfs_ntfs_remount(pfs_ntfs_sbi_t *p_sbi, pfs_ntfs_mount_state_e mount_mode)
{
    if (NULL == p_sbi)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (PFS_NTFS_MOUNT_RDONLY != mount_mode && PFS_NTFS_MOUNT_RDWR != mount_mode)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (!pfs_ntfs_sbi_is_mounted(p_sbi))
    {
        return PFS_NTFS_RET_EBUSY;
    }

    if (PFS_NTFS_MOUNT_RDWR == mount_mode && !p_sbi->lcn_bmp.valid)
    {
        int rc = pfs_ntfs_lcn_bitmap_load(p_sbi);

        if (PFS_NTFS_RET_OK != rc)
        {
            return rc;
        }
    }

    if (PFS_NTFS_MOUNT_RDWR == mount_mode && !p_sbi->mft_bmp.valid)
    {
        int rc = pfs_ntfs_mft_bitmap_load(p_sbi);

        if (PFS_NTFS_RET_OK != rc)
        {
            return rc;
        }
    }

    p_sbi->mount_state = mount_mode;
    return PFS_NTFS_RET_OK;
}

int pfs_ntfs_umount(pfs_ntfs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (!pfs_ntfs_sbi_is_mounted(p_sbi))
    {
        return PFS_NTFS_RET_EBUSY;
    }

    // 落盘后再拆 sbi; 否则 VHD 上 MFT 位图/$I30 可能未进宿主文件
    if (NULL != p_sbi->p_blkio)
    {
        (void)pfs_blkio_fsync(p_sbi->p_blkio);
    }

    pfs_ntfs_mft_reset(&p_sbi->mft);
    pfs_ntfs_upcase_reset(&p_sbi->upcase);
    pfs_ntfs_inode_reset(&p_sbi->root);
    pfs_ntfs_lcn_bitmap_reset(&p_sbi->lcn_bmp);
    pfs_ntfs_mft_bitmap_reset(&p_sbi->mft_bmp);
    (void)pfs_cache_cluster_deinit(&p_sbi->cache);
    p_sbi->p_blkio = NULL;
    p_sbi->part_offset = 0;
    memset(&p_sbi->bs, 0, sizeof(p_sbi->bs));
    p_sbi->mount_state = PFS_NTFS_UNMOUNTED;
    return PFS_NTFS_RET_OK;
}

int pfs_ntfs_probe(pfs_blkio_t *p_blkio, int64_t part_offset)
{
    if (NULL == p_blkio || 0 > part_offset)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (!pfs_blkio_is_open(p_blkio))
    {
        return PFS_NTFS_RET_EBUSY;
    }

    ntfs_boot_sector_t boot;
    pfs_ntfs_bs_t bs;
    int rc = pfs_ntfs_read_boot(p_blkio, part_offset, &boot);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    return pfs_ntfs_parse_boot(&bs, &boot);
}

// end
