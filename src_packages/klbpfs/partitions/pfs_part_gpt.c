// Doc Encode : UTF-8 BOM, Unix(LF)
// GPT 只读探测 (阶段 A+B: 主 GPT, 失败回退 Alternate); 对齐 linux-7.1 block/partitions/efi.c
#include "partitions/pfs_part_internal.h"
#include "partitions/pfs_part_gpt_raw.h"
#include "partitions/pfs_part_mbr_raw.h"
#include "pfs_endian.h"
#include "base/pfs_mem.h"

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// 分区项数组上限 (防异常 header 导致巨量分配); 典型 128 项 x 128B = 16KiB
#define PFS_PART_GPT_PT_BYTES_MAX               (1024u * 256u)

static int pfs_part_gpt_map_blkio_rc(int rc)
{
    if (rc >= 0)
    {
        return PFS_PART_RET_OK;
    }
    if (PFS_BLKIO_RET_PARAM == rc || PFS_BLKIO_RET_STATE == rc)
    {
        return PFS_PART_RET_EINVAL;
    }
    return PFS_PART_RET_EIO;
}

static int pfs_part_gpt_map_xfer_rc(int rc, int expect)
{
    if (rc == expect)
    {
        return PFS_PART_RET_OK;
    }
    else if (rc >= 0)
    {
        return PFS_PART_RET_EIO;
    }
    return pfs_part_gpt_map_blkio_rc(rc);
}

// EFI CRC32: Ethernet poly, seed ~0, 终值再 ^ ~0 (linux efi_crc32)
static uint32_t pfs_part_gpt_crc32_tab[256];
static bool pfs_part_gpt_crc32_ready = false;

static void pfs_part_gpt_crc32_init(void)
{
    if (pfs_part_gpt_crc32_ready)
    {
        return;
    }
    for (uint32_t i = 0; i < 256u; i++)
    {
        uint32_t c = i;
        for (uint32_t j = 0; j < 8u; j++)
        {
            if (c & 1u)
            {
                c = 0xEDB88320u ^ (c >> 1);
            }
            else
            {
                c = c >> 1;
            }
        }
        pfs_part_gpt_crc32_tab[i] = c;
    }
    pfs_part_gpt_crc32_ready = true;
}

static uint32_t pfs_part_gpt_efi_crc32(const void *buf, size_t len)
{
    const uint8_t *p = (const uint8_t *)buf;
    uint32_t crc = 0xFFFFFFFFu;

    pfs_part_gpt_crc32_init();
    for (size_t i = 0; i < len; i++)
    {
        crc = pfs_part_gpt_crc32_tab[(crc ^ p[i]) & 0xFFu] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}

static bool pfs_part_gpt_guid_is_null(const pfs_part_guid_t *p_guid)
{
    static const uint8_t z[16];

    if (NULL == p_guid)
    {
        return true;
    }
    return 0 == memcmp(p_guid->b, z, sizeof(z));
}

static int pfs_part_gpt_read_lba(pfs_blkio_t *p_blkio, uint64_t lba, void *buf, int bytes)
{
    int sector_size = pfs_blkio_logical_sector_size(p_blkio);
    if (sector_size <= 0)
    {
        return PFS_PART_RET_EINVAL;
    }
    if (lba > (uint64_t)INT64_MAX / (uint64_t)sector_size)
    {
        return PFS_PART_RET_EINVAL;
    }
    int64_t off = (int64_t)(lba * (uint64_t)sector_size);
    int rc = pfs_blkio_pread(p_blkio, buf, bytes, off);
    return pfs_part_gpt_map_xfer_rc(rc, bytes);
}

static int pfs_part_gpt_disk_last_lba(pfs_blkio_t *p_blkio, uint64_t *p_last_lba)
{
    if (NULL == p_last_lba)
    {
        return PFS_PART_RET_EINVAL;
    }
    int sector_size = pfs_blkio_logical_sector_size(p_blkio);
    if (sector_size < PFS_PART_MBR_SECTOR_SIZE ||
            (sector_size & (sector_size - 1)) != 0)
    {
        return PFS_PART_RET_EINVAL;
    }
    int64_t disk_bytes = pfs_blkio_get_size(p_blkio);
    if (disk_bytes < (int64_t)sector_size * 2)
    {
        return PFS_PART_RET_ENOEXEC;
    }
    *p_last_lba = (uint64_t)(disk_bytes / (int64_t)sector_size) - 1ULL;
    return PFS_PART_RET_OK;
}

// 有效 Protective/Hybrid MBR: 签名 AA55, 且存在 os_type=0xEE 且 start_lba==1
static int pfs_part_gpt_pmbr_ok(pfs_blkio_t *p_blkio)
{
    if (sizeof(pfs_part_mbr_t) != (size_t)PFS_PART_MBR_SECTOR_SIZE)
    {
        return PFS_PART_RET_EINVAL;
    }

    int sector_size = pfs_blkio_logical_sector_size(p_blkio);
    if (sector_size < PFS_PART_MBR_SECTOR_SIZE ||
            sector_size > 4096 ||
            (sector_size & (sector_size - 1)) != 0)
    {
        return PFS_PART_RET_EINVAL;
    }

    uint8_t sec_buf[4096];
    pfs_part_mbr_t mbr;
    int rc;
    if (sector_size == PFS_PART_MBR_SECTOR_SIZE)
    {
        rc = pfs_blkio_pread(p_blkio, &mbr, PFS_PART_MBR_SECTOR_SIZE, 0);
        rc = pfs_part_gpt_map_xfer_rc(rc, PFS_PART_MBR_SECTOR_SIZE);
    }
    else
    {
        rc = pfs_blkio_pread(p_blkio, sec_buf, sector_size, 0);
        rc = pfs_part_gpt_map_xfer_rc(rc, sector_size);
        if (PFS_PART_RET_OK == rc)
        {
            memcpy(&mbr, sec_buf, sizeof(mbr));
        }
    }
    if (PFS_PART_RET_OK != rc)
    {
        return rc;
    }

    uint16_t sig = pfs_le16_to_cpu(mbr.signature);
    if (PFS_PART_MBR_SIGNATURE != sig)
    {
        return PFS_PART_RET_ENOEXEC;
    }

    for (int i = 0; i < PFS_PART_MBR_PRIMARY_COUNT; i++)
    {
        if (PFS_PART_MBR_OSTYPE_EFI_GPT == mbr.partition[i].sys_ind &&
                PFS_PART_GPT_PRIMARY_LBA == pfs_le32_to_cpu(mbr.partition[i].start_lba))
        {
            return PFS_PART_RET_OK;
        }
    }
    return PFS_PART_RET_ENOEXEC;
}

// 主 Header 未完整校验时, 若签名为 EFI PART 则取出 alternate_lba 供回退
static int pfs_part_gpt_peek_primary_alt_lba(pfs_blkio_t *p_blkio, uint64_t *p_alt_lba)
{
    if (NULL == p_alt_lba)
    {
        return PFS_PART_RET_EINVAL;
    }
    *p_alt_lba = 0;

    int sector_size = pfs_blkio_logical_sector_size(p_blkio);
    if (sector_size < PFS_PART_MBR_SECTOR_SIZE)
    {
        return PFS_PART_RET_EINVAL;
    }

    uint8_t *p_hdr_sec = (uint8_t *)pfs_malloc((size_t)sector_size);
    if (NULL == p_hdr_sec)
    {
        return PFS_PART_RET_ENOMEM;
    }

    int rc = pfs_part_gpt_read_lba(p_blkio, PFS_PART_GPT_PRIMARY_LBA, p_hdr_sec, sector_size);
    if (PFS_PART_RET_OK != rc)
    {
        pfs_free(p_hdr_sec);
        return rc;
    }

    pfs_part_gpt_header_t *p_hdr = (pfs_part_gpt_header_t *)(void *)p_hdr_sec;
    if (PFS_PART_GPT_HEADER_SIGNATURE == pfs_le64_to_cpu(p_hdr->signature))
    {
        *p_alt_lba = pfs_le64_to_cpu(p_hdr->alternate_lba);
    }
    pfs_free(p_hdr_sec);
    return PFS_PART_RET_OK;
}

// 在指定 Header LBA 加载并校验 GPT, 填 *p_out
static int pfs_part_gpt_load_at(pfs_blkio_t *p_blkio, uint64_t hdr_lba, pfs_part_table_t *p_out)
{
    int sector_size = pfs_blkio_logical_sector_size(p_blkio);
    if (sector_size < PFS_PART_MBR_SECTOR_SIZE ||
            (sector_size & (sector_size - 1)) != 0)
    {
        return PFS_PART_RET_EINVAL;
    }
    if (sizeof(pfs_part_gpt_header_t) > (size_t)sector_size ||
            sizeof(pfs_part_gpt_entry_t) != 128u)
    {
        return PFS_PART_RET_EINVAL;
    }

    uint64_t last_lba;
    int rc = pfs_part_gpt_disk_last_lba(p_blkio, &last_lba);
    if (PFS_PART_RET_OK != rc)
    {
        return rc;
    }
    if (hdr_lba > last_lba)
    {
        return PFS_PART_RET_EBADMSG;
    }

    uint8_t *p_hdr_sec = (uint8_t *)pfs_malloc((size_t)sector_size);
    if (NULL == p_hdr_sec)
    {
        return PFS_PART_RET_ENOMEM;
    }

    uint8_t *p_ptes = NULL;

    rc = pfs_part_gpt_read_lba(p_blkio, hdr_lba, p_hdr_sec, sector_size);
    if (PFS_PART_RET_OK != rc)
    {
        goto out;
    }

    pfs_part_gpt_header_t *p_hdr = (pfs_part_gpt_header_t *)(void *)p_hdr_sec;
    if (PFS_PART_GPT_HEADER_SIGNATURE != pfs_le64_to_cpu(p_hdr->signature))
    {
        rc = PFS_PART_RET_ENOEXEC;
        goto out;
    }

    uint32_t header_size = pfs_le32_to_cpu(p_hdr->header_size);
    if (header_size > (uint32_t)sector_size ||
            header_size < (uint32_t)sizeof(pfs_part_gpt_header_t))
    {
        rc = PFS_PART_RET_EBADMSG;
        goto out;
    }

    uint32_t orig_crc = pfs_le32_to_cpu(p_hdr->header_crc32);
    p_hdr->header_crc32 = 0;
    uint32_t calc_crc = pfs_part_gpt_efi_crc32(p_hdr_sec, (size_t)header_size);
    p_hdr->header_crc32 = pfs_cpu_to_le32(orig_crc);
    if (calc_crc != orig_crc)
    {
        rc = PFS_PART_RET_EBADMSG;
        goto out;
    }

    uint64_t my_lba = pfs_le64_to_cpu(p_hdr->my_lba);
    if (my_lba != hdr_lba)
    {
        rc = PFS_PART_RET_EBADMSG;
        goto out;
    }

    uint64_t first_usable = pfs_le64_to_cpu(p_hdr->first_usable_lba);
    uint64_t last_usable = pfs_le64_to_cpu(p_hdr->last_usable_lba);
    if (first_usable > last_lba || last_usable > last_lba || last_usable < first_usable)
    {
        rc = PFS_PART_RET_EBADMSG;
        goto out;
    }

    uint32_t ent_size = pfs_le32_to_cpu(p_hdr->sizeof_partition_entry);
    if (ent_size != (uint32_t)sizeof(pfs_part_gpt_entry_t))
    {
        rc = PFS_PART_RET_EBADMSG;
        goto out;
    }

    uint32_t num_ents = pfs_le32_to_cpu(p_hdr->num_partition_entries);
    uint64_t pt_bytes = (uint64_t)num_ents * (uint64_t)ent_size;
    if (0 == num_ents || pt_bytes > (uint64_t)PFS_PART_GPT_PT_BYTES_MAX)
    {
        rc = PFS_PART_RET_EBADMSG;
        goto out;
    }

    uint64_t entry_lba = pfs_le64_to_cpu(p_hdr->partition_entry_lba);
    p_ptes = (uint8_t *)pfs_malloc((size_t)pt_bytes);
    if (NULL == p_ptes)
    {
        rc = PFS_PART_RET_ENOMEM;
        goto out;
    }

    {
        if (entry_lba > (uint64_t)INT64_MAX / (uint64_t)sector_size)
        {
            rc = PFS_PART_RET_EBADMSG;
            goto out;
        }
        int64_t off = (int64_t)(entry_lba * (uint64_t)sector_size);
        if (pt_bytes > (uint64_t)INT_MAX)
        {
            rc = PFS_PART_RET_EBADMSG;
            goto out;
        }
        int want = (int)pt_bytes;
        int got = pfs_blkio_pread(p_blkio, p_ptes, want, off);
        rc = pfs_part_gpt_map_xfer_rc(got, want);
        if (PFS_PART_RET_OK != rc)
        {
            goto out;
        }
    }

    calc_crc = pfs_part_gpt_efi_crc32(p_ptes, (size_t)pt_bytes);
    if (calc_crc != pfs_le32_to_cpu(p_hdr->partition_entry_array_crc32))
    {
        rc = PFS_PART_RET_EBADMSG;
        goto out;
    }

    memset(p_out, 0, sizeof(*p_out));
    int count = 0;
    for (uint32_t i = 0; i < num_ents; i++)
    {
        const pfs_part_gpt_entry_t *p_ent =
                (const pfs_part_gpt_entry_t *)(void *)(p_ptes + (size_t)i * (size_t)ent_size);
        uint64_t start_lba;
        uint64_t end_lba;
        uint64_t attrs;

        if (pfs_part_gpt_guid_is_null(&p_ent->type_guid))
        {
            continue;
        }
        start_lba = pfs_le64_to_cpu(p_ent->starting_lba);
        end_lba = pfs_le64_to_cpu(p_ent->ending_lba);
        if (start_lba > last_lba || end_lba > last_lba || end_lba < start_lba)
        {
            continue;
        }
        if (count >= PFS_PART_ENTRY_MAX)
        {
            rc = PFS_PART_RET_EBADMSG;
            goto out;
        }
        attrs = pfs_le64_to_cpu(p_ent->attributes);
        p_out->entries[count].start_lba = start_lba;
        p_out->entries[count].size_lba = end_lba - start_lba + 1ULL;
        p_out->entries[count].bootable =
                (0 != (attrs & PFS_PART_GPT_ATTR_LEGACY_BIOS_BOOTABLE));
        p_out->entries[count].mbr_sys_ind = 0;
        count++;
    }

    p_out->scheme = PFS_PART_SCHEME_GPT;
    p_out->count = count;
    rc = PFS_PART_RET_OK;

out:
    pfs_free(p_ptes);
    pfs_free(p_hdr_sec);
    return rc;
}

int pfs_part_gpt_probe(pfs_blkio_t *p_blkio, pfs_part_table_t *p_out)
{
    if (NULL == p_blkio || NULL == p_out)
    {
        return PFS_PART_RET_EINVAL;
    }
    if (!pfs_blkio_is_open(p_blkio))
    {
        return PFS_PART_RET_EINVAL;
    }

    memset(p_out, 0, sizeof(*p_out));
    p_out->scheme = PFS_PART_SCHEME_NONE;

    int rc = pfs_part_gpt_pmbr_ok(p_blkio);
    if (PFS_PART_RET_OK != rc)
    {
        return rc;
    }

    // 优先主 GPT (LBA1)
    int rc_pri = pfs_part_gpt_load_at(p_blkio, PFS_PART_GPT_PRIMARY_LBA, p_out);
    if (PFS_PART_RET_OK == rc_pri)
    {
        return PFS_PART_RET_OK;
    }
    if (PFS_PART_RET_ENOMEM == rc_pri || PFS_PART_RET_EINVAL == rc_pri)
    {
        return rc_pri;
    }

    // 阶段 B: 主表失败则试 Alternate (先 peek 主头 alternate_lba, 再盘尾 last_lba)
    // 相对内核默认 (主坏且无 force_gpt 不试备表), 用户态探测更偏向可恢复
    uint64_t tried_alt = 0;
    uint64_t alt_lba = 0;
    rc = pfs_part_gpt_peek_primary_alt_lba(p_blkio, &alt_lba);
    if (PFS_PART_RET_ENOMEM == rc || PFS_PART_RET_EINVAL == rc)
    {
        return rc;
    }
    if (PFS_PART_RET_OK == rc &&
            0 != alt_lba &&
            alt_lba != (uint64_t)PFS_PART_GPT_PRIMARY_LBA)
    {
        rc = pfs_part_gpt_load_at(p_blkio, alt_lba, p_out);
        if (PFS_PART_RET_OK == rc)
        {
            return PFS_PART_RET_OK;
        }
        if (PFS_PART_RET_ENOMEM == rc || PFS_PART_RET_EINVAL == rc)
        {
            return rc;
        }
        tried_alt = alt_lba;
    }

    uint64_t last_lba;
    rc = pfs_part_gpt_disk_last_lba(p_blkio, &last_lba);
    if (PFS_PART_RET_OK != rc)
    {
        return (PFS_PART_RET_ENOEXEC == rc) ? rc_pri : rc;
    }
    if (last_lba != (uint64_t)PFS_PART_GPT_PRIMARY_LBA && last_lba != tried_alt)
    {
        rc = pfs_part_gpt_load_at(p_blkio, last_lba, p_out);
        if (PFS_PART_RET_OK == rc)
        {
            return PFS_PART_RET_OK;
        }
        if (PFS_PART_RET_ENOMEM == rc || PFS_PART_RET_EINVAL == rc)
        {
            return rc;
        }
    }

    // 主备皆失败: 保留主表错误码 (常见 EBADMSG/ENOEXEC/EIO)
    memset(p_out, 0, sizeof(*p_out));
    p_out->scheme = PFS_PART_SCHEME_NONE;
    return rc_pri;
}

// end
