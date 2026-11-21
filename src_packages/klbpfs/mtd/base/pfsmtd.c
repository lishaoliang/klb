// Doc Encode : UTF-8 BOM, Unix(LF)
// pfsmtd 对外入口: mkfs / mkvol 按 fs_type 分派到各目标 FS
#include "pfsmtd.h"

#include "squashfs/pfs_squashfs.h"
#include "ubifs/pfs_ubifs.h"
#include "jffs2/pfs_jffs2.h"
#include "yaffs/pfs_yaffs.h"
#include "erofs/pfs_erofs.h"
#include "cramfs/pfs_cramfs.h"
#include "romfs/pfs_romfs.h"

#include <stddef.h>

static const pfs_fs_ops_t *pfsmtd_ops_by_type(pfs_fs_type_e fs_type)
{
    switch ((int)fs_type) {
    case PFSMTD_SQUASHFS:
        return pfsmtd_get_squashfs();
    case PFSMTD_UBIFS:
        return pfsmtd_get_ubifs();
    case PFSMTD_JFFS2:
        return pfsmtd_get_jffs2();
    case PFSMTD_YAFFS:
        return pfsmtd_get_yaffs();
    case PFSMTD_EROFS:
        return pfsmtd_get_erofs();
    case PFSMTD_CRAMFS:
        return pfsmtd_get_cramfs();
    case PFSMTD_ROMFS:
        return pfsmtd_get_romfs();
    default:
        break;
    }
    return NULL;
}

int pfsmtd_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes, pfs_fs_type_e fs_type)
{
    if (NULL == p_blkio || part_offset < 0)
    {
        return PFS_EINVAL;
    }
    if (!pfs_blkio_is_open(p_blkio))
    {
        return PFS_EINVAL;
    }

    const pfs_fs_ops_t *p_ops = pfsmtd_ops_by_type(fs_type);
    if (NULL == p_ops)
    {
        return PFS_EINVAL;
    }
    if (NULL == p_ops->ctrl.mkfs)
    {
        return PFS_EOPNOTSUPP;
    }
    return p_ops->ctrl.mkfs(p_blkio, part_offset, vol_bytes);
}

int pfsmtd_mkvol(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes, pfs_fs_type_e fs_type, const char *src_dir, const pfsmtd_mkvol_opts_t *opts)
{
    if (NULL == p_blkio || part_offset < 0 || NULL == src_dir || '\0' == src_dir[0])
    {
        return PFS_EINVAL;
    }
    if (!pfs_blkio_is_open(p_blkio))
    {
        return PFS_EINVAL;
    }

    const char *volume_id = NULL;
    if (NULL != opts)
    {
        volume_id = opts->volume_id;
    }

    // 只读镜像类: 一次打包; 可写 flash 类: mkfs + 灌入 src_dir
    // squashfs/romfs: 可灌树; cramfs/erofs: 空树 mkfs, 非空暂 EOPNOTSUPP
    switch ((int)fs_type) {
    case PFSMTD_SQUASHFS:
        return pfs_squashfs_mkvol(p_blkio, part_offset, vol_bytes, src_dir, volume_id);
    case PFSMTD_UBIFS:
        return pfs_ubifs_mkvol(p_blkio, part_offset, vol_bytes, src_dir, volume_id);
    case PFSMTD_JFFS2: {
        uint32_t erase_size = 0;
        if (NULL != opts)
        {
            erase_size = opts->jffs2_erase_size;
        }
        return pfs_jffs2_mkvol_ex(p_blkio, part_offset, vol_bytes, src_dir, volume_id, erase_size);
    }
    case PFSMTD_YAFFS:
        return pfs_yaffs_mkvol(p_blkio, part_offset, vol_bytes, src_dir, volume_id);
    case PFSMTD_EROFS:
        return pfs_erofs_mkvol(p_blkio, part_offset, vol_bytes, src_dir, volume_id);
    case PFSMTD_CRAMFS:
        return pfs_cramfs_mkvol(p_blkio, part_offset, vol_bytes, src_dir, volume_id);
    case PFSMTD_ROMFS:
        return pfs_romfs_mkvol(p_blkio, part_offset, vol_bytes, src_dir, volume_id);
    default:
        break;
    }
    return PFS_EINVAL;
}

// end
