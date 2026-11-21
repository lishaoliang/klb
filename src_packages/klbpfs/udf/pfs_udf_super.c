// Doc Encode : UTF-8 BOM, Unix(LF)
// UDF super: probe/mount/remount; Type1 rewritable + SBD 时可 RDWR
#include "udf/pfs_udf_internal.h"
#include "base/pfs_mem.h"

#include <stdbool.h>
#include <string.h>

#define PFS_UDF_MAX_PD                  8

typedef struct _pfs_udf_pd_entry_t {
    uint16_t partition_number;
    uint32_t partition_start;
    uint32_t partition_len;
    uint32_t access_type;
    uint32_t bitmap_ext_length;
    uint32_t bitmap_ext_position;
} pfs_udf_pd_entry_t;

typedef struct _pfs_udf_vol_scan_t {
    int                     have_lvd;
    uint32_t                block_size;
    pfs_udf_lb_addr_t       fileset_loc;
    pfs_udf_pd_entry_t      pd[PFS_UDF_MAX_PD];
    int                     n_pd;
    pfs_udf_sbi_t           sbi_stub;
} pfs_udf_vol_scan_t;

static void pfs_udf_vol_scan_init(pfs_udf_vol_scan_t *p_scan)
{
    memset(p_scan, 0, sizeof(*p_scan));
}

static int pfs_udf_pd_store(pfs_udf_vol_scan_t *p_scan, const pfs_udf_partition_desc_t *p_pd)
{
    if (NULL == p_scan || NULL == p_pd)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (p_scan->n_pd >= PFS_UDF_MAX_PD)
    {
        return PFS_UDF_RET_ERANGE;
    }
    p_scan->pd[p_scan->n_pd].partition_number = pfs_udf_le16(&p_pd->partitionNumber);
    p_scan->pd[p_scan->n_pd].partition_start = pfs_udf_le32(&p_pd->partitionStartingLocation);
    p_scan->pd[p_scan->n_pd].partition_len = pfs_udf_le32(&p_pd->partitionLength);
    p_scan->pd[p_scan->n_pd].access_type = pfs_udf_le32(&p_pd->accessType);
    {
        const pfs_udf_partition_header_desc_t *p_phd;
        p_phd = (const pfs_udf_partition_header_desc_t *)p_pd->partitionContentsUse;
        p_scan->pd[p_scan->n_pd].bitmap_ext_length =
                pfs_udf_le32(&p_phd->unallocSpaceBitmap.extLength);
        p_scan->pd[p_scan->n_pd].bitmap_ext_position =
                pfs_udf_le32(&p_phd->unallocSpaceBitmap.extPosition);
    }
    pfs_udf_pmap_apply_pd(&p_scan->sbi_stub, p_scan->pd[p_scan->n_pd].partition_number,
            p_scan->pd[p_scan->n_pd].partition_start,
            p_scan->pd[p_scan->n_pd].partition_len);
    p_scan->n_pd++;
    return PFS_UDF_RET_OK;
}

static int pfs_udf_pd_match(pfs_udf_vol_scan_t *p_scan)
{
    if (NULL == p_scan || 0 == p_scan->have_lvd)
    {
        return PFS_UDF_RET_EBADMSG;
    }
    if (0 == pfs_udf_pmap_have_type1_pd(&p_scan->sbi_stub))
    {
        return PFS_UDF_RET_EBADMSG;
    }
    return PFS_UDF_RET_OK;
}

static int pfs_udf_lvd_parse(pfs_udf_vol_scan_t *p_scan, const pfs_udf_logical_vol_desc_t *p_lvd,
        int block_size)
{
    if (NULL == p_scan || NULL == p_lvd)
    {
        return PFS_UDF_RET_EINVAL;
    }

    p_scan->block_size = pfs_udf_le32(&p_lvd->logicalBlockSize);
    if (0 == p_scan->block_size ||
            0 != (p_scan->block_size % 512u) ||
            p_scan->block_size > 8192u)
    {
        return PFS_UDF_RET_EBADMSG;
    }

    uint32_t map_len = pfs_udf_le32(&p_lvd->mapTableLength);
    uint32_t map_count = pfs_udf_le32(&p_lvd->numPartitionMaps);
    const uint8_t *p_maps = (const uint8_t *)p_lvd + sizeof(pfs_udf_logical_vol_desc_t);
    if (map_len > (uint32_t)block_size - sizeof(pfs_udf_logical_vol_desc_t))
    {
        return PFS_UDF_RET_EBADMSG;
    }

    p_scan->sbi_stub.n_maps = 0;
    bool have_type1 = false;
    uint32_t off = 0;
    for (int i = 0; i < (int)map_count && off < map_len; i++)
    {
        uint8_t map_type;
        uint8_t map_len1;

        if (off + 2u > map_len)
        {
            break;
        }
        map_type = p_maps[off];
        map_len1 = p_maps[off + 1];
        if (0 == map_len1 || off + (uint32_t)map_len1 > map_len)
        {
            return PFS_UDF_RET_EBADMSG;
        }
        if (PFS_UDF_GP_PARTMAP_TYPE1 == map_type &&
                map_len1 >= sizeof(pfs_udf_partmap1_t))
        {
            int rc = pfs_udf_pmap_add_type1(&p_scan->sbi_stub,
                    (const pfs_udf_partmap1_t *)(p_maps + off));
            if (PFS_UDF_RET_OK != rc)
            {
                return rc;
            }
            have_type1 = true;
        }
        else if (PFS_UDF_GP_PARTMAP_TYPE2 == map_type)
        {
            int rc = pfs_udf_pmap_add_type2(&p_scan->sbi_stub, p_maps + off, map_len1);
            if (PFS_UDF_RET_OK != rc)
            {
                return rc;
            }
        }
        else
        {
            return PFS_UDF_RET_EOPNOTSUPP;
        }
        off += (uint32_t)map_len1;
    }
    if (!have_type1)
    {
        return PFS_UDF_RET_EOPNOTSUPP;
    }

    {
        const pfs_udf_long_ad_t *p_la;
        p_la = (const pfs_udf_long_ad_t *)p_lvd->logicalVolContentsUse;
        memcpy(&p_scan->fileset_loc, &p_la->extLocation, sizeof(pfs_udf_lb_addr_t));
    }
    p_scan->have_lvd = 1;
    return PFS_UDF_RET_OK;
}

static int pfs_udf_process_sequence(pfs_blkio_t *p_blkio, int64_t part_offset,
        uint32_t sector_size, uint32_t block_size, uint32_t main_s, uint32_t main_e,
        pfs_udf_vol_scan_t *p_scan)
{
    if (NULL == p_blkio || NULL == p_scan)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (main_e < main_s)
    {
        return PFS_UDF_RET_EBADMSG;
    }

    uint8_t *buf = (uint8_t *)pfs_malloc((size_t)block_size);
    if (NULL == buf)
    {
        return PFS_UDF_RET_ENOMEM;
    }

    int done = 0;
    for (uint32_t block = main_s; !done && block <= main_e; block++)
    {
        pfs_udf_tag_t *p_tag;
        uint16_t ident;

        int rc = pfs_udf_read_tagged(p_blkio, part_offset, sector_size, block_size, block, 0, buf);
        if (PFS_UDF_RET_OK != rc)
        {
            pfs_free(buf);
            return rc;
        }
        p_tag = (pfs_udf_tag_t *)buf;
        ident = pfs_udf_le16(&p_tag->tagIdent);
        switch (ident)
        {
        case PFS_UDF_TAG_IDENT_PVD:
            break;
        case PFS_UDF_TAG_IDENT_PD:
            rc = pfs_udf_pd_store(p_scan, (const pfs_udf_partition_desc_t *)buf);
            if (PFS_UDF_RET_OK != rc)
            {
                pfs_free(buf);
                return rc;
            }
            break;
        case PFS_UDF_TAG_IDENT_LVD:
            rc = pfs_udf_lvd_parse(p_scan, (const pfs_udf_logical_vol_desc_t *)buf,
                    (int)block_size);
            if (PFS_UDF_RET_OK != rc)
            {
                pfs_free(buf);
                return rc;
            }
            break;
        case PFS_UDF_TAG_IDENT_TD:
            done = 1;
            break;
        case PFS_UDF_TAG_IDENT_VDP: {
            const pfs_udf_vol_desc_ptr_t *p_vdp;
            uint32_t next_s;
            uint32_t next_e;
            p_vdp = (const pfs_udf_vol_desc_ptr_t *)buf;
            next_s = pfs_udf_le32(&p_vdp->nextVolDescSeqExt.extLocation);
            next_e = next_s +
                    (pfs_udf_le32(&p_vdp->nextVolDescSeqExt.extLength) / block_size) - 1u;
            if (next_e >= next_s)
            {
                rc = pfs_udf_process_sequence(p_blkio, part_offset, sector_size, block_size,
                        next_s, next_e, p_scan);
                if (PFS_UDF_RET_OK != rc)
                {
                    pfs_free(buf);
                    return rc;
                }
            }
            break;
        }
        default:
            break;
        }
    }

    pfs_free(buf);
    if (0 == p_scan->have_lvd)
    {
        return PFS_UDF_RET_EBADMSG;
    }
    // VDS 常为 PD 先于 LVD; pd_store 时 map 尚未建立, 此处按已存 PD 回填 phys
    {
        for (int pi = 0; pi < p_scan->n_pd; pi++)
        {
            pfs_udf_pmap_apply_pd(&p_scan->sbi_stub, p_scan->pd[pi].partition_number,
                    p_scan->pd[pi].partition_start, p_scan->pd[pi].partition_len);
        }
    }
    return pfs_udf_pd_match(p_scan);
}

static int pfs_udf_try_anchor(pfs_blkio_t *p_blkio, int64_t part_offset, uint32_t sector_size,
        uint32_t anchor_sector, pfs_udf_vol_scan_t *p_scan)
{
    if (NULL == p_blkio || NULL == p_scan)
    {
        return PFS_UDF_RET_EINVAL;
    }

    // block_size 可为 2048; AVDP 结构仅 512B, 须按整块读入再解析, 禁止栈上小缓冲
    uint32_t block_size = sector_size;
    if (block_size < (uint32_t)sizeof(pfs_udf_anchor_vol_desc_ptr_t))
    {
        return PFS_UDF_RET_EBADMSG;
    }
    uint8_t *buf = (uint8_t *)pfs_malloc((size_t)block_size);
    if (NULL == buf)
    {
        return PFS_UDF_RET_ENOMEM;
    }

    int rc = pfs_udf_read_tagged(p_blkio, part_offset, sector_size, block_size, anchor_sector,
            PFS_UDF_TAG_IDENT_AVDP, buf);
    if (PFS_UDF_RET_OK != rc)
    {
        pfs_free(buf);
        return rc;
    }

    const pfs_udf_anchor_vol_desc_ptr_t *p_avdp =
            (const pfs_udf_anchor_vol_desc_ptr_t *)buf;
    uint32_t main_s = pfs_udf_le32(&p_avdp->mainVolDescSeqExt.extLocation);
    if (0 == main_s)
    {
        pfs_free(buf);
        return PFS_UDF_RET_EBADMSG;
    }
    uint32_t main_e = main_s + (pfs_udf_le32(&p_avdp->mainVolDescSeqExt.extLength) / block_size);
    if (main_e > 0)
    {
        main_e--;
    }
    pfs_free(buf);
    return pfs_udf_process_sequence(p_blkio, part_offset, sector_size, block_size, main_s, main_e,
            p_scan);
}

static int pfs_udf_scan_anchors(pfs_blkio_t *p_blkio, int64_t part_offset,
        pfs_udf_vol_scan_t *p_scan, uint32_t *p_sector_size)
{
    uint32_t sector_sizes[2] = { 2048u, 512u };
    int64_t dev_size = pfs_blkio_get_size(p_blkio);

    for (int si = 0; si < 2; si++)
    {
        uint32_t ss;
        uint32_t anchors[4];
        int n_anchor;

        ss = sector_sizes[si];
        n_anchor = 0;
        anchors[n_anchor++] = 256u;
        if (dev_size > 0)
        {
            uint32_t last_sec;
            last_sec = (uint32_t)(dev_size / (int64_t)ss);
            if (last_sec > 256u)
            {
                anchors[n_anchor++] = last_sec - 256u;
            }
            if (last_sec > 0)
            {
                anchors[n_anchor++] = last_sec - 1u;
            }
        }
        anchors[n_anchor++] = 512u;

        pfs_udf_vol_scan_init(p_scan);
        for (int ai = 0; ai < n_anchor; ai++)
        {
            int rc = pfs_udf_try_anchor(p_blkio, part_offset, ss, anchors[ai], p_scan);
            if (PFS_UDF_RET_OK == rc)
            {
                *p_sector_size = ss;
                return PFS_UDF_RET_OK;
            }
            pfs_udf_vol_scan_init(p_scan);
        }
    }
    return PFS_UDF_RET_ENOEXEC;
}

static int pfs_udf_fill_sbi_from_scan(pfs_udf_sbi_t *p_sbi, const pfs_udf_vol_scan_t *p_scan,
        uint32_t sector_size)
{

    if (NULL == p_sbi || NULL == p_scan)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (0 == pfs_udf_pmap_have_type1_pd(&p_scan->sbi_stub))
    {
        return PFS_UDF_RET_EBADMSG;
    }

    p_sbi->sector_size = sector_size;
    p_sbi->block_size = p_scan->block_size;
    if (0 == p_sbi->block_size || 0 != (p_sbi->block_size % p_sbi->sector_size))
    {
        return PFS_UDF_RET_EBADMSG;
    }
    p_sbi->blocks_per_sector = p_sbi->block_size / p_sbi->sector_size;
    p_sbi->n_maps = p_scan->sbi_stub.n_maps;
    memcpy(p_sbi->maps, p_scan->sbi_stub.maps, (size_t)p_sbi->n_maps * sizeof(p_sbi->maps[0]));

    for (int i = 0; i < p_sbi->n_maps; i++)
    {
        if (PFS_UDF_PMAP_TYPE1 == p_sbi->maps[i].type && 0 != p_sbi->maps[i].phys_len)
        {
            p_sbi->partition_start = p_sbi->maps[i].phys_start;
            p_sbi->partition_len = p_sbi->maps[i].phys_len;
            p_sbi->partition_num = p_sbi->maps[i].partition_num;
            break;
        }
    }
    return PFS_UDF_RET_OK;
}

static void pfs_udf_apply_pd_bitmap(pfs_udf_sbi_t *p_sbi, const pfs_udf_vol_scan_t *p_scan)
{

    if (NULL == p_sbi || NULL == p_scan)
    {
        return;
    }
    for (int pi = 0; pi < p_scan->n_pd; pi++)
    {
        if (p_scan->pd[pi].partition_number != p_sbi->partition_num)
        {
            continue;
        }
        p_sbi->pd_access_type = p_scan->pd[pi].access_type;
        if (0 != (p_scan->pd[pi].bitmap_ext_length & PFS_UDF_EXT_LENGTH_MASK))
        {
            p_sbi->have_unalloc_bitmap = 1;
            p_sbi->bitmap_lba = p_scan->pd[pi].bitmap_ext_position;
            if (0 != p_sbi->block_size)
            {
                p_sbi->bitmap_blocks = (p_scan->pd[pi].bitmap_ext_length &
                        PFS_UDF_EXT_LENGTH_MASK) / p_sbi->block_size;
            }
        }
        break;
    }
    p_sbi->bitmap_part_ref = 0;
    for (int i = 0; i < p_sbi->n_maps; i++)
    {
        if (PFS_UDF_PMAP_TYPE1 == p_sbi->maps[i].type &&
                p_sbi->maps[i].partition_num == p_sbi->partition_num)
        {
            p_sbi->bitmap_part_ref = (uint16_t)i;
            break;
        }
    }
}

static int pfs_udf_finish_mount(pfs_udf_sbi_t *p_sbi, pfs_udf_mount_state_e mode)
{

    if (PFS_UDF_MOUNT_RDWR == mode)
    {
        if (!pfs_udf_volume_can_rw(p_sbi))
        {
            pfs_udf_maps_free(p_sbi);
            return PFS_UDF_RET_EPERM;
        }
        int rc = pfs_udf_bitmap_load(p_sbi);
        if (PFS_UDF_RET_OK != rc)
        {
            pfs_udf_bitmap_free(p_sbi);
            pfs_udf_maps_free(p_sbi);
            return rc;
        }
        p_sbi->mount_state = PFS_UDF_MOUNT_RDWR;
    }
    else if (PFS_UDF_MOUNT_RDONLY == mode)
    {
        p_sbi->mount_state = PFS_UDF_MOUNT_RDONLY;
    }
    else
    {
        pfs_udf_maps_free(p_sbi);
        return PFS_UDF_RET_EINVAL;
    }
    if (0 == p_sbi->next_unique_id)
    {
        p_sbi->next_unique_id = 16;
    }
    return PFS_UDF_RET_OK;
}

static int pfs_udf_load_root(pfs_udf_sbi_t *p_sbi, const pfs_udf_lb_addr_t *p_fsd_loc,
        pfs_udf_inode_t *p_root)
{
    if (NULL == p_sbi || NULL == p_fsd_loc || NULL == p_root)
    {
        return PFS_UDF_RET_EINVAL;
    }

    uint8_t *blk = (uint8_t *)pfs_malloc((size_t)p_sbi->block_size);
    if (NULL == blk)
    {
        return PFS_UDF_RET_ENOMEM;
    }

    uint16_t fsd_ref = pfs_udf_le16(&p_fsd_loc->partitionReferenceNum);
    int rc = pfs_udf_read_part_block_ref(p_sbi, fsd_ref,
            pfs_udf_le32(&p_fsd_loc->logicalBlockNum), blk);
    if (PFS_UDF_RET_OK != rc)
    {
        pfs_free(blk);
        return rc;
    }
    const pfs_udf_file_set_desc_t *p_fsd = (const pfs_udf_file_set_desc_t *)blk;
    if (PFS_UDF_TAG_IDENT_FSD != pfs_udf_le16(&p_fsd->descTag.tagIdent))
    {
        pfs_free(blk);
        return PFS_UDF_RET_EBADMSG;
    }
    rc = pfs_udf_icb_read(p_sbi, &p_fsd->rootDirectoryICB.extLocation, p_root);
    pfs_free(blk);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    if (PFS_UDF_TYPE_DIR != p_root->type)
    {
        return PFS_UDF_RET_EBADMSG;
    }
    return PFS_UDF_RET_OK;
}

int pfs_udf_read_volumes(pfs_blkio_t *p_blkio, int64_t part_offset, pfs_udf_sbi_t *p_out)
{
    if (NULL == p_blkio || 0 > part_offset || NULL == p_out)
    {
        return PFS_UDF_RET_EINVAL;
    }

    pfs_udf_vol_scan_t scan;
    uint32_t sector_size = 0;
    int rc = pfs_udf_scan_anchors(p_blkio, part_offset, &scan, &sector_size);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }

    memset(p_out, 0, sizeof(*p_out));
    p_out->mount_state = PFS_UDF_UNMOUNTED;
    p_out->p_blkio = p_blkio;
    p_out->part_offset = part_offset;

    rc = pfs_udf_fill_sbi_from_scan(p_out, &scan, sector_size);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    pfs_udf_apply_pd_bitmap(p_out, &scan);

    rc = pfs_udf_maps_postload(p_out);
    if (PFS_UDF_RET_OK != rc)
    {
        pfs_udf_maps_free(p_out);
        return rc;
    }

    rc = pfs_udf_load_root(p_out, &scan.fileset_loc, &p_out->root);
    if (PFS_UDF_RET_OK != rc)
    {
        pfs_udf_maps_free(p_out);
        return rc;
    }
    return PFS_UDF_RET_OK;
}

int pfs_udf_init(pfs_udf_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return PFS_UDF_RET_EINVAL;
    }
    memset(p_sbi, 0, sizeof(*p_sbi));
    p_sbi->mount_state = PFS_UDF_UNMOUNTED;
    return PFS_UDF_RET_OK;
}

void pfs_udf_deinit(pfs_udf_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return;
    }
    (void)pfs_udf_umount(p_sbi);
    memset(p_sbi, 0, sizeof(*p_sbi));
}

int pfs_udf_probe(pfs_blkio_t *p_blkio, int64_t part_offset)
{
    pfs_udf_sbi_t tmp;

    int rc = pfs_udf_read_volumes(p_blkio, part_offset, &tmp);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    pfs_udf_maps_free(&tmp);
    return PFS_UDF_RET_OK;
}

int pfs_udf_mount(pfs_udf_sbi_t *p_sbi, pfs_blkio_t *p_blkio, int64_t part_offset,
        pfs_udf_mount_state_e mode)
{

    if (NULL == p_sbi || NULL == p_blkio || 0 > part_offset)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (PFS_UDF_UNMOUNTED != p_sbi->mount_state)
    {
        return PFS_UDF_RET_EBUSY;
    }

    int rc = pfs_udf_read_volumes(p_blkio, part_offset, p_sbi);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    return pfs_udf_finish_mount(p_sbi, mode);
}

int pfs_udf_remount(pfs_udf_sbi_t *p_sbi, pfs_udf_mount_state_e mode)
{

    if (NULL == p_sbi)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (PFS_UDF_UNMOUNTED == p_sbi->mount_state)
    {
        return PFS_UDF_RET_EBUSY;
    }
    if (PFS_UDF_MOUNT_RDWR == mode)
    {
        if (!pfs_udf_volume_can_rw(p_sbi))
        {
            return PFS_UDF_RET_EPERM;
        }
        if (NULL == p_sbi->bitmap)
        {
            int rc = pfs_udf_bitmap_load(p_sbi);
            if (PFS_UDF_RET_OK != rc)
            {
                return rc;
            }
        }
        p_sbi->mount_state = PFS_UDF_MOUNT_RDWR;
        if (0 == p_sbi->next_unique_id)
        {
            p_sbi->next_unique_id = 16;
        }
        return PFS_UDF_RET_OK;
    }
    if (PFS_UDF_MOUNT_RDONLY == mode)
    {
        if (PFS_UDF_MOUNT_RDWR == p_sbi->mount_state)
        {
            int rc = pfs_udf_bitmap_flush(p_sbi);
            if (PFS_UDF_RET_OK != rc)
            {
                return rc;
            }
        }
        p_sbi->mount_state = PFS_UDF_MOUNT_RDONLY;
        return PFS_UDF_RET_OK;
    }
    return PFS_UDF_RET_EINVAL;
}

int pfs_udf_umount(pfs_udf_sbi_t *p_sbi)
{

    if (NULL == p_sbi)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (PFS_UDF_MOUNT_RDWR == p_sbi->mount_state)
    {
        int rc = pfs_udf_bitmap_flush(p_sbi);
        if (PFS_UDF_RET_OK != rc)
        {
            return rc;
        }
    }
    pfs_udf_bitmap_free(p_sbi);
    pfs_udf_maps_free(p_sbi);
    p_sbi->p_blkio = NULL;
    p_sbi->part_offset = 0;
    p_sbi->mount_state = PFS_UDF_UNMOUNTED;
    p_sbi->have_unalloc_bitmap = 0;
    p_sbi->bitmap_lba = 0;
    p_sbi->bitmap_blocks = 0;
    p_sbi->pd_access_type = 0;
    return PFS_UDF_RET_OK;
}

int pfs_udf_sbi_is_mounted(const pfs_udf_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return 0;
    }
    return (PFS_UDF_UNMOUNTED != p_sbi->mount_state) ? 1 : 0;
}

int pfs_udf_sbi_can_write(const pfs_udf_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return 0;
    }
    return (PFS_UDF_MOUNT_RDWR == p_sbi->mount_state) ? 1 : 0;
}

// end
