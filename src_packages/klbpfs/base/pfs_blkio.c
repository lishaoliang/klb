// Doc Encode : UTF-8 BOM, Unix(LF)
#include "pfs_blkio.h"
#include "base/pfs_blkio_raw.h"
#include "blkio_vhd/pfs_vhd.h"
#include "blkio_vhdx/pfs_vhdx.h"
#include "blkio_qcow2/pfs_qcow2.h"
#include "blkio_vmdk/pfs_vmdk.h"
#include "blkio_vdi/pfs_vdi.h"

#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <stdbool.h>


#define PFS_BLKIO_STATUS_MASK ( PFS_BLKIO_NOT_OPEN | PFS_BLKIO_CLOSED | PFS_BLKIO_ERR | \
                                PFS_BLKIO_ERR_READ | PFS_BLKIO_ERR_WRITE | PFS_BLKIO_ERR_FSYNC)

#define PFS_BLKIO_IO_ERR_MASK (PFS_BLKIO_ERR | PFS_BLKIO_ERR_READ | PFS_BLKIO_ERR_WRITE | PFS_BLKIO_ERR_FSYNC)


int pfs_blkio_align_size(const pfs_blkio_t *p_blkio)
{
    if (NULL == p_blkio)
    {
        return 0;
    }
    return p_blkio->align_size;
}

bool pfs_blkio_is_open(pfs_blkio_t *p_blkio)
{
    if (NULL == p_blkio)
    {
        return false;
    }
    if (p_blkio->status & (PFS_BLKIO_NOT_OPEN | PFS_BLKIO_CLOSED))
    {
        return false;
    }
    if (NULL == p_blkio->ops || NULL == p_blkio->priv)
    {
        return false;
    }
    return true;
}

static bool pfs_blkio_status_valid(pfs_blkio_t *p_blkio, int status)
{
    if (NULL == p_blkio)
    {
        return false;
    }
    if (0 != (status & ~PFS_BLKIO_STATUS_MASK))
    {
        return false;
    }
    if ((status & PFS_BLKIO_NOT_OPEN) && (status & PFS_BLKIO_CLOSED))
    {
        return false;
    }
    if (pfs_blkio_is_open(p_blkio) && (status & (PFS_BLKIO_NOT_OPEN | PFS_BLKIO_CLOSED)))
    {
        return false;
    }
    if (!pfs_blkio_is_open(p_blkio))
    {
        if (0 == status || 0 == (status & (PFS_BLKIO_NOT_OPEN | PFS_BLKIO_CLOSED)))
        {
            return false;
        }
    }
    return true;
}

int pfs_blkio_set_status(pfs_blkio_t *p_blkio, int status)
{
    if (NULL == p_blkio || !pfs_blkio_status_valid(p_blkio, status))
    {
        return PFS_BLKIO_RET_PARAM;
    }

    p_blkio->status = status;
    if (0 == (status & PFS_BLKIO_IO_ERR_MASK))
    {
        p_blkio->last_error = 0;
    }
    return 0;
}

int pfs_blkio_last_error(pfs_blkio_t *p_blkio)
{
    if (NULL == p_blkio)
    {
        return 0;
    }
    return p_blkio->last_error;
}

int pfs_blkio_status(pfs_blkio_t *p_blkio)
{
    if (NULL == p_blkio)
    {
        return PFS_BLKIO_ERR;
    }
    return p_blkio->status;
}

int pfs_blkio_init(pfs_blkio_t *p_blkio)
{
    if (NULL == p_blkio)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    memset(p_blkio, 0, sizeof(*p_blkio));
    p_blkio->ops = NULL;
    p_blkio->priv = NULL;
    p_blkio->status = PFS_BLKIO_NOT_OPEN;
    return PFS_BLKIO_RET_OK;
}

void pfs_blkio_deinit(pfs_blkio_t *p_blkio)
{
    if (NULL == p_blkio)
    {
        return;
    }

    if (NULL != p_blkio->priv || (NULL != p_blkio->ops && 0 == (p_blkio->status & (PFS_BLKIO_NOT_OPEN | PFS_BLKIO_CLOSED))))
    {
        pfs_blkio_close(p_blkio);
    }
}

// 内建 type -> 静态 ops; 非内建返回 NULL
static const pfs_blkio_ops_t *pfs_blkio_ops_by_type(pfs_blkio_type_e type)
{
    switch (type)
    {
    case PFS_BLKIO_RAW:
        return &g_pfs_blkio_raw_ops;
    case PFS_BLKIO_VHD:
        return &g_pfs_vhd_ops;
    case PFS_BLKIO_VHDX:
        return &g_pfs_vhdx_ops;
    case PFS_BLKIO_QCOW2:
        return &g_pfs_qcow2_ops;
    case PFS_BLKIO_VMDK:
        return &g_pfs_vmdk_ops;
    case PFS_BLKIO_VDI:
        return &g_pfs_vdi_ops;
    default:
        return NULL;
    }
}

// 本句柄已 register 的 type -> ops; 未找到返回 NULL
static const pfs_blkio_ops_t *pfs_blkio_ops_find_registered(const pfs_blkio_t *p_blkio, pfs_blkio_type_e type)
{
    if (NULL == p_blkio)
    {
        return NULL;
    }
    for (int i = 0; i < p_blkio->blk_ops_count; i++)
    {
        if (NULL != p_blkio->blk_ops[i] && type == p_blkio->blk_ops[i]->type)
        {
            return p_blkio->blk_ops[i];
        }
    }
    return NULL;
}

int pfs_blkio_register(pfs_blkio_t *p_blkio, pfs_blkio_type_e type, const pfs_blkio_ops_t *ops)
{
    if (NULL == p_blkio || NULL == ops)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (type != ops->type)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    // 仅允许扩展段; 内建镜像勿 register
    if ((unsigned)type < (unsigned)PFS_BLKIO_MTD_MIN || (unsigned)type > (unsigned)PFS_BLKIO_TYPE_MAX)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (NULL == ops->open || NULL == ops->close || NULL == ops->pread || NULL == ops->pwrite)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    for (int i = 0; i < p_blkio->blk_ops_count; i++)
    {
        if (NULL != p_blkio->blk_ops[i] && type == p_blkio->blk_ops[i]->type)
        {
            return PFS_BLKIO_RET_PARAM;
        }
    }
    if (p_blkio->blk_ops_count >= PFS_BLKIO_OPS_MAX)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    p_blkio->blk_ops[p_blkio->blk_ops_count] = ops;
    p_blkio->blk_ops_count++;
    return PFS_BLKIO_RET_OK;
}

// 尝试本句柄已 register 的外部表; 全 PROBE 则返回 PROBE 供继续内建序
static int pfs_blkio_open_registered(pfs_blkio_t *p_blkio, const char *p_devpath, pfs_blkio_oflags_e flags, int sector_size)
{
    for (int i = 0; i < p_blkio->blk_ops_count; i++)
    {
        const pfs_blkio_ops_t *ops = p_blkio->blk_ops[i];
        if (NULL == ops || NULL == ops->open)
        {
            continue;
        }
        p_blkio->ops = ops;
        int rc = ops->open(p_blkio, p_devpath, flags, sector_size);
        if (PFS_BLKIO_RET_OK == rc)
        {
            return rc;
        }
        p_blkio->ops = NULL;
        if (PFS_BLKIO_RET_PROBE != rc)
        {
            return rc;
        }
    }
    return PFS_BLKIO_RET_PROBE;
}

int pfs_blkio_open(pfs_blkio_t *p_blkio, const char *p_devpath, pfs_blkio_oflags_e flags, int sector_size)
{
    if (NULL == p_blkio || NULL == p_devpath)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (NULL != p_blkio->priv || NULL != p_blkio->ops)
    {
        return PFS_BLKIO_RET_STATE;
    }

    // 先试本句柄 register 的外部后端 (注册序)
    int rc = pfs_blkio_open_registered(p_blkio, p_devpath, flags, sector_size);
    if (PFS_BLKIO_RET_PROBE != rc)
    {
        return rc;
    }

    // 探测序: vhdx -> vhd -> qcow2 -> vmdk -> vdi -> raw
    // PROBE=非该镜像格式, 回退下一后端; 其它错误 (含 Diff/打桩已识别) 不回退
    p_blkio->ops = &g_pfs_vhdx_ops;
    rc = p_blkio->ops->open(p_blkio, p_devpath, flags, sector_size);
    if (PFS_BLKIO_RET_OK == rc)
    {
        return rc;
    }
    p_blkio->ops = NULL;
    if (PFS_BLKIO_RET_PROBE != rc)
    {
        return rc;
    }

    p_blkio->ops = &g_pfs_vhd_ops;
    rc = p_blkio->ops->open(p_blkio, p_devpath, flags, sector_size);
    if (PFS_BLKIO_RET_OK == rc)
    {
        return rc;
    }
    p_blkio->ops = NULL;
    if (PFS_BLKIO_RET_PROBE != rc)
    {
        return rc;
    }

    p_blkio->ops = &g_pfs_qcow2_ops;
    rc = p_blkio->ops->open(p_blkio, p_devpath, flags, sector_size);
    if (PFS_BLKIO_RET_OK == rc)
    {
        return rc;
    }
    p_blkio->ops = NULL;
    if (PFS_BLKIO_RET_PROBE != rc)
    {
        return rc;
    }

    p_blkio->ops = &g_pfs_vmdk_ops;
    rc = p_blkio->ops->open(p_blkio, p_devpath, flags, sector_size);
    if (PFS_BLKIO_RET_OK == rc)
    {
        return rc;
    }
    p_blkio->ops = NULL;
    if (PFS_BLKIO_RET_PROBE != rc)
    {
        return rc;
    }

    p_blkio->ops = &g_pfs_vdi_ops;
    rc = p_blkio->ops->open(p_blkio, p_devpath, flags, sector_size);
    if (PFS_BLKIO_RET_OK == rc)
    {
        return rc;
    }
    p_blkio->ops = NULL;
    if (PFS_BLKIO_RET_PROBE != rc)
    {
        return rc;
    }

    p_blkio->ops = &g_pfs_blkio_raw_ops;
    rc = p_blkio->ops->open(p_blkio, p_devpath, flags, sector_size);
    if (PFS_BLKIO_RET_OK != rc)
    {
        p_blkio->ops = NULL;
    }
    return rc;
}

int pfs_blkio_open_ex(pfs_blkio_t *p_blkio, const char *p_devpath, pfs_blkio_oflags_e flags, int sector_size, pfs_blkio_type_e type)
{
    if (NULL == p_blkio || NULL == p_devpath)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    // 内建优先; 否则查本句柄已 register 的扩展表
    const pfs_blkio_ops_t *ops = pfs_blkio_ops_by_type(type);
    if (NULL == ops)
    {
        ops = pfs_blkio_ops_find_registered(p_blkio, type);
    }
    if (NULL == ops || NULL == ops->open)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (NULL != p_blkio->priv || NULL != p_blkio->ops)
    {
        return PFS_BLKIO_RET_STATE;
    }

    p_blkio->ops = ops;
    int rc = ops->open(p_blkio, p_devpath, flags, sector_size);
    if (PFS_BLKIO_RET_OK != rc)
    {
        p_blkio->ops = NULL;
    }
    return rc;
}

int pfs_blkio_open_ops(pfs_blkio_t *p_blkio, const char *p_devpath, pfs_blkio_oflags_e flags, int sector_size, const pfs_blkio_ops_t *ops)
{
    if (NULL == p_blkio || NULL == ops || NULL == ops->open)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    // 仅允许扩展段类型; 内建镜像请用 pfs_blkio_open / open_ex
    if ((unsigned)ops->type < (unsigned)PFS_BLKIO_MTD_MIN)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (NULL != p_blkio->priv || NULL != p_blkio->ops)
    {
        return PFS_BLKIO_RET_STATE;
    }

    p_blkio->ops = ops;
    int rc = ops->open(p_blkio, p_devpath, flags, sector_size);
    if (PFS_BLKIO_RET_OK != rc)
    {
        p_blkio->ops = NULL;
    }
    return rc;
}

int pfs_blkio_open_attach(pfs_blkio_t *p_blkio, const pfs_blkio_ops_t *ops, void *priv)
{
    if (NULL == p_blkio || NULL == ops || NULL == priv)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (NULL == ops->pread || NULL == ops->pwrite || NULL == ops->close)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    // 仅允许扩展段类型; 内建镜像请用 pfs_blkio_open / open_ex
    if ((unsigned)ops->type < (unsigned)PFS_BLKIO_MTD_MIN)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (NULL != p_blkio->priv || NULL != p_blkio->ops)
    {
        return PFS_BLKIO_RET_STATE;
    }

    p_blkio->ops = ops;
    p_blkio->priv = priv;
    p_blkio->align_size = 0;
    p_blkio->last_error = 0;
    p_blkio->status = 0;
    return PFS_BLKIO_RET_OK;
}

int pfs_blkio_open_direct(pfs_blkio_t *p_blkio, const char *p_devpath, pfs_blkio_oflags_e flags, int align_size)
{
    if (NULL == p_blkio || NULL == p_devpath)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (NULL != p_blkio->priv || NULL != p_blkio->ops)
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (NULL == g_pfs_blkio_raw_ops.open_direct)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    p_blkio->ops = &g_pfs_blkio_raw_ops;
    int rc = p_blkio->ops->open_direct(p_blkio, p_devpath, flags, align_size);
    if (PFS_BLKIO_RET_OK != rc)
    {
        p_blkio->ops = NULL;
    }
    return rc;
}

int pfs_blkio_pread(pfs_blkio_t *p_blkio, void *buf, int size, int64_t offset)
{
    if (NULL == p_blkio || NULL == p_blkio->ops || NULL == p_blkio->ops->pread)
    {
        return PFS_BLKIO_RET_STATE;
    }
    return p_blkio->ops->pread(p_blkio, buf, size, offset);
}

int pfs_blkio_pwrite(pfs_blkio_t *p_blkio, const void *buf, int size, int64_t offset)
{
    if (NULL == p_blkio || NULL == p_blkio->ops || NULL == p_blkio->ops->pwrite)
    {
        return PFS_BLKIO_RET_STATE;
    }
    return p_blkio->ops->pwrite(p_blkio, buf, size, offset);
}

int pfs_blkio_fsync(pfs_blkio_t *p_blkio)
{
    if (NULL == p_blkio || NULL == p_blkio->ops || NULL == p_blkio->ops->fsync)
    {
        return PFS_BLKIO_RET_STATE;
    }
    return p_blkio->ops->fsync(p_blkio);
}

int64_t pfs_blkio_get_size(const pfs_blkio_t *p_blkio)
{
    if (NULL == p_blkio || NULL == p_blkio->ops || NULL == p_blkio->ops->get_size)
    {
        return (int64_t)PFS_BLKIO_RET_STATE;
    }
    return p_blkio->ops->get_size(p_blkio);
}

int pfs_blkio_logical_sector_size(const pfs_blkio_t *p_blkio)
{
    if (NULL == p_blkio || NULL == p_blkio->ops || NULL == p_blkio->ops->logical_sector_size)
    {
        return PFS_BLKIO_RET_STATE;
    }
    return p_blkio->ops->logical_sector_size(p_blkio);
}

int pfs_blkio_create_image(pfs_blkio_type_e type, const char *path, uint64_t size_bytes, uint32_t flags, void *param, int param_size)
{
    if (NULL == path || '\0' == path[0])
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 == size_bytes || 0 != (size_bytes % 512ull))
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if ((NULL != param && 0 >= param_size) || (NULL == param && 0 != param_size))
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 == (flags & PFS_BLKIO_IMAGE_FORCE))
    {
        struct stat st;

        if (0 == stat(path, &st))
        {
            return PFS_BLKIO_RET_PARAM;
        }
    }

    if ((flags & PFS_BLKIO_IMAGE_FIXED) && (flags & PFS_BLKIO_IMAGE_DYNAMIC))
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 == (flags & (PFS_BLKIO_IMAGE_FIXED | PFS_BLKIO_IMAGE_DYNAMIC)))
    {
        flags |= PFS_BLKIO_IMAGE_DYNAMIC;
    }

    switch (type)
    {
    case PFS_BLKIO_VHD:
        if (flags & PFS_BLKIO_IMAGE_FIXED)
        {
            return pfs_vhd_create_fixed(path, size_bytes);
        }
        return pfs_vhd_create_dynamic(path, size_bytes);
    case PFS_BLKIO_VHDX:
        if (flags & PFS_BLKIO_IMAGE_FIXED)
        {
            return pfs_vhdx_create_fixed(path, size_bytes);
        }
        return pfs_vhdx_create_dynamic(path, size_bytes);
    case PFS_BLKIO_QCOW2:
        if (flags & PFS_BLKIO_IMAGE_FIXED)
        {
            return PFS_BLKIO_RET_PARAM;
        }
        return pfs_qcow2_create(path, size_bytes);
    default:
        return PFS_BLKIO_RET_PARAM;
    }
}

int pfs_blkio_close(pfs_blkio_t *p_blkio)
{
    if (NULL == p_blkio)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (NULL == p_blkio->ops || NULL == p_blkio->ops->close)
    {
        p_blkio->ops = NULL;
        p_blkio->status = PFS_BLKIO_CLOSED;
        p_blkio->align_size = 0;
        return PFS_BLKIO_RET_OK;
    }

    int rc = p_blkio->ops->close(p_blkio);
    if (PFS_BLKIO_RET_OK == rc)
    {
        p_blkio->ops = NULL;
    }
    return rc;
}

int pfs_blkio_sector_size(const char *p_devpath)
{
    return pfs_blkio_raw_sector_size(p_devpath);
}

int pfs_blkio_sector_size_physical(const char *p_devpath)
{
    return pfs_blkio_raw_sector_size_physical(p_devpath);
}

// end
