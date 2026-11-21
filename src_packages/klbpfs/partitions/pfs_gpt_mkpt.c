// Doc Encode : UTF-8 BOM, Unix(LF)
// 做 GPT 分区表; 支持 count>=1 均分可用区
#include "pfs.h"
#include "pfs_part.h"
#include "partitions/pfs_part_gpt_raw.h"
#include "partitions/pfs_part_internal.h"
#include "partitions/pfs_part_mbr_raw.h"
#include "pfs_endian.h"
#include "base/pfs_mem.h"

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define PFS_PART_GPT_ALIGN_BYTES            (1024u * 1024u)
#define PFS_PART_GPT_NUM_ENTRIES            128u

// Microsoft Basic Data Partition (FAT/exFAT/NTFS 常用)
// EBD0A0A2-B9E5-4433-87C0-68B6B72699C7
static const uint8_t g_pfs_part_gpt_type_basic_data[16] = {
    0xA2u, 0xA0u, 0xD0u, 0xEBu, 0xE5u, 0xB9u, 0x33u, 0x44u,
    0x87u, 0xC0u, 0x68u, 0xB6u, 0xB7u, 0x26u, 0x99u, 0xC7u
};

static int pfs_gpt_mkpt_map_blkio_rc(int rc)
{
    if (rc >= 0)
    {
        return PFS_OK;
    }
    if (PFS_BLKIO_RET_PARAM == rc || PFS_BLKIO_RET_STATE == rc)
    {
        return PFS_EINVAL;
    }
    return PFS_EIO;
}

static int pfs_gpt_mkpt_map_xfer_rc(int rc, int expect)
{
    if (rc == expect)
    {
        return PFS_OK;
    }
    else if (rc >= 0)
    {
        return PFS_EIO;
    }
    return pfs_gpt_mkpt_map_blkio_rc(rc);
}

static uint32_t pfs_gpt_mkpt_crc32_tab[256];
static bool pfs_gpt_mkpt_crc32_ready = false;

static void pfs_gpt_mkpt_crc32_init(void)
{
    if (pfs_gpt_mkpt_crc32_ready)
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
        pfs_gpt_mkpt_crc32_tab[i] = c;
    }
    pfs_gpt_mkpt_crc32_ready = true;
}

static uint32_t pfs_gpt_mkpt_efi_crc32(const void *buf, size_t len)
{
    const uint8_t *p = (const uint8_t *)buf;
    uint32_t crc = 0xFFFFFFFFu;

    pfs_gpt_mkpt_crc32_init();
    for (size_t i = 0; i < len; i++)
    {
        crc = pfs_gpt_mkpt_crc32_tab[(crc ^ p[i]) & 0xFFu] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}

static int pfs_gpt_mkpt_fs_ok(pfs_fs_type_e fs_type)
{
    switch (fs_type)
    {
    case PFS_NONE:
    case PFS_FAT12:
    case PFS_FAT16:
    case PFS_FAT32:
    case PFS_EXFAT:
    case PFS_NTFS:
        return PFS_OK;
    default:
        return PFS_EINVAL;
    }
}

static void pfs_gpt_mkpt_guid_from_seed(pfs_part_guid_t *p_guid, uint32_t s0, uint32_t s1, uint32_t s2, uint32_t s3)
{
    if (NULL == p_guid)
    {
        return;
    }
    uint8_t *b = p_guid->b;
    b[0] = (uint8_t)s0;
    b[1] = (uint8_t)(s0 >> 8);
    b[2] = (uint8_t)(s0 >> 16);
    b[3] = (uint8_t)(s0 >> 24);
    b[4] = (uint8_t)s1;
    b[5] = (uint8_t)(s1 >> 8);
    b[6] = (uint8_t)s2;
    b[7] = (uint8_t)(s2 >> 8);
    b[8] = (uint8_t)s3;
    b[9] = (uint8_t)(s3 >> 8);
    b[10] = (uint8_t)(s3 >> 16);
    b[11] = (uint8_t)(s3 >> 24);
    b[12] = 0x50u;
    b[13] = 0x46u;
    b[14] = 0x53u;
    b[15] = 0x47u; // "PFSG"
}

static void pfs_gpt_mkpt_set_chs_lba_only(pfs_part_mbr_ent_t *p_ent)
{
    p_ent->start_head = 0xFEu;
    p_ent->start_sector = 0xFFu;
    p_ent->start_cyl = 0xFFu;
    p_ent->end_head = 0xFEu;
    p_ent->end_sector = 0xFFu;
    p_ent->end_cyl = 0xFFu;
}

static void pfs_gpt_mkpt_fill_header(pfs_part_gpt_header_t *p_hdr, uint64_t my_lba,
        uint64_t alt_lba, uint64_t first_usable, uint64_t last_usable,
        const pfs_part_guid_t *p_disk_guid, uint64_t entry_lba, uint32_t entry_crc)
{
    memset(p_hdr, 0, sizeof(*p_hdr));
    p_hdr->signature = pfs_cpu_to_le64(PFS_PART_GPT_HEADER_SIGNATURE);
    p_hdr->revision = pfs_cpu_to_le32(PFS_PART_GPT_HEADER_REVISION_V1);
    p_hdr->header_size = pfs_cpu_to_le32((uint32_t)sizeof(pfs_part_gpt_header_t));
    p_hdr->header_crc32 = 0;
    p_hdr->my_lba = pfs_cpu_to_le64(my_lba);
    p_hdr->alternate_lba = pfs_cpu_to_le64(alt_lba);
    p_hdr->first_usable_lba = pfs_cpu_to_le64(first_usable);
    p_hdr->last_usable_lba = pfs_cpu_to_le64(last_usable);
    p_hdr->disk_guid = *p_disk_guid;
    p_hdr->partition_entry_lba = pfs_cpu_to_le64(entry_lba);
    p_hdr->num_partition_entries = pfs_cpu_to_le32(PFS_PART_GPT_NUM_ENTRIES);
    p_hdr->sizeof_partition_entry = pfs_cpu_to_le32((uint32_t)sizeof(pfs_part_gpt_entry_t));
    p_hdr->partition_entry_array_crc32 = pfs_cpu_to_le32(entry_crc);
}

static int pfs_gpt_mkpt_write_lba(pfs_blkio_t *p_blkio, uint64_t lba, const void *buf, int bytes)
{
    int sector_size = pfs_blkio_logical_sector_size(p_blkio);
    if (sector_size <= 0)
    {
        return PFS_EINVAL;
    }
    if (lba > (uint64_t)INT64_MAX / (uint64_t)sector_size)
    {
        return PFS_EINVAL;
    }
    int64_t off = (int64_t)(lba * (uint64_t)sector_size);
    int rc = pfs_blkio_pwrite(p_blkio, buf, bytes, off);
    return pfs_gpt_mkpt_map_xfer_rc(rc, bytes);
}

static int pfs_gpt_mkpt_write_header_sec(pfs_blkio_t *p_blkio, uint64_t lba, const pfs_part_gpt_header_t *p_hdr_in)
{
    int sector_size = pfs_blkio_logical_sector_size(p_blkio);
    if (sector_size < (int)sizeof(pfs_part_gpt_header_t))
    {
        return PFS_EINVAL;
    }

    uint8_t *p_sec = (uint8_t *)pfs_mallocz((size_t)sector_size);
    if (NULL == p_sec)
    {
        return PFS_ENOMEM;
    }

    pfs_part_gpt_header_t *p_hdr = (pfs_part_gpt_header_t *)(void *)p_sec;
    *p_hdr = *p_hdr_in;
    p_hdr->header_crc32 = 0;
    uint32_t crc = pfs_gpt_mkpt_efi_crc32(p_sec, sizeof(pfs_part_gpt_header_t));
    p_hdr->header_crc32 = pfs_cpu_to_le32(crc);

    int rc = pfs_gpt_mkpt_write_lba(p_blkio, lba, p_sec, sector_size);
    pfs_free(p_sec);
    return rc;
}

// count>=1 均分可用区; 起止由实现按 1MiB 对齐自算, 结果写入 p_out
int pfs_gpt_mkpt(pfs_blkio_t *p_blkio, uint64_t total_bytes, const pfs_mkpt_table_t *p_in, pfs_part_table_t *p_out)
{
    if (NULL == p_blkio || NULL == p_in || NULL == p_out)
    {
        return PFS_EINVAL;
    }
    if (!pfs_blkio_is_open(p_blkio))
    {
        return PFS_EINVAL;
    }
    if (p_in->count < 1 || p_in->count > PFS_PART_ENTRY_MAX)
    {
        return PFS_EINVAL;
    }

    for (int i = 0; i < p_in->count; i++)
    {
        int rc = pfs_gpt_mkpt_fs_ok(p_in->entries[i].fs_type);
        if (PFS_OK != rc)
        {
            return rc;
        }
    }

    pfs_part_mbr_t mbr;
    if (sizeof(pfs_part_gpt_entry_t) != 128u ||
            sizeof(pfs_part_gpt_header_t) != 92u ||
            sizeof(mbr) != (size_t)PFS_PART_MBR_SECTOR_SIZE)
    {
        return PFS_EINVAL;
    }

    int sector_size = pfs_blkio_logical_sector_size(p_blkio);
    if (sector_size < PFS_PART_MBR_SECTOR_SIZE ||
            sector_size > 4096 ||
            (sector_size & (sector_size - 1)) != 0)
    {
        return PFS_EINVAL;
    }

    uint8_t sec_buf[4096];

    uint64_t disk_bytes = total_bytes;
    if (0 == disk_bytes)
    {
        int64_t dev_bytes = pfs_blkio_get_size(p_blkio);
        if (dev_bytes <= 0)
        {
            return PFS_EINVAL;
        }
        disk_bytes = (uint64_t)dev_bytes;
    }
    else
    {
        int64_t dev_bytes = pfs_blkio_get_size(p_blkio);
        if (dev_bytes > 0 && disk_bytes > (uint64_t)dev_bytes)
        {
            return PFS_EINVAL;
        }
    }

    if (0 != (disk_bytes % (uint64_t)sector_size))
    {
        return PFS_EINVAL;
    }

    uint64_t total_sectors = disk_bytes / (uint64_t)sector_size;
    if (total_sectors < 4u)
    {
        return PFS_EINVAL;
    }
    uint64_t last_lba = total_sectors - 1ULL;

    uint64_t entry_bytes = (uint64_t)PFS_PART_GPT_NUM_ENTRIES * (uint64_t)sizeof(pfs_part_gpt_entry_t);
    uint64_t entry_sectors = (entry_bytes + (uint64_t)sector_size - 1ULL) / (uint64_t)sector_size;
    if (0 == entry_sectors || entry_bytes > (uint64_t)INT_MAX)
    {
        return PFS_EINVAL;
    }

    // LBA0 PMBR, LBA1 pri hdr, LBA2.. pri entries, data, alt entries, last alt hdr
    if (total_sectors < (2ULL + entry_sectors + 1ULL + entry_sectors + 1ULL))
    {
        return PFS_EINVAL;
    }

    uint64_t first_usable = 2ULL + entry_sectors;
    uint64_t alt_entry_lba = last_lba - entry_sectors;
    if (alt_entry_lba < first_usable)
    {
        return PFS_EINVAL;
    }
    uint64_t last_usable = alt_entry_lba - 1ULL;
    if (last_usable < first_usable)
    {
        return PFS_EINVAL;
    }

    uint64_t align_lba = (uint64_t)PFS_PART_GPT_ALIGN_BYTES / (uint64_t)sector_size;
    if (0 == align_lba)
    {
        align_lba = 1;
    }

    uint64_t part_starts[PFS_PART_ENTRY_MAX];
    uint64_t part_sizes[PFS_PART_ENTRY_MAX];
    int part_count = p_in->count;
    if (align_lba >= first_usable && align_lba <= last_usable)
    {
        part_starts[0] = align_lba;
    }
    else
    {
        part_starts[0] = first_usable;
    }

    int rc = pfs_part_mkpt_split_even(part_starts[0], last_usable, align_lba, part_count,
            part_starts, part_sizes);
    if (PFS_PART_RET_OK != rc)
    {
        return rc;
    }

    pfs_fs_type_e fs_type = p_in->entries[0].fs_type;
    uint32_t seed = (uint32_t)total_sectors ^ ((uint32_t)fs_type << 16) ^ 0x47505431u;
    if (0 == seed)
    {
        seed = 0x47505431u;
    }
    pfs_part_guid_t disk_guid;
    pfs_gpt_mkpt_guid_from_seed(&disk_guid, seed, seed ^ 0x11111111u, seed ^ 0x22222222u,
            seed ^ 0x33333333u);

    uint8_t *p_ptes = (uint8_t *)pfs_mallocz((size_t)entry_bytes);
    if (NULL == p_ptes)
    {
        return PFS_ENOMEM;
    }

    memset(p_ptes, 0, (size_t)entry_bytes);

    for (int i = 0; i < part_count; i++)
    {
        char name_buf[8];
        const char *name;
        pfs_part_gpt_entry_t *p_ge_row;
        uint32_t part_seed;
        pfs_part_guid_t part_guid;

        p_ge_row = (pfs_part_gpt_entry_t *)(void *)(p_ptes + (size_t)i * sizeof(pfs_part_gpt_entry_t));
        fs_type = p_in->entries[i].fs_type;
        bool bootable = p_in->entries[i].bootable;
        part_seed = seed ^ (uint32_t)(i + 1) * 0x10001u;
        pfs_gpt_mkpt_guid_from_seed(&part_guid, part_seed, part_seed ^ 0x10u, part_seed ^ 0x20u,
                part_seed ^ 0x30u);

        memcpy(p_ge_row->type_guid.b, g_pfs_part_gpt_type_basic_data, 16);
        p_ge_row->unique_guid = part_guid;
        p_ge_row->starting_lba = pfs_cpu_to_le64(part_starts[i]);
        p_ge_row->ending_lba = pfs_cpu_to_le64(part_starts[i] + part_sizes[i] - 1ULL);
        p_ge_row->attributes = bootable ?
                pfs_cpu_to_le64(PFS_PART_GPT_ATTR_LEGACY_BIOS_BOOTABLE) : 0;

        snprintf(name_buf, sizeof(name_buf), "pfs%02d", (int)i + 1);
        name = name_buf;
        for (size_t j = 0; name[j] != '\0' && j < 36u; j++)
        {
            p_ge_row->name_utf16[j] = pfs_cpu_to_le16((uint16_t)(unsigned char)name[j]);
        }
    }

    uint32_t entry_crc = pfs_gpt_mkpt_efi_crc32(p_ptes, (size_t)entry_bytes);
    pfs_part_gpt_header_t hdr;

    // 主分区项数组 @ LBA2
    rc = pfs_gpt_mkpt_write_lba(p_blkio, 2, p_ptes, (int)entry_bytes);
    if (PFS_OK != rc)
    {
        goto out;
    }

    // 备分区项数组
    rc = pfs_gpt_mkpt_write_lba(p_blkio, alt_entry_lba, p_ptes, (int)entry_bytes);
    if (PFS_OK != rc)
    {
        goto out;
    }

    pfs_gpt_mkpt_fill_header(&hdr, PFS_PART_GPT_PRIMARY_LBA, last_lba, first_usable, last_usable,
            &disk_guid, 2, entry_crc);
    rc = pfs_gpt_mkpt_write_header_sec(p_blkio, PFS_PART_GPT_PRIMARY_LBA, &hdr);
    if (PFS_OK != rc)
    {
        goto out;
    }

    pfs_gpt_mkpt_fill_header(&hdr, last_lba, PFS_PART_GPT_PRIMARY_LBA, first_usable, last_usable,
            &disk_guid, alt_entry_lba, entry_crc);
    rc = pfs_gpt_mkpt_write_header_sec(p_blkio, last_lba, &hdr);
    if (PFS_OK != rc)
    {
        goto out;
    }

    // Protective MBR
    memset(&mbr, 0, sizeof(mbr));
    mbr.disk_signature = pfs_cpu_to_le32(seed);
    mbr.signature = pfs_cpu_to_le16((uint16_t)PFS_PART_MBR_SIGNATURE);
    pfs_part_mbr_ent_t *p_ent = &mbr.partition[0];
    p_ent->boot_ind = 0;
    p_ent->sys_ind = PFS_PART_MBR_OSTYPE_EFI_GPT;
    pfs_gpt_mkpt_set_chs_lba_only(p_ent);
    p_ent->start_lba = pfs_cpu_to_le32(1u);
    uint64_t pmbr_size = total_sectors - 1ULL;
    if (pmbr_size > 0xFFFFFFFFu)
    {
        pmbr_size = 0xFFFFFFFFu;
    }
    p_ent->size_lba = pfs_cpu_to_le32((uint32_t)pmbr_size);

    if (sector_size == PFS_PART_MBR_SECTOR_SIZE)
    {
        rc = pfs_blkio_pwrite(p_blkio, &mbr, PFS_PART_MBR_SECTOR_SIZE, 0);
        rc = pfs_gpt_mkpt_map_xfer_rc(rc, PFS_PART_MBR_SECTOR_SIZE);
    }
    else
    {
        rc = pfs_blkio_pread(p_blkio, sec_buf, sector_size, 0);
        rc = pfs_gpt_mkpt_map_xfer_rc(rc, sector_size);
        if (PFS_OK == rc)
        {
            memcpy(sec_buf, &mbr, sizeof(mbr));
            rc = pfs_blkio_pwrite(p_blkio, sec_buf, sector_size, 0);
            rc = pfs_gpt_mkpt_map_xfer_rc(rc, sector_size);
        }
    }
    if (PFS_OK != rc)
    {
        goto out;
    }

    rc = pfs_blkio_fsync(p_blkio);
    if (PFS_BLKIO_RET_OK != rc)
    {
        rc = pfs_gpt_mkpt_map_blkio_rc(rc);
        goto out;
    }

    p_out->scheme = PFS_PART_SCHEME_GPT;
    p_out->count = part_count;
    for (int i = 0; i < part_count; i++)
    {
        memset(&p_out->entries[i], 0, sizeof(p_out->entries[i]));
        p_out->entries[i].start_lba = part_starts[i];
        p_out->entries[i].size_lba = part_sizes[i];
        p_out->entries[i].bootable = p_in->entries[i].bootable;
        p_out->entries[i].mbr_sys_ind = 0;
    }
    rc = PFS_OK;

out:
    pfs_free(p_ptes);
    return rc;
}

// end
