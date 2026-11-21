// Doc Encode : UTF-8 BOM, Unix(LF)
#include "pfs_klua_internal.h"
#include "pfs_klua_blkio.h"
#include "pfs_klua_util.h"
#include "pfsmtd.h"

#include <string.h>

static int pfs_klua_mkfs_one(lua_State *L, pfs_klua_blkio_sess_t *p_sess, int tbl_idx, int array_pos,
        int is_mkvol)
{
    pfs_fs_type_e fs_type = PFS_EXFAT;
    const char *fs_str = pfs_klua_opts_string(L, tbl_idx, "fs", NULL);

    if (NULL == fs_str)
    {
        fs_str = pfs_klua_opts_string(L, tbl_idx, "type", "exfat");
    }
    if (0 != pfs_klua_parse_fs_type(fs_str, &fs_type))
    {
        return PFS_EINVAL;
    }

    const char *label;
    const char *src_dir = NULL;

    if (0 != is_mkvol)
    {
        src_dir = pfs_klua_opts_string(L, tbl_idx, "src", NULL);
        if (NULL == src_dir || '\0' == src_dir[0])
        {
            return PFS_EINVAL;
        }
        label = pfs_klua_opts_string(L, tbl_idx, "label", NULL);
    }
    else
    {
        label = pfs_klua_opts_string(L, tbl_idx, "label", NULL);
        if (NULL != label && '\0' != label[0])
        {
            return PFS_EINVAL;
        }
    }

    int64_t part_offset;

    if (0 != pfs_klua_resolve_format_offset(L, tbl_idx, array_pos, &p_sess->blkio, &part_offset))
    {
        return PFS_EINVAL;
    }

    uint64_t vol_bytes;
    int have_vol;

    if (0 != pfs_klua_resolve_format_vol(L, tbl_idx, &p_sess->blkio, part_offset, &vol_bytes, &have_vol))
    {
        return PFS_EINVAL;
    }

    if (0 != is_mkvol)
    {
        pfs_mkvol_opts_t vol_opts = { 0 };

        vol_opts.volume_id = label;
        return pfs_mkvol(&p_sess->blkio, part_offset, vol_bytes, fs_type, src_dir, &vol_opts);
    }

    return pfs_mkfs(&p_sess->blkio, part_offset, vol_bytes, fs_type);
}

static int pfs_klua_mkfs_common(lua_State *L, int is_mkvol)
{
    const char *path = NULL;
    int opts_idx = 0;

    if (0 != pfs_klua_parse_path_opts(L, &path, &opts_idx))
    {
        return pfs_klua_push_rc(L, PFS_EINVAL);
    }

    int part_count = (opts_idx > 0 && pfs_klua_opts_is_part_array(L, opts_idx))
            ? pfs_klua_opts_part_array_count(L, opts_idx)
            : 1;

    if (part_count <= 0 || part_count > PFS_PART_ENTRY_MAX)
    {
        return pfs_klua_push_rc(L, PFS_EINVAL);
    }

    pfs_klua_blkio_sess_t sess;

    pfs_klua_blkio_sess_init(&sess);
    int rc = pfs_klua_blkio_sess_open_rdwr(&sess, path);
    if (PFS_OK != rc)
    {
        pfs_klua_blkio_sess_done(&sess);
        return pfs_klua_push_rc(L, rc);
    }

    for (int i = 0; i < part_count; i++)
    {
        int last_rc;

        if (opts_idx > 0 && pfs_klua_opts_is_part_array(L, opts_idx))
        {
            lua_rawgeti(L, opts_idx, i + 1);
            int tbl_idx = lua_gettop(L);

            last_rc = pfs_klua_mkfs_one(L, &sess, tbl_idx, i + 1, is_mkvol);
            lua_pop(L, 1);
        }
        else if (opts_idx > 0)
        {
            last_rc = pfs_klua_mkfs_one(L, &sess, opts_idx, 1, is_mkvol);
        }
        else
        {
            lua_createtable(L, 0, 0);
            int tbl_idx = lua_gettop(L);

            last_rc = pfs_klua_mkfs_one(L, &sess, tbl_idx, 1, is_mkvol);
            lua_pop(L, 1);
        }

        if (PFS_OK != last_rc)
        {
            pfs_klua_blkio_sess_done(&sess);
            return pfs_klua_push_rc(L, last_rc);
        }
    }

    pfs_klua_blkio_sess_done(&sess);
    return pfs_klua_push_rc(L, PFS_OK);
}

int pfs_klua_lua_mkfs(lua_State *L)
{
    return pfs_klua_mkfs_common(L, 0);
}

int pfs_klua_lua_mkvol(lua_State *L)
{
    return pfs_klua_mkfs_common(L, 1);
}

// end
