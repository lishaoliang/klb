// Doc Encode : UTF-8 BOM, Unix(LF)
// 制作 NTFS 卷 (用户态 mkfs; 无内核对应)
#include "ntfs/pfs_ntfs_internal.h"

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <time.h>

#define PFS_NTFS_MKFS_SECTOR_SIZE           512U
#define PFS_NTFS_MKFS_MFT_RECORD_SIZE       1024U
#define PFS_NTFS_MKFS_BOOT_SECTORS          16U
// 无运行时 $MFT 增长时须够用; 32 仅够系统+十来个用户项, mkvol 百级文件会 ENOSPC
#define PFS_NTFS_MKFS_MFT_RECORD_COUNT      1024U
#define PFS_NTFS_MKFS_MIN_VOL_SECTORS       2048U
#define PFS_NTFS_MKFS_LOGFILE_BYTES_MIN     (256U * 1024U)
#define PFS_NTFS_MKFS_UPCASE_CHARS          65536U

#define PFS_NTFS_MKFS_INDEX_BLOCK_SIZE  4096U

#define PFS_NTFS_MKFS_DIR_FILE_ATTRS \
    (NTFS_FILE_ATTR_HIDDEN | NTFS_FILE_ATTR_SYSTEM | NTFS_FILE_ATTR_DIRECTORY | \
     NTFS_FILE_ATTR_DUP_FILE_NAME_INDEX_PRESENT)

#define PFS_NTFS_MKFS_LOG_SYSTEM_PAGE_SIZE  4096U
#define PFS_NTFS_MKFS_LOG_PAGE_SIZE         4096U
#define PFS_NTFS_MKFS_LOGFILE_NO_CLIENT     0xffffU
#define PFS_NTFS_MKFS_SECURITY_ID           0x100U
#define PFS_NTFS_MKFS_SECURITY_ID_META      0x101U
#define PFS_NTFS_MKFS_SDS_MIRROR_OFF        0x40000U
#define PFS_NTFS_MKFS_SDS_SLOT              0x80U
#define PFS_NTFS_MKFS_SDS_FIRST_BUCKET      0x100U
#define PFS_NTFS_MKFS_SDS_ENTRY_COUNT       2U
#define PFS_NTFS_MKFS_SDS_HDR_LEN           0x7CU

typedef struct _pfs_ntfs_mkfs_restart_page_header_t {
    uint32_t magic;
    uint16_t usa_ofs;
    uint16_t usa_count;
    uint64_t chkdsk_lsn;
    uint32_t system_page_size;
    uint32_t log_page_size;
    uint16_t restart_area_offset;
    uint16_t minor_ver;
    uint16_t major_ver;
} pfs_ntfs_mkfs_restart_page_header_t;

typedef struct _pfs_ntfs_mkfs_restart_area_t {
    uint64_t current_lsn;
    uint16_t log_clients;
    uint16_t client_free_list;
    uint16_t client_in_use_list;
    uint16_t flags;
    uint32_t seq_number_bits;
    uint16_t restart_area_length;
    uint16_t client_array_offset;
    uint64_t file_size;
    uint32_t last_lsn_data_length;
    uint16_t log_record_header_length;
    uint16_t log_page_data_offset;
    uint32_t restart_log_open_count;
    uint32_t reserved;
} pfs_ntfs_mkfs_restart_area_t;

typedef struct _pfs_ntfs_mkfs_log_client_record_t {
    uint64_t oldest_lsn;
    uint64_t client_restart_lsn;
    uint16_t prev_client;
    uint16_t next_client;
    uint16_t seq_number;
    uint8_t reserved[6];
    uint32_t client_name_length;
    uint16_t client_name[64];
} pfs_ntfs_mkfs_log_client_record_t;

typedef struct _pfs_ntfs_mkfs_layout_t {
    uint64_t boot_lcn;
    uint32_t boot_clusters;
    uint64_t mftbmp_lcn;
    uint32_t mftbmp_clusters;
    uint32_t mftbmp_bytes;
    uint64_t mft_lcn;
    uint32_t mft_clusters;
    uint32_t mft_record_count;
    uint64_t mftmirr_lcn;
    uint32_t mftmirr_clusters;
    uint32_t mftmirr_records;
    uint32_t mftmirr_bytes;
    uint64_t bitmap_lcn;
    uint32_t bitmap_clusters;
    uint64_t upcase_lcn;
    uint32_t upcase_clusters;
    uint64_t logfile_lcn;
    uint32_t logfile_clusters;
    uint64_t logfile_bytes;
    uint64_t attrdef_lcn;
    uint32_t attrdef_clusters;
    uint64_t root_idx_lcn;
    uint32_t root_idx_clusters;
    uint64_t sds_lcn;
    uint32_t sds_clusters;
    uint32_t sds_bytes;
    uint64_t sdh_idx_lcn;
    uint64_t sii_idx_lcn;
    uint32_t secure_idx_clusters;
    uint64_t total_clusters;
    uint32_t bitmap_bytes;
    uint32_t attrdef_bytes;
} pfs_ntfs_mkfs_layout_t;

typedef struct _pfs_ntfs_mkfs_mft_t {
    uint8_t             *buf;
    uint32_t            size;
    uint32_t            attrs_off;
    uint16_t            next_instance;
    uint32_t            rec_num;
    uint16_t            seq_num;
    uint16_t            flags;
} pfs_ntfs_mkfs_mft_t;

#pragma pack(push, 1)
typedef struct _pfs_ntfs_mkfs_sds_hdr_t {
    uint32_t hash;
    uint32_t security_id;
    uint64_t offset;
    uint32_t length;
} pfs_ntfs_mkfs_sds_hdr_t;
#pragma pack(pop)

typedef char pfs_ntfs_mkfs_sa_sds[(sizeof(pfs_ntfs_mkfs_sds_hdr_t) == 20) ? 1 : -1];

static int pfs_ntfs_mkfs_map_blkio_rc(int rc)
{
    if (rc >= 0)
    {
        return PFS_NTFS_RET_OK;
    }
    if (PFS_BLKIO_RET_PARAM == rc || PFS_BLKIO_RET_STATE == rc)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    return PFS_NTFS_RET_EIO;
}

static int pfs_ntfs_mkfs_pwrite(
        pfs_blkio_t *p_blkio,
        int64_t part_offset,
        int64_t vol_byte_off,
        const void *buf,
        int size)
{

    if (NULL == p_blkio || NULL == buf || size <= 0 || vol_byte_off < 0)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (part_offset < 0)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    int rc = pfs_blkio_pwrite(p_blkio, buf, size, part_offset + vol_byte_off);
    if (rc < 0)
    {
        return pfs_ntfs_mkfs_map_blkio_rc(rc);
    }
    if (rc != size)
    {
        return PFS_NTFS_RET_EIO;
    }
    return PFS_NTFS_RET_OK;
}

static uint32_t pfs_ntfs_mkfs_align8(uint32_t v)
{
    return (v + 7U) & ~7U;
}

static uint32_t pfs_ntfs_mkfs_align16(uint32_t v)
{
    return (v + 15U) & ~15U;
}

// 对齐 mkntfs: 按卷大小选 $LogFile; >=12GiB 用 64MiB (过小会导致 Windows 拒挂/蓝屏)
static uint64_t pfs_ntfs_mkfs_pick_logfile_bytes(uint64_t vol_bytes, uint32_t cluster_size)
{
    uint64_t sz = PFS_NTFS_MKFS_LOGFILE_BYTES_MIN;
    if (vol_bytes >= (2048ULL * 1024ULL))
    {
        if (vol_bytes < 4000000ULL)
        {
            sz = 512ULL * 1024ULL;
        }
        else if (vol_bytes <= (200ULL * 1024ULL * 1024ULL))
        {
            sz = 2048ULL * 1024ULL;
        }
        else if (vol_bytes >= (12ULL << 30))
        {
            sz = 64ULL << 20;
        }
        else
        {
            sz = (vol_bytes / 200ULL) & ~((uint64_t)cluster_size - 1ULL);
        }
    }
    if (sz < PFS_NTFS_MKFS_LOGFILE_BYTES_MIN)
    {
        sz = PFS_NTFS_MKFS_LOGFILE_BYTES_MIN;
    }
    sz = ((sz + (uint64_t)cluster_size - 1ULL) / (uint64_t)cluster_size) * (uint64_t)cluster_size;
    return sz;
}

// cluster<=4KiB: 镜像 4 条; 更大: N = cluster_size / mft_record_size
static uint32_t pfs_ntfs_mkfs_mftmirr_records(uint32_t cluster_size)
{
    uint32_t four_rec = 4U * PFS_NTFS_MKFS_MFT_RECORD_SIZE;

    if (cluster_size <= four_rec)
    {
        return 4U;
    }
    return cluster_size / PFS_NTFS_MKFS_MFT_RECORD_SIZE;
}

// 系统文件 seq = record 号; record 0 与 $Quota/$ObjId/$Reparse(24..26) 为 1 (mkntfs)
static uint16_t pfs_ntfs_mkfs_seq_for_rec(uint32_t rec_num)
{
    if (0U == rec_num || (rec_num >= 24U && rec_num <= 26U))
    {
        return 1U;
    }
    return (uint16_t)rec_num;
}

static uint8_t pfs_ntfs_mkfs_pick_sectors_per_cluster(uint64_t vol_bytes)
{
    if (vol_bytes >= (2ULL * 1024ULL * 1024ULL * 1024ULL))
    {
        return 128;
    }
    if (vol_bytes >= (512ULL * 1024ULL * 1024ULL))
    {
        return 64;
    }
    if (vol_bytes >= (256ULL * 1024ULL * 1024ULL))
    {
        return 32;
    }
    if (vol_bytes >= (128ULL * 1024ULL * 1024ULL))
    {
        return 16;
    }
    if (vol_bytes >= (64ULL * 1024ULL * 1024ULL))
    {
        return 8;
    }
    if (vol_bytes >= (32ULL * 1024ULL * 1024ULL))
    {
        return 4;
    }
    if (vol_bytes >= (16ULL * 1024ULL * 1024ULL))
    {
        return 2;
    }
    return 1;
}

static uint8_t pfs_ntfs_mkfs_bpb_sectors_per_cluster(uint8_t sectors_per_cluster)
{
    if (sectors_per_cluster <= 128)
    {
        return sectors_per_cluster;
    }
    {
        uint8_t bits = 0;
        uint8_t v = sectors_per_cluster;
        while (v > 1)
        {
            v >>= 1;
            bits++;
        }
        return (uint8_t)(257U - bits);
    }
}

// boot: clusters_per_mft / clusters_per_index_record (signed log2 when record < cluster)
static int8_t pfs_ntfs_mkfs_clusters_per_record(uint32_t record_bytes, uint32_t cluster_size)
{
    if (record_bytes >= cluster_size && 0U == (record_bytes % cluster_size))
    {
        return (int8_t)(record_bytes / cluster_size);
    }
    {
        uint32_t bits = 0;
        uint32_t sz = record_bytes;
        while (sz > 1U)
        {
            sz >>= 1;
            bits++;
        }
        return (int8_t)(-(int)bits);
    }
}

// INDEX_ROOT.clusters_per_index_block: record>=cluster 用簇数; 否则用扇区数 (与 boot log2 不同)
static uint8_t pfs_ntfs_mkfs_index_root_clusters_per_block(uint32_t index_block_size, uint32_t cluster_size)
{
    if (index_block_size >= cluster_size)
    {
        return (uint8_t)(index_block_size / cluster_size);
    }
    return (uint8_t)(index_block_size / PFS_NTFS_MKFS_SECTOR_SIZE);
}

static uint32_t pfs_ntfs_mkfs_clusters_for_bytes(uint32_t bytes, uint32_t cluster_size)
{
    if (0U == bytes || 0U == cluster_size)
    {
        return 0;
    }
    return (bytes + cluster_size - 1U) / cluster_size;
}

static uint64_t pfs_ntfs_mkfs_now_le(void)
{
    static int seeded = 0;
    uint64_t t;

    if (0 == seeded)
    {
        srand((unsigned int)time(NULL));
        seeded = 1;
    }
    t = (uint64_t)time(NULL);
    t = t * 10000000ULL + 116444736000000000ULL;
    return pfs_cpu_to_le64(t);
}

static uint64_t pfs_ntfs_mkfs_rand_serial(void)
{
    static int seeded = 0;
    uint64_t sig;

    if (0 == seeded)
    {
        srand((unsigned int)time(NULL));
        seeded = 1;
    }
    sig = ((uint64_t)(unsigned int)rand() << 32) ^ (uint64_t)(unsigned int)rand();
    if (0ULL == sig)
    {
        sig = (uint64_t)(unsigned int)rand() | 0x8000000000000000ULL;
    }
    return pfs_cpu_to_le64(sig);
}

static void pfs_ntfs_mkfs_ascii_to_utf16le(uint16_t *dst, const char *ascii, uint8_t *p_len)
{
    uint8_t n = 0;

    while (NULL != ascii && '\0' != *ascii && n < 255U)
    {
        dst[n] = pfs_cpu_to_le16((uint16_t)(unsigned char)*ascii);
        n++;
        ascii++;
    }
    if (NULL != p_len)
    {
        *p_len = n;
    }
}

static void pfs_ntfs_mkfs_utf16_name4(uint16_t *dst, char c0, char c1, char c2, char c3)
{
    dst[0] = pfs_cpu_to_le16((uint16_t)(unsigned char)c0);
    dst[1] = pfs_cpu_to_le16((uint16_t)(unsigned char)c1);
    dst[2] = pfs_cpu_to_le16((uint16_t)(unsigned char)c2);
    dst[3] = pfs_cpu_to_le16((uint16_t)(unsigned char)c3);
}

static void pfs_ntfs_mkfs_apply_usa(uint8_t *buf, uint32_t buf_size, uint16_t usa_ofs)
{
    uint16_t *p_usa;
    uint16_t usn;
    uint32_t sectors;

    if (NULL == buf || 0U == buf_size || usa_ofs + 4U > buf_size)
    {
        return;
    }
    sectors = buf_size / PFS_NTFS_MKFS_SECTOR_SIZE;
    p_usa = (uint16_t *)(buf + usa_ofs);
    usn = (uint16_t)(rand() & 0xffff);
    if (0U == usn)
    {
        usn = 1;
    }
    p_usa[0] = pfs_cpu_to_le16(usn);
    for (uint32_t i = 0; i < sectors; i++)
    {
        uint32_t sec_end = (i + 1U) * PFS_NTFS_MKFS_SECTOR_SIZE;
        if (sec_end > buf_size)
        {
            break;
        }
        p_usa[i + 1U] = pfs_read_le16(buf + sec_end - 2U);
        buf[sec_end - 2U] = (uint8_t)(usn & 0xff);
        buf[sec_end - 1U] = (uint8_t)((usn >> 8) & 0xff);
    }
}

static uint32_t pfs_ntfs_mkfs_log_seq_number_bits(uint64_t file_size)
{
    uint32_t bits = 0;
    while (file_size > 0ULL)
    {
        file_size >>= 1;
        bits++;
    }
    return 67U - bits;
}

static int pfs_ntfs_mkfs_write_logfile_restart_page(
        uint8_t *page,
        uint32_t page_size,
        uint64_t logfile_bytes)
{
    pfs_ntfs_mkfs_restart_page_header_t *p_rph;
    pfs_ntfs_mkfs_restart_area_t *p_ra;
    pfs_ntfs_mkfs_log_client_record_t *p_cr;
    uint16_t usa_ofs;
    uint16_t ra_ofs;
    uint16_t ca_ofs;
    uint16_t ra_len;

    if (NULL == page || page_size < PFS_NTFS_MKFS_LOG_SYSTEM_PAGE_SIZE)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    memset(page, 0, page_size);
    p_rph = (pfs_ntfs_mkfs_restart_page_header_t *)page;
    usa_ofs = (uint16_t)sizeof(pfs_ntfs_mkfs_restart_page_header_t);
    ra_ofs = (uint16_t)((usa_ofs + (uint16_t)(2U * (1U + page_size / PFS_NTFS_MKFS_SECTOR_SIZE)) + 7U) & ~7U);
    ca_ofs = 0x40U;
    ra_len = (uint16_t)(ca_ofs + (uint16_t)sizeof(pfs_ntfs_mkfs_log_client_record_t));
    p_rph->magic = NTFS_MAGIC_RSTR;
    p_rph->usa_ofs = pfs_cpu_to_le16(usa_ofs);
    p_rph->usa_count = pfs_cpu_to_le16((uint16_t)(1U + page_size / PFS_NTFS_MKFS_SECTOR_SIZE));
    p_rph->chkdsk_lsn = pfs_cpu_to_le64(0);
    p_rph->system_page_size = pfs_cpu_to_le32(PFS_NTFS_MKFS_LOG_SYSTEM_PAGE_SIZE);
    p_rph->log_page_size = pfs_cpu_to_le32(PFS_NTFS_MKFS_LOG_PAGE_SIZE);
    p_rph->restart_area_offset = pfs_cpu_to_le16(ra_ofs);
    p_rph->minor_ver = pfs_cpu_to_le16(1);
    p_rph->major_ver = pfs_cpu_to_le16(1);
    p_ra = (pfs_ntfs_mkfs_restart_area_t *)(page + ra_ofs);
    p_ra->current_lsn = pfs_cpu_to_le64(0);
    p_ra->log_clients = pfs_cpu_to_le16(1);
    // WinXP+ clean: free=NO_CLIENT, in_use=0, flags=RESTART_VOLUME_IS_CLEAN
    p_ra->client_free_list = pfs_cpu_to_le16(PFS_NTFS_MKFS_LOGFILE_NO_CLIENT);
    p_ra->client_in_use_list = pfs_cpu_to_le16(0);
    p_ra->flags = pfs_cpu_to_le16(0x0002);
    p_ra->seq_number_bits = pfs_cpu_to_le32(pfs_ntfs_mkfs_log_seq_number_bits(logfile_bytes));
    p_ra->restart_area_length = pfs_cpu_to_le16(ra_len);
    p_ra->client_array_offset = pfs_cpu_to_le16(ca_ofs);
    p_ra->file_size = pfs_cpu_to_le64(logfile_bytes);
    p_ra->last_lsn_data_length = pfs_cpu_to_le32(0);
    p_ra->log_record_header_length = pfs_cpu_to_le16(0x38);
    p_ra->log_page_data_offset = pfs_cpu_to_le16(0x40);
    p_ra->restart_log_open_count = pfs_cpu_to_le32((uint32_t)time(NULL));
    p_ra->reserved = pfs_cpu_to_le32(0);
    p_cr = (pfs_ntfs_mkfs_log_client_record_t *)((uint8_t *)p_ra + ca_ofs);
    p_cr->oldest_lsn = pfs_cpu_to_le64(0);
    p_cr->client_restart_lsn = pfs_cpu_to_le64(0);
    p_cr->prev_client = pfs_cpu_to_le16(PFS_NTFS_MKFS_LOGFILE_NO_CLIENT);
    p_cr->next_client = pfs_cpu_to_le16(PFS_NTFS_MKFS_LOGFILE_NO_CLIENT);
    p_cr->seq_number = pfs_cpu_to_le16(0);
    memset(p_cr->reserved, 0, sizeof(p_cr->reserved));
    p_cr->client_name_length = pfs_cpu_to_le32(8);
    p_cr->client_name[0] = pfs_cpu_to_le16('N');
    p_cr->client_name[1] = pfs_cpu_to_le16('T');
    p_cr->client_name[2] = pfs_cpu_to_le16('F');
    p_cr->client_name[3] = pfs_cpu_to_le16('S');
    pfs_ntfs_mkfs_apply_usa(page, page_size, usa_ofs);
    return PFS_NTFS_RET_OK;
}

static int pfs_ntfs_mkfs_write_logfile_restart(
        pfs_blkio_t *p_blkio,
        int64_t part_offset,
        uint64_t logfile_lcn,
        uint32_t cluster_size,
        uint64_t logfile_bytes)
{
    uint8_t page[PFS_NTFS_MKFS_LOG_SYSTEM_PAGE_SIZE];

    int rc = pfs_ntfs_mkfs_write_logfile_restart_page(
            page, (uint32_t)sizeof(page), logfile_bytes);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_ntfs_mkfs_pwrite(
            p_blkio,
            part_offset,
            (int64_t)(logfile_lcn * (uint64_t)cluster_size),
            page,
            (int)sizeof(page));
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    return pfs_ntfs_mkfs_pwrite(
            p_blkio,
            part_offset,
            (int64_t)(logfile_lcn * (uint64_t)cluster_size + PFS_NTFS_MKFS_LOG_SYSTEM_PAGE_SIZE),
            page,
            (int)sizeof(page));
}

// 写 LE 有符号压缩整数; 对齐 linux-7.1 fs/ntfs/runlist.c ntfs_write_significant_bytes
static int pfs_ntfs_mkfs_write_significant_bytes(int64_t n, uint8_t *dst, int cap)
{
    int64_t v = n;
    int len = 0;

    if (NULL == dst || cap < 1)
    {
        return -1;
    }
    do {
        if (len >= cap)
        {
            return -1;
        }
        dst[len++] = (uint8_t)(v & 0xff);
        v >>= 8;
    } while (v != 0 && v != -1);

    // 符号位须与值符号一致; 否则再补一字节
    if (n >= 0)
    {
        if (0 != (dst[len - 1] & 0x80))
        {
            if (len >= cap)
            {
                return -1;
            }
            dst[len++] = 0;
        }
    }
    else
    {
        if (0 == (dst[len - 1] & 0x80))
        {
            if (len >= cap)
            {
                return -1;
            }
            dst[len++] = 0xff;
        }
    }
    return len;
}

// 单段 run: [header][len LE][lcn delta LE][0]; header = len_sz | (off_sz << 4)
static int pfs_ntfs_mkfs_build_mapping_pairs(uint8_t *dst, int cap, uint64_t lcn, uint64_t run_len)
{
    uint8_t len_buf[8];
    uint8_t off_buf[8];
    int len_sz;
    int off_sz;
    int need;

    if (NULL == dst || cap < 3 || 0ULL == run_len)
    {
        return -1;
    }
    len_sz = pfs_ntfs_mkfs_write_significant_bytes((int64_t)run_len, len_buf, (int)sizeof(len_buf));
    off_sz = pfs_ntfs_mkfs_write_significant_bytes((int64_t)lcn, off_buf, (int)sizeof(off_buf));
    if (len_sz < 1 || off_sz < 1 || len_sz > 15 || off_sz > 15)
    {
        return -1;
    }
    need = 1 + len_sz + off_sz + 1;
    if (need > cap)
    {
        return -1;
    }
    dst[0] = (uint8_t)(len_sz | (off_sz << 4));
    memcpy(dst + 1, len_buf, (size_t)len_sz);
    memcpy(dst + 1 + len_sz, off_buf, (size_t)off_sz);
    dst[1 + len_sz + off_sz] = 0;
    return need;
}

// 稀疏单段 (无 LCN delta): header 高 nibble=0, 表示 hole
static int pfs_ntfs_mkfs_build_mapping_pairs_sparse(uint8_t *dst, int cap, uint64_t run_len)
{
    uint8_t len_buf[8];
    int len_sz;
    int need;

    if (NULL == dst || cap < 3 || 0ULL == run_len)
    {
        return -1;
    }
    len_sz = pfs_ntfs_mkfs_write_significant_bytes((int64_t)run_len, len_buf, (int)sizeof(len_buf));
    if (len_sz < 1 || len_sz > 15)
    {
        return -1;
    }
    need = 1 + len_sz + 1;
    if (need > cap)
    {
        return -1;
    }
    dst[0] = (uint8_t)len_sz;
    memcpy(dst + 1, len_buf, (size_t)len_sz);
    dst[1 + len_sz] = 0;
    return need;
}

static int pfs_ntfs_mkfs_mft_init(pfs_ntfs_mkfs_mft_t *p_mft, uint32_t rec_num, uint16_t flags)
{
    ntfs_mft_record_t *p_hdr;

    if (NULL == p_mft || NULL == p_mft->buf || 0U == p_mft->size)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    memset(p_mft->buf, 0, p_mft->size);
    p_mft->rec_num = rec_num;
    p_mft->seq_num = pfs_ntfs_mkfs_seq_for_rec(rec_num);
    p_mft->flags = flags;
    p_mft->next_instance = 0;
    p_mft->attrs_off = 56U;
    p_hdr = (ntfs_mft_record_t *)p_mft->buf;
    p_hdr->magic = NTFS_MAGIC_FILE;
    p_hdr->usa_ofs = pfs_cpu_to_le16(48);
    p_hdr->usa_count = pfs_cpu_to_le16((uint16_t)(p_mft->size / PFS_NTFS_MKFS_SECTOR_SIZE + 1U));
    p_hdr->sequence_number = pfs_cpu_to_le16(p_mft->seq_num);
    p_hdr->link_count = pfs_cpu_to_le16(1);
    p_hdr->attrs_offset = pfs_cpu_to_le16((uint16_t)p_mft->attrs_off);
    p_hdr->flags = flags;
    p_hdr->bytes_allocated = pfs_cpu_to_le32(p_mft->size);
    p_hdr->base_mft_record = pfs_cpu_to_le64(0);
    p_hdr->next_attr_instance = pfs_cpu_to_le16(0);
    p_hdr->mft_record_number = pfs_cpu_to_le32(rec_num);
    return PFS_NTFS_RET_OK;
}

static uint32_t pfs_ntfs_mkfs_mft_attr_append_off(pfs_ntfs_mkfs_mft_t *p_mft)
{
    ntfs_mft_record_t *p_hdr = (ntfs_mft_record_t *)p_mft->buf;
    uint32_t off = (uint32_t)pfs_le16_to_cpu(p_hdr->attrs_offset);
    ntfs_attr_record_t *p_a;

    while (off + (uint32_t)sizeof(uint32_t) <= p_mft->size)
    {
        p_a = (ntfs_attr_record_t *)(p_mft->buf + off);
        if (NTFS_AT_END == p_a->type)
        {
            return off;
        }
        if (0U == p_a->length)
        {
            break;
        }
        off += pfs_le32_to_cpu(p_a->length);
    }
    return off;
}

static int pfs_ntfs_mkfs_mft_add_resident(
        pfs_ntfs_mkfs_mft_t *p_mft,
        uint32_t type,
        const uint16_t *name,
        uint8_t name_len,
        const void *val,
        uint32_t val_len,
        uint16_t attr_flags)
{
    ntfs_attr_record_t *p_a;
    uint32_t append_off;
    uint32_t hdr = 24U;
    uint32_t name_bytes;
    uint32_t name_off;
    uint32_t val_off;
    uint32_t total;

    if (NULL == p_mft || (NULL == val && val_len > 0U))
    {
        return PFS_NTFS_RET_EINVAL;
    }
    name_bytes = (uint32_t)name_len * 2U;
    name_off = hdr;
    val_off = pfs_ntfs_mkfs_align8(hdr + name_bytes);
    total = pfs_ntfs_mkfs_align8(val_off + val_len) + (uint32_t)sizeof(uint32_t);
    append_off = pfs_ntfs_mkfs_mft_attr_append_off(p_mft);
    if (append_off + total > p_mft->size)
    {
        return PFS_NTFS_RET_ENOSPC;
    }
    p_a = (ntfs_attr_record_t *)(p_mft->buf + append_off);
    memset(p_a, 0, pfs_ntfs_mkfs_align8(val_off + val_len));
    p_a->type = type;
    p_a->non_resident = 0;
    p_a->name_length = name_len;
    // 无名属性也写 name_offset=头长 (Windows SI 同款); 有名则指向名起点
    p_a->name_offset = pfs_cpu_to_le16((uint16_t)name_off);
    p_a->flags = attr_flags;
    p_a->instance = pfs_cpu_to_le16(p_mft->next_instance++);
    p_a->data.resident.value_length = pfs_cpu_to_le32(val_len);
    p_a->data.resident.value_offset = pfs_cpu_to_le16((uint16_t)val_off);
    // $FILE_NAME 在目录索引中被引用, Windows 要求 RESIDENT_ATTR_IS_INDEXED
    p_a->data.resident.flags = (NTFS_AT_FILE_NAME == type) ? (uint8_t)NTFS_RESIDENT_ATTR_IS_INDEXED : (uint8_t)0;
    p_a->data.resident.reserved = 0;
    if (name_len > 0U && NULL != name)
    {
        memcpy((uint8_t *)p_a + name_off, name, name_bytes);
    }
    if (val_len > 0U && NULL != val)
    {
        memcpy((uint8_t *)p_a + val_off, val, val_len);
    }
    p_a->length = pfs_cpu_to_le32(pfs_ntfs_mkfs_align8(val_off + val_len));
    return PFS_NTFS_RET_OK;
}

static int pfs_ntfs_mkfs_mft_add_nonres(
        pfs_ntfs_mkfs_mft_t *p_mft,
        uint32_t type,
        const uint16_t *name,
        uint8_t name_len,
        uint64_t lcn,
        uint64_t run_clusters,
        uint64_t data_size,
        uint32_t cluster_size,
        uint16_t attr_flags)
{
    ntfs_attr_record_t *p_a;
    uint8_t mp_buf[32];
    int mp_len;
    uint32_t append_off;
    uint32_t hdr = 64U;
    uint32_t name_bytes;
    uint32_t name_off;
    uint32_t mp_off;
    uint32_t total;
    uint64_t alloc_size;

    if (NULL == p_mft || 0ULL == run_clusters || 0U == cluster_size)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    mp_len = pfs_ntfs_mkfs_build_mapping_pairs(mp_buf, (int)sizeof(mp_buf), lcn, run_clusters);
    if (mp_len < 0)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    name_bytes = (uint32_t)name_len * 2U;
    name_off = hdr;
    mp_off = pfs_ntfs_mkfs_align8(hdr + name_bytes);
    total = pfs_ntfs_mkfs_align8(mp_off + (uint32_t)mp_len) + (uint32_t)sizeof(uint32_t);
    append_off = pfs_ntfs_mkfs_mft_attr_append_off(p_mft);
    if (append_off + total > p_mft->size)
    {
        return PFS_NTFS_RET_ENOSPC;
    }
    p_a = (ntfs_attr_record_t *)(p_mft->buf + append_off);
    memset(p_a, 0, pfs_ntfs_mkfs_align8(mp_off + (uint32_t)mp_len));
    alloc_size = run_clusters * (uint64_t)cluster_size;
    p_a->type = type;
    p_a->non_resident = 1;
    p_a->name_length = name_len;
    p_a->name_offset = pfs_cpu_to_le16((uint16_t)name_off);
    p_a->flags = attr_flags;
    p_a->instance = pfs_cpu_to_le16(p_mft->next_instance++);
    p_a->data.non_resident.lowest_vcn = pfs_cpu_to_le64(0);
    p_a->data.non_resident.highest_vcn = pfs_cpu_to_le64(run_clusters - 1ULL);
    p_a->data.non_resident.mapping_pairs_offset = pfs_cpu_to_le16((uint16_t)mp_off);
    p_a->data.non_resident.compression_unit = 0;
    p_a->data.non_resident.allocated_size = pfs_cpu_to_le64(alloc_size);
    p_a->data.non_resident.data_size = pfs_cpu_to_le64(data_size);
    p_a->data.non_resident.initialized_size = pfs_cpu_to_le64(data_size);
    if (name_len > 0U && NULL != name)
    {
        memcpy((uint8_t *)p_a + name_off, name, name_bytes);
    }
    memcpy((uint8_t *)p_a + mp_off, mp_buf, (size_t)mp_len);
    p_a->length = pfs_cpu_to_le32(pfs_ntfs_mkfs_align8(mp_off + (uint32_t)mp_len));
    return PFS_NTFS_RET_OK;
}

// $BadClus/$Bad: mkntfs 同款 -- flags=0, 64 字节头, allocated=data=卷大小, init=0, runlist 全 hole
static int pfs_ntfs_mkfs_mft_add_nonres_hole(
        pfs_ntfs_mkfs_mft_t *p_mft,
        uint32_t type,
        const uint16_t *name,
        uint8_t name_len,
        uint64_t run_clusters,
        uint32_t cluster_size)
{
    ntfs_attr_record_t *p_a;
    uint8_t mp_buf[32];
    int mp_len;
    uint32_t append_off;
    uint32_t hdr = 64U;
    uint32_t name_bytes;
    uint32_t name_off;
    uint32_t mp_off;
    uint32_t total;
    uint64_t data_size;

    if (NULL == p_mft || 0ULL == run_clusters || 0U == cluster_size)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    mp_len = pfs_ntfs_mkfs_build_mapping_pairs_sparse(mp_buf, (int)sizeof(mp_buf), run_clusters);
    if (mp_len < 0)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    name_bytes = (uint32_t)name_len * 2U;
    name_off = hdr;
    mp_off = pfs_ntfs_mkfs_align8(hdr + name_bytes);
    total = pfs_ntfs_mkfs_align8(mp_off + (uint32_t)mp_len) + (uint32_t)sizeof(uint32_t);
    append_off = pfs_ntfs_mkfs_mft_attr_append_off(p_mft);
    if (append_off + total > p_mft->size)
    {
        return PFS_NTFS_RET_ENOSPC;
    }
    p_a = (ntfs_attr_record_t *)(p_mft->buf + append_off);
    memset(p_a, 0, pfs_ntfs_mkfs_align8(mp_off + (uint32_t)mp_len));
    data_size = run_clusters * (uint64_t)cluster_size;
    p_a->type = type;
    p_a->non_resident = 1;
    p_a->name_length = name_len;
    p_a->name_offset = pfs_cpu_to_le16((uint16_t)name_off);
    p_a->flags = pfs_cpu_to_le16(0);
    p_a->instance = pfs_cpu_to_le16(p_mft->next_instance++);
    p_a->data.non_resident.lowest_vcn = pfs_cpu_to_le64(0);
    p_a->data.non_resident.highest_vcn = pfs_cpu_to_le64(run_clusters - 1ULL);
    p_a->data.non_resident.mapping_pairs_offset = pfs_cpu_to_le16((uint16_t)mp_off);
    p_a->data.non_resident.compression_unit = 0;
    p_a->data.non_resident.allocated_size = pfs_cpu_to_le64(data_size);
    p_a->data.non_resident.data_size = pfs_cpu_to_le64(data_size);
    p_a->data.non_resident.initialized_size = pfs_cpu_to_le64(0);
    if (name_len > 0U && NULL != name)
    {
        memcpy((uint8_t *)p_a + name_off, name, name_bytes);
    }
    memcpy((uint8_t *)p_a + mp_off, mp_buf, (size_t)mp_len);
    p_a->length = pfs_cpu_to_le32(pfs_ntfs_mkfs_align8(mp_off + (uint32_t)mp_len));
    return PFS_NTFS_RET_OK;
}

static int pfs_ntfs_mkfs_mft_finalize(pfs_ntfs_mkfs_mft_t *p_mft);

// 未使用槽: 合法 FILE + 无 IN_USE + 仅 AT_END (勿留全 0)
static int pfs_ntfs_mkfs_init_free_mft_record(pfs_ntfs_mkfs_mft_t *p_mft, uint32_t rec_num)
{
    ntfs_mft_record_t *p_hdr;

    int rc = pfs_ntfs_mkfs_mft_init(p_mft, rec_num, 0);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    p_hdr = (ntfs_mft_record_t *)p_mft->buf;
    p_hdr->link_count = pfs_cpu_to_le16(0);
    return pfs_ntfs_mkfs_mft_finalize(p_mft);
}

static int pfs_ntfs_mkfs_mft_finalize(pfs_ntfs_mkfs_mft_t *p_mft)
{
    ntfs_mft_record_t *p_hdr;
    ntfs_attr_record_t *p_end;
    uint16_t *p_usa;
    uint16_t usn;
    uint32_t sectors;
    uint32_t used;
    uint32_t append_off;

    if (NULL == p_mft || NULL == p_mft->buf)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    append_off = pfs_ntfs_mkfs_mft_attr_append_off(p_mft);
    if (append_off + (uint32_t)sizeof(uint32_t) > p_mft->size)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    p_end = (ntfs_attr_record_t *)(p_mft->buf + append_off);
    p_end->type = NTFS_AT_END;
    p_end->length = 0;
    used = pfs_ntfs_mkfs_align8(append_off + (uint32_t)sizeof(uint32_t));
    p_hdr = (ntfs_mft_record_t *)p_mft->buf;
    p_hdr->bytes_in_use = pfs_cpu_to_le32(used);
    p_hdr->next_attr_instance = pfs_cpu_to_le16(p_mft->next_instance);
    sectors = p_mft->size / PFS_NTFS_MKFS_SECTOR_SIZE;
    p_usa = (uint16_t *)(p_mft->buf + 48);
    usn = (uint16_t)(rand() & 0xffff);
    if (0U == usn)
    {
        usn = 1;
    }
    p_usa[0] = pfs_cpu_to_le16(usn);
    for (uint32_t i = 0; i < sectors; i++)
    {
        uint32_t sec_end = (i + 1U) * PFS_NTFS_MKFS_SECTOR_SIZE;
        p_usa[i + 1U] = pfs_read_le16(p_mft->buf + sec_end - 2U);
        p_mft->buf[sec_end - 2U] = (uint8_t)(usn & 0xff);
        p_mft->buf[sec_end - 1U] = (uint8_t)((usn >> 8) & 0xff);
    }
    return PFS_NTFS_RET_OK;
}

static int pfs_ntfs_mkfs_add_std_info(
        pfs_ntfs_mkfs_mft_t *p_mft,
        uint32_t file_attrs,
        uint64_t now_le,
        uint32_t security_id)
{
    ntfs_standard_information_t si;
    uint32_t si_len;

    memset(&si, 0, sizeof(si));
    si.creation_time = now_le;
    si.last_data_change_time = now_le;
    si.last_mft_change_time = now_le;
    si.last_access_time = now_le;
    si.file_attributes = file_attrs;
    // security_id!=0: NTFS 3.x 用 72 字节 SI; 否则 48 字节 (mkntfs)
    if (0U != security_id)
    {
        si.ver.v3.security_id = pfs_cpu_to_le32(security_id);
        si_len = 72U;
    }
    else
    {
        si_len = 48U;
    }
    return pfs_ntfs_mkfs_mft_add_resident(
            p_mft,
            NTFS_AT_STANDARD_INFORMATION,
            NULL,
            0,
            &si,
            si_len,
            pfs_cpu_to_le16(0));
}

// 首桶两条 SDS (id 0x100/0x101); length=0x7C = SECURITY_HDR(0x14)+SD(0x68); 槽位 0x80
// id100: Owner/Group=Administrators; DACL=Administrators+Everyone Full(OI|CI) -- 资源管理器可访问
// id101: $Secure 元数据用 Administrators+SYSTEM Full(OI|CI)
static const uint8_t pfs_ntfs_mkfs_sds_id100[PFS_NTFS_MKFS_SDS_HDR_LEN] = {
    0x02, 0x84, 0xDA, 0xD7, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x7C, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x80, 0x48, 0x00, 0x00, 0x00, 0x58, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x02, 0x00, 0x34, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x00, 0x03, 0x18, 0x00, 0xFF, 0x01, 0x1F, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
    0x20, 0x00, 0x00, 0x00, 0x20, 0x02, 0x00, 0x00, 0x00, 0x03, 0x14, 0x00, 0xFF, 0x01, 0x1F, 0x00,
    0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05, 0x20, 0x00, 0x00, 0x00, 0x20, 0x02, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05, 0x20, 0x00, 0x00, 0x00, 0x20, 0x02, 0x00, 0x00,
};

static const uint8_t pfs_ntfs_mkfs_sds_id101[PFS_NTFS_MKFS_SDS_HDR_LEN] = {
    0x02, 0x84, 0xFA, 0xE9, 0x01, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x7C, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x80, 0x48, 0x00, 0x00, 0x00, 0x58, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x02, 0x00, 0x34, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x00, 0x03, 0x18, 0x00, 0xFF, 0x01, 0x1F, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
    0x20, 0x00, 0x00, 0x00, 0x20, 0x02, 0x00, 0x00, 0x00, 0x03, 0x14, 0x00, 0xFF, 0x01, 0x1F, 0x00,
    0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x12, 0x00, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05, 0x20, 0x00, 0x00, 0x00, 0x20, 0x02, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05, 0x20, 0x00, 0x00, 0x00, 0x20, 0x02, 0x00, 0x00,
};

// 驻留 $SECURITY_DESCRIPTOR = id100 的 SD 体 (无 SECURITY_HDR)
static const uint8_t pfs_ntfs_mkfs_default_sd_bytes[0x68] = {
    0x01, 0x00, 0x04, 0x80, 0x48, 0x00, 0x00, 0x00, 0x58, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x02, 0x00, 0x34, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x00, 0x03, 0x18, 0x00, 0xFF, 0x01, 0x1F, 0x00,
    0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x20, 0x00, 0x00, 0x00,
    0x20, 0x02, 0x00, 0x00, 0x00, 0x03, 0x14, 0x00, 0xFF, 0x01, 0x1F, 0x00,
    0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x20, 0x00, 0x00, 0x00,
    0x20, 0x02, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
    0x20, 0x00, 0x00, 0x00, 0x20, 0x02, 0x00, 0x00,
};

typedef struct _pfs_ntfs_mkfs_sds_ref_t {
    uint32_t hash;
    uint32_t security_id;
    uint64_t offset;
    uint32_t length;
} pfs_ntfs_mkfs_sds_ref_t;

static uint32_t pfs_ntfs_mkfs_build_default_sd(uint8_t *dst, uint32_t cap)
{
    if (NULL == dst || cap < (uint32_t)sizeof(pfs_ntfs_mkfs_default_sd_bytes))
    {
        return 0U;
    }
    memcpy(dst, pfs_ntfs_mkfs_default_sd_bytes, sizeof(pfs_ntfs_mkfs_default_sd_bytes));
    return (uint32_t)sizeof(pfs_ntfs_mkfs_default_sd_bytes);
}

static void pfs_ntfs_mkfs_sds_refs(pfs_ntfs_mkfs_sds_ref_t *refs, uint32_t count)
{
    if (NULL == refs || count < PFS_NTFS_MKFS_SDS_ENTRY_COUNT)
    {
        return;
    }
    refs[0].hash = pfs_read_le32(pfs_ntfs_mkfs_sds_id100);
    refs[0].security_id = PFS_NTFS_MKFS_SECURITY_ID;
    refs[0].offset = 0ULL;
    refs[0].length = PFS_NTFS_MKFS_SDS_HDR_LEN;
    refs[1].hash = pfs_read_le32(pfs_ntfs_mkfs_sds_id101);
    refs[1].security_id = PFS_NTFS_MKFS_SECURITY_ID_META;
    refs[1].offset = PFS_NTFS_MKFS_SDS_SLOT;
    refs[1].length = PFS_NTFS_MKFS_SDS_HDR_LEN;
}

static int pfs_ntfs_mkfs_build_sds(uint8_t **pp_buf, uint32_t *p_bytes)
{
    uint32_t total;
    uint8_t *p_buf;

    if (NULL == pp_buf || NULL == p_bytes)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    total = PFS_NTFS_MKFS_SDS_MIRROR_OFF + PFS_NTFS_MKFS_SDS_FIRST_BUCKET;
    p_buf = (uint8_t *)calloc(1, total);
    if (NULL == p_buf)
    {
        return PFS_NTFS_RET_ENOMEM;
    }
    memcpy(p_buf + 0, pfs_ntfs_mkfs_sds_id100, sizeof(pfs_ntfs_mkfs_sds_id100));
    memcpy(p_buf + PFS_NTFS_MKFS_SDS_SLOT, pfs_ntfs_mkfs_sds_id101, sizeof(pfs_ntfs_mkfs_sds_id101));
    // 0x40000 镜像与主副本逐字节相同 (含各自 offset)
    memcpy(p_buf + PFS_NTFS_MKFS_SDS_MIRROR_OFF, p_buf, PFS_NTFS_MKFS_SDS_FIRST_BUCKET);
    *pp_buf = p_buf;
    *p_bytes = total;
    return PFS_NTFS_RET_OK;
}

static int pfs_ntfs_mkfs_add_file_name(
        pfs_ntfs_mkfs_mft_t *p_mft,
        uint64_t parent_le,
        const char *name,
        uint32_t file_attrs,
        uint64_t now_le,
        uint64_t alloc_size,
        uint64_t data_size,
        uint8_t name_type,
        int is_dir)
{
    uint16_t uname[256];
    uint8_t name_len = 0;
    uint8_t buf[512];
    ntfs_file_name_attr_t *p_fn;
    uint32_t fn_size;

    pfs_ntfs_mkfs_ascii_to_utf16le(uname, name, &name_len);
    fn_size = (uint32_t)(sizeof(ntfs_file_name_attr_t) + (uint32_t)name_len * 2U);
    if (fn_size > sizeof(buf))
    {
        return PFS_NTFS_RET_EINVAL;
    }
    memset(buf, 0, sizeof(buf));
    p_fn = (ntfs_file_name_attr_t *)buf;
    p_fn->parent_directory = parent_le;
    p_fn->creation_time = now_le;
    p_fn->last_data_change_time = now_le;
    p_fn->last_mft_change_time = now_le;
    p_fn->last_access_time = now_le;
    p_fn->allocated_size = pfs_cpu_to_le64(alloc_size);
    p_fn->data_size = pfs_cpu_to_le64(data_size);
    p_fn->file_attributes = file_attrs;
    p_fn->file_name_length = name_len;
    p_fn->file_name_type = name_type;
    memcpy(p_fn->file_name, uname, (size_t)name_len * 2U);
    (void)is_dir;
    return pfs_ntfs_mkfs_mft_add_resident(
            p_mft,
            NTFS_AT_FILE_NAME,
            NULL,
            0,
            buf,
            fn_size,
            pfs_cpu_to_le16(0));
}

static int pfs_ntfs_mkfs_add_index_root_empty(pfs_ntfs_mkfs_mft_t *p_mft, uint32_t cluster_size)
{
    uint16_t iname[4];
    uint8_t buf[256];
    ntfs_index_root_t *p_root;
    ntfs_index_entry_header_t *p_ent;
    uint32_t val_len;

    pfs_ntfs_mkfs_utf16_name4(iname, '$', 'I', '3', '0');
    memset(buf, 0, sizeof(buf));
    p_root = (ntfs_index_root_t *)buf;
    p_root->type = NTFS_AT_FILE_NAME;
    p_root->collation_rule = NTFS_COLLATION_FILE_NAME;
    // 空目录 SMALL_INDEX; index_block_size 对齐常见 4096
    p_root->index_block_size = pfs_cpu_to_le32(PFS_NTFS_MKFS_INDEX_BLOCK_SIZE);
    p_root->clusters_per_index_block = pfs_ntfs_mkfs_index_root_clusters_per_block(
            PFS_NTFS_MKFS_INDEX_BLOCK_SIZE, cluster_size);
    p_root->index.entries_offset = pfs_cpu_to_le32((uint32_t)sizeof(ntfs_index_header_t));
    // index_length / allocated_size 相对 index header 起点 (含 entries_offset 之前的头)
    p_root->index.index_length = pfs_cpu_to_le32(
            (uint32_t)(sizeof(ntfs_index_header_t) + sizeof(ntfs_index_entry_header_t)));
    p_root->index.allocated_size = p_root->index.index_length;
    p_root->index.flags = (uint8_t)NTFS_SMALL_INDEX;
    p_ent = (ntfs_index_entry_header_t *)((uint8_t *)p_root + sizeof(ntfs_index_root_t));
    p_ent->length = pfs_cpu_to_le16((uint16_t)sizeof(ntfs_index_entry_header_t));
    p_ent->key_length = 0;
    p_ent->flags = NTFS_INDEX_ENTRY_END;
    p_ent->reserved = 0;
    val_len = (uint32_t)(sizeof(ntfs_index_root_t) + sizeof(ntfs_index_entry_header_t));
    return pfs_ntfs_mkfs_mft_add_resident(
            p_mft,
            NTFS_AT_INDEX_ROOT,
            iname,
            4,
            buf,
            val_len,
            pfs_cpu_to_le16(0));
}

static int pfs_ntfs_mkfs_add_index_root_large_empty(pfs_ntfs_mkfs_mft_t *p_mft, uint32_t cluster_size)
{
    uint16_t iname[4];
    uint8_t buf[256];
    ntfs_index_root_t *p_root;
    ntfs_index_entry_header_t *p_ent;
    uint32_t ent_len;
    uint32_t val_len;
    uint64_t *p_vcn;

    // LARGE 根: NODE|END + 尾部 VCN0, 指向 INDEX_ALLOCATION 首块
    pfs_ntfs_mkfs_utf16_name4(iname, '$', 'I', '3', '0');
    memset(buf, 0, sizeof(buf));
    p_root = (ntfs_index_root_t *)buf;
    p_root->type = NTFS_AT_FILE_NAME;
    p_root->collation_rule = NTFS_COLLATION_FILE_NAME;
    p_root->index_block_size = pfs_cpu_to_le32(PFS_NTFS_MKFS_INDEX_BLOCK_SIZE);
    p_root->clusters_per_index_block = pfs_ntfs_mkfs_index_root_clusters_per_block(
            PFS_NTFS_MKFS_INDEX_BLOCK_SIZE, cluster_size);
    p_root->index.entries_offset = pfs_cpu_to_le32((uint32_t)sizeof(ntfs_index_header_t));
    ent_len = (uint32_t)(sizeof(ntfs_index_entry_header_t) + sizeof(uint64_t));
    p_root->index.index_length = pfs_cpu_to_le32(
            (uint32_t)sizeof(ntfs_index_header_t) + ent_len);
    p_root->index.allocated_size = p_root->index.index_length;
    p_root->index.flags = (uint8_t)NTFS_LARGE_INDEX;
    p_ent = (ntfs_index_entry_header_t *)((uint8_t *)p_root + sizeof(ntfs_index_root_t));
    p_ent->length = pfs_cpu_to_le16((uint16_t)ent_len);
    p_ent->key_length = 0;
    p_ent->flags = (uint16_t)(NTFS_INDEX_ENTRY_NODE | NTFS_INDEX_ENTRY_END);
    p_ent->reserved = 0;
    p_vcn = (uint64_t *)((uint8_t *)p_ent + ent_len - sizeof(uint64_t));
    *p_vcn = pfs_cpu_to_le64(0ULL);
    val_len = (uint32_t)(sizeof(ntfs_index_root_t) + ent_len);
    return pfs_ntfs_mkfs_mft_add_resident(
            p_mft,
            NTFS_AT_INDEX_ROOT,
            iname,
            4,
            buf,
            val_len,
            pfs_cpu_to_le16(0));
}

static int pfs_ntfs_mkfs_add_index_root_view_empty(
        pfs_ntfs_mkfs_mft_t *p_mft,
        const uint16_t *name,
        uint8_t name_len,
        uint32_t collation,
        uint32_t cluster_size)
{
    uint8_t buf[256];
    ntfs_index_root_t *p_root;
    ntfs_index_entry_header_t *p_ent;
    uint32_t val_len;

    memset(buf, 0, sizeof(buf));
    p_root = (ntfs_index_root_t *)buf;
    p_root->type = pfs_cpu_to_le32(0);
    p_root->collation_rule = collation;
    p_root->index_block_size = pfs_cpu_to_le32(PFS_NTFS_MKFS_INDEX_BLOCK_SIZE);
    p_root->clusters_per_index_block = pfs_ntfs_mkfs_index_root_clusters_per_block(
            PFS_NTFS_MKFS_INDEX_BLOCK_SIZE, cluster_size);
    p_root->index.entries_offset = pfs_cpu_to_le32((uint32_t)sizeof(ntfs_index_header_t));
    p_root->index.index_length = pfs_cpu_to_le32(
            (uint32_t)(sizeof(ntfs_index_header_t) + sizeof(ntfs_index_entry_header_t)));
    p_root->index.allocated_size = p_root->index.index_length;
    p_root->index.flags = (uint8_t)NTFS_SMALL_INDEX;
    p_ent = (ntfs_index_entry_header_t *)((uint8_t *)p_root + sizeof(ntfs_index_root_t));
    p_ent->length = pfs_cpu_to_le16((uint16_t)sizeof(ntfs_index_entry_header_t));
    p_ent->key_length = 0;
    p_ent->flags = NTFS_INDEX_ENTRY_END;
    p_ent->reserved = 0;
    val_len = (uint32_t)(sizeof(ntfs_index_root_t) + sizeof(ntfs_index_entry_header_t));
    return pfs_ntfs_mkfs_mft_add_resident(
            p_mft,
            NTFS_AT_INDEX_ROOT,
            name,
            name_len,
            buf,
            val_len,
            pfs_cpu_to_le16(0));
}

static int pfs_ntfs_mkfs_add_index_root_view_large_empty(
        pfs_ntfs_mkfs_mft_t *p_mft,
        const uint16_t *name,
        uint8_t name_len,
        uint32_t collation,
        uint32_t cluster_size)
{
    uint8_t buf[256];
    ntfs_index_root_t *p_root;
    ntfs_index_entry_header_t *p_ent;
    uint32_t ent_len;
    uint32_t val_len;
    uint64_t *p_vcn;

    // Windows $Secure: LARGE 空根 + NODE|END VCN0, 条目在 INDEX_ALLOCATION
    memset(buf, 0, sizeof(buf));
    p_root = (ntfs_index_root_t *)buf;
    p_root->type = pfs_cpu_to_le32(0);
    p_root->collation_rule = collation;
    p_root->index_block_size = pfs_cpu_to_le32(PFS_NTFS_MKFS_INDEX_BLOCK_SIZE);
    p_root->clusters_per_index_block = pfs_ntfs_mkfs_index_root_clusters_per_block(
            PFS_NTFS_MKFS_INDEX_BLOCK_SIZE, cluster_size);
    p_root->index.entries_offset = pfs_cpu_to_le32((uint32_t)sizeof(ntfs_index_header_t));
    ent_len = (uint32_t)(sizeof(ntfs_index_entry_header_t) + sizeof(uint64_t));
    p_root->index.index_length = pfs_cpu_to_le32(
            (uint32_t)sizeof(ntfs_index_header_t) + ent_len);
    p_root->index.allocated_size = p_root->index.index_length;
    p_root->index.flags = (uint8_t)NTFS_LARGE_INDEX;
    p_ent = (ntfs_index_entry_header_t *)((uint8_t *)p_root + sizeof(ntfs_index_root_t));
    p_ent->length = pfs_cpu_to_le16((uint16_t)ent_len);
    p_ent->key_length = 0;
    p_ent->flags = (uint16_t)(NTFS_INDEX_ENTRY_NODE | NTFS_INDEX_ENTRY_END);
    p_ent->reserved = 0;
    p_vcn = (uint64_t *)((uint8_t *)p_ent + ent_len - sizeof(uint64_t));
    *p_vcn = pfs_cpu_to_le64(0ULL);
    val_len = (uint32_t)(sizeof(ntfs_index_root_t) + ent_len);
    return pfs_ntfs_mkfs_mft_add_resident(
            p_mft,
            NTFS_AT_INDEX_ROOT,
            name,
            name_len,
            buf,
            val_len,
            pfs_cpu_to_le16(0));
}

static int pfs_ntfs_mkfs_build_secure_indx(
        uint8_t *buf,
        uint32_t buf_size,
        int is_sdh,
        const pfs_ntfs_mkfs_sds_ref_t *refs,
        uint32_t ref_count)
{
    ntfs_index_block_t *p_ib;
    ntfs_index_header_t *p_ih;
    ntfs_index_entry_header_t *p_ent;
    ntfs_index_entry_header_t *p_end;
    pfs_ntfs_mkfs_sds_hdr_t *p_data;
    uint16_t usa_count;
    uint32_t entries_off;
    uint32_t ih_base;
    uint32_t key_len;
    uint32_t data_len;
    uint32_t data_off;
    uint32_t ent_len;
    uint32_t ent_off;
    uint8_t *p_key;

    if (NULL == buf || buf_size < PFS_NTFS_MKFS_INDEX_BLOCK_SIZE || NULL == refs || 0U == ref_count)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    key_len = is_sdh ? 8U : 4U;
    data_len = (uint32_t)sizeof(pfs_ntfs_mkfs_sds_hdr_t);
    data_off = (uint32_t)sizeof(ntfs_index_entry_header_t) + key_len;
    ent_len = pfs_ntfs_mkfs_align8(data_off + data_len + (is_sdh ? (uint32_t)sizeof(uint32_t) : 0U));
    memset(buf, 0, buf_size);
    p_ib = (ntfs_index_block_t *)buf;
    usa_count = (uint16_t)(1U + buf_size / PFS_NTFS_MKFS_SECTOR_SIZE);
    entries_off = (uint32_t)((sizeof(ntfs_index_header_t) + (uint32_t)usa_count * 2U + 7U) & ~7U);
    ih_base = (uint32_t)(sizeof(ntfs_index_block_t) - sizeof(ntfs_index_header_t));
    p_ib->magic = NTFS_MAGIC_INDX;
    p_ib->usa_ofs = pfs_cpu_to_le16((uint16_t)sizeof(ntfs_index_block_t));
    p_ib->usa_count = pfs_cpu_to_le16(usa_count);
    p_ib->lsn = pfs_cpu_to_le64(0);
    p_ib->index_block_vcn = pfs_cpu_to_le64(0);
    p_ih = &p_ib->index;
    p_ih->entries_offset = pfs_cpu_to_le32(entries_off);
    p_ih->allocated_size = pfs_cpu_to_le32(
            buf_size - (uint32_t)(sizeof(ntfs_index_block_t) - sizeof(ntfs_index_header_t)));
    p_ih->flags = (uint8_t)NTFS_LEAF_NODE;
    ent_off = ih_base + entries_off;
    for (uint32_t i = 0; i < ref_count; i++)
    {
        if (ent_off + ent_len + (uint32_t)sizeof(ntfs_index_entry_header_t) > buf_size)
        {
            return PFS_NTFS_RET_ENOSPC;
        }
        p_ent = (ntfs_index_entry_header_t *)(buf + ent_off);
        memset(p_ent, 0, ent_len);
        p_ent->length = pfs_cpu_to_le16((uint16_t)ent_len);
        p_ent->key_length = pfs_cpu_to_le16((uint16_t)key_len);
        p_ent->flags = 0;
        p_ent->data.vi.data_offset = pfs_cpu_to_le16((uint16_t)data_off);
        p_ent->data.vi.data_length = pfs_cpu_to_le16((uint16_t)data_len);
        p_key = (uint8_t *)p_ent + sizeof(ntfs_index_entry_header_t);
        if (is_sdh)
        {
            ((uint32_t *)p_key)[0] = pfs_cpu_to_le32(refs[i].hash);
            ((uint32_t *)p_key)[1] = pfs_cpu_to_le32(refs[i].security_id);
        }
        else
        {
            ((uint32_t *)p_key)[0] = pfs_cpu_to_le32(refs[i].security_id);
        }
        p_data = (pfs_ntfs_mkfs_sds_hdr_t *)((uint8_t *)p_ent + data_off);
        p_data->hash = pfs_cpu_to_le32(refs[i].hash);
        p_data->security_id = pfs_cpu_to_le32(refs[i].security_id);
        p_data->offset = pfs_cpu_to_le64(refs[i].offset);
        p_data->length = pfs_cpu_to_le32(refs[i].length);
        if (is_sdh)
        {
            ((uint32_t *)(p_data + 1))[0] = pfs_cpu_to_le32(0x00490049U);
        }
        ent_off += ent_len;
    }
    p_end = (ntfs_index_entry_header_t *)(buf + ent_off);
    memset(p_end, 0, sizeof(*p_end));
    p_end->length = pfs_cpu_to_le16((uint16_t)sizeof(ntfs_index_entry_header_t));
    p_end->flags = NTFS_INDEX_ENTRY_END;
    ent_off += (uint32_t)sizeof(ntfs_index_entry_header_t);
    p_ih->index_length = pfs_cpu_to_le32(ent_off - ih_base);
    pfs_ntfs_mkfs_apply_usa(buf, buf_size, (uint16_t)sizeof(ntfs_index_block_t));
    return PFS_NTFS_RET_OK;
}

static int pfs_ntfs_mkfs_indx_append_file_name_entry(
        uint8_t *buf,
        uint32_t buf_size,
        uint32_t *p_off,
        const char *name,
        uint32_t rec_num,
        uint32_t file_attrs,
        uint64_t parent_le,
        uint64_t now_le,
        uint64_t alloc_size,
        uint64_t data_size,
        uint8_t name_type)
{
    uint16_t uname[256];
    uint8_t name_len = 0;
    uint8_t key_buf[512];
    ntfs_file_name_attr_t *p_fn;
    ntfs_index_entry_header_t *p_ie;
    uint32_t key_len;
    uint32_t ent_len;
    uint32_t off;

    if (NULL == buf || NULL == p_off)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    off = *p_off;
    pfs_ntfs_mkfs_ascii_to_utf16le(uname, name, &name_len);
    key_len = (uint32_t)(sizeof(ntfs_file_name_attr_t) + (uint32_t)name_len * 2U);
    if (key_len > sizeof(key_buf))
    {
        return PFS_NTFS_RET_EINVAL;
    }
    ent_len = pfs_ntfs_mkfs_align8((uint32_t)sizeof(ntfs_index_entry_header_t) + key_len);
    if (off + ent_len + (uint32_t)sizeof(ntfs_index_entry_header_t) > buf_size)
    {
        return PFS_NTFS_RET_ENOSPC;
    }
    memset(key_buf, 0, sizeof(key_buf));
    p_fn = (ntfs_file_name_attr_t *)key_buf;
    p_fn->parent_directory = parent_le;
    p_fn->creation_time = now_le;
    p_fn->last_data_change_time = now_le;
    p_fn->last_mft_change_time = now_le;
    p_fn->last_access_time = now_le;
    p_fn->allocated_size = pfs_cpu_to_le64(alloc_size);
    p_fn->data_size = pfs_cpu_to_le64(data_size);
    p_fn->file_attributes = file_attrs;
    p_fn->file_name_length = name_len;
    p_fn->file_name_type = name_type;
    memcpy(p_fn->file_name, uname, (size_t)name_len * 2U);
    p_ie = (ntfs_index_entry_header_t *)(buf + off);
    memset(p_ie, 0, ent_len);
    p_ie->data.dir.indexed_file = NTFS_MK_LE_MREF(rec_num, pfs_ntfs_mkfs_seq_for_rec(rec_num));
    p_ie->length = pfs_cpu_to_le16((uint16_t)ent_len);
    p_ie->key_length = pfs_cpu_to_le16((uint16_t)key_len);
    p_ie->flags = 0;
    p_ie->reserved = 0;
    memcpy((uint8_t *)p_ie + sizeof(ntfs_index_entry_header_t), key_buf, key_len);
    *p_off = off + ent_len;
    return PFS_NTFS_RET_OK;
}

static int pfs_ntfs_mkfs_indx_append_end(uint8_t *buf, uint32_t buf_size, uint32_t *p_off)
{
    ntfs_index_entry_header_t *p_ie;
    uint32_t off;

    if (NULL == buf || NULL == p_off)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    off = *p_off;
    if (off + (uint32_t)sizeof(ntfs_index_entry_header_t) > buf_size)
    {
        return PFS_NTFS_RET_ENOSPC;
    }
    p_ie = (ntfs_index_entry_header_t *)(buf + off);
    memset(p_ie, 0, sizeof(*p_ie));
    p_ie->length = pfs_cpu_to_le16((uint16_t)sizeof(ntfs_index_entry_header_t));
    p_ie->key_length = 0;
    p_ie->flags = NTFS_INDEX_ENTRY_END;
    p_ie->reserved = 0;
    *p_off = off + (uint32_t)sizeof(ntfs_index_entry_header_t);
    return PFS_NTFS_RET_OK;
}

static int pfs_ntfs_mkfs_build_root_indx(
        uint8_t *buf,
        uint32_t buf_size,
        uint64_t parent_le,
        uint64_t now_le,
        const pfs_ntfs_mkfs_layout_t *p_lo,
        uint32_t cluster_size)
{
    ntfs_index_block_t *p_ib;
    ntfs_index_header_t *p_ih;
    uint16_t usa_count;
    uint32_t ih_base;
    uint32_t entries_off;
    uint32_t ent_off;
    uint32_t index_len;

    if (NULL == buf || buf_size < PFS_NTFS_MKFS_INDEX_BLOCK_SIZE || NULL == p_lo)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    {
    const struct {
        const char *name;
        uint32_t rec_num;
        uint32_t file_attrs;
        uint64_t data_size;
    } root_entries[] = {
        { "$AttrDef", NTFS_FILE_AttrDef, NTFS_FILE_ATTR_HIDDEN | NTFS_FILE_ATTR_SYSTEM, 0 },
        { "$BadClus", NTFS_FILE_BadClus, NTFS_FILE_ATTR_HIDDEN | NTFS_FILE_ATTR_SYSTEM, 0 },
        { "$Bitmap", NTFS_FILE_Bitmap, NTFS_FILE_ATTR_HIDDEN | NTFS_FILE_ATTR_SYSTEM, p_lo->bitmap_bytes },
        { "$Boot", NTFS_FILE_Boot, NTFS_FILE_ATTR_HIDDEN | NTFS_FILE_ATTR_SYSTEM,
          (uint64_t)PFS_NTFS_MKFS_BOOT_SECTORS * PFS_NTFS_MKFS_SECTOR_SIZE },
        { "$Extend", NTFS_FILE_Extend, PFS_NTFS_MKFS_DIR_FILE_ATTRS, 0 },
        { "$LogFile", NTFS_FILE_LogFile, NTFS_FILE_ATTR_HIDDEN | NTFS_FILE_ATTR_SYSTEM, 0 },
        { "$MFT", NTFS_FILE_MFT, NTFS_FILE_ATTR_HIDDEN | NTFS_FILE_ATTR_SYSTEM,
          (uint64_t)p_lo->mft_record_count * PFS_NTFS_MKFS_MFT_RECORD_SIZE },
        { "$MFTMirr", NTFS_FILE_MFTMirr, NTFS_FILE_ATTR_HIDDEN | NTFS_FILE_ATTR_SYSTEM,
          (uint64_t)p_lo->mftmirr_bytes },
        // 与 MFT #9 SI/FN 一致: VIEW_INDEX 复制位 0x20000000
        { "$Secure", NTFS_FILE_Secure,
          NTFS_FILE_ATTR_HIDDEN | NTFS_FILE_ATTR_SYSTEM | NTFS_FILE_ATTR_DUP_VIEW_INDEX_PRESENT, 0 },
        { "$UpCase", NTFS_FILE_UpCase, NTFS_FILE_ATTR_HIDDEN | NTFS_FILE_ATTR_SYSTEM,
          (uint64_t)PFS_NTFS_MKFS_UPCASE_CHARS * 2U },
        { "$Volume", NTFS_FILE_Volume, NTFS_FILE_ATTR_HIDDEN | NTFS_FILE_ATTR_SYSTEM, 0 },
        // Windows 根目录仅有 "." (与 MFT #5 $FILE_NAME 对应); 无 FAT 式 ".."
        { ".", NTFS_FILE_root, PFS_NTFS_MKFS_DIR_FILE_ATTRS, 0 },
    };

    memset(buf, 0, buf_size);
    p_ib = (ntfs_index_block_t *)buf;
    usa_count = (uint16_t)(1U + buf_size / PFS_NTFS_MKFS_SECTOR_SIZE);
    entries_off = (uint32_t)((sizeof(ntfs_index_header_t) + (uint32_t)usa_count * 2U + 7U) & ~7U);
    ih_base = (uint32_t)(sizeof(ntfs_index_block_t) - sizeof(ntfs_index_header_t));
    p_ib->magic = NTFS_MAGIC_INDX;
    p_ib->usa_ofs = pfs_cpu_to_le16((uint16_t)sizeof(ntfs_index_block_t));
    p_ib->usa_count = pfs_cpu_to_le16(usa_count);
    p_ib->lsn = pfs_cpu_to_le64(0);
    p_ib->index_block_vcn = pfs_cpu_to_le64(0);
    p_ih = &p_ib->index;
    p_ih->entries_offset = pfs_cpu_to_le32(entries_off);
    p_ih->allocated_size = pfs_cpu_to_le32(
            buf_size - (uint32_t)(sizeof(ntfs_index_block_t) - sizeof(ntfs_index_header_t)));
    p_ih->flags = (uint8_t)NTFS_LEAF_NODE;
    ent_off = ih_base + entries_off;
    int rc;
    for (uint32_t i = 0; i < (uint32_t)(sizeof(root_entries) / sizeof(root_entries[0])); i++)
    {
        uint64_t alloc = root_entries[i].data_size;
        uint64_t data_sz = root_entries[i].data_size;
        // 系统短名与 . / .. : Windows 要求 WIN32_AND_DOS
        uint8_t name_type = NTFS_FILE_NAME_WIN32_AND_DOS;

        if (root_entries[i].rec_num == NTFS_FILE_Bitmap)
        {
            alloc = (uint64_t)p_lo->bitmap_clusters * (uint64_t)cluster_size;
        }
        else if (root_entries[i].rec_num == NTFS_FILE_MFT)
        {
            alloc = (uint64_t)p_lo->mft_clusters * (uint64_t)cluster_size;
        }
        else if (root_entries[i].rec_num == NTFS_FILE_MFTMirr)
        {
            alloc = (uint64_t)p_lo->mftmirr_clusters * (uint64_t)cluster_size;
        }
        else if (root_entries[i].rec_num == NTFS_FILE_LogFile)
        {
            data_sz = p_lo->logfile_bytes;
            alloc = (uint64_t)p_lo->logfile_clusters * (uint64_t)cluster_size;
        }
        else if (root_entries[i].rec_num == NTFS_FILE_Boot)
        {
            alloc = (uint64_t)p_lo->boot_clusters * (uint64_t)cluster_size;
        }
        else if (root_entries[i].rec_num == NTFS_FILE_UpCase)
        {
            alloc = (uint64_t)p_lo->upcase_clusters * (uint64_t)cluster_size;
        }
        else if (root_entries[i].rec_num == NTFS_FILE_AttrDef)
        {
            data_sz = p_lo->attrdef_bytes;
            alloc = (uint64_t)p_lo->attrdef_clusters * (uint64_t)cluster_size;
        }
        rc = pfs_ntfs_mkfs_indx_append_file_name_entry(
                buf,
                buf_size,
                &ent_off,
                root_entries[i].name,
                root_entries[i].rec_num,
                root_entries[i].file_attrs,
                parent_le,
                now_le,
                alloc,
                data_sz,
                name_type);
        if (PFS_NTFS_RET_OK != rc)
        {
            return rc;
        }
    }
    rc = pfs_ntfs_mkfs_indx_append_end(buf, buf_size, &ent_off);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    // index_length 相对 index header 起点 (= entries_offset + 条目总长)
    index_len = ent_off - ih_base;
    p_ih->index_length = pfs_cpu_to_le32(index_len);
    pfs_ntfs_mkfs_apply_usa(buf, buf_size, (uint16_t)sizeof(ntfs_index_block_t));
    }
    return PFS_NTFS_RET_OK;
}

static int pfs_ntfs_mkfs_build_root_record(
        pfs_ntfs_mkfs_mft_t *p_mft,
        const pfs_ntfs_mkfs_layout_t *p_lo,
        uint32_t cluster_size,
        uint64_t parent_le,
        uint64_t now_le)
{
    uint16_t i30[4];
    uint8_t ibm[8];
    uint8_t sd[128];
    uint32_t sd_len;
    ntfs_mft_record_t *p_hdr;

    int rc = pfs_ntfs_mkfs_mft_init(
            p_mft,
            NTFS_FILE_root,
            (uint16_t)(NTFS_MFT_RECORD_IN_USE | NTFS_MFT_RECORD_IS_DIRECTORY));
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    // mkntfs: root security_id=0 + 驻留 $SECURITY_DESCRIPTOR
    rc = pfs_ntfs_mkfs_add_std_info(p_mft, PFS_NTFS_MKFS_DIR_FILE_ATTRS, now_le, 0);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_ntfs_mkfs_add_file_name(
            p_mft,
            parent_le,
            ".",
            PFS_NTFS_MKFS_DIR_FILE_ATTRS,
            now_le,
            0,
            0,
            NTFS_FILE_NAME_WIN32_AND_DOS,
            1);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    sd_len = pfs_ntfs_mkfs_build_default_sd(sd, (uint32_t)sizeof(sd));
    if (0U == sd_len)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    rc = pfs_ntfs_mkfs_mft_add_resident(
            p_mft, NTFS_AT_SECURITY_DESCRIPTOR, NULL, 0, sd, sd_len, pfs_cpu_to_le16(0));
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_ntfs_mkfs_add_index_root_large_empty(p_mft, cluster_size);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    pfs_ntfs_mkfs_utf16_name4(i30, '$', 'I', '3', '0');
    rc = pfs_ntfs_mkfs_mft_add_nonres(
            p_mft,
            NTFS_AT_INDEX_ALLOCATION,
            i30,
            4,
            p_lo->root_idx_lcn,
            p_lo->root_idx_clusters,
            PFS_NTFS_MKFS_INDEX_BLOCK_SIZE,
            cluster_size,
            pfs_cpu_to_le16(0));
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    memset(ibm, 0, sizeof(ibm));
    ibm[0] = 0x01;
    // mkntfs/Linux: $I30 $BITMAP 至少 8 字节
    rc = pfs_ntfs_mkfs_mft_add_resident(
            p_mft, NTFS_AT_BITMAP, i30, 4, ibm, (uint32_t)sizeof(ibm), pfs_cpu_to_le16(0));
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    p_hdr = (ntfs_mft_record_t *)p_mft->buf;
    // 仅 1 个 $FILE_NAME (.); .. 只在索引中, 不增加 link_count
    p_hdr->link_count = pfs_cpu_to_le16(1);
    return pfs_ntfs_mkfs_mft_finalize(p_mft);
}

static int pfs_ntfs_mkfs_build_secure_record(
        pfs_ntfs_mkfs_mft_t *p_mft,
        const pfs_ntfs_mkfs_layout_t *p_lo,
        uint64_t parent_le,
        uint64_t now_le,
        uint32_t cluster_size)
{
    uint16_t sds[4];
    uint16_t sii[4];
    uint16_t sdh[4];
    uint8_t ibm[8];
    uint64_t ia_bytes;

    int rc = pfs_ntfs_mkfs_mft_init(
            p_mft,
            NTFS_FILE_Secure,
            (uint16_t)(NTFS_MFT_RECORD_IN_USE | NTFS_MFT_RECORD_IS_VIEW_INDEX));
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    // Windows: $Secure 自身用 security_id 0x101
    rc = pfs_ntfs_mkfs_add_std_info(
            p_mft,
            NTFS_FILE_ATTR_HIDDEN | NTFS_FILE_ATTR_SYSTEM | NTFS_FILE_ATTR_DUP_VIEW_INDEX_PRESENT,
            now_le,
            PFS_NTFS_MKFS_SECURITY_ID_META);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_ntfs_mkfs_add_file_name(
            p_mft,
            parent_le,
            "$Secure",
            NTFS_FILE_ATTR_HIDDEN | NTFS_FILE_ATTR_SYSTEM | NTFS_FILE_ATTR_DUP_VIEW_INDEX_PRESENT,
            now_le,
            0,
            0,
            NTFS_FILE_NAME_WIN32_AND_DOS,
            0);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    pfs_ntfs_mkfs_utf16_name4(sds, '$', 'S', 'D', 'S');
    rc = pfs_ntfs_mkfs_mft_add_nonres(
            p_mft,
            NTFS_AT_DATA,
            sds,
            4,
            p_lo->sds_lcn,
            p_lo->sds_clusters,
            p_lo->sds_bytes,
            cluster_size,
            pfs_cpu_to_le16(0));
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    // Windows Format: LARGE 空根 + INDEX_ALLOCATION + $BITMAP; $SDH 在 $SII 前
    memset(ibm, 0, sizeof(ibm));
    ibm[0] = 0x01;
    ia_bytes = (uint64_t)p_lo->secure_idx_clusters * (uint64_t)cluster_size;
    pfs_ntfs_mkfs_utf16_name4(sdh, '$', 'S', 'D', 'H');
    rc = pfs_ntfs_mkfs_add_index_root_view_large_empty(
            p_mft, sdh, 4, NTFS_COLLATION_NTOFS_SECURITY_HASH, cluster_size);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    pfs_ntfs_mkfs_utf16_name4(sii, '$', 'S', 'I', 'I');
    rc = pfs_ntfs_mkfs_add_index_root_view_large_empty(
            p_mft, sii, 4, NTFS_COLLATION_NTOFS_ULONG, cluster_size);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_ntfs_mkfs_mft_add_nonres(
            p_mft,
            NTFS_AT_INDEX_ALLOCATION,
            sdh,
            4,
            p_lo->sdh_idx_lcn,
            p_lo->secure_idx_clusters,
            ia_bytes,
            cluster_size,
            pfs_cpu_to_le16(0));
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_ntfs_mkfs_mft_add_nonres(
            p_mft,
            NTFS_AT_INDEX_ALLOCATION,
            sii,
            4,
            p_lo->sii_idx_lcn,
            p_lo->secure_idx_clusters,
            ia_bytes,
            cluster_size,
            pfs_cpu_to_le16(0));
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_ntfs_mkfs_mft_add_resident(
            p_mft, NTFS_AT_BITMAP, sdh, 4, ibm, (uint32_t)sizeof(ibm), pfs_cpu_to_le16(0));
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_ntfs_mkfs_mft_add_resident(
            p_mft, NTFS_AT_BITMAP, sii, 4, ibm, (uint32_t)sizeof(ibm), pfs_cpu_to_le16(0));
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    return pfs_ntfs_mkfs_mft_finalize(p_mft);
}

static int pfs_ntfs_mkfs_build_badclus_record(
        pfs_ntfs_mkfs_mft_t *p_mft,
        uint64_t parent_le,
        uint64_t now_le,
        const pfs_ntfs_mkfs_layout_t *p_lo,
        uint32_t cluster_size)
{
    uint16_t bad[4];

    int rc = pfs_ntfs_mkfs_mft_init(p_mft, NTFS_FILE_BadClus, NTFS_MFT_RECORD_IN_USE);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_ntfs_mkfs_add_std_info(
            p_mft, NTFS_FILE_ATTR_HIDDEN | NTFS_FILE_ATTR_SYSTEM, now_le, PFS_NTFS_MKFS_SECURITY_ID);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_ntfs_mkfs_add_file_name(
            p_mft,
            parent_le,
            "$BadClus",
            NTFS_FILE_ATTR_HIDDEN | NTFS_FILE_ATTR_SYSTEM,
            now_le,
            0,
            0,
            NTFS_FILE_NAME_WIN32_AND_DOS,
            0);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    // mkntfs: 未命名空 $DATA + 命名 $Bad (hole runlist, flags=0)
    rc = pfs_ntfs_mkfs_mft_add_resident(
            p_mft, NTFS_AT_DATA, NULL, 0, NULL, 0, pfs_cpu_to_le16(0));
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    pfs_ntfs_mkfs_utf16_name4(bad, '$', 'B', 'a', 'd');
    rc = pfs_ntfs_mkfs_mft_add_nonres_hole(
            p_mft,
            NTFS_AT_DATA,
            bad,
            4,
            p_lo->total_clusters,
            cluster_size);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    return pfs_ntfs_mkfs_mft_finalize(p_mft);
}

// mkntfs: MFT 12..15 预留 in-use, 空 $DATA + 驻留 SD, security_id=0
static int pfs_ntfs_mkfs_build_reserved_record(
        pfs_ntfs_mkfs_mft_t *p_mft,
        uint32_t rec_num,
        uint64_t now_le)
{
    uint8_t sd[128];
    uint32_t sd_len;

    int rc = pfs_ntfs_mkfs_mft_init(p_mft, rec_num, NTFS_MFT_RECORD_IN_USE);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_ntfs_mkfs_add_std_info(
            p_mft, NTFS_FILE_ATTR_HIDDEN | NTFS_FILE_ATTR_SYSTEM, now_le, 0);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    // 属性须按 type 升序: SI(0x10) -> SD(0x50) -> DATA(0x80)
    sd_len = pfs_ntfs_mkfs_build_default_sd(sd, (uint32_t)sizeof(sd));
    if (0U == sd_len)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    rc = pfs_ntfs_mkfs_mft_add_resident(
            p_mft, NTFS_AT_SECURITY_DESCRIPTOR, NULL, 0, sd, sd_len, pfs_cpu_to_le16(0));
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_ntfs_mkfs_mft_add_resident(
            p_mft, NTFS_AT_DATA, NULL, 0, NULL, 0, pfs_cpu_to_le16(0));
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    return pfs_ntfs_mkfs_mft_finalize(p_mft);
}

// $Extend 下 $Quota(24)/$ObjId(25)/$Reparse(26); 空 view index
static int pfs_ntfs_mkfs_build_extend_child(
        pfs_ntfs_mkfs_mft_t *p_mft,
        uint32_t rec_num,
        const char *name,
        uint64_t extend_parent,
        uint64_t now_le,
        uint32_t cluster_size,
        uint32_t collation0,
        const char *idx0,
        uint32_t collation1,
        const char *idx1)
{
    uint16_t in0[8];
    uint16_t in1[8];
    uint8_t n0 = 0;
    uint8_t n1 = 0;
    uint32_t attrs;

    attrs = NTFS_FILE_ATTR_HIDDEN | NTFS_FILE_ATTR_SYSTEM | NTFS_FILE_ATTR_ARCHIVE |
            NTFS_FILE_ATTR_DUP_VIEW_INDEX_PRESENT;
    int rc = pfs_ntfs_mkfs_mft_init(
            p_mft,
            rec_num,
            (uint16_t)(NTFS_MFT_RECORD_IN_USE | NTFS_MFT_RECORD_IS_4 | NTFS_MFT_RECORD_IS_VIEW_INDEX));
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_ntfs_mkfs_add_std_info(p_mft, attrs, now_le, PFS_NTFS_MKFS_SECURITY_ID);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_ntfs_mkfs_add_file_name(
            p_mft, extend_parent, name, attrs, now_le, 0, 0, NTFS_FILE_NAME_WIN32_AND_DOS, 0);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    pfs_ntfs_mkfs_ascii_to_utf16le(in0, idx0, &n0);
    rc = pfs_ntfs_mkfs_add_index_root_view_empty(p_mft, in0, n0, collation0, cluster_size);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    if (NULL != idx1)
    {
        pfs_ntfs_mkfs_ascii_to_utf16le(in1, idx1, &n1);
        rc = pfs_ntfs_mkfs_add_index_root_view_empty(p_mft, in1, n1, collation1, cluster_size);
        if (PFS_NTFS_RET_OK != rc)
        {
            return rc;
        }
    }
    return pfs_ntfs_mkfs_mft_finalize(p_mft);
}

static int pfs_ntfs_mkfs_build_extend_record(
        pfs_ntfs_mkfs_mft_t *p_mft,
        uint64_t root_parent,
        uint64_t now_le,
        uint32_t cluster_size)
{
    uint16_t i30[4];
    uint8_t buf[512];
    ntfs_index_root_t *p_root;
    uint32_t ent_off;
    uint32_t val_len;
    uint32_t attrs;
    const struct {
        const char *name;
        uint32_t rec_num;
    } kids[] = {
        { "$ObjId", 25 },
        { "$Quota", 24 },
        { "$Reparse", 26 },
    };

    attrs = PFS_NTFS_MKFS_DIR_FILE_ATTRS;
    int rc = pfs_ntfs_mkfs_mft_init(
            p_mft,
            NTFS_FILE_Extend,
            (uint16_t)(NTFS_MFT_RECORD_IN_USE | NTFS_MFT_RECORD_IS_DIRECTORY));
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_ntfs_mkfs_add_std_info(p_mft, attrs, now_le, PFS_NTFS_MKFS_SECURITY_ID_META);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_ntfs_mkfs_add_file_name(
            p_mft, root_parent, "$Extend", attrs, now_le, 0, 0, NTFS_FILE_NAME_WIN32_AND_DOS, 1);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    pfs_ntfs_mkfs_utf16_name4(i30, '$', 'I', '3', '0');
    memset(buf, 0, sizeof(buf));
    p_root = (ntfs_index_root_t *)buf;
    p_root->type = NTFS_AT_FILE_NAME;
    p_root->collation_rule = NTFS_COLLATION_FILE_NAME;
    p_root->index_block_size = pfs_cpu_to_le32(PFS_NTFS_MKFS_INDEX_BLOCK_SIZE);
    p_root->clusters_per_index_block = pfs_ntfs_mkfs_index_root_clusters_per_block(
            PFS_NTFS_MKFS_INDEX_BLOCK_SIZE, cluster_size);
    p_root->index.entries_offset = pfs_cpu_to_le32((uint32_t)sizeof(ntfs_index_header_t));
    p_root->index.flags = (uint8_t)NTFS_SMALL_INDEX;
    ent_off = (uint32_t)sizeof(ntfs_index_root_t);
    for (uint32_t ki = 0; ki < (uint32_t)(sizeof(kids) / sizeof(kids[0])); ki++)
    {
        rc = pfs_ntfs_mkfs_indx_append_file_name_entry(
                buf,
                (uint32_t)sizeof(buf),
                &ent_off,
                kids[ki].name,
                kids[ki].rec_num,
                NTFS_FILE_ATTR_HIDDEN | NTFS_FILE_ATTR_SYSTEM | NTFS_FILE_ATTR_ARCHIVE |
                        NTFS_FILE_ATTR_DUP_VIEW_INDEX_PRESENT,
                NTFS_MK_LE_MREF(NTFS_FILE_Extend, pfs_ntfs_mkfs_seq_for_rec(NTFS_FILE_Extend)),
                now_le,
                0,
                0,
                NTFS_FILE_NAME_WIN32_AND_DOS);
        if (PFS_NTFS_RET_OK != rc)
        {
            return rc;
        }
    }
    rc = pfs_ntfs_mkfs_indx_append_end(buf, (uint32_t)sizeof(buf), &ent_off);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    // index_length / allocated_size 相对 embedded index header
    p_root->index.index_length = pfs_cpu_to_le32(
            ent_off - (uint32_t)offsetof(ntfs_index_root_t, index));
    p_root->index.allocated_size = p_root->index.index_length;
    val_len = ent_off;
    rc = pfs_ntfs_mkfs_mft_add_resident(
            p_mft, NTFS_AT_INDEX_ROOT, i30, 4, buf, val_len, pfs_cpu_to_le16(0));
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    return pfs_ntfs_mkfs_mft_finalize(p_mft);
}

static int pfs_ntfs_mkfs_build_mft_self_record(
        pfs_ntfs_mkfs_mft_t *p_mft,
        const pfs_ntfs_mkfs_layout_t *p_lo,
        uint32_t cluster_size,
        uint64_t parent_le,
        uint64_t now_le)
{
    uint64_t mft_data_bytes;
    uint64_t mft_alloc_bytes;

    if (NULL == p_lo || 0U == p_lo->mft_record_count || 0U == p_lo->mftbmp_clusters)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    mft_data_bytes = (uint64_t)p_lo->mft_record_count * PFS_NTFS_MKFS_MFT_RECORD_SIZE;
    mft_alloc_bytes = (uint64_t)p_lo->mft_clusters * (uint64_t)cluster_size;
    int rc = pfs_ntfs_mkfs_mft_init(p_mft, NTFS_FILE_MFT, NTFS_MFT_RECORD_IN_USE);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_ntfs_mkfs_add_std_info(
            p_mft, NTFS_FILE_ATTR_HIDDEN | NTFS_FILE_ATTR_SYSTEM, now_le, PFS_NTFS_MKFS_SECURITY_ID);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_ntfs_mkfs_add_file_name(
            p_mft,
            parent_le,
            "$MFT",
            NTFS_FILE_ATTR_HIDDEN | NTFS_FILE_ATTR_SYSTEM,
            now_le,
            mft_alloc_bytes,
            mft_data_bytes,
            NTFS_FILE_NAME_WIN32_AND_DOS,
            0);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_ntfs_mkfs_mft_add_nonres(
            p_mft,
            NTFS_AT_DATA,
            NULL,
            0,
            p_lo->mft_lcn,
            p_lo->mft_clusters,
            mft_data_bytes,
            cluster_size,
            pfs_cpu_to_le16(0));
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    // Windows 要求 $MFT/$BITMAP 非驻留
    rc = pfs_ntfs_mkfs_mft_add_nonres(
            p_mft,
            NTFS_AT_BITMAP,
            NULL,
            0,
            p_lo->mftbmp_lcn,
            p_lo->mftbmp_clusters,
            (uint64_t)p_lo->mftbmp_bytes,
            cluster_size,
            pfs_cpu_to_le16(0));
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    return pfs_ntfs_mkfs_mft_finalize(p_mft);
}

static void pfs_ntfs_mkfs_put_attrdef_name(ntfs_attr_def_t *p_ad, const char *ascii)
{
    uint16_t *p_name = p_ad->name;

    for (int i = 0; ascii[i] != '\0' && i < 0x40; i++)
    {
        p_name[i] = pfs_cpu_to_le16((uint16_t)(unsigned char)ascii[i]);
    }
}

static int pfs_ntfs_mkfs_build_attrdef(uint8_t **pp_buf, uint32_t *p_bytes)
{
    // 对齐 ntfs-3g attrdef_ntfs3x_array / Windows Format: 无 $PROPERTY_SET;
    // 15 项有效定义 + 1 项全 0 终止槽 = 16 * 160 = 2560; collation 全 0
    static const struct {
        const char *name;
        uint32_t type;
        uint32_t flags;
        uint64_t min_size;
        uint64_t max_size;
    } defs[] = {
        { "$STANDARD_INFORMATION", NTFS_AT_STANDARD_INFORMATION, NTFS_ATTR_DEF_RESIDENT, 48, 72 },
        { "$ATTRIBUTE_LIST", NTFS_AT_ATTRIBUTE_LIST, NTFS_ATTR_DEF_ALWAYS_LOG, 0, 0xffffffffffffffffULL },
        { "$FILE_NAME", NTFS_AT_FILE_NAME,
          (uint32_t)(NTFS_ATTR_DEF_INDEXABLE | NTFS_ATTR_DEF_RESIDENT), 68, 578 },
        { "$OBJECT_ID", NTFS_AT_OBJECT_ID, NTFS_ATTR_DEF_RESIDENT, 0, 256 },
        { "$SECURITY_DESCRIPTOR", NTFS_AT_SECURITY_DESCRIPTOR, NTFS_ATTR_DEF_ALWAYS_LOG, 0, 0xffffffffffffffffULL },
        { "$VOLUME_NAME", NTFS_AT_VOLUME_NAME, NTFS_ATTR_DEF_RESIDENT, 2, 256 },
        { "$VOLUME_INFORMATION", NTFS_AT_VOLUME_INFORMATION, NTFS_ATTR_DEF_RESIDENT, 12, 12 },
        { "$DATA", NTFS_AT_DATA, 0, 0, 0xffffffffffffffffULL },
        { "$INDEX_ROOT", NTFS_AT_INDEX_ROOT, NTFS_ATTR_DEF_RESIDENT, 0, 0xffffffffffffffffULL },
        { "$INDEX_ALLOCATION", NTFS_AT_INDEX_ALLOCATION, NTFS_ATTR_DEF_ALWAYS_LOG, 0, 0xffffffffffffffffULL },
        { "$BITMAP", NTFS_AT_BITMAP, NTFS_ATTR_DEF_ALWAYS_LOG, 0, 0xffffffffffffffffULL },
        { "$REPARSE_POINT", NTFS_AT_REPARSE_POINT, NTFS_ATTR_DEF_ALWAYS_LOG, 0, 0x4000ULL },
        { "$EA_INFORMATION", NTFS_AT_EA_INFORMATION, NTFS_ATTR_DEF_RESIDENT, 8, 8 },
        { "$EA", NTFS_AT_EA, 0, 0, 0x10000ULL },
        { "$LOGGED_UTILITY_STREAM", NTFS_AT_LOGGED_UTILITY_STREAM, NTFS_ATTR_DEF_ALWAYS_LOG, 0, 0x10000ULL },
    };
    uint8_t *p_buf;
    uint32_t n;

    n = (uint32_t)(sizeof(defs) / sizeof(defs[0]));
    // +1: 末项 type=0 终止槽 (与 Windows/ntfs-3g 2560 字节表一致)
    p_buf = (uint8_t *)calloc(n + 1U, sizeof(ntfs_attr_def_t));
    if (NULL == p_buf)
    {
        return PFS_NTFS_RET_ENOMEM;
    }
    for (uint32_t i = 0; i < n; i++)
    {
        ntfs_attr_def_t *p_ad = (ntfs_attr_def_t *)(p_buf + i * sizeof(ntfs_attr_def_t));
        pfs_ntfs_mkfs_put_attrdef_name(p_ad, defs[i].name);
        p_ad->type = defs[i].type;
        p_ad->display_rule = 0;
        p_ad->collation_rule = 0;
        p_ad->flags = defs[i].flags;
        p_ad->min_size = pfs_cpu_to_le64(defs[i].min_size);
        p_ad->max_size = pfs_cpu_to_le64(defs[i].max_size);
    }
    *pp_buf = p_buf;
    *p_bytes = (n + 1U) * (uint32_t)sizeof(ntfs_attr_def_t);
    return PFS_NTFS_RET_OK;
}

static int pfs_ntfs_mkfs_calc_layout(
        uint64_t vol_bytes,
        uint32_t cluster_size,
        uint32_t attrdef_bytes,
        pfs_ntfs_mkfs_layout_t *p_lo)
{
    uint64_t lcn;
    uint64_t total_clusters;
    uint64_t mirr_end;
    uint64_t mft_pref;
    uint64_t mft_region;
    uint64_t tail_clusters;
    uint64_t min_mft_lcn;
    uint32_t entry_len;

    if (NULL == p_lo || 0U == cluster_size || 0U == attrdef_bytes)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    total_clusters = vol_bytes / cluster_size;
    if (0ULL == total_clusters)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    memset(p_lo, 0, sizeof(*p_lo));
    p_lo->total_clusters = total_clusters;
    p_lo->mftmirr_records = pfs_ntfs_mkfs_mftmirr_records(cluster_size);
    p_lo->mftmirr_bytes = p_lo->mftmirr_records * PFS_NTFS_MKFS_MFT_RECORD_SIZE;
    // $MFT 至少容纳镜像条数, 且不少于 PFS_NTFS_MKFS_MFT_RECORD_COUNT (无增长时供 mkvol)
    p_lo->mft_record_count = p_lo->mftmirr_records;
    if (p_lo->mft_record_count < PFS_NTFS_MKFS_MFT_RECORD_COUNT)
    {
        p_lo->mft_record_count = PFS_NTFS_MKFS_MFT_RECORD_COUNT;
    }
    // Windows format: boot@0, $MFTMirr 紧随 boot (通常 LCN1), $MFT 靠后
    p_lo->boot_lcn = 0;
    p_lo->boot_clusters = pfs_ntfs_mkfs_clusters_for_bytes(
            PFS_NTFS_MKFS_BOOT_SECTORS * PFS_NTFS_MKFS_SECTOR_SIZE, cluster_size);
    p_lo->mftmirr_lcn = p_lo->boot_clusters;
    p_lo->mftmirr_clusters = pfs_ntfs_mkfs_clusters_for_bytes(p_lo->mftmirr_bytes, cluster_size);
    if (0U == p_lo->mftmirr_clusters)
    {
        p_lo->mftmirr_clusters = 1U;
    }
    mirr_end = p_lo->mftmirr_lcn + (uint64_t)p_lo->mftmirr_clusters;

    p_lo->mftbmp_bytes = (p_lo->mft_record_count + 7U) / 8U;
    if (p_lo->mftbmp_bytes < 8U)
    {
        p_lo->mftbmp_bytes = 8U;
    }
    p_lo->mftbmp_bytes = (p_lo->mftbmp_bytes + 7U) & ~7U;
    p_lo->mftbmp_clusters = pfs_ntfs_mkfs_clusters_for_bytes(p_lo->mftbmp_bytes, cluster_size);
    if (0U == p_lo->mftbmp_clusters)
    {
        p_lo->mftbmp_clusters = 1U;
    }
    p_lo->mft_clusters = pfs_ntfs_mkfs_clusters_for_bytes(
            p_lo->mft_record_count * PFS_NTFS_MKFS_MFT_RECORD_SIZE, cluster_size);
    if (0U == p_lo->mft_clusters)
    {
        p_lo->mft_clusters = 1U;
    }
    mft_region = (uint64_t)p_lo->mftbmp_clusters + (uint64_t)p_lo->mft_clusters;

    p_lo->bitmap_bytes = (uint32_t)((total_clusters + 7ULL) / 8ULL);
    p_lo->bitmap_bytes = (p_lo->bitmap_bytes + 7U) & ~7U;
    p_lo->bitmap_clusters = pfs_ntfs_mkfs_clusters_for_bytes(p_lo->bitmap_bytes, cluster_size);
    p_lo->upcase_clusters = pfs_ntfs_mkfs_clusters_for_bytes(
            PFS_NTFS_MKFS_UPCASE_CHARS * 2U, cluster_size);
    p_lo->logfile_bytes = pfs_ntfs_mkfs_pick_logfile_bytes(vol_bytes, cluster_size);
    p_lo->logfile_clusters = (uint32_t)((p_lo->logfile_bytes + (uint64_t)cluster_size - 1ULL) /
            (uint64_t)cluster_size);
    if (0U == p_lo->logfile_clusters)
    {
        p_lo->logfile_clusters = 1U;
    }
    p_lo->attrdef_bytes = attrdef_bytes;
    p_lo->attrdef_clusters = pfs_ntfs_mkfs_clusters_for_bytes(attrdef_bytes, cluster_size);
    p_lo->root_idx_clusters = pfs_ntfs_mkfs_clusters_for_bytes(
            PFS_NTFS_MKFS_INDEX_BLOCK_SIZE, cluster_size);
    if (0U == p_lo->root_idx_clusters)
    {
        p_lo->root_idx_clusters = 1U;
    }
    p_lo->secure_idx_clusters = p_lo->root_idx_clusters;
    entry_len = PFS_NTFS_MKFS_SDS_FIRST_BUCKET;
    if (0U == entry_len)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    p_lo->sds_bytes = PFS_NTFS_MKFS_SDS_MIRROR_OFF + entry_len;
    p_lo->sds_clusters = pfs_ntfs_mkfs_clusters_for_bytes(p_lo->sds_bytes, cluster_size);
    if (0U == p_lo->sds_clusters)
    {
        p_lo->sds_clusters = 1U;
    }
    tail_clusters =
            (uint64_t)p_lo->bitmap_clusters +
            (uint64_t)p_lo->upcase_clusters +
            (uint64_t)p_lo->logfile_clusters +
            (uint64_t)p_lo->attrdef_clusters +
            (uint64_t)p_lo->root_idx_clusters +
            (uint64_t)p_lo->sds_clusters +
            (uint64_t)p_lo->secure_idx_clusters * 2ULL;

    // 大卷: Windows 常把 $MFT 放在约 3GiB; 小卷: 12.5% (mkntfs mft zone 倍率 1)
    if (vol_bytes >= (4ULL * 1024ULL * 1024ULL * 1024ULL))
    {
        mft_pref = (3ULL * 1024ULL * 1024ULL * 1024ULL) / (uint64_t)cluster_size;
    }
    else
    {
        mft_pref = total_clusters >> 3;
    }
    min_mft_lcn = mirr_end;
    // 小簇时 MFT 区物理偏移至少 16KiB (mkntfs)
    {
        uint64_t min16 = (16384ULL + (uint64_t)cluster_size - 1ULL) / (uint64_t)cluster_size;

        if (min_mft_lcn < min16)
        {
            min_mft_lcn = min16;
        }
    }
    if (mft_pref < min_mft_lcn)
    {
        mft_pref = min_mft_lcn;
    }
    // 放不下则紧随 $MFTMirr 打包
    if (mft_pref + mft_region + tail_clusters > total_clusters)
    {
        mft_pref = min_mft_lcn;
    }
    if (mft_pref + mft_region + tail_clusters > total_clusters)
    {
        return PFS_NTFS_RET_ENOSPC;
    }

    p_lo->mftbmp_lcn = mft_pref;
    p_lo->mft_lcn = mft_pref + (uint64_t)p_lo->mftbmp_clusters;
    lcn = p_lo->mft_lcn + (uint64_t)p_lo->mft_clusters;
    p_lo->bitmap_lcn = lcn;
    lcn += (uint64_t)p_lo->bitmap_clusters;
    p_lo->upcase_lcn = lcn;
    lcn += (uint64_t)p_lo->upcase_clusters;
    p_lo->logfile_lcn = lcn;
    lcn += (uint64_t)p_lo->logfile_clusters;
    p_lo->attrdef_lcn = lcn;
    lcn += (uint64_t)p_lo->attrdef_clusters;
    p_lo->root_idx_lcn = lcn;
    lcn += (uint64_t)p_lo->root_idx_clusters;
    p_lo->sds_lcn = lcn;
    lcn += (uint64_t)p_lo->sds_clusters;
    p_lo->sdh_idx_lcn = lcn;
    lcn += (uint64_t)p_lo->secure_idx_clusters;
    p_lo->sii_idx_lcn = lcn;
    lcn += (uint64_t)p_lo->secure_idx_clusters;
    if (lcn > total_clusters)
    {
        return PFS_NTFS_RET_ENOSPC;
    }
    return PFS_NTFS_RET_OK;
}

static void pfs_ntfs_mkfs_bitmap_set_range(uint8_t *buf, uint32_t bitmap_bytes, uint64_t start, uint64_t count)
{

    for (uint64_t clu = start; clu < start + count; clu++)
    {
        uint64_t bit = clu;
        if (bit / 8ULL < bitmap_bytes)
        {
            buf[bit / 8ULL] |= (uint8_t)(1U << (bit % 8ULL));
        }
    }
}

// 超出卷簇数的位图尾部位须置 1 (不可分配; 对齐 Windows/mkntfs)
static void pfs_ntfs_mkfs_bitmap_set_out_of_volume(uint8_t *buf, uint32_t bitmap_bytes, uint64_t total_clusters)
{
    uint64_t bit_cap;

    if (NULL == buf || 0U == bitmap_bytes)
    {
        return;
    }
    bit_cap = (uint64_t)bitmap_bytes * 8ULL;
    for (uint64_t bit = total_clusters; bit < bit_cap; bit++)
    {
        buf[bit / 8ULL] |= (uint8_t)(1U << (bit % 8ULL));
    }
}

// 对齐 linux-7.1 fs/ntfs/upcase.c generate_default_upcase
static int pfs_ntfs_mkfs_build_upcase(uint16_t **pp_upcase)
{
    static const int uc_run_table[][3] = {
        { 0x0061, 0x007B, -32 }, { 0x0451, 0x045D, -80 }, { 0x1F70, 0x1F72, 74 },
        { 0x00E0, 0x00F7, -32 }, { 0x045E, 0x0460, -80 }, { 0x1F72, 0x1F76, 86 },
        { 0x00F8, 0x00FF, -32 }, { 0x0561, 0x0587, -48 }, { 0x1F76, 0x1F78, 100 },
        { 0x0256, 0x0258, -205 }, { 0x1F00, 0x1F08, 8 }, { 0x1F78, 0x1F7A, 128 },
        { 0x028A, 0x028C, -217 }, { 0x1F10, 0x1F16, 8 }, { 0x1F7A, 0x1F7C, 112 },
        { 0x03AC, 0x03AD, -38 }, { 0x1F20, 0x1F28, 8 }, { 0x1F7C, 0x1F7E, 126 },
        { 0x03AD, 0x03B0, -37 }, { 0x1F30, 0x1F38, 8 }, { 0x1FB0, 0x1FB2, 8 },
        { 0x03B1, 0x03C2, -32 }, { 0x1F40, 0x1F46, 8 }, { 0x1FD0, 0x1FD2, 8 },
        { 0x03C2, 0x03C3, -31 }, { 0x1F51, 0x1F52, 8 }, { 0x1FE0, 0x1FE2, 8 },
        { 0x03C3, 0x03CC, -32 }, { 0x1F53, 0x1F54, 8 }, { 0x1FE5, 0x1FE6, 7 },
        { 0x03CC, 0x03CD, -64 }, { 0x1F55, 0x1F56, 8 }, { 0x2170, 0x2180, -16 },
        { 0x03CD, 0x03CF, -63 }, { 0x1F57, 0x1F58, 8 }, { 0x24D0, 0x24EA, -26 },
        { 0x0430, 0x0450, -32 }, { 0x1F60, 0x1F68, 8 }, { 0xFF41, 0xFF5B, -32 },
        { 0, 0, 0 }
    };
    static const int uc_dup_table[][2] = {
        { 0x0100, 0x012F }, { 0x01A0, 0x01A6 }, { 0x03E2, 0x03EF }, { 0x04CB, 0x04CC },
        { 0x0132, 0x0137 }, { 0x01B3, 0x01B7 }, { 0x0460, 0x0481 }, { 0x04D0, 0x04EB },
        { 0x0139, 0x0149 }, { 0x01CD, 0x01DD }, { 0x0490, 0x04BF }, { 0x04EE, 0x04F5 },
        { 0x014A, 0x0178 }, { 0x01DE, 0x01EF }, { 0x04BF, 0x04BF }, { 0x04F8, 0x04F9 },
        { 0x0179, 0x017E }, { 0x01F4, 0x01F5 }, { 0x04C1, 0x04C4 }, { 0x1E00, 0x1E95 },
        { 0x018B, 0x018B }, { 0x01FA, 0x0218 }, { 0x04C7, 0x04C8 }, { 0x1EA0, 0x1EF9 },
        { 0, 0 }
    };
    static const int uc_word_table[][2] = {
        { 0x00FF, 0x0178 }, { 0x01AD, 0x01AC }, { 0x01F3, 0x01F1 }, { 0x0269, 0x0196 },
        { 0x0183, 0x0182 }, { 0x01B0, 0x01AF }, { 0x0253, 0x0181 }, { 0x026F, 0x019C },
        { 0x0185, 0x0184 }, { 0x01B9, 0x01B8 }, { 0x0254, 0x0186 }, { 0x0272, 0x019D },
        { 0x0188, 0x0187 }, { 0x01BD, 0x01BC }, { 0x0259, 0x018F }, { 0x0275, 0x019F },
        { 0x018C, 0x018B }, { 0x01C6, 0x01C4 }, { 0x025B, 0x0190 }, { 0x0283, 0x01A9 },
        { 0x0192, 0x0191 }, { 0x01C9, 0x01C7 }, { 0x0260, 0x0193 }, { 0x0288, 0x01AE },
        { 0x0199, 0x0198 }, { 0x01CC, 0x01CA }, { 0x0263, 0x0194 }, { 0x0292, 0x01B7 },
        { 0x01A8, 0x01A7 }, { 0x01DD, 0x018E }, { 0x0268, 0x0197 },
        { 0, 0 }
    };
    uint16_t *p;

    p = (uint16_t *)malloc(PFS_NTFS_MKFS_UPCASE_CHARS * sizeof(uint16_t));
    if (NULL == p)
    {
        return PFS_NTFS_RET_ENOMEM;
    }
    for (uint32_t i = 0; i < PFS_NTFS_MKFS_UPCASE_CHARS; i++)
    {
        p[i] = pfs_cpu_to_le16((uint16_t)i);
    }
    for (uint32_t r = 0; 0 != uc_run_table[r][0]; r++)
    {
        for (uint32_t i = (uint32_t)uc_run_table[r][0]; i < (uint32_t)uc_run_table[r][1]; i++)
        {
            int v = (int)pfs_le16_to_cpu(p[i]) + uc_run_table[r][2];
            p[i] = pfs_cpu_to_le16((uint16_t)v);
        }
    }
    for (uint32_t r = 0; 0 != uc_dup_table[r][0]; r++)
    {
        for (int i = (uint32_t)uc_dup_table[r][0]; i < (uint32_t)uc_dup_table[r][1]; i += 2U)
        {
            int v = (int)pfs_le16_to_cpu(p[i + 1U]) - 1;
            p[i + 1U] = pfs_cpu_to_le16((uint16_t)v);
        }
    }
    for (uint32_t r = 0; 0 != uc_word_table[r][0]; r++)
    {
        p[uc_word_table[r][0]] = pfs_cpu_to_le16((uint16_t)uc_word_table[r][1]);
    }
    *pp_upcase = p;
    return PFS_NTFS_RET_OK;
}

static int pfs_ntfs_mkfs_write_cluster(
        pfs_blkio_t *p_blkio,
        int64_t part_offset,
        uint64_t lcn,
        uint32_t cluster_size,
        const void *data,
        uint32_t data_len)
{
    uint8_t *p_buf;

    p_buf = (uint8_t *)calloc(1, cluster_size);
    if (NULL == p_buf)
    {
        return PFS_NTFS_RET_ENOMEM;
    }
    if (NULL != data && data_len > 0U)
    {
        memcpy(p_buf, data, data_len);
    }
    int rc = pfs_ntfs_mkfs_pwrite(
            p_blkio,
            part_offset,
            (int64_t)(lcn * (uint64_t)cluster_size),
            p_buf,
            (int)cluster_size);
    free(p_buf);
    return rc;
}

static int pfs_ntfs_mkfs_write_boot_sector(
        pfs_blkio_t *p_blkio,
        int64_t part_offset,
        uint64_t vol_sectors,
        const pfs_ntfs_mkfs_layout_t *p_lo,
        uint32_t cluster_size,
        uint8_t bpb_spc,
        int8_t cpmft,
        int8_t cpidx,
        uint64_t serial_le)
{
    ntfs_boot_sector_t bs_host;
    ntfs_boot_sector_t bs_le;
    uint8_t sec[PFS_NTFS_MKFS_SECTOR_SIZE];
    uint32_t hidden;

    (void)cluster_size;
    // 与 Windows format 对齐: SPT/Heads/hidden/physical_drive; checksum 常为 0
    hidden = 0U;
    if (part_offset > 0 && 0 == (part_offset % (int64_t)PFS_NTFS_MKFS_SECTOR_SIZE))
    {
        uint64_t h = (uint64_t)part_offset / (uint64_t)PFS_NTFS_MKFS_SECTOR_SIZE;
        if (h <= 0xffffffffULL)
        {
            hidden = (uint32_t)h;
        }
    }
    memset(&bs_host, 0, sizeof(bs_host));
    bs_host.jump[0] = 0xEB;
    bs_host.jump[1] = 0x52;
    bs_host.jump[2] = 0x90;
    bs_host.oem_id = NTFS_OEM_ID;
    bs_host.bpb.bytes_per_sector = 512;
    bs_host.bpb.sectors_per_cluster = bpb_spc;
    bs_host.bpb.media_type = 0xF8;
    bs_host.bpb.sectors_per_track = 63;
    bs_host.bpb.heads = 255;
    bs_host.bpb.hidden_sectors = hidden;
    // mkntfs: physical_drive=0x80, extended_boot_signature=0x80
    bs_host.unused[0] = 0x80;
    bs_host.unused[1] = 0x00;
    bs_host.unused[2] = 0x80;
    bs_host.unused[3] = 0x00;
    bs_host.number_of_sectors = vol_sectors;
    bs_host.mft_lcn = p_lo->mft_lcn;
    bs_host.mftmirr_lcn = p_lo->mftmirr_lcn;
    bs_host.clusters_per_mft_record = cpmft;
    bs_host.clusters_per_index_record = cpidx;
    bs_host.volume_serial_number = serial_le;
    bs_host.checksum = 0;
    bs_host.end_of_sector_marker = 0xAA55;
    memset(sec, 0, sizeof(sec));
    bs_le = bs_host;
    bs_le.oem_id = NTFS_OEM_ID;
    bs_le.bpb.bytes_per_sector = pfs_cpu_to_le16(bs_host.bpb.bytes_per_sector);
    bs_le.bpb.sectors_per_track = pfs_cpu_to_le16(bs_host.bpb.sectors_per_track);
    bs_le.bpb.heads = pfs_cpu_to_le16(bs_host.bpb.heads);
    bs_le.bpb.hidden_sectors = pfs_cpu_to_le32(bs_host.bpb.hidden_sectors);
    bs_le.number_of_sectors = pfs_cpu_to_le64(bs_host.number_of_sectors);
    bs_le.mft_lcn = pfs_cpu_to_le64(bs_host.mft_lcn);
    bs_le.mftmirr_lcn = pfs_cpu_to_le64(bs_host.mftmirr_lcn);
    bs_le.volume_serial_number = serial_le;
    bs_le.checksum = 0;
    bs_le.end_of_sector_marker = pfs_cpu_to_le16(0xAA55);
    memcpy(sec, &bs_le, sizeof(bs_le));
    int rc = pfs_ntfs_mkfs_pwrite(p_blkio, part_offset, 0, sec, (int)sizeof(sec));
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    return pfs_ntfs_mkfs_pwrite(
            p_blkio,
            part_offset,
            (int64_t)((vol_sectors - 1ULL) * PFS_NTFS_MKFS_SECTOR_SIZE),
            sec,
            (int)sizeof(sec));
}

static int pfs_ntfs_mkfs_zero_boot_gap(
        pfs_blkio_t *p_blkio,
        int64_t part_offset,
        uint32_t cluster_size,
        uint32_t boot_clusters)
{
    uint8_t *p_z;
    uint64_t boot_bytes;

    if (0U == boot_clusters || 0U == cluster_size)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    // $Boot allocated = boot_clusters*cluster; 写满扇区1..末尾为零 (扇区0已是 BPB)
    boot_bytes = (uint64_t)boot_clusters * (uint64_t)cluster_size;
    p_z = (uint8_t *)calloc(1, PFS_NTFS_MKFS_SECTOR_SIZE);
    if (NULL == p_z)
    {
        return PFS_NTFS_RET_ENOMEM;
    }
    for (uint64_t off = PFS_NTFS_MKFS_SECTOR_SIZE; off < boot_bytes; off += PFS_NTFS_MKFS_SECTOR_SIZE)
    {
        int rc = pfs_ntfs_mkfs_pwrite(
                p_blkio,
                part_offset,
                (int64_t)off,
                p_z,
                (int)PFS_NTFS_MKFS_SECTOR_SIZE);
        if (PFS_NTFS_RET_OK != rc)
        {
            free(p_z);
            return rc;
        }
    }
    free(p_z);
    return PFS_NTFS_RET_OK;
}

static int pfs_ntfs_mkfs_build_record(
        pfs_ntfs_mkfs_mft_t *p_mft,
        uint32_t rec_num,
        uint16_t mft_flags,
        const char *name,
        uint32_t file_attrs,
        uint64_t parent_le,
        uint64_t now_le,
        const pfs_ntfs_mkfs_layout_t *p_lo,
        uint32_t cluster_size,
        int want_index,
        int empty_data,
        uint64_t data_lcn,
        uint32_t data_clusters,
        uint64_t data_size,
        uint32_t security_id)
{

    int rc = pfs_ntfs_mkfs_mft_init(p_mft, rec_num, mft_flags);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_ntfs_mkfs_add_std_info(p_mft, file_attrs, now_le, security_id);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    {
        uint64_t alloc_size = data_size;

        if (data_clusters > 0U)
        {
            alloc_size = (uint64_t)data_clusters * (uint64_t)cluster_size;
        }
        rc = pfs_ntfs_mkfs_add_file_name(
                p_mft,
                parent_le,
                name,
                file_attrs,
                now_le,
                alloc_size,
                data_size,
                NTFS_FILE_NAME_WIN32_AND_DOS,
                (mft_flags & NTFS_MFT_RECORD_IS_DIRECTORY) ? 1 : 0);
    }
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    if (want_index)
    {
        rc = pfs_ntfs_mkfs_add_index_root_empty(p_mft, cluster_size);
        if (PFS_NTFS_RET_OK != rc)
        {
            return rc;
        }
    }
    if (empty_data)
    {
        rc = pfs_ntfs_mkfs_mft_add_resident(
                p_mft, NTFS_AT_DATA, NULL, 0, NULL, 0, pfs_cpu_to_le16(0));
    }
    else if (data_clusters > 0U)
    {
        rc = pfs_ntfs_mkfs_mft_add_nonres(
                p_mft,
                NTFS_AT_DATA,
                NULL,
                0,
                data_lcn,
                data_clusters,
                data_size,
                cluster_size,
                pfs_cpu_to_le16(0));
    }
    else
    {
        rc = PFS_NTFS_RET_OK;
    }
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    (void)p_lo;
    return pfs_ntfs_mkfs_mft_finalize(p_mft);
}

static int pfs_ntfs_mkfs_build_mft(
        uint8_t *p_mft_buf,
        const pfs_ntfs_mkfs_layout_t *p_lo,
        uint32_t cluster_size,
        uint64_t now_le,
        uint64_t serial_le)
{
    pfs_ntfs_mkfs_mft_t mft;
    ntfs_volume_information_t vi;
    uint64_t root_parent;
    uint64_t extend_parent;

    (void)serial_le;
    if (NULL == p_lo || 0U == p_lo->mft_record_count)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    root_parent = NTFS_MK_LE_MREF(NTFS_FILE_root, pfs_ntfs_mkfs_seq_for_rec(NTFS_FILE_root));
    extend_parent = NTFS_MK_LE_MREF(NTFS_FILE_Extend, pfs_ntfs_mkfs_seq_for_rec(NTFS_FILE_Extend));
    for (uint32_t i = 0; i < p_lo->mft_record_count; i++)
    {
        memset(&mft, 0, sizeof(mft));
        mft.buf = p_mft_buf + i * PFS_NTFS_MKFS_MFT_RECORD_SIZE;
        mft.size = PFS_NTFS_MKFS_MFT_RECORD_SIZE;
        int rc;
        switch (i)
    {
        case NTFS_FILE_MFT:
            rc = pfs_ntfs_mkfs_build_mft_self_record(
                    &mft, p_lo, cluster_size, root_parent, now_le);
            break;
        case NTFS_FILE_MFTMirr:
            rc = pfs_ntfs_mkfs_build_record(
                    &mft, i, NTFS_MFT_RECORD_IN_USE, "$MFTMirr", NTFS_FILE_ATTR_HIDDEN | NTFS_FILE_ATTR_SYSTEM,
                    root_parent, now_le, p_lo, cluster_size, 0, 0,
                    p_lo->mftmirr_lcn, p_lo->mftmirr_clusters,
                    (uint64_t)p_lo->mftmirr_bytes, PFS_NTFS_MKFS_SECURITY_ID);
            break;
        case NTFS_FILE_LogFile:
            rc = pfs_ntfs_mkfs_build_record(
                    &mft, i, NTFS_MFT_RECORD_IN_USE, "$LogFile", NTFS_FILE_ATTR_HIDDEN | NTFS_FILE_ATTR_SYSTEM,
                    root_parent, now_le, p_lo, cluster_size, 0, 0,
                    p_lo->logfile_lcn, p_lo->logfile_clusters, p_lo->logfile_bytes,
                    PFS_NTFS_MKFS_SECURITY_ID);
            break;
        case NTFS_FILE_Volume:
            rc = pfs_ntfs_mkfs_mft_init(&mft, i, NTFS_MFT_RECORD_IN_USE);
            if (PFS_NTFS_RET_OK == rc)
            {
                rc = pfs_ntfs_mkfs_add_std_info(
                        &mft, NTFS_FILE_ATTR_HIDDEN | NTFS_FILE_ATTR_SYSTEM, now_le, 0);
            }
            if (PFS_NTFS_RET_OK == rc)
            {
                rc = pfs_ntfs_mkfs_add_file_name(
                        &mft, root_parent, "$Volume",
                        NTFS_FILE_ATTR_HIDDEN | NTFS_FILE_ATTR_SYSTEM,
                        now_le, 0, 0, NTFS_FILE_NAME_WIN32_AND_DOS, 0);
            }
            // 空卷标不写 $VOLUME_NAME (AttrDef min_size=2; 有名时再写)
            if (PFS_NTFS_RET_OK == rc)
            {
                memset(&vi, 0, sizeof(vi));
                vi.major_ver = 3;
                vi.minor_ver = 1;
                vi.flags = 0;
                rc = pfs_ntfs_mkfs_mft_add_resident(
                        &mft, NTFS_AT_VOLUME_INFORMATION, NULL, 0, &vi, (uint32_t)sizeof(vi), pfs_cpu_to_le16(0));
            }
            // Windows: $Volume 须有无名空 $DATA
            if (PFS_NTFS_RET_OK == rc)
            {
                rc = pfs_ntfs_mkfs_mft_add_resident(
                        &mft, NTFS_AT_DATA, NULL, 0, NULL, 0, pfs_cpu_to_le16(0));
            }
            if (PFS_NTFS_RET_OK == rc)
            {
                rc = pfs_ntfs_mkfs_mft_finalize(&mft);
            }
            break;
        case NTFS_FILE_AttrDef:
            rc = pfs_ntfs_mkfs_build_record(
                    &mft, i, NTFS_MFT_RECORD_IN_USE, "$AttrDef", NTFS_FILE_ATTR_HIDDEN | NTFS_FILE_ATTR_SYSTEM,
                    root_parent, now_le, p_lo, cluster_size, 0, 0,
                    p_lo->attrdef_lcn, p_lo->attrdef_clusters, p_lo->attrdef_bytes, 0);
            break;
        case NTFS_FILE_root:
            rc = pfs_ntfs_mkfs_build_root_record(
                    &mft, p_lo, cluster_size, root_parent, now_le);
            break;
        case NTFS_FILE_Bitmap:
            rc = pfs_ntfs_mkfs_build_record(
                    &mft, i, NTFS_MFT_RECORD_IN_USE, "$Bitmap", NTFS_FILE_ATTR_HIDDEN | NTFS_FILE_ATTR_SYSTEM,
                    root_parent, now_le, p_lo, cluster_size, 0, 0,
                    p_lo->bitmap_lcn, p_lo->bitmap_clusters, p_lo->bitmap_bytes,
                    PFS_NTFS_MKFS_SECURITY_ID);
            break;
        case NTFS_FILE_Boot:
            rc = pfs_ntfs_mkfs_build_record(
                    &mft, i, NTFS_MFT_RECORD_IN_USE, "$Boot", NTFS_FILE_ATTR_HIDDEN | NTFS_FILE_ATTR_SYSTEM,
                    root_parent, now_le, p_lo, cluster_size, 0, 0,
                    p_lo->boot_lcn, p_lo->boot_clusters,
                    (uint64_t)PFS_NTFS_MKFS_BOOT_SECTORS * PFS_NTFS_MKFS_SECTOR_SIZE, 0);
            break;
        case NTFS_FILE_BadClus:
            rc = pfs_ntfs_mkfs_build_badclus_record(
                    &mft, root_parent, now_le, p_lo, cluster_size);
            break;
        case NTFS_FILE_Secure:
            rc = pfs_ntfs_mkfs_build_secure_record(
                    &mft,
                    p_lo,
                    root_parent,
                    now_le,
                    cluster_size);
            break;
        case NTFS_FILE_UpCase:
            rc = pfs_ntfs_mkfs_build_record(
                    &mft, i, NTFS_MFT_RECORD_IN_USE, "$UpCase", NTFS_FILE_ATTR_HIDDEN | NTFS_FILE_ATTR_SYSTEM,
                    root_parent, now_le, p_lo, cluster_size, 0, 0,
                    p_lo->upcase_lcn, p_lo->upcase_clusters,
                    (uint64_t)PFS_NTFS_MKFS_UPCASE_CHARS * 2U, PFS_NTFS_MKFS_SECURITY_ID);
            break;
        case NTFS_FILE_Extend:
            rc = pfs_ntfs_mkfs_build_extend_record(&mft, root_parent, now_le, cluster_size);
            break;
        case NTFS_FILE_reserved12:
        case NTFS_FILE_reserved13:
        case NTFS_FILE_reserved14:
        case NTFS_FILE_reserved15:
            rc = pfs_ntfs_mkfs_build_reserved_record(&mft, i, now_le);
            break;
        case 24:
            // 同 type 属性按名字升序: $O 在 $Q 前
            rc = pfs_ntfs_mkfs_build_extend_child(
                    &mft, 24, "$Quota", extend_parent, now_le, cluster_size,
                    NTFS_COLLATION_NTOFS_SID, "$O",
                    NTFS_COLLATION_NTOFS_ULONG, "$Q");
            break;
        case 25:
            rc = pfs_ntfs_mkfs_build_extend_child(
                    &mft, 25, "$ObjId", extend_parent, now_le, cluster_size,
                    NTFS_COLLATION_NTOFS_ULONGS, "$O",
                    0, NULL);
            break;
        case 26:
            rc = pfs_ntfs_mkfs_build_extend_child(
                    &mft, 26, "$Reparse", extend_parent, now_le, cluster_size,
                    NTFS_COLLATION_NTOFS_ULONGS, "$R",
                    0, NULL);
            break;
        default:
            // 16..23 与 27+ : 合法空闲 FILE (全 0 会被 Windows 判损坏)
            rc = pfs_ntfs_mkfs_init_free_mft_record(&mft, i);
            break;
        }
        if (PFS_NTFS_RET_OK != rc)
        {
            return rc;
        }
    }
    return PFS_NTFS_RET_OK;
}

int pfs_ntfs_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes)
{
    const uint32_t sector_size = PFS_NTFS_MKFS_SECTOR_SIZE;
    uint8_t sectors_per_cluster;
    uint8_t bpb_spc;
    uint32_t cluster_size;
    int8_t cpmft;
    int8_t cpidx;
    uint64_t vol_sectors;
    uint64_t now_le;
    uint64_t serial_le;
    pfs_ntfs_mkfs_layout_t lo;
    uint8_t *p_attrdef = NULL;
    uint32_t attrdef_bytes = 0;
    uint8_t *p_mft = NULL;
    uint8_t *p_bitmap = NULL;
    uint8_t *p_root_indx = NULL;
    uint8_t *p_sdh_indx = NULL;
    uint8_t *p_sii_indx = NULL;
    uint8_t *p_sds = NULL;
    uint16_t *p_upcase = NULL;
    pfs_ntfs_mkfs_sds_ref_t sds_refs[PFS_NTFS_MKFS_SDS_ENTRY_COUNT];
    uint32_t sds_bytes = 0;
    uint32_t mft_bytes;

    if (NULL == p_blkio || part_offset < 0)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (!pfs_blkio_is_open(p_blkio))
    {
        return PFS_NTFS_RET_EBUSY;
    }
    if (0U != (part_offset % (int64_t)sector_size))
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (vol_bytes < (uint64_t)PFS_NTFS_MKFS_MIN_VOL_SECTORS * sector_size ||
            0U != (vol_bytes % sector_size))
    {
        return PFS_NTFS_RET_EINVAL;
    }

    sectors_per_cluster = pfs_ntfs_mkfs_pick_sectors_per_cluster(vol_bytes);
    bpb_spc = pfs_ntfs_mkfs_bpb_sectors_per_cluster(sectors_per_cluster);
    cluster_size = sector_size * (uint32_t)sectors_per_cluster;
    cpmft = pfs_ntfs_mkfs_clusters_per_record(PFS_NTFS_MKFS_MFT_RECORD_SIZE, cluster_size);
    cpidx = pfs_ntfs_mkfs_clusters_per_record(PFS_NTFS_MKFS_INDEX_BLOCK_SIZE, cluster_size);
    // 扇区数与 $Bitmap 簇数对齐 (丢弃不足一簇的尾扇区)
    vol_sectors = (vol_bytes / (uint64_t)cluster_size) * (uint64_t)sectors_per_cluster;
    if (vol_sectors < (uint64_t)PFS_NTFS_MKFS_MIN_VOL_SECTORS)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    vol_bytes = vol_sectors * (uint64_t)sector_size;
    now_le = pfs_ntfs_mkfs_now_le();
    serial_le = pfs_ntfs_mkfs_rand_serial();

    int rc = pfs_ntfs_mkfs_build_attrdef(&p_attrdef, &attrdef_bytes);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_ntfs_mkfs_calc_layout(vol_bytes, cluster_size, attrdef_bytes, &lo);
    if (PFS_NTFS_RET_OK != rc)
    {
        free(p_attrdef);
        return rc;
    }
    rc = pfs_ntfs_mkfs_build_sds(&p_sds, &sds_bytes);
    if (PFS_NTFS_RET_OK != rc)
    {
        free(p_attrdef);
        return rc;
    }

    mft_bytes = lo.mft_record_count * PFS_NTFS_MKFS_MFT_RECORD_SIZE;
    p_mft = (uint8_t *)calloc(1, mft_bytes);
    p_bitmap = (uint8_t *)calloc(1, lo.bitmap_bytes);
    if (NULL == p_mft || NULL == p_bitmap)
    {
        free(p_attrdef);
        free(p_sds);
        free(p_mft);
        free(p_bitmap);
        return PFS_NTFS_RET_ENOMEM;
    }
    rc = pfs_ntfs_mkfs_build_upcase(&p_upcase);
    if (PFS_NTFS_RET_OK != rc)
    {
        free(p_attrdef);
        free(p_sds);
        free(p_mft);
        free(p_bitmap);
        return rc;
    }
    rc = pfs_ntfs_mkfs_build_mft(p_mft, &lo, cluster_size, now_le, serial_le);
    if (PFS_NTFS_RET_OK != rc)
    {
        free(p_attrdef);
        free(p_sds);
        free(p_mft);
        free(p_bitmap);
        free(p_upcase);
        return rc;
    }

    pfs_ntfs_mkfs_bitmap_set_range(p_bitmap, lo.bitmap_bytes, 0, lo.boot_clusters);
    pfs_ntfs_mkfs_bitmap_set_range(p_bitmap, lo.bitmap_bytes, lo.mftbmp_lcn, lo.mftbmp_clusters);
    pfs_ntfs_mkfs_bitmap_set_range(p_bitmap, lo.bitmap_bytes, lo.mft_lcn, lo.mft_clusters);
    pfs_ntfs_mkfs_bitmap_set_range(p_bitmap, lo.bitmap_bytes, lo.mftmirr_lcn, lo.mftmirr_clusters);
    pfs_ntfs_mkfs_bitmap_set_range(p_bitmap, lo.bitmap_bytes, lo.bitmap_lcn, lo.bitmap_clusters);
    pfs_ntfs_mkfs_bitmap_set_range(p_bitmap, lo.bitmap_bytes, lo.upcase_lcn, lo.upcase_clusters);
    pfs_ntfs_mkfs_bitmap_set_range(p_bitmap, lo.bitmap_bytes, lo.logfile_lcn, lo.logfile_clusters);
    pfs_ntfs_mkfs_bitmap_set_range(p_bitmap, lo.bitmap_bytes, lo.attrdef_lcn, lo.attrdef_clusters);
    pfs_ntfs_mkfs_bitmap_set_range(p_bitmap, lo.bitmap_bytes, lo.root_idx_lcn, lo.root_idx_clusters);
    pfs_ntfs_mkfs_bitmap_set_range(p_bitmap, lo.bitmap_bytes, lo.sds_lcn, lo.sds_clusters);
    pfs_ntfs_mkfs_bitmap_set_range(p_bitmap, lo.bitmap_bytes, lo.sdh_idx_lcn, lo.secure_idx_clusters);
    pfs_ntfs_mkfs_bitmap_set_range(p_bitmap, lo.bitmap_bytes, lo.sii_idx_lcn, lo.secure_idx_clusters);
    pfs_ntfs_mkfs_bitmap_set_out_of_volume(p_bitmap, lo.bitmap_bytes, lo.total_clusters);

    p_root_indx = (uint8_t *)calloc(1, PFS_NTFS_MKFS_INDEX_BLOCK_SIZE);
    p_sdh_indx = (uint8_t *)calloc(1, PFS_NTFS_MKFS_INDEX_BLOCK_SIZE);
    p_sii_indx = (uint8_t *)calloc(1, PFS_NTFS_MKFS_INDEX_BLOCK_SIZE);
    if (NULL == p_root_indx || NULL == p_sdh_indx || NULL == p_sii_indx)
    {
        free(p_attrdef);
        free(p_sds);
        free(p_mft);
        free(p_bitmap);
        free(p_upcase);
        free(p_root_indx);
        free(p_sdh_indx);
        free(p_sii_indx);
        return PFS_NTFS_RET_ENOMEM;
    }
    rc = pfs_ntfs_mkfs_build_root_indx(
            p_root_indx,
            PFS_NTFS_MKFS_INDEX_BLOCK_SIZE,
            NTFS_MK_LE_MREF(NTFS_FILE_root, pfs_ntfs_mkfs_seq_for_rec(NTFS_FILE_root)),
            now_le,
            &lo,
            cluster_size);
    if (PFS_NTFS_RET_OK != rc)
    {
        free(p_attrdef);
        free(p_sds);
        free(p_mft);
        free(p_bitmap);
        free(p_upcase);
        free(p_root_indx);
        free(p_sdh_indx);
        free(p_sii_indx);
        return rc;
    }
    pfs_ntfs_mkfs_sds_refs(sds_refs, PFS_NTFS_MKFS_SDS_ENTRY_COUNT);
    rc = pfs_ntfs_mkfs_build_secure_indx(
            p_sdh_indx, PFS_NTFS_MKFS_INDEX_BLOCK_SIZE, 1, sds_refs, PFS_NTFS_MKFS_SDS_ENTRY_COUNT);
    if (PFS_NTFS_RET_OK != rc)
    {
        free(p_attrdef);
        free(p_sds);
        free(p_mft);
        free(p_bitmap);
        free(p_upcase);
        free(p_root_indx);
        free(p_sdh_indx);
        free(p_sii_indx);
        return rc;
    }
    rc = pfs_ntfs_mkfs_build_secure_indx(
            p_sii_indx, PFS_NTFS_MKFS_INDEX_BLOCK_SIZE, 0, sds_refs, PFS_NTFS_MKFS_SDS_ENTRY_COUNT);
    if (PFS_NTFS_RET_OK != rc)
    {
        free(p_attrdef);
        free(p_sds);
        free(p_mft);
        free(p_bitmap);
        free(p_upcase);
        free(p_root_indx);
        free(p_sdh_indx);
        free(p_sii_indx);
        return rc;
    }

    rc = pfs_ntfs_mkfs_write_boot_sector(
            p_blkio, part_offset, vol_sectors, &lo, cluster_size, bpb_spc, cpmft, cpidx, serial_le);
    if (PFS_NTFS_RET_OK == rc)
    {
        rc = pfs_ntfs_mkfs_zero_boot_gap(p_blkio, part_offset, cluster_size, lo.boot_clusters);
    }
    // 写 $MFT/$BITMAP (非驻留; bit0..11 = 系统文件在用)
    if (PFS_NTFS_RET_OK == rc)
    {
        uint8_t *p_mftbmp;

        p_mftbmp = (uint8_t *)calloc(1, (size_t)lo.mftbmp_clusters * cluster_size);
        if (NULL == p_mftbmp)
        {
            rc = PFS_NTFS_RET_ENOMEM;
        }
        else
        {
            p_mftbmp[0] = 0xff;
            p_mftbmp[1] = 0xff;
            // bits 16..23 空闲; 24..26 = $Quota/$ObjId/$Reparse
            p_mftbmp[2] = 0x00;
            p_mftbmp[3] = 0x07;
            for (uint32_t clu = 0; PFS_NTFS_RET_OK == rc && clu < lo.mftbmp_clusters; clu++)
            {
                uint32_t off = clu * cluster_size;
                uint32_t len = cluster_size;
                if (off + len > lo.mftbmp_bytes)
                {
                    len = lo.mftbmp_bytes - off;
                }
                rc = pfs_ntfs_mkfs_write_cluster(
                        p_blkio, part_offset, lo.mftbmp_lcn + clu, cluster_size, p_mftbmp + off, len);
            }
            free(p_mftbmp);
        }
    }
    if (PFS_NTFS_RET_OK == rc)
    {
        for (uint32_t clu = 0; PFS_NTFS_RET_OK == rc && clu < lo.mft_clusters; clu++)
        {
            uint32_t off = clu * cluster_size;
            uint32_t len = cluster_size;
            if (off + len > mft_bytes)
            {
                len = mft_bytes - off;
            }
            rc = pfs_ntfs_mkfs_write_cluster(
                    p_blkio, part_offset, lo.mft_lcn + clu, cluster_size, p_mft + off, len);
        }
    }
    if (PFS_NTFS_RET_OK == rc)
    {
        for (uint32_t clu = 0; PFS_NTFS_RET_OK == rc && clu < lo.mftmirr_clusters; clu++)
        {
            uint32_t off = clu * cluster_size;
            uint32_t len = cluster_size;
            if (off + len > lo.mftmirr_bytes)
            {
                len = lo.mftmirr_bytes - off;
            }
            rc = pfs_ntfs_mkfs_write_cluster(
                    p_blkio, part_offset, lo.mftmirr_lcn + clu, cluster_size, p_mft + off, len);
        }
    }
    if (PFS_NTFS_RET_OK == rc)
    {
        for (uint32_t clu = 0; PFS_NTFS_RET_OK == rc && clu < lo.bitmap_clusters; clu++)
        {
            uint32_t off = clu * cluster_size;
            uint32_t len = cluster_size;
            if (off + len > lo.bitmap_bytes)
            {
                len = lo.bitmap_bytes - off;
            }
            rc = pfs_ntfs_mkfs_write_cluster(
                    p_blkio, part_offset, lo.bitmap_lcn + clu, cluster_size, p_bitmap + off, len);
        }
    }
    if (PFS_NTFS_RET_OK == rc)
    {
        for (uint32_t clu = 0; PFS_NTFS_RET_OK == rc && clu < lo.upcase_clusters; clu++)
        {
            uint32_t off = clu * cluster_size;
            uint32_t len = cluster_size;
            uint32_t upcase_bytes = PFS_NTFS_MKFS_UPCASE_CHARS * 2U;
            if (off + len > upcase_bytes)
            {
                len = upcase_bytes - off;
            }
            rc = pfs_ntfs_mkfs_write_cluster(
                    p_blkio,
                    part_offset,
                    lo.upcase_lcn + clu,
                    cluster_size,
                    (uint8_t *)p_upcase + off,
                    len);
        }
    }
    // $LogFile 先填 0xFF (空日志), 再写双份 RSTR
    if (PFS_NTFS_RET_OK == rc)
    {
        uint8_t *p_ff;

        p_ff = (uint8_t *)malloc(cluster_size);
        if (NULL == p_ff)
        {
            rc = PFS_NTFS_RET_ENOMEM;
        }
        else
        {
            memset(p_ff, 0xff, cluster_size);
            for (uint32_t clu = 0; PFS_NTFS_RET_OK == rc && clu < lo.logfile_clusters; clu++)
            {
                rc = pfs_ntfs_mkfs_write_cluster(
                        p_blkio, part_offset, lo.logfile_lcn + clu, cluster_size, p_ff, cluster_size);
            }
            free(p_ff);
        }
    }
    if (PFS_NTFS_RET_OK == rc)
    {
        rc = pfs_ntfs_mkfs_write_logfile_restart(
                p_blkio, part_offset, lo.logfile_lcn, cluster_size, lo.logfile_bytes);
    }
    if (PFS_NTFS_RET_OK == rc)
    {
        for (uint32_t clu = 0; PFS_NTFS_RET_OK == rc && clu < lo.attrdef_clusters; clu++)
        {
            uint32_t off = clu * cluster_size;
            uint32_t len = cluster_size;
            if (off + len > attrdef_bytes)
            {
                len = attrdef_bytes - off;
            }
            rc = pfs_ntfs_mkfs_write_cluster(
                    p_blkio, part_offset, lo.attrdef_lcn + clu, cluster_size, p_attrdef + off, len);
        }
    }
    if (PFS_NTFS_RET_OK == rc)
    {
        for (uint32_t clu = 0; PFS_NTFS_RET_OK == rc && clu < lo.root_idx_clusters; clu++)
        {
            uint32_t off = clu * cluster_size;
            uint32_t len = cluster_size;
            if (off + len > PFS_NTFS_MKFS_INDEX_BLOCK_SIZE)
            {
                len = PFS_NTFS_MKFS_INDEX_BLOCK_SIZE - off;
            }
            rc = pfs_ntfs_mkfs_write_cluster(
                    p_blkio,
                    part_offset,
                    lo.root_idx_lcn + clu,
                    cluster_size,
                    p_root_indx + off,
                    len);
        }
    }
    if (PFS_NTFS_RET_OK == rc)
    {
        for (uint32_t clu = 0; PFS_NTFS_RET_OK == rc && clu < lo.secure_idx_clusters; clu++)
        {
            uint32_t off = clu * cluster_size;
            uint32_t len = cluster_size;
            if (off + len > PFS_NTFS_MKFS_INDEX_BLOCK_SIZE)
            {
                len = PFS_NTFS_MKFS_INDEX_BLOCK_SIZE - off;
            }
            rc = pfs_ntfs_mkfs_write_cluster(
                    p_blkio,
                    part_offset,
                    lo.sdh_idx_lcn + clu,
                    cluster_size,
                    p_sdh_indx + off,
                    len);
        }
    }
    if (PFS_NTFS_RET_OK == rc)
    {
        for (uint32_t clu = 0; PFS_NTFS_RET_OK == rc && clu < lo.secure_idx_clusters; clu++)
        {
            uint32_t off = clu * cluster_size;
            uint32_t len = cluster_size;
            if (off + len > PFS_NTFS_MKFS_INDEX_BLOCK_SIZE)
            {
                len = PFS_NTFS_MKFS_INDEX_BLOCK_SIZE - off;
            }
            rc = pfs_ntfs_mkfs_write_cluster(
                    p_blkio,
                    part_offset,
                    lo.sii_idx_lcn + clu,
                    cluster_size,
                    p_sii_indx + off,
                    len);
        }
    }

    if (PFS_NTFS_RET_OK == rc)
    {
        for (uint32_t clu = 0; PFS_NTFS_RET_OK == rc && clu < lo.sds_clusters; clu++)
        {
            uint32_t off = clu * cluster_size;
            uint32_t len = cluster_size;
            if (off + len > lo.sds_bytes)
            {
                len = lo.sds_bytes - off;
            }
            rc = pfs_ntfs_mkfs_write_cluster(
                    p_blkio,
                    part_offset,
                    lo.sds_lcn + clu,
                    cluster_size,
                    p_sds + off,
                    len);
        }
    }

    free(p_attrdef);
    free(p_sds);
    free(p_mft);
    free(p_bitmap);
    free(p_upcase);
    free(p_root_indx);
    free(p_sdh_indx);
    free(p_sii_indx);

    if (PFS_NTFS_RET_OK == rc)
    {
        int sync_rc = pfs_blkio_fsync(p_blkio);
        if (sync_rc < 0)
        {
            rc = pfs_ntfs_mkfs_map_blkio_rc(sync_rc);
        }
    }
    return rc;
}

// end
