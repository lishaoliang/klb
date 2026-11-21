// Doc Encode : UTF-8 BOM, Unix(LF)
#include "pfs_klua.h"
#include "pfs_klua_internal.h"
#include "pfs_klua_blkio.h"
#include "pfs_klua_util.h"

#include <string.h>

#define PFS_KLUA_IMAGE_DEFAULT_TYPE     "vhdx"
#define PFS_KLUA_IMAGE_DEFAULT_SIZE     (8ull * 1024ull * 1024ull * 1024ull)

static int pfs_klua_image_parse_type(const char *text, pfs_blkio_type_e *p_out)
{
    if (NULL == text || NULL == p_out || '\0' == text[0])
    {
        return -1;
    }
    if (0 == strcmp(text, "vhd"))
    {
        *p_out = PFS_BLKIO_VHD;
        return 0;
    }
    else if (0 == strcmp(text, "vhdx"))
    {
        *p_out = PFS_BLKIO_VHDX;
        return 0;
    }
    else if (0 == strcmp(text, "qcow2"))
    {
        *p_out = PFS_BLKIO_QCOW2;
        return 0;
    }
    return -1;
}

static const char *pfs_klua_image_canonical_type(const char *text)
{
    if (NULL == text || '\0' == text[0])
    {
        return NULL;
    }
    if (pfs_klua_str_ieq(text, "vhd"))
    {
        return "vhd";
    }
    else if (pfs_klua_str_ieq(text, "vhdx"))
    {
        return "vhdx";
    }
    else if (pfs_klua_str_ieq(text, "qcow2"))
    {
        return "qcow2";
    }
    else if (pfs_klua_str_ieq(text, "vmdk"))
    {
        return "vmdk";
    }
    else if (pfs_klua_str_ieq(text, "vdi"))
    {
        return "vdi";
    }
    return NULL;
}

static const char *pfs_klua_image_path_ext(const char *path)
{
    if (NULL == path || '\0' == path[0])
    {
        return NULL;
    }

    const char *last_dot = NULL;

    for (const char *p = path; '\0' != *p; p++)
    {
        if ('.' == *p)
        {
            last_dot = p;
        }
    }
    if (NULL == last_dot || '\0' == last_dot[1])
    {
        return NULL;
    }
    return last_dot + 1;
}

/// @brief type 解析: 1) opts.type 已知名; 2) path 后缀; 3) 默认 vhdx
static const char *pfs_klua_image_resolve_type(lua_State *L, int opts_idx, const char *path)
{
    if (opts_idx > 0)
    {
        const char *opt_type = pfs_klua_opts_string(L, opts_idx, "type", NULL);

        if (NULL != opt_type)
        {
            const char *canonical = pfs_klua_image_canonical_type(opt_type);

            if (NULL != canonical)
            {
                return canonical;
            }
        }
    }

    const char *from_ext = pfs_klua_image_canonical_type(pfs_klua_image_path_ext(path));

    if (NULL != from_ext)
    {
        return from_ext;
    }

    return PFS_KLUA_IMAGE_DEFAULT_TYPE;
}

/// @brief kpfs.image.create — 对齐 kpfs_image.md 伪代码
static int pfs_klua_image_create(lua_State *L)
{
    const char *path = NULL;
    int opts_idx = 0;

    if (0 != pfs_klua_parse_path_opts(L, &path, &opts_idx))
    {
        return pfs_klua_push_rc(L, PFS_EINVAL);
    }

    const char *img_type = pfs_klua_image_resolve_type(L, opts_idx, path);

    if (0 == strcmp(img_type, "vmdk") || 0 == strcmp(img_type, "vdi"))
    {
        return pfs_klua_push_rc(L, PFS_EOPNOTSUPP);
    }

    pfs_blkio_type_e type;

    if (0 != pfs_klua_image_parse_type(img_type, &type))
    {
        return pfs_klua_push_rc(L, PFS_EINVAL);
    }

    uint64_t size_bytes = PFS_KLUA_IMAGE_DEFAULT_SIZE;
    int have_size = 0;

    if (opts_idx > 0)
    {
        if (0 != pfs_klua_parse_size_bytes(L, opts_idx, "size", &size_bytes, &have_size))
        {
            return pfs_klua_push_rc(L, PFS_EINVAL);
        }
    }
    if (0 == have_size)
    {
        size_bytes = PFS_KLUA_IMAGE_DEFAULT_SIZE;
    }
    if (0 == size_bytes || 0 != (size_bytes % 512ull))
    {
        return pfs_klua_push_rc(L, PFS_EINVAL);
    }

    int is_fixed = 0;
    int have_layout = 0;

    if (opts_idx > 0)
    {
        const char *layout = pfs_klua_opts_string(L, opts_idx, "layout", NULL);

        if (NULL != layout)
        {
            have_layout = 1;
            if (0 == strcmp(layout, "fixed"))
            {
                is_fixed = 1;
            }
            else if (0 == strcmp(layout, "dynamic"))
            {
                is_fixed = 0;
            }
            else
            {
                return pfs_klua_push_rc(L, PFS_EINVAL);
            }
        }
    }

    if (PFS_BLKIO_QCOW2 == type && 0 != have_layout && 0 != is_fixed)
    {
        return pfs_klua_push_rc(L, PFS_EINVAL);
    }

    uint32_t flags = PFS_BLKIO_IMAGE_FORCE;

    if (0 != is_fixed)
    {
        flags |= PFS_BLKIO_IMAGE_FIXED;
    }
    else
    {
        flags |= PFS_BLKIO_IMAGE_DYNAMIC;
    }

    int rc = pfs_blkio_create_image(type, path, size_bytes, flags, NULL, 0);

    return pfs_klua_push_blkio_rc(L, rc);
}

/// @brief kpfs.image.info — 固定 info, rc, msg; 对齐 kpfs_image.md 伪代码
static int pfs_klua_image_info(lua_State *L)
{
    const char *path = NULL;
    int opts_idx = 0;

    if (0 != pfs_klua_parse_path_opts(L, &path, &opts_idx))
    {
        return pfs_klua_push_info_rc_fail(L, PFS_EINVAL);
    }

    pfs_klua_blkio_sess_t sess;

    pfs_klua_blkio_sess_init(&sess);
    int rc = pfs_klua_blkio_sess_open_rdonly(&sess, path);
    if (PFS_OK != rc)
    {
        pfs_klua_blkio_sess_done(&sess);
        return pfs_klua_push_info_rc_fail(L, rc);
    }

    lua_createtable(L, 0, 5);
    pfs_klua_push_blkio_container(L, path, &sess.blkio);

    pfs_klua_blkio_sess_done(&sess);
    return pfs_klua_push_info_rc_ok(L);
}

int pfs_klua_open_kpfs_image(lua_State *L)
{
    static const luaL_Reg s_lib[] = {
        { "create",     pfs_klua_image_create },
        { "info",       pfs_klua_image_info },
        { NULL,         NULL }
    };

    luaL_newlib(L, s_lib);
    return 1;
}

// end
