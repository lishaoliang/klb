// Doc Encode : UTF-8 BOM, Unix(LF)
// qcow2 guest 簇映射读写: 未分配/ZERO 读 0; 写时分配; 压缩拒绝
#include "blkio_qcow2/pfs_qcow2_internal.h"
#include "base/pfs_blkio_raw.h"
#include "base/pfs_mem.h"

#include <stdbool.h>
#include <string.h>

static bool pfs_qcow2_range_ok(const pfs_qcow2_priv_t *p_priv, int64_t offset, int size)
{
    if (offset < 0 || size < 0 || size > PFS_BLKIO_IO_SIZE_MAX)
    {
        return false;
    }
    if (0 == size)
    {
        return true;
    }

    uint64_t off_u = (uint64_t)offset;
    if (off_u >= p_priv->virtual_size)
    {
        return false;
    }
    uint64_t end_u = off_u + (uint64_t)size;
    if (end_u < off_u || end_u > p_priv->virtual_size)
    {
        return false;
    }
    return true;
}

static int pfs_qcow2_read_one_cluster(pfs_qcow2_priv_t *p_priv, uint8_t *buf,
        int size, int64_t offset)
{
    uint64_t guest_clu_off = ((uint64_t)offset) & ~((uint64_t)p_priv->cluster_size - 1ull);
    uint32_t off_in_clu = (uint32_t)((uint64_t)offset - guest_clu_off);
    uint64_t l2_entry = 0;
    int rc = pfs_qcow2_l2_get(p_priv, guest_clu_off, &l2_entry);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    if (0 != (l2_entry & PFS_QCOW2_OFLAG_COMPRESSED))
    {
        return PFS_BLKIO_RET_STATE;
    }

    if (0 != (l2_entry & PFS_QCOW2_OFLAG_ZERO) || 0 == (l2_entry & PFS_QCOW2_OFFSET_MASK))
    {
        memset(buf, 0, (size_t)size);
        return size;
    }

    uint64_t host_off = (l2_entry & PFS_QCOW2_OFFSET_MASK) + (uint64_t)off_in_clu;
    if ((int64_t)(host_off + (uint64_t)size) > p_priv->host_file_size)
    {
        return PFS_BLKIO_RET_IO;
    }

    rc = pfs_blkio_raw_pread(&p_priv->host, buf, size, (int64_t)host_off);
    if (rc < 0)
    {
        return rc;
    }
    else if (rc != size)
    {
        return PFS_BLKIO_RET_IO;
    }
    return size;
}

int pfs_qcow2_cluster_pread(pfs_qcow2_priv_t *p_priv, void *buf, int size, int64_t offset)
{
    if (NULL == p_priv || NULL == buf)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 == size)
    {
        return PFS_BLKIO_RET_OK;
    }
    if (!pfs_qcow2_range_ok(p_priv, offset, size))
    {
        return PFS_BLKIO_RET_PARAM;
    }

    uint8_t *p = (uint8_t *)buf;
    int done = 0;
    while (done < size)
    {
        uint32_t off_in_clu = (uint32_t)(((uint64_t)offset + (uint64_t)done) &
                ((uint64_t)p_priv->cluster_size - 1ull));
        int n = (int)(p_priv->cluster_size - off_in_clu);
        if (n > size - done)
        {
            n = size - done;
        }
        int rc = pfs_qcow2_read_one_cluster(p_priv, p + done, n, offset + (int64_t)done);
        if (rc < 0)
        {
            return rc;
        }
        else if (rc != n)
        {
            return PFS_BLKIO_RET_IO;
        }
        done += n;
    }
    return size;
}

// 确保 guest 簇可写并返回宿主偏移 (含簇内偏移前的簇基址)
static int pfs_qcow2_ensure_guest_cluster(pfs_qcow2_priv_t *p_priv, uint64_t guest_clu_off,
        uint64_t *p_host_clu_off)
{
    uint64_t l2_entry = 0;
    int rc = pfs_qcow2_l2_get(p_priv, guest_clu_off, &l2_entry);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    if (0 != (l2_entry & PFS_QCOW2_OFLAG_COMPRESSED))
    {
        return PFS_BLKIO_RET_STATE;
    }

    uint64_t old_off = l2_entry & PFS_QCOW2_OFFSET_MASK;

    // 已分配且 COPIED: 可原地写
    if (0 != old_off && 0 != (l2_entry & PFS_QCOW2_OFLAG_COPIED) &&
            0 == (l2_entry & PFS_QCOW2_OFLAG_ZERO))
    {
        *p_host_clu_off = old_off;
        return PFS_BLKIO_RET_OK;
    }

    // 需新簇: 未分配 / ZERO / 需 COW
    uint64_t new_off = 0;
    rc = pfs_qcow2_alloc_cluster(p_priv, &new_off);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    uint64_t old_rc = 0;
    if (0 != old_off && 0 == (l2_entry & PFS_QCOW2_OFLAG_ZERO))
    {
        // COW: 复制旧数据; 须先 l2_set 成功再减旧 refcount, 避免失败后簇被重用撞车
        uint8_t *p_tmp = (uint8_t *)pfs_malloc((size_t)p_priv->cluster_size);
        if (NULL == p_tmp)
        {
            return PFS_BLKIO_RET_IO;
        }
        rc = pfs_blkio_raw_pread(&p_priv->host, p_tmp, (int)p_priv->cluster_size, (int64_t)old_off);
        if (rc < 0)
        {
            pfs_free(p_tmp);
            return rc;
        }
        else if (rc != (int)p_priv->cluster_size)
        {
            pfs_free(p_tmp);
            return PFS_BLKIO_RET_IO;
        }
        rc = pfs_blkio_raw_pwrite(&p_priv->host, p_tmp, (int)p_priv->cluster_size, (int64_t)new_off);
        pfs_free(p_tmp);
        if (rc < 0)
        {
            return rc;
        }
        else if (rc != (int)p_priv->cluster_size)
        {
            return PFS_BLKIO_RET_IO;
        }

        rc = pfs_qcow2_refcount_get(p_priv, old_off, &old_rc);
        if (PFS_BLKIO_RET_OK != rc)
        {
            return rc;
        }
    }

    rc = pfs_qcow2_l2_set(p_priv, guest_clu_off, new_off | PFS_QCOW2_OFLAG_COPIED);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    if (old_rc > 0)
    {
        rc = pfs_qcow2_refcount_set(p_priv, old_off, old_rc - 1ull);
        if (PFS_BLKIO_RET_OK != rc)
        {
            return rc;
        }
    }

    *p_host_clu_off = new_off;
    return PFS_BLKIO_RET_OK;
}

static int pfs_qcow2_write_one_cluster(pfs_qcow2_priv_t *p_priv, const uint8_t *buf,
        int size, int64_t offset)
{
    uint64_t guest_clu_off = ((uint64_t)offset) & ~((uint64_t)p_priv->cluster_size - 1ull);
    uint32_t off_in_clu = (uint32_t)((uint64_t)offset - guest_clu_off);
    uint64_t host_clu_off = 0;
    int rc = pfs_qcow2_ensure_guest_cluster(p_priv, guest_clu_off, &host_clu_off);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    rc = pfs_blkio_raw_pwrite(&p_priv->host, buf, size,
            (int64_t)(host_clu_off + (uint64_t)off_in_clu));
    if (rc < 0)
    {
        return rc;
    }
    else if (rc != size)
    {
        return PFS_BLKIO_RET_IO;
    }
    if ((int64_t)(host_clu_off + p_priv->cluster_size) > p_priv->host_file_size)
    {
        p_priv->host_file_size = (int64_t)(host_clu_off + p_priv->cluster_size);
    }
    return size;
}

int pfs_qcow2_cluster_pwrite(pfs_qcow2_priv_t *p_priv, const void *buf, int size, int64_t offset)
{
    if (NULL == p_priv || NULL == buf)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 == size)
    {
        return PFS_BLKIO_RET_OK;
    }
    if (!pfs_qcow2_range_ok(p_priv, offset, size))
    {
        return PFS_BLKIO_RET_PARAM;
    }

    const uint8_t *p = (const uint8_t *)buf;
    int done = 0;
    while (done < size)
    {
        uint32_t off_in_clu = (uint32_t)(((uint64_t)offset + (uint64_t)done) &
                ((uint64_t)p_priv->cluster_size - 1ull));
        int n = (int)(p_priv->cluster_size - off_in_clu);
        if (n > size - done)
        {
            n = size - done;
        }
        int rc = pfs_qcow2_write_one_cluster(p_priv, p + done, n, offset + (int64_t)done);
        if (rc < 0)
        {
            return rc;
        }
        else if (rc != n)
        {
            return PFS_BLKIO_RET_IO;
        }
        done += n;
    }
    return size;
}

// end
