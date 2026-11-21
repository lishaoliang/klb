// Doc Encode : UTF-8 BOM, Unix(LF)
// VDI probe: 偏移 64 处 signature; 校验 1.1 / DYNAMIC|FIXED / 布局
#include "blkio_vdi/pfs_vdi_internal.h"
#include "base/pfs_blkio_raw.h"
#include "pfs_endian.h"

#include <string.h>
#include <stdbool.h>

static bool pfs_vdi_uuid_is_null(const uint8_t *p)
{

    for (uint32_t i = 0; i < 16u; i++)
    {
        if (0u != p[i])
        {
            return false;
        }
    }
    return true;
}

int pfs_vdi_probe_header(pfs_blkio_t *p_host, pfs_vdi_header_t *p_hdr)
{
    if (NULL == p_host || NULL == p_hdr)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    uint8_t raw[PFS_VDI_PROBE_SIZE];
    memset(raw, 0, sizeof(raw));
    int rc = pfs_blkio_raw_pread(p_host, raw, (int)sizeof(raw), 0);
    if (0 > rc)
    {
        if (PFS_BLKIO_RET_PARAM == rc || PFS_BLKIO_RET_STATE == rc)
        {
            return rc;
        }
        return PFS_BLKIO_RET_PROBE;
    }
    if (rc < (int)(PFS_VDI_PRE_HEADER_SIZE + 4))
    {
        return PFS_BLKIO_RET_PROBE;
    }

    const uint8_t *p = raw + PFS_VDI_PRE_HEADER_SIZE;
    uint32_t signature = pfs_read_le32(p + PFS_VDI_OFF_SIGNATURE);
    if (PFS_VDI_SIGNATURE != signature)
    {
        return PFS_BLKIO_RET_PROBE;
    }

    if (rc < (int)(PFS_VDI_PRE_HEADER_SIZE + PFS_VDI_HEADER_SIZE_MIN))
    {
        return PFS_BLKIO_RET_STATE;
    }

    memset(p_hdr, 0, sizeof(*p_hdr));
    p_hdr->signature = signature;
    p_hdr->version = pfs_read_le32(p + PFS_VDI_OFF_VERSION);
    p_hdr->header_size = pfs_read_le32(p + PFS_VDI_OFF_HEADER_SIZE);
    p_hdr->image_type = pfs_read_le32(p + PFS_VDI_OFF_IMAGE_TYPE);
    p_hdr->offset_blocks = pfs_read_le32(p + PFS_VDI_OFF_OFFSET_BLOCKS);
    p_hdr->offset_data = pfs_read_le32(p + PFS_VDI_OFF_OFFSET_DATA);
    p_hdr->sector_size = pfs_read_le32(p + PFS_VDI_OFF_SECTOR_SIZE);
    p_hdr->disk_size = pfs_read_le64(p + PFS_VDI_OFF_DISK_SIZE);
    p_hdr->block_size = pfs_read_le32(p + PFS_VDI_OFF_BLOCK_SIZE);
    p_hdr->block_extra = pfs_read_le32(p + PFS_VDI_OFF_BLOCK_EXTRA);
    p_hdr->blocks_in_hdd = pfs_read_le32(p + PFS_VDI_OFF_BLOCKS_IN_HDD);
    p_hdr->blocks_allocated = pfs_read_le32(p + PFS_VDI_OFF_BLOCKS_ALLOCATED);

    if (PFS_VDI_VERSION_1_1 != p_hdr->version)
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (PFS_VDI_TYPE_DYNAMIC != p_hdr->image_type &&
            PFS_VDI_TYPE_FIXED != p_hdr->image_type)
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (PFS_VDI_SECTOR_SIZE != p_hdr->sector_size)
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (PFS_VDI_DEFAULT_BLOCK_SIZE != p_hdr->block_size)
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (0u != p_hdr->block_extra)
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (0u != (p_hdr->offset_blocks % PFS_VDI_SECTOR_SIZE) ||
            0u != (p_hdr->offset_data % PFS_VDI_SECTOR_SIZE))
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (0u == p_hdr->blocks_in_hdd ||
            p_hdr->blocks_in_hdd > PFS_VDI_BLOCKS_IN_IMAGE_MAX)
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (p_hdr->blocks_allocated > p_hdr->blocks_in_hdd)
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (p_hdr->disk_size > (uint64_t)p_hdr->blocks_in_hdd * (uint64_t)p_hdr->block_size)
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (rc >= (int)(PFS_VDI_PRE_HEADER_SIZE + PFS_VDI_OFF_UUID_PARENT + 16u))
    {
        if (!pfs_vdi_uuid_is_null(p + PFS_VDI_OFF_UUID_LINK) ||
                !pfs_vdi_uuid_is_null(p + PFS_VDI_OFF_UUID_PARENT))
        {
            return PFS_BLKIO_RET_STATE;
        }
    }

    // 奇数容量向上对齐扇区 (与 QEMU 一致)
    if (0u != (p_hdr->disk_size % (uint64_t)PFS_VDI_SECTOR_SIZE))
    {
        p_hdr->disk_size = (p_hdr->disk_size + (uint64_t)PFS_VDI_SECTOR_SIZE - 1ull) &
                ~((uint64_t)PFS_VDI_SECTOR_SIZE - 1ull);
    }

    return PFS_BLKIO_RET_OK;
}

// end
