// Doc Encode : UTF-8 BOM, Unix(LF)
// romfs 目录流 (链 next; . / .. 在镜像内)
#include "romfs/pfs_romfs_internal.h"

#include <string.h>
#include <stdbool.h>

int pfs_romfs_dir_open(pfs_romfs_sbi_t *p_sbi, const char *path, pfs_romfs_dir_meta_t *p_dm)
{
    if (NULL == p_sbi || NULL == path || NULL == p_dm)
    {
        return PFS_EINVAL;
    }
    if (!pfs_romfs_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }

    memset(p_dm, 0, sizeof(*p_dm));

    pfs_romfs_fh_t fh;
    bool is_dir;

    int rc = pfs_romfs_lookup_path(p_sbi, path, &fh, &is_dir);
    if (PFS_OK != rc)
    {
        return rc;
    }
    if (!is_dir)
    {
        return PFS_EINVAL;
    }

    p_dm->fh = fh;
    p_dm->cur = fh.spec & PFS_ROMFS_FH_MASK;
    return PFS_OK;
}

void pfs_romfs_dir_close(pfs_romfs_sbi_t *p_sbi, pfs_romfs_dir_meta_t *p_dm)
{
    (void)p_sbi;
    if (NULL != p_dm)
    {
        memset(p_dm, 0, sizeof(*p_dm));
    }
}

int pfs_romfs_dir_readdir(pfs_romfs_sbi_t *p_sbi, pfs_romfs_dir_meta_t *p_dm, pfs_dirent_t *p_ent)
{
    if (NULL == p_sbi || NULL == p_dm || NULL == p_ent)
    {
        return PFS_EINVAL;
    }
    if (!pfs_romfs_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }

    memset(p_ent, 0, sizeof(*p_ent));
    if (0 == p_dm->cur || p_dm->cur + PFS_ROMFS_FH_SIZE > p_sbi->img_size)
    {
        return PFS_ENOENT;
    }

    pfs_romfs_fh_t ent;
    char name[PFS_ROMFS_MAXFN + 1];
    int nlen;

    int rc = pfs_romfs_read_fh(p_sbi, p_dm->cur, &ent, name, (int)sizeof(name), &nlen);
    if (PFS_OK != rc)
    {
        return rc;
    }

    p_dm->cur = ent.next;

    uint8_t dtype = PFS_DT_UNKNOWN;
    if (PFS_ROMFH_HRD == ent.type)
    {
        pfs_romfs_fh_t resolved;

        rc = pfs_romfs_resolve_fh(p_sbi, ent.pos, &resolved);
        if (PFS_OK == rc)
        {
            if (PFS_ROMFH_DIR == resolved.type)
            {
                dtype = PFS_DT_DIR;
            }
            else if (PFS_ROMFH_REG == resolved.type || PFS_ROMFH_SYM == resolved.type)
            {
                dtype = PFS_DT_REG;
            }
        }
    }
    else if (PFS_ROMFH_DIR == ent.type)
    {
        dtype = PFS_DT_DIR;
    }
    else if (PFS_ROMFH_REG == ent.type || PFS_ROMFH_SYM == ent.type)
    {
        dtype = PFS_DT_REG;
    }

    p_ent->d_type = dtype;
    size_t ncopy = (size_t)nlen;
    if (ncopy > sizeof(p_ent->d_name) - 1)
    {
        ncopy = sizeof(p_ent->d_name) - 1;
    }
    if (ncopy > 0)
    {
        memcpy(p_ent->d_name, name, ncopy);
    }
    p_ent->d_name[ncopy] = '\0';
    return PFS_OK;
}

// end
