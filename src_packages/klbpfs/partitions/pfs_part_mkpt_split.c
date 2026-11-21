// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_part_mkpt_split.c
/// @brief   mkpt 均分可用 LBA 区间
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    partitions 模块内部; 非稳定公开 API
///////////////////////////////////////////////////////////////////////////

#include "pfs_part_internal.h"

#include <stddef.h>
#include <stdint.h>

int pfs_part_mkpt_split_even(uint64_t first_usable, uint64_t last_usable, uint64_t align_lba,
        int count, uint64_t *p_starts, uint64_t *p_sizes)
{
    if (NULL == p_starts || NULL == p_sizes)
    {
        return PFS_PART_RET_EINVAL;
    }
    if (count < 1 || count > PFS_PART_ENTRY_MAX)
    {
        return PFS_PART_RET_EINVAL;
    }
    if (0 == align_lba)
    {
        return PFS_PART_RET_EINVAL;
    }
    if (last_usable < first_usable)
    {
        return PFS_PART_RET_EINVAL;
    }

    uint64_t usable = last_usable - first_usable + 1ULL;
    if (usable < (uint64_t)count)
    {
        return PFS_PART_RET_EINVAL;
    }

    uint64_t chunk = usable / (uint64_t)count;
    chunk = (chunk / align_lba) * align_lba;
    if (0 == chunk)
    {
        return PFS_PART_RET_EINVAL;
    }

    uint64_t start = first_usable;
    for (int i = 0; i < count; i++)
    {
        uint64_t size;

        if (i == count - 1)
        {
            size = last_usable - start + 1ULL;
        }
        else
        {
            size = chunk;
        }

        if (0 == size || start > last_usable || start + size - 1ULL > last_usable)
        {
            return PFS_PART_RET_EINVAL;
        }

        p_starts[i] = start;
        p_sizes[i] = size;
        start += size;
    }

    return PFS_PART_RET_OK;
}

// end
