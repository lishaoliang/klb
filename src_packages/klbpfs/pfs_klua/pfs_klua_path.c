// Doc Encode : UTF-8 BOM, Unix(LF)
#include "pfs_klua_path.h"
#include "pfs_klua_ex.h"
#include "pfs_disk.h"
#include "pfs_part.h"

#include <string.h>

static bool pfs_klua_path_letter_ok(char c)
{
    if (c >= 'A' && c <= 'Z')
    {
        return true;
    }
    else if (c >= 'a' && c <= 'z')
    {
        return true;
    }
    return false;
}

int pfs_klua_path_parse(const char *drive_path, pfs_klua_drive_path_t *p_out)
{
    if (NULL == drive_path || NULL == p_out || '\0' == drive_path[0])
    {
        return PFS_EINVAL;
    }

    memset(p_out, 0, sizeof(*p_out));

    const char *p = drive_path;
    int disk_i = 0;
    int part_digits = 0;
    int have_part_digits = 0;

    while (pfs_klua_path_letter_ok(*p))
    {
        if (disk_i + 1 >= (int)sizeof(p_out->disk_name))
        {
            return PFS_EINVAL;
        }
        p_out->disk_name[disk_i++] = *p++;
    }
    if (disk_i <= 0)
    {
        return PFS_EINVAL;
    }
    p_out->disk_name[disk_i] = '\0';

    while (*p >= '0' && *p <= '9')
    {
        have_part_digits = 1;
        part_digits = part_digits * 10 + (*p - '0');
        p++;
        if (part_digits > PFS_PART_ENTRY_MAX)
        {
            return PFS_EINVAL;
        }
    }

    if (0 == have_part_digits)
    {
        p_out->part_slot = 0;
    }
    else if (part_digits < 1)
    {
        return PFS_EINVAL;
    }
    else
    {
        p_out->part_slot = part_digits - 1;
    }

    if (*p != ':')
    {
        return PFS_EINVAL;
    }
    p++;

    if ('\0' == *p)
    {
        p_out->vfs_path[0] = '/';
        p_out->vfs_path[1] = '\0';
        return PFS_OK;
    }
    if (*p != '/')
    {
        return PFS_EINVAL;
    }

    size_t vfs_len = strlen(p);
    if (vfs_len >= sizeof(p_out->vfs_path))
    {
        return PFS_EINVAL;
    }
    memcpy(p_out->vfs_path, p, vfs_len + 1);
    return PFS_OK;
}

int pfs_klua_path_resolve_ctx(lua_State *L, const pfs_klua_drive_path_t *p_parsed, pfs_ctx_t **p_out_ctx)
{
    if (NULL == L || NULL == p_parsed || NULL == p_out_ctx)
    {
        return PFS_EINVAL;
    }

    *p_out_ctx = NULL;

    int name_len = (int)strlen(p_parsed->disk_name);
    if (name_len <= 0)
    {
        return PFS_EINVAL;
    }

    pfs_klua_ex_t *p_ex = pfs_klua_ex_get_by_L(L);
    if (NULL == p_ex)
    {
        return PFS_EINVAL;
    }

    pfs_disk_t *p_disk = pfs_klua_ex_get_disk(p_ex, p_parsed->disk_name, name_len);
    if (NULL == p_disk)
    {
        return PFS_ENOENT;
    }

    if (p_parsed->part_slot < 0 || p_parsed->part_slot >= pfs_disk_part_count(p_disk))
    {
        return PFS_EINVAL;
    }

    pfs_disk_part_t *p_part = &p_disk->parts[p_parsed->part_slot];
    if (0 == p_part->mounted || NULL == p_part->p_ctx)
    {
        return PFS_ENOENT;
    }

    *p_out_ctx = p_part->p_ctx;
    return PFS_OK;
}

int pfs_klua_path_resolve_drive(lua_State *L, const char *drive_path,
        pfs_klua_drive_path_t *p_parsed, pfs_ctx_t **p_out_ctx)
{
    if (NULL == drive_path || NULL == p_parsed || NULL == p_out_ctx)
    {
        return PFS_EINVAL;
    }

    int rc = pfs_klua_path_parse(drive_path, p_parsed);
    if (PFS_OK != rc)
    {
        return rc;
    }

    return pfs_klua_path_resolve_ctx(L, p_parsed, p_out_ctx);
}

// end
