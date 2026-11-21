// Doc Encode : UTF-8 BOM, Unix(LF)
// yaffs2 目录流 (合成 . / .. + 同 parent 子对象)
#include "yaffs/pfs_yaffs_internal.h"

#include <string.h>
#include <stdbool.h>

static uint8_t pfs_yaffs_map_dtype(const pfs_yaffs_obj_t *p_obj)
{
    if (NULL == p_obj)
    {
        return PFS_DT_UNKNOWN;
    }
    if (PFS_YAFFS_OBJECT_TYPE_DIRECTORY == p_obj->type)
    {
        return PFS_DT_DIR;
    }
    if (PFS_YAFFS_OBJECT_TYPE_FILE == p_obj->type)
    {
        return PFS_DT_REG;
    }
    return PFS_DT_UNKNOWN;
}

int pfs_yaffs_dir_open(pfs_yaffs_sbi_t *p_sbi, const char *path, pfs_yaffs_dir_meta_t *p_dm)
{
    if (NULL == p_sbi || NULL == path || NULL == p_dm)
    {
        return PFS_EINVAL;
    }
    if (!pfs_yaffs_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }

    memset(p_dm, 0, sizeof(*p_dm));

    uint32_t obj_id;
    bool is_dir;

    int rc = pfs_yaffs_lookup_path(p_sbi, path, &obj_id, &is_dir);
    if (PFS_OK != rc)
    {
        return rc;
    }
    if (!is_dir)
    {
        return PFS_EINVAL;
    }

    p_dm->obj_id = obj_id;
    p_dm->invent = 0;
    p_dm->p_next = p_sbi->obj_list;
    return PFS_OK;
}

void pfs_yaffs_dir_close(pfs_yaffs_sbi_t *p_sbi, pfs_yaffs_dir_meta_t *p_dm)
{
    (void)p_sbi;
    if (NULL != p_dm)
    {
        memset(p_dm, 0, sizeof(*p_dm));
    }
}

int pfs_yaffs_dir_readdir(pfs_yaffs_sbi_t *p_sbi, pfs_yaffs_dir_meta_t *p_dm, pfs_dirent_t *p_ent)
{
    if (NULL == p_sbi || NULL == p_dm || NULL == p_ent)
    {
        return PFS_EINVAL;
    }
    if (!pfs_yaffs_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }

    memset(p_ent, 0, sizeof(*p_ent));
    if (0 == p_dm->invent)
    {
        p_ent->d_type = PFS_DT_DIR;
        p_ent->d_name[0] = '.';
        p_ent->d_name[1] = '\0';
        p_dm->invent = 1;
        return PFS_OK;
    }
    if (1 == p_dm->invent)
    {
        p_ent->d_type = PFS_DT_DIR;
        p_ent->d_name[0] = '.';
        p_ent->d_name[1] = '.';
        p_ent->d_name[2] = '\0';
        p_dm->invent = 2;
        return PFS_OK;
    }

    while (NULL != p_dm->p_next)
    {
        pfs_yaffs_obj_t *p_obj = p_dm->p_next;

        p_dm->p_next = p_obj->next;
        if (!p_obj->has_hdr || !pfs_yaffs_obj_visible(p_obj))
        {
            continue;
        }
        if (p_obj->parent_id != p_dm->obj_id)
        {
            continue;
        }
        if (p_obj->obj_id == p_dm->obj_id)
        {
            continue;
        }
        if (PFS_YAFFS_OBJECTID_UNLINKED == p_obj->obj_id
                || PFS_YAFFS_OBJECTID_DELETED == p_obj->obj_id)
        {
            continue;
        }
        if ('\0' == p_obj->name[0] && PFS_YAFFS_OBJECTID_LOSTNFOUND != p_obj->obj_id)
        {
            continue;
        }
        if (PFS_YAFFS_OBJECTID_LOSTNFOUND == p_obj->obj_id && '\0' == p_obj->name[0])
        {
            memcpy(p_ent->d_name, "lost+found", 11);
            p_ent->d_type = PFS_DT_DIR;
            return PFS_OK;
        }

        p_ent->d_type = pfs_yaffs_map_dtype(p_obj);

        size_t ncopy = strlen(p_obj->name);
        if (ncopy > sizeof(p_ent->d_name) - 1)
        {
            ncopy = sizeof(p_ent->d_name) - 1;
        }
        memcpy(p_ent->d_name, p_obj->name, ncopy);
        p_ent->d_name[ncopy] = '\0';
        return PFS_OK;
    }
    return PFS_ENOENT;
}

// end
