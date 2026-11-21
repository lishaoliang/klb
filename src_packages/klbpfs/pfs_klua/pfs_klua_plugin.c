// Doc Encode : UTF-8 BOM, Unix(LF)
#include "pfs_klua.h"
#include "pfs_klua_internal.h"

typedef struct pfs_klua_prelib_item_t_
{
    const char *    name;
    lua_CFunction   openlib;
} pfs_klua_prelib_item_t;

/// @brief plugins klua 预加载导出表 (klbappex_pre_*)
static const pfs_klua_prelib_item_t s_pfs_klua_prelib[] =
{
    { "kpfs",           pfs_klua_open_kpfs },
    { "kpfs.vfs",       pfs_klua_open_kpfs_vfs },
    { "kpfs.probe",     pfs_klua_open_kpfs_probe },
    { "kpfs.disk",      pfs_klua_open_kpfs_disk },
    { "kpfs.image",     pfs_klua_open_kpfs_image },
    { "kpfs.mtd",       pfs_klua_open_kpfs_mtd },
};

int klbappex_pre_count(void)
{
    return (int)(sizeof(s_pfs_klua_prelib) / sizeof(s_pfs_klua_prelib[0]));
}

int klbappex_pre_open(int idx, lua_CFunction *p_out_preload, const char **p_out_name)
{
    int count = (int)(sizeof(s_pfs_klua_prelib) / sizeof(s_pfs_klua_prelib[0]));

    if (idx < 0 || idx >= count || NULL == p_out_preload || NULL == p_out_name)
    {
        return -1;
    }

    *p_out_preload = s_pfs_klua_prelib[idx].openlib;
    *p_out_name = s_pfs_klua_prelib[idx].name;

    return 0;
}

// end
