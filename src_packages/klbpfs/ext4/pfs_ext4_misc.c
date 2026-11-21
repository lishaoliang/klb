// Doc Encode : UTF-8 BOM, Unix(LF)
// ext4 misc: sb 字段访问 / blkio 映射 / 块读写
#include "ext4/pfs_ext4_internal.h"

#include <string.h>

uint32_t pfs_ext4_sb_u32(const pfs_ext4_sb_raw_t sb, unsigned off)
{
    return pfs_read_le32(&sb[off]);
}

uint16_t pfs_ext4_sb_u16(const pfs_ext4_sb_raw_t sb, unsigned off)
{
    return pfs_read_le16(&sb[off]);
}

void pfs_ext4_sb_set_u32(pfs_ext4_sb_raw_t sb, unsigned off, uint32_t v)
{
    pfs_write_le32(&sb[off], v);
}

void pfs_ext4_sb_set_u16(pfs_ext4_sb_raw_t sb, unsigned off, uint16_t v)
{
    pfs_write_le16(&sb[off], v);
}

int pfs_ext4_map_blkio_rc(int rc)
{
    if (rc >= 0)
    {
        return PFS_EXT4_RET_OK;
    }
    if (PFS_BLKIO_RET_PARAM == rc)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (PFS_BLKIO_RET_STATE == rc)
    {
        return PFS_EXT4_RET_EBUSY;
    }
    return PFS_EXT4_RET_EIO;
}

int64_t pfs_ext4_block_to_off(const pfs_ext4_sbi_t *p_sbi, uint64_t block)
{
    return p_sbi->part_offset + (int64_t)block * (int64_t)p_sbi->block_size;
}

int pfs_ext4_read_block(pfs_ext4_sbi_t *p_sbi, uint64_t block, void *buf)
{

    if (NULL == p_sbi || NULL == buf || NULL == p_sbi->p_blkio)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (block >= p_sbi->blocks_count)
    {
        return PFS_EXT4_RET_ERANGE;
    }
    int rc = pfs_blkio_pread(p_sbi->p_blkio, buf, (int)p_sbi->block_size,
            pfs_ext4_block_to_off(p_sbi, block));
    if (rc < 0)
    {
        return pfs_ext4_map_blkio_rc(rc);
    }
    if (rc != (int)p_sbi->block_size)
    {
        return PFS_EXT4_RET_EIO;
    }
    return PFS_EXT4_RET_OK;
}

int pfs_ext4_write_block(pfs_ext4_sbi_t *p_sbi, uint64_t block, const void *buf)
{

    if (NULL == p_sbi || NULL == buf || NULL == p_sbi->p_blkio)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (!pfs_ext4_sbi_can_write(p_sbi))
    {
        return PFS_EXT4_RET_EPERM;
    }
    if (block >= p_sbi->blocks_count)
    {
        return PFS_EXT4_RET_ERANGE;
    }
    int rc = pfs_blkio_pwrite(p_sbi->p_blkio, buf, (int)p_sbi->block_size,
            pfs_ext4_block_to_off(p_sbi, block));
    if (rc < 0)
    {
        return pfs_ext4_map_blkio_rc(rc);
    }
    if (rc != (int)p_sbi->block_size)
    {
        return PFS_EXT4_RET_EIO;
    }
    return PFS_EXT4_RET_OK;
}

int pfs_ext4_sync_sb(pfs_ext4_sbi_t *p_sbi)
{

    if (NULL == p_sbi || NULL == p_sbi->p_blkio)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    int rc = pfs_blkio_pwrite(p_sbi->p_blkio, p_sbi->sb_raw, (int)PFS_EXT4_SUPER_SIZE,
            p_sbi->part_offset + (int64_t)PFS_EXT4_SUPER_OFFSET);
    if (rc < 0)
    {
        return pfs_ext4_map_blkio_rc(rc);
    }
    if (rc != (int)PFS_EXT4_SUPER_SIZE)
    {
        return PFS_EXT4_RET_EIO;
    }
    return PFS_EXT4_RET_OK;
}

// end
