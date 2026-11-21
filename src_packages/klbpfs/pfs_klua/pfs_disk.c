// Doc Encode : UTF-8 BOM, Unix(LF)
#include "pfs_disk.h"

#include "pfsmtd.h"
#include "klbmem/klb_mem.h"

#include <stdint.h>
#include <string.h>

static bool pfs_disk_sector_size_valid(int sector_size)
{
    return (PFS_BLKIO_ALIGN_512 == sector_size || PFS_BLKIO_ALIGN_1K == sector_size ||
            PFS_BLKIO_ALIGN_2K == sector_size || PFS_BLKIO_ALIGN_4K == sector_size);
}

static int pfs_disk_resolve_sector_size(int opts_sector_size, pfs_blkio_t *p_blkio)
{
    if (0 != opts_sector_size)
    {
        if (!pfs_disk_sector_size_valid(opts_sector_size))
        {
            return PFS_BLKIO_ALIGN_512;
        }
        return opts_sector_size;
    }

    if (NULL != p_blkio)
    {
        int sector_size = pfs_blkio_logical_sector_size(p_blkio);

        if (sector_size > 0 && pfs_disk_sector_size_valid(sector_size))
        {
            return sector_size;
        }
    }

    return PFS_BLKIO_ALIGN_512;
}

static int64_t pfs_disk_part_offset_from_entry(const pfs_part_entry_t *p_entry, int sector_size)
{
    if (NULL == p_entry || sector_size <= 0)
    {
        return 0;
    }
    if (0 == p_entry->size_lba)
    {
        return 0;
    }
    if (p_entry->start_lba > (uint64_t)INT64_MAX / (uint64_t)sector_size)
    {
        return -1;
    }
    return (int64_t)(p_entry->start_lba * (uint64_t)sector_size);
}

static void pfs_disk_part_clear(pfs_disk_part_t *p_part)
{
    if (NULL == p_part)
    {
        return;
    }

    if (0 != p_part->mounted && NULL != p_part->p_ctx)
    {
        (void)pfs_ctx_umount(p_part->p_ctx);
    }
    if (NULL != p_part->p_ctx)
    {
        pfs_ctx_destroy(p_part->p_ctx);
    }

    memset(p_part, 0, sizeof(*p_part));
    p_part->mount_err = PFS_OK;
    p_part->fs_type = PFS_NONE;
}

static void pfs_disk_clear_parts(pfs_disk_t *p_disk)
{
    if (NULL == p_disk)
    {
        return;
    }

    for (int i = 0; i < p_disk->part_count; i++)
    {
        pfs_disk_part_clear(&p_disk->parts[i]);
    }
    p_disk->part_count = 0;
    p_disk->part_scheme = PFS_PART_SCHEME_NONE;
}

/// @brief 归一 mount 风格: 0=r; 非 0 均视为 rw (含 Lua "w" 与历史 w=2)
static int pfs_disk_mnt_style_normalize(int mnt_style)
{
    if (mnt_style <= 0)
    {
        return 0;
    }
    return 1;
}

static int pfs_disk_part_mount_ex(pfs_disk_t *p_disk, pfs_disk_part_t *p_part, int mode, int log_fail)
{
    if (NULL == p_disk || NULL == p_part || 0 == p_disk->blkio_open || NULL == p_part->p_ctx)
    {
        return PFS_EINVAL;
    }

    if (0 != p_part->mounted)
    {
        if (p_part->mount_mode == mode)
        {
            return PFS_OK;
        }
        int rc = pfs_ctx_remount(p_part->p_ctx, mode);

        if (PFS_OK == rc)
        {
            p_part->mount_mode = mode;
            p_part->mount_err = PFS_OK;
            p_part->fs_type = pfs_ctx_fs_type(p_part->p_ctx);
            return PFS_OK;
        }
        (void)pfs_ctx_umount(p_part->p_ctx);
        p_part->mounted = 0;
        p_part->mount_mode = 0;
        p_part->fs_type = PFS_NONE;
    }

    int rc = pfs_ctx_mount(p_part->p_ctx, &p_disk->blkio, p_part->part_offset, mode);
    if (PFS_OK != rc)
    {
        p_part->mount_err = rc;
        (void)log_fail;
        return rc;
    }

    p_part->mounted = 1;
    p_part->mount_mode = mode;
    p_part->mount_err = PFS_OK;
    p_part->fs_type = pfs_ctx_fs_type(p_part->p_ctx);
    return PFS_OK;
}

static int pfs_disk_part_mount_by_style(pfs_disk_t *p_disk, pfs_disk_part_t *p_part, int mnt_style)
{
    if (NULL == p_disk || NULL == p_part)
    {
        return PFS_EINVAL;
    }

    mnt_style = pfs_disk_mnt_style_normalize(mnt_style);
    if (0 == mnt_style)
    {
        return pfs_disk_part_mount_ex(p_disk, p_part, PFS_MNT_RDONLY, 1);
    }

    if (PFS_OK == pfs_disk_part_mount_ex(p_disk, p_part, PFS_MNT_RDWR, 0))
    {
        return PFS_OK;
    }
    return pfs_disk_part_mount_ex(p_disk, p_part, PFS_MNT_RDONLY, 1);
}

static int pfs_disk_part_resolve_style(const pfs_disk_mount_opts_t *p_opts, int part_i)
{
    if (NULL == p_opts)
    {
        return 0;
    }

    if (part_i >= 0 && part_i < PFS_PART_ENTRY_MAX &&
            p_opts->part_mnt_style[part_i] != PFS_DISK_MNT_STYLE_DFLT)
    {
        return p_opts->part_mnt_style[part_i];
    }

    if (p_opts->default_mnt_style < 0)
    {
        return 0;
    }
    return p_opts->default_mnt_style;
}

static bool pfs_disk_mount_needs_rdwr(const pfs_disk_mount_opts_t *p_opts, int part_count)
{
    if (NULL == p_opts)
    {
        return false;
    }

    int style = p_opts->default_mnt_style;
    if (style < 0)
    {
        style = 0;
    }
    if (0 != pfs_disk_mnt_style_normalize(style))
    {
        return true;
    }

    for (int i = 0; i < part_count && i < PFS_PART_ENTRY_MAX; i++)
    {
        style = pfs_disk_part_resolve_style(p_opts, i);
        if (0 != pfs_disk_mnt_style_normalize(style))
        {
            return true;
        }
    }

    return false;
}

static int pfs_disk_remount_part_style(pfs_disk_part_t *p_part, int mnt_style)
{
    if (NULL == p_part || NULL == p_part->p_ctx || 0 == p_part->mounted)
    {
        return PFS_EBUSY;
    }

    mnt_style = pfs_disk_mnt_style_normalize(mnt_style);
    int target_mode = (0 == mnt_style) ? PFS_MNT_RDONLY : PFS_MNT_RDWR;

    if (p_part->mount_mode == target_mode)
    {
        return PFS_OK;
    }

    int rc = pfs_ctx_remount(p_part->p_ctx, target_mode);
    if (PFS_OK == rc)
    {
        p_part->mount_mode = target_mode;
        p_part->mount_err = PFS_OK;
        return PFS_OK;
    }

    if (1 == mnt_style)
    {
        rc = pfs_ctx_remount(p_part->p_ctx, PFS_MNT_RDONLY);
        if (PFS_OK == rc)
        {
            p_part->mount_mode = PFS_MNT_RDONLY;
            p_part->mount_err = PFS_OK;
            return PFS_OK;
        }
    }

    p_part->mount_err = rc;
    return rc;
}

static int pfs_disk_part_create(pfs_disk_part_t *p_part)
{
    if (NULL == p_part)
    {
        return PFS_EINVAL;
    }
    if (NULL != p_part->p_ctx)
    {
        return PFS_OK;
    }

    p_part->p_ctx = pfs_ctx_create();
    if (NULL == p_part->p_ctx)
    {
        p_part->mount_err = PFS_ENOMEM;
        return PFS_ENOMEM;
    }
    PFSMTD_REGISTER_FS_ALL(p_part->p_ctx);
    return PFS_OK;
}

static int pfs_disk_mount_all_parts(pfs_disk_t *p_disk, const pfs_disk_mount_opts_t *p_opts)
{
    if (NULL == p_disk || p_disk->part_count <= 0)
    {
        return PFS_EINVAL;
    }

    for (int i = 0; i < p_disk->part_count; i++)
    {
        int rc = pfs_disk_part_create(&p_disk->parts[i]);
        if (PFS_OK != rc)
        {
            return rc;
        }

        int style = pfs_disk_part_resolve_style(p_opts, i);
        (void)pfs_disk_part_mount_by_style(p_disk, &p_disk->parts[i], style);
    }

    return PFS_OK;
}

static int pfs_disk_fill_parts_from_table(pfs_disk_t *p_disk, const pfs_part_table_t *p_tbl)
{
    if (NULL == p_disk || NULL == p_tbl)
    {
        return PFS_EINVAL;
    }

    int sector_size = p_disk->sector_size;
    p_disk->part_scheme = p_tbl->scheme;
    p_disk->part_count = p_tbl->count;
    if (p_disk->part_count <= 0)
    {
        p_disk->part_count = 0;
        return PFS_OK;
    }
    if (p_disk->part_count > PFS_PART_ENTRY_MAX)
    {
        p_disk->part_count = PFS_PART_ENTRY_MAX;
    }

    for (int i = 0; i < p_disk->part_count; i++)
    {
        pfs_disk_part_t *p_part = &p_disk->parts[i];

        memset(p_part, 0, sizeof(*p_part));
        p_part->mount_err = PFS_OK;
        p_part->fs_type = PFS_NONE;
        p_part->start_lba = p_tbl->entries[i].start_lba;
        p_part->size_lba = p_tbl->entries[i].size_lba;
        int64_t part_offset = pfs_disk_part_offset_from_entry(&p_tbl->entries[i], sector_size);
        if (0 > part_offset)
        {
            p_part->mount_err = PFS_EINVAL;
            continue;
        }
        p_part->part_offset = part_offset;
    }

    return PFS_OK;
}

static int pfs_disk_setup_bare_part(pfs_disk_t *p_disk, int64_t part_offset)
{
    if (NULL == p_disk)
    {
        return PFS_EINVAL;
    }
    if (0 > part_offset)
    {
        return PFS_EINVAL;
    }

    p_disk->part_scheme = PFS_PART_SCHEME_NONE;
    p_disk->part_count = 1;
    pfs_disk_part_t *p_part = &p_disk->parts[0];
    memset(p_part, 0, sizeof(*p_part));
    p_part->mount_err = PFS_OK;
    p_part->fs_type = PFS_NONE;
    p_part->part_offset = part_offset;
    return PFS_OK;
}

void pfs_disk_init(pfs_disk_t *p_disk)
{
    if (NULL == p_disk)
    {
        return;
    }

    memset(p_disk, 0, sizeof(*p_disk));
    p_disk->part_scheme = PFS_PART_SCHEME_NONE;
}

void pfs_disk_deinit(pfs_disk_t *p_disk)
{
    if (NULL == p_disk)
    {
        return;
    }

    pfs_disk_clear_parts(p_disk);

    if (NULL != p_disk->image_path)
    {
        KLB_FREE(p_disk->image_path);
        p_disk->image_path = NULL;
    }

    if (0 != p_disk->blkio_open)
    {
        pfs_blkio_close(&p_disk->blkio);
        pfs_blkio_deinit(&p_disk->blkio);
        p_disk->blkio_open = 0;
    }

    p_disk->sector_size = 0;
}

void pfs_disk_umount(pfs_disk_t *p_disk, int part_index)
{
    if (NULL == p_disk)
    {
        return;
    }

    if (part_index < 0)
    {
        pfs_disk_deinit(p_disk);
        pfs_disk_init(p_disk);
        return;
    }

    if (part_index >= p_disk->part_count)
    {
        return;
    }

    pfs_disk_part_clear(&p_disk->parts[part_index]);
}

static int pfs_disk_mount_init_opts(pfs_disk_mount_opts_t *p_opts)
{
    if (NULL == p_opts)
    {
        return PFS_EINVAL;
    }

    p_opts->default_mnt_style = 0;
    for (int i = 0; i < PFS_PART_ENTRY_MAX; i++)
    {
        p_opts->part_mnt_style[i] = PFS_DISK_MNT_STYLE_DFLT;
    }
    p_opts->sector_size = 0;
    p_opts->part_offset = 0;
    return PFS_OK;
}

int pfs_disk_mount(pfs_disk_t *p_disk, const char *p_image_path, const pfs_disk_mount_opts_t *p_opts)
{
    if (NULL == p_disk || NULL == p_image_path || '\0' == p_image_path[0])
    {
        return PFS_EINVAL;
    }

    if (0 != p_disk->blkio_open)
    {
        pfs_disk_umount(p_disk, -1);
    }

    pfs_disk_mount_opts_t opts;

    pfs_disk_mount_init_opts(&opts);
    if (NULL != p_opts)
    {
        opts = *p_opts;
    }

    if (PFS_BLKIO_RET_OK != pfs_blkio_init(&p_disk->blkio))
    {
        return PFS_EIO;
    }

    int oflags = PFS_BLKIO_O_RDONLY;

    if (pfs_disk_mount_needs_rdwr(&opts, PFS_PART_ENTRY_MAX))
    {
        oflags = PFS_BLKIO_O_RDWR;
    }

    int rc = pfs_blkio_open(&p_disk->blkio, p_image_path, (pfs_blkio_oflags_e)oflags,
            pfs_disk_resolve_sector_size(opts.sector_size, &p_disk->blkio));
    if (PFS_BLKIO_RET_OK != rc)
    {
        pfs_blkio_deinit(&p_disk->blkio);
        return PFS_EIO;
    }

    p_disk->blkio_open = 1;
    p_disk->sector_size = pfs_disk_resolve_sector_size(opts.sector_size, &p_disk->blkio);

    pfs_part_table_t part_tbl;

    rc = pfs_part_probe(&p_disk->blkio, &part_tbl);
    if (PFS_OK != rc)
    {
        pfs_disk_deinit(p_disk);
        return rc;
    }

    if (part_tbl.count >= 1)
    {
        rc = pfs_disk_fill_parts_from_table(p_disk, &part_tbl);
        if (PFS_OK != rc)
        {
            pfs_disk_deinit(p_disk);
            return rc;
        }
    }
    else
    {
        int64_t bare_offset = opts.part_offset;

        rc = pfs_disk_setup_bare_part(p_disk, bare_offset);
        if (PFS_OK != rc)
        {
            pfs_disk_deinit(p_disk);
            return rc;
        }
    }

    rc = pfs_disk_mount_all_parts(p_disk, &opts);
    if (PFS_OK != rc)
    {
        pfs_disk_deinit(p_disk);
        return rc;
    }

    size_t path_len = strlen(p_image_path);
    char *p_copy = KLB_MALLOC(char, path_len + 1, 0);

    if (NULL == p_copy)
    {
        pfs_disk_deinit(p_disk);
        return PFS_ENOMEM;
    }
    memcpy(p_copy, p_image_path, path_len + 1);
    p_disk->image_path = p_copy;

    return PFS_OK;
}

int pfs_disk_remount(pfs_disk_t *p_disk, int part_index, int mnt_style)
{
    if (NULL == p_disk || 0 == pfs_disk_is_mounted(p_disk, -1))
    {
        return PFS_EINVAL;
    }

    if (mnt_style < 0)
    {
        mnt_style = 0;
    }
    else
    {
        mnt_style = pfs_disk_mnt_style_normalize(mnt_style);
    }

    if (part_index >= 0)
    {
        if (part_index >= p_disk->part_count)
        {
            return PFS_EINVAL;
        }

        return pfs_disk_remount_part_style(&p_disk->parts[part_index], mnt_style);
    }

    int ok_count = 0;
    int tried = 0;
    int last_fail = PFS_OK;

    for (int i = 0; i < p_disk->part_count; i++)
    {
        pfs_disk_part_t *p_part = &p_disk->parts[i];

        if (0 == p_part->mounted || NULL == p_part->p_ctx)
        {
            continue;
        }

        tried++;
        int rc = pfs_disk_remount_part_style(p_part, mnt_style);

        if (PFS_OK == rc)
        {
            ok_count++;
        }
        else
        {
            last_fail = rc;
        }
    }

    if (0 == tried)
    {
        return PFS_EBUSY;
    }

    if (0 == ok_count)
    {
        return (PFS_OK != last_fail) ? last_fail : PFS_EOPNOTSUPP;
    }

    return PFS_OK;
}

int pfs_disk_is_mounted(const pfs_disk_t *p_disk, int part_index)
{
    if (NULL == p_disk)
    {
        return 0;
    }

    if (part_index < 0)
    {
        return (0 != p_disk->blkio_open) ? 1 : 0;
    }

    if (0 == p_disk->blkio_open || part_index >= p_disk->part_count)
    {
        return 0;
    }

    return (0 != p_disk->parts[part_index].mounted) ? 1 : 0;
}

int pfs_disk_part_count(const pfs_disk_t *p_disk)
{
    if (NULL == p_disk || 0 == p_disk->blkio_open)
    {
        return 0;
    }
    return p_disk->part_count;
}

const pfs_disk_part_t *pfs_disk_part_info(const pfs_disk_t *p_disk, int part_index)
{
    if (NULL == p_disk || 0 == p_disk->blkio_open || p_disk->part_count <= 0)
    {
        return NULL;
    }
    if (part_index < 1 || part_index > p_disk->part_count)
    {
        return NULL;
    }

    int idx = part_index - 1;

    return &p_disk->parts[idx];
}

// end
