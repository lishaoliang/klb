// Doc Encode : UTF-8 BOM, Unix(LF)
// upcase / 字符串 (linux-7.1 fs/ntfs/upcase.c + unistr.c)
#include "ntfs/pfs_ntfs_internal.h"
#include "base/pfs_mem.h"

void pfs_ntfs_upcase_reset(pfs_ntfs_upcase_t *p_up)
{
    if (NULL == p_up)
    {
        return;
    }
    if (NULL != p_up->p_entries)
    {
        pfs_free(p_up->p_entries);
        p_up->p_entries = NULL;
    }
    memset(p_up, 0, sizeof(*p_up));
}

int pfs_ntfs_upcase_load(pfs_ntfs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return PFS_NTFS_RET_EINVAL;
    }

    pfs_ntfs_upcase_reset(&p_sbi->upcase);
    pfs_ntfs_inode_t ino;
    memset(&ino, 0, sizeof(ino));

    int rc = pfs_ntfs_inode_load(p_sbi, NTFS_FILE_UpCase, &ino);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }

    if (0 >= ino.data_size || 0U != ((uint64_t)ino.data_size & 1ULL))
    {
        pfs_ntfs_inode_reset(&ino);
        return PFS_NTFS_RET_EBADMSG;
    }
    if (ino.data_size > (int64_t)PFS_NTFS_UPCASE_CHARS * 2)
    {
        pfs_ntfs_inode_reset(&ino);
        return PFS_NTFS_RET_EBADMSG;
    }

    uint32_t raw_size = (uint32_t)ino.data_size;
    uint16_t *p_host = (uint16_t *)pfs_malloc(raw_size);
    if (NULL == p_host)
    {
        pfs_ntfs_inode_reset(&ino);
        return PFS_NTFS_RET_ENOMEM;
    }

    if (ino.data_resident)
    {
        if (ino.data_resident_size != raw_size)
        {
            pfs_free(p_host);
            pfs_ntfs_inode_reset(&ino);
            return PFS_NTFS_RET_EBADMSG;
        }
        for (uint32_t i = 0; i < raw_size / 2U; i++)
        {
            p_host[i] = pfs_le16_to_cpu(((uint16_t *)ino.p_data_resident)[i]);
        }
    }
    else
    {
        uint8_t *p_raw = (uint8_t *)pfs_malloc(raw_size);
        if (NULL == p_raw)
        {
            pfs_free(p_host);
            pfs_ntfs_inode_reset(&ino);
            return PFS_NTFS_RET_ENOMEM;
        }
        int got;
        rc = pfs_ntfs_rl_pread(p_sbi, &ino.data_runlist, ino.data_size, 0, p_raw, (int)raw_size, &got);
        if (PFS_NTFS_RET_OK != rc)
        {
            pfs_free(p_raw);
            pfs_free(p_host);
            pfs_ntfs_inode_reset(&ino);
            return rc;
        }
        else if (got != (int)raw_size)
        {
            pfs_free(p_raw);
            pfs_free(p_host);
            pfs_ntfs_inode_reset(&ino);
            return PFS_NTFS_RET_EIO;
        }
        for (int i = 0; i < raw_size / 2U; i++)
        {
            p_host[i] = pfs_le16_to_cpu(((uint16_t *)p_raw)[i]);
        }
        pfs_free(p_raw);
    }

    p_sbi->upcase.p_entries = p_host;
    p_sbi->upcase.entries_count = raw_size / 2U;
    p_sbi->upcase.valid = true;
    pfs_ntfs_inode_reset(&ino);
    return PFS_NTFS_RET_OK;
}

// end
