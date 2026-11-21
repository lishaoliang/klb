// Doc Encode : UTF-8 BOM, Unix(LF)
// qcow2 probe: 校验 magic/version, 转主机序头; v3 再读 feature/refcount_order
#include "blkio_qcow2/pfs_qcow2_internal.h"
#include "base/pfs_blkio_raw.h"
#include "pfs_endian.h"

#include <string.h>

int pfs_qcow2_probe_header(pfs_blkio_t *p_host, pfs_qcow2_header_t *p_hdr)
{
    if (NULL == p_host || NULL == p_hdr)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    uint8_t raw[PFS_QCOW2_HEADER_SIZE_V3];
    memset(raw, 0, sizeof(raw));
    int rc = pfs_blkio_raw_pread(p_host, raw, (int)sizeof(raw), 0);
    if (rc < 0)
    {
        if (PFS_BLKIO_RET_PARAM == rc || PFS_BLKIO_RET_STATE == rc)
        {
            return rc;
        }
        return PFS_BLKIO_RET_PROBE;
    }
    if (rc < 8)
    {
        return PFS_BLKIO_RET_PROBE;
    }

    uint32_t magic = pfs_read_be32(raw);
    if (PFS_QCOW2_MAGIC != magic)
    {
        return PFS_BLKIO_RET_PROBE;
    }

    uint32_t version = pfs_read_be32(raw + 4);
    if (PFS_QCOW2_VERSION_2 != version && PFS_QCOW2_VERSION_3 != version)
    {
        // 同族魔数但非 v2/v3: 勿回退 raw
        return PFS_BLKIO_RET_STATE;
    }

    int want = (PFS_QCOW2_VERSION_3 == version) ? PFS_QCOW2_HEADER_SIZE_V3 :
            PFS_QCOW2_HEADER_SIZE_MIN;
    if (rc < want)
    {
        return PFS_BLKIO_RET_STATE;
    }

    memset(p_hdr, 0, sizeof(*p_hdr));
    p_hdr->magic = magic;
    p_hdr->version = version;
    p_hdr->backing_file_offset = pfs_read_be64(raw + 8);
    p_hdr->backing_file_size = pfs_read_be32(raw + 16);
    p_hdr->cluster_bits = pfs_read_be32(raw + 20);
    p_hdr->size = pfs_read_be64(raw + 24);
    p_hdr->crypt_method = pfs_read_be32(raw + 32);
    p_hdr->l1_size = pfs_read_be32(raw + 36);
    p_hdr->l1_table_offset = pfs_read_be64(raw + 40);
    p_hdr->refcount_table_offset = pfs_read_be64(raw + 48);
    p_hdr->refcount_table_clusters = pfs_read_be32(raw + 56);
    p_hdr->nb_snapshots = pfs_read_be32(raw + 60);
    p_hdr->snapshots_offset = pfs_read_be64(raw + 64);

    if (PFS_QCOW2_VERSION_2 == version)
    {
        p_hdr->refcount_order = 4u;
        p_hdr->header_length = PFS_QCOW2_HEADER_SIZE_MIN;
    }
    else
    {
        p_hdr->incompatible_features = pfs_read_be64(raw + 72);
        p_hdr->compatible_features = pfs_read_be64(raw + 80);
        p_hdr->autoclear_features = pfs_read_be64(raw + 88);
        p_hdr->refcount_order = pfs_read_be32(raw + 96);
        p_hdr->header_length = pfs_read_be32(raw + 100);
        if (p_hdr->header_length < PFS_QCOW2_HEADER_SIZE_V3 || 0 != (p_hdr->header_length % 8u))
        {
            return PFS_BLKIO_RET_STATE;
        }
        if (p_hdr->refcount_order > 6u)
        {
            return PFS_BLKIO_RET_STATE;
        }
    }

    return PFS_BLKIO_RET_OK;
}

// end
