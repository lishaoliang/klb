// Doc Encode : UTF-8 BOM, Unix(LF)
// 简易对齐缓冲 (仅分配/释放内存)
#include "base/pfs_buf.h"
#include "base/pfs_mem.h"

#include <limits.h>
#include <string.h>

// 返回值数值对齐 pfs_ret_e
#define PFS_BUF_OK          0
#define PFS_BUF_ENOMEM      (-12)
#define PFS_BUF_EBUSY       (-16)
#define PFS_BUF_EINVAL      (-22)

int pfs_buf_init(pfs_buf_t *p_buf, int buf_len)
{
    if (NULL == p_buf || buf_len <= 0)
    {
        return PFS_BUF_EINVAL;
    }
    // 已持有数据区则须先 deinit
    if (NULL != p_buf->p_data)
    {
        return PFS_BUF_EBUSY;
    }

    // 容量向上取整到 4K
    uint32_t alloc_len = (uint32_t)buf_len;
    alloc_len = (alloc_len + PFS_BUF_ALIGN_4K - 1u) & ~(PFS_BUF_ALIGN_4K - 1u);
    if (alloc_len > (uint32_t)INT_MAX)
    {
        return PFS_BUF_EINVAL;
    }

    uint8_t *p_data = (uint8_t *)pfs_aligned_malloc((size_t)PFS_BUF_ALIGN_4K, (size_t)alloc_len);
    if (NULL == p_data)
    {
        return PFS_BUF_ENOMEM;
    }

    memset(p_buf, 0, sizeof(*p_buf));
    p_buf->buf_len = (int)alloc_len;
    p_buf->start = 0;
    p_buf->end = 0;
    p_buf->size = 0;
    p_buf->p_data = p_data;
    return PFS_BUF_OK;
}

int pfs_buf_deinit(pfs_buf_t *p_buf)
{
    if (NULL == p_buf)
    {
        return PFS_BUF_EINVAL;
    }

    if (NULL != p_buf->p_data)
    {
        pfs_aligned_free(p_buf->p_data);
        p_buf->p_data = NULL;
    }

    p_buf->buf_len = 0;
    p_buf->start = 0;
    p_buf->end = 0;
    p_buf->size = 0;
    return PFS_BUF_OK;
}

// end
