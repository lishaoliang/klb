// Doc Encode : UTF-8 BOM, Unix(LF)
#include "pfs_klua_ex.h"
#include "pfs_disk.h"
#include "pfs_klua_util.h"
#include "klua/klua_env_extension.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_hlist.h"

#include <assert.h>
#include <string.h>

struct pfs_klua_ex_t_
{
    klua_env_t *        p_env;          ///< Lua 环境
    lua_State *         L;              ///< 主线程 L

    klb_hlist_t *       p_disk_hlist;   ///< 活动 disk* 登记表 (quit 兜底)
};

//////////////////////////////////////////////////////////////////////////

static int pfs_klua_ex_quit(pfs_klua_ex_t *p_ex)
{
    while (0 < klb_hlist_size(p_ex->p_disk_hlist))
    {
        pfs_disk_t *p_disk = (pfs_disk_t *)klb_hlist_pop_head(p_ex->p_disk_hlist);

        if (NULL != p_disk)
        {
            pfs_disk_deinit(p_disk);
            KLB_FREE(p_disk);
        }
    }

    return 0;
}

static void *pfs_klua_ex_create(klua_env_t *p_env)
{
    pfs_klua_ex_t *p_ex = KLB_MALLOCZ(pfs_klua_ex_t, 1, 0);

    p_ex->p_env = p_env;
    p_ex->L = klua_env_get_L(p_env);
    p_ex->p_disk_hlist = klb_hlist_create(0);

    return p_ex;
}

static void pfs_klua_ex_destroy(void *ptr)
{
    pfs_klua_ex_t *p_ex = (pfs_klua_ex_t *)ptr;

    pfs_klua_ex_quit(p_ex);
    KLB_FREE_BY(p_ex->p_disk_hlist, klb_hlist_destroy);
    KLB_FREE(p_ex);
}

static int pfs_klua_ex_ctrl(void *ptr, klua_env_t *p_env, int opt, uint8_t *p_param_in_out, int param_size)
{
    pfs_klua_ex_t *p_ex = (pfs_klua_ex_t *)ptr;

    (void)p_env;
    (void)p_param_in_out;
    (void)param_size;

    switch (opt)
    {
    case KLUA_ENV_EX_quit:
        return pfs_klua_ex_quit(p_ex);
        break;

    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////

static pfs_disk_t *pfs_klua_ex_find_disk(pfs_klua_ex_t *p_ex, const char *p_name, int name_len)
{
    if (NULL == p_ex || NULL == p_name || name_len <= 0)
    {
        return NULL;
    }

    return (pfs_disk_t *)klb_hlist_find(p_ex->p_disk_hlist, p_name, name_len);
}

int pfs_klua_ex_mount(pfs_klua_ex_t *p_ex, const char *p_name, const char *p_image_path, const pfs_disk_mount_opts_t *p_opts)
{
    if (NULL == p_ex || !pfs_klua_mount_name_ok(p_name) ||
        NULL == p_image_path || '\0' == p_image_path[0])
    {
        return PFS_EINVAL;
    }

    int name_len = (int)strlen(p_name);

    if (NULL != pfs_klua_ex_find_disk(p_ex, p_name, name_len))
    {
        return PFS_EEXIST;
    }

    pfs_disk_t *p_disk = KLB_MALLOC(pfs_disk_t, 1, 0);

    pfs_disk_init(p_disk);

    pfs_disk_mount_opts_t opts;

    memset(&opts, 0, sizeof(opts));
    opts.default_mnt_style = 0;
    if (NULL != p_opts)
    {
        opts = *p_opts;
    }

    int rc = pfs_disk_mount(p_disk, p_image_path, &opts);
    if (PFS_OK != rc)
    {
        pfs_disk_deinit(p_disk);
        KLB_FREE(p_disk);
        return rc;
    }

    if (NULL == klb_hlist_push_tail(p_ex->p_disk_hlist, p_name, name_len, p_disk))
    {
        pfs_disk_deinit(p_disk);
        KLB_FREE(p_disk);
        return PFS_EEXIST;
    }

    return PFS_OK;
}

int pfs_klua_ex_remount(pfs_klua_ex_t *p_ex, const char *p_name, int part_index, int mnt_style)
{
    if (NULL == p_ex || !pfs_klua_mount_name_ok(p_name))
    {
        return PFS_EINVAL;
    }

    if (mnt_style < 0)
    {
        mnt_style = 0;
    }
    else if (mnt_style > 0)
    {
        mnt_style = 1;
    }

    int name_len = (int)strlen(p_name);
    pfs_disk_t *p_disk = pfs_klua_ex_find_disk(p_ex, p_name, name_len);

    if (NULL == p_disk)
    {
        return PFS_ENOENT;
    }

    return pfs_disk_remount(p_disk, part_index, mnt_style);
}

int pfs_klua_ex_umount(pfs_klua_ex_t *p_ex, const char *p_name, int part_index)
{
    if (NULL == p_ex || !pfs_klua_mount_name_ok(p_name))
    {
        return PFS_EINVAL;
    }

    int name_len = (int)strlen(p_name);
    pfs_disk_t *p_disk = pfs_klua_ex_find_disk(p_ex, p_name, name_len);

    if (NULL == p_disk)
    {
        return PFS_ENOENT;
    }

    if (part_index < 0)
    {
        p_disk = (pfs_disk_t *)klb_hlist_remove_bykey(p_ex->p_disk_hlist, p_name, name_len);
        pfs_disk_deinit(p_disk);
        KLB_FREE(p_disk);
        return PFS_OK;
    }

    if (part_index >= pfs_disk_part_count(p_disk))
    {
        return PFS_EINVAL;
    }

    pfs_disk_umount(p_disk, part_index);
    return PFS_OK;
}

pfs_disk_t *pfs_klua_ex_get_disk(pfs_klua_ex_t *p_ex, const char *p_name, int name_len)
{
    return pfs_klua_ex_find_disk(p_ex, p_name, name_len);
}

int pfs_klua_ex_disk_count(pfs_klua_ex_t *p_ex)
{
    if (NULL == p_ex)
    {
        return 0;
    }

    return klb_hlist_size(p_ex->p_disk_hlist);
}

void pfs_klua_ex_foreach_disk(pfs_klua_ex_t *p_ex,
        void (*cb)(const char *p_key, int key_len, pfs_disk_t *p_disk, void *p_ctx),
        void *p_ctx)
{
    klb_hlist_iter_t *p_iter;

    if (NULL == p_ex || NULL == cb)
    {
        return;
    }

    for (p_iter = klb_hlist_begin(p_ex->p_disk_hlist); NULL != p_iter; p_iter = klb_hlist_next(p_iter))
    {
        int key_len = 0;
        const char *p_key = (const char *)klb_hlist_key(p_iter, &key_len);
        pfs_disk_t *p_disk = (pfs_disk_t *)klb_hlist_data(p_iter);

        cb(p_key, key_len, p_disk, p_ctx);
    }
}

//////////////////////////////////////////////////////////////////////////

static int pfs_klua_ex_register(klua_env_t *p_env)
{
    klua_env_extension_t ex = { 0 };
    ex.cb_create = pfs_klua_ex_create;
    ex.cb_destroy = pfs_klua_ex_destroy;
    ex.cb_ctrl = pfs_klua_ex_ctrl;

    klua_env_register_extension(p_env, KLUAEX_kpfs_env, &ex);

    return 0;
}

/// @brief 获取 env 扩展; 未注册时 register + get_extension (cb_create), 幂等
pfs_klua_ex_t *pfs_klua_ex_get(klua_env_t *p_env)
{
    assert(NULL != p_env);

    pfs_klua_ex_t *p_ex = (pfs_klua_ex_t *)klua_env_get_extension(p_env, KLUAEX_kpfs_env);
    if (NULL == p_ex)
    {
        pfs_klua_ex_register(p_env);
        p_ex = (pfs_klua_ex_t *)klua_env_get_extension(p_env, KLUAEX_kpfs_env);
    }

    return p_ex;
}

pfs_klua_ex_t *pfs_klua_ex_get_by_L(lua_State *L)
{
    assert(NULL != L);
    return pfs_klua_ex_get(klua_env_get_by_L(L));
}

// end
