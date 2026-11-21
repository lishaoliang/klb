// Doc Encode : UTF-8 BOM, Unix(LF)
#include "pfs_klua_internal.h"
#include "pfs_klua_blkio.h"
#include "pfs_klua_util.h"
#include "pfs_part.h"

#include <string.h>

int pfs_klua_lua_mkpt(lua_State *L)
{
    const char *path = NULL;
    int opts_idx = 0;

    if (0 != pfs_klua_parse_path_opts(L, &path, &opts_idx))
    {
        return pfs_klua_push_rc(L, PFS_EINVAL);
    }

    pfs_part_scheme_e scheme = PFS_PART_SCHEME_MBR;
    if (opts_idx > 0)
    {
        const char *scheme_str = pfs_klua_opts_string(L, opts_idx, "scheme", "mbr");
        if (0 != pfs_klua_parse_part_scheme(scheme_str, &scheme))
        {
            return pfs_klua_push_rc(L, PFS_EINVAL);
        }
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

    int sector_size = pfs_blkio_logical_sector_size(&sess.blkio);
    if (sector_size <= 0)
    {
        sector_size = 512;
    }

    pfs_mkpt_table_t mkpt_in = { 0 };
    pfs_part_table_t part_out;

    mkpt_in.count = part_count;

    for (int i = 0; i < part_count; i++)
    {
        if (opts_idx > 0 && pfs_klua_opts_is_part_array(L, opts_idx))
        {
            lua_rawgeti(L, opts_idx, i + 1);
            int tbl_idx = lua_gettop(L);

            if (0 != pfs_klua_parse_mkpt_entry_lua(L, tbl_idx, sector_size, &mkpt_in.entries[i]))
            {
                lua_pop(L, 1);
                pfs_klua_blkio_sess_done(&sess);
                return pfs_klua_push_rc(L, PFS_EINVAL);
            }
            lua_pop(L, 1);
        }
        else
        {
            if (0 != pfs_klua_parse_mkpt_entry_lua(L, opts_idx, sector_size, &mkpt_in.entries[i]))
            {
                pfs_klua_blkio_sess_done(&sess);
                return pfs_klua_push_rc(L, PFS_EINVAL);
            }
        }
    }

    if (PFS_PART_SCHEME_MBR == scheme)
    {
        rc = pfs_mbr_mkpt(&sess.blkio, 0, &mkpt_in, &part_out);
    }
    else
    {
        rc = pfs_gpt_mkpt(&sess.blkio, 0, &mkpt_in, &part_out);
    }

    pfs_klua_blkio_sess_done(&sess);
    return pfs_klua_push_rc(L, rc);
}

// end
