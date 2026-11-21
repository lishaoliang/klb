// Doc Encode : UTF-8 BOM, Unix(LF)
// ubifs 节点 I/O (对照 fs/ubifs/io.c; CRC + LEB 读写)
#include "ubifs/pfs_ubifs_internal.h"
#include "pfsmtd.h"
#include "zlib.h"

#include <string.h>

void pfs_ubifs_put_le16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xffu);
    p[1] = (uint8_t)((v >> 8) & 0xffu);
}

void pfs_ubifs_put_le32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xffu);
    p[1] = (uint8_t)((v >> 8) & 0xffu);
    p[2] = (uint8_t)((v >> 16) & 0xffu);
    p[3] = (uint8_t)((v >> 24) & 0xffu);
}

void pfs_ubifs_put_le64(uint8_t *p, uint64_t v)
{
    pfs_ubifs_put_le32(p, (uint32_t)(v & 0xffffffffu));
    pfs_ubifs_put_le32(p + 4, (uint32_t)(v >> 32));
}

uint32_t pfs_ubifs_crc_node(const void *node, int len)
{
    const uint8_t *p = (const uint8_t *)node;

    if (NULL == p || len < PFS_UBIFS_CH_SZ)
    {
        return 0;
    }
    return (uint32_t)crc32((uLong)PFS_UBIFS_CRC32_INIT, p + 8, (uInt)(len - 8));
}

void pfs_ubifs_prepare_node(void *node, int len, uint8_t node_type, uint64_t sqnum)
{
    if (NULL == node || len < PFS_UBIFS_CH_SZ)
    {
        return;
    }
    uint8_t *p = (uint8_t *)node;
    pfs_ubifs_put_le32(p + 0, PFS_UBIFS_NODE_MAGIC);
    pfs_ubifs_put_le64(p + 8, sqnum);
    pfs_ubifs_put_le32(p + 16, (uint32_t)len);
    p[20] = node_type;
    p[21] = PFS_UBIFS_NO_NODE_GROUP;
    p[22] = 0;
    p[23] = 0;
    uint32_t crc = pfs_ubifs_crc_node(p, len);
    pfs_ubifs_put_le32(p + 4, crc);
}

int pfs_ubifs_check_node(const void *buf, int len)
{
    if (NULL == buf || len < PFS_UBIFS_CH_SZ)
    {
        return PFS_EINVAL;
    }
    const uint8_t *p = (const uint8_t *)buf;
    uint32_t magic = pfs_ubifs_le32(p + 0);
    uint32_t crc_disk = pfs_ubifs_le32(p + 4);
    uint32_t nlen = pfs_ubifs_le32(p + 16);
    if (PFS_UBIFS_NODE_MAGIC != magic)
    {
        return PFS_ENOEXEC;
    }
    if ((int)nlen < PFS_UBIFS_CH_SZ || (int)nlen > len)
    {
        return PFS_EIO;
    }
    uint32_t crc_calc = pfs_ubifs_crc_node(p, (int)nlen);
    if (crc_calc != crc_disk)
    {
        return PFS_EIO;
    }
    return PFS_OK;
}

int pfs_ubifs_leb_read(pfs_ubifs_sbi_t *p_sbi, int lnum, int offs, void *buf, int len)
{
    if (NULL == p_sbi || NULL == p_sbi->p_blkio || NULL == buf || len < 0)
    {
        return PFS_EINVAL;
    }
    if (p_sbi->leb_size <= 0 || lnum < 0 || offs < 0)
    {
        return PFS_EINVAL;
    }
    if (offs > p_sbi->leb_size || len > p_sbi->leb_size - offs)
    {
        return PFS_EINVAL;
    }

    int mtd_leb = pfsmtd_ubi_leb_size(p_sbi->p_blkio);
    int rc;
    if (mtd_leb > 0 && mtd_leb == p_sbi->leb_size)
    {
        rc = pfsmtd_ubi_leb_read(p_sbi->p_blkio, lnum, offs, buf, len);
    }
    else
    {
        int64_t off = p_sbi->part_offset + (int64_t)lnum * (int64_t)p_sbi->leb_size + (int64_t)offs;
        rc = pfs_blkio_pread(p_sbi->p_blkio, buf, len, off);
    }
    if (rc != len)
    {
        return (0 > rc) ? pfs_ubifs_map_blkio_rc(rc) : PFS_EIO;
    }
    return PFS_OK;
}

int pfs_ubifs_leb_write(pfs_ubifs_sbi_t *p_sbi, int lnum, int offs, const void *buf, int len)
{
    if (NULL == p_sbi || NULL == p_sbi->p_blkio || NULL == buf || len < 0)
    {
        return PFS_EINVAL;
    }
    if (p_sbi->leb_size <= 0 || lnum < 0 || offs < 0)
    {
        return PFS_EINVAL;
    }
    if (offs > p_sbi->leb_size || len > p_sbi->leb_size - offs)
    {
        return PFS_EINVAL;
    }

    int mtd_leb = pfsmtd_ubi_leb_size(p_sbi->p_blkio);
    int rc;
    if (mtd_leb > 0 && mtd_leb == p_sbi->leb_size)
    {
        rc = pfsmtd_ubi_leb_write(p_sbi->p_blkio, lnum, offs, buf, len);
    }
    else
    {
        int64_t off = p_sbi->part_offset + (int64_t)lnum * (int64_t)p_sbi->leb_size + (int64_t)offs;
        rc = pfs_blkio_pwrite(p_sbi->p_blkio, buf, len, off);
    }
    if (rc != len)
    {
        return (0 > rc) ? pfs_ubifs_map_blkio_rc(rc) : PFS_EIO;
    }
    return PFS_OK;
}

int pfs_ubifs_read_node(pfs_ubifs_sbi_t *p_sbi, int lnum, int offs, void *buf, int len)
{

    if (len < PFS_UBIFS_CH_SZ)
    {
        return PFS_EINVAL;
    }
    int rc = pfs_ubifs_leb_read(p_sbi, lnum, offs, buf, len);
    if (PFS_OK != rc)
    {
        return rc;
    }
    return pfs_ubifs_check_node(buf, len);
}

int pfs_ubifs_write_node(pfs_ubifs_sbi_t *p_sbi, int lnum, int offs, void *buf, int len)
{
    if (NULL == buf || len < PFS_UBIFS_CH_SZ)
    {
        return PFS_EINVAL;
    }
    return pfs_ubifs_leb_write(p_sbi, lnum, offs, buf, len);
}

// end
