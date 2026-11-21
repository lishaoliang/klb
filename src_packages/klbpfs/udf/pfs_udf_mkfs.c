// Doc Encode : UTF-8 BOM, Unix(LF)
// UDF mkfs / mkvol (2048-byte logical block; Type1 map; AD_SHORT only)
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "udf/pfs_udf_internal.h"
#include "base/pfs_mem.h"
#include "pfs_endian.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#define PFS_UDF_MK_SECTOR_SIZE          2048u
#define PFS_UDF_MK_BLOCK_SIZE           2048u
#define PFS_UDF_MK_VRS_BEA              16u
#define PFS_UDF_MK_VRS_NSR              17u
#define PFS_UDF_MK_VRS_TEA              18u
#define PFS_UDF_MK_VDS_START            32u
#define PFS_UDF_MK_VDS_BLOCKS           4u
#define PFS_UDF_MK_AVDP_SECTOR          256u
#define PFS_UDF_MK_PARTITION_START      288u
#define PFS_UDF_MK_MIN_VOL_SECTORS      320u
#define PFS_UDF_MK_PARTITION_NUM        0u
#define PFS_UDF_MK_VOL_SEQ_NUM          1u
#define PFS_UDF_MK_DESC_VERSION         0x0003u
#define PFS_UDF_MK_MKVOL_MAX_NODES      8192
#define PFS_UDF_MK_MKVOL_PATH_MAX       1024
#define PFS_UDF_MK_MKVOL_COPY_CHUNK     (64 * 1024)
#define PFS_UDF_MK_CS0_MAX              254
#define PFS_UDF_MK_FID_NAME_MAX         240

typedef struct _pfs_udf_mk_node_t {
    char                    host_path[PFS_UDF_MK_MKVOL_PATH_MAX];
    char                    name[PFS_NAME_MAX + 1];
    int                     is_dir;
    uint64_t                file_size;
    int                     parent_idx;
    int                     child_head;
    int                     sibling;
    uint32_t                fe_lba;
    uint32_t                data_lba;
    uint32_t                data_len;
    uint32_t                data_blocks;
} pfs_udf_mk_node_t;

typedef struct _pfs_udf_mk_ctx_t {
    pfs_blkio_t            *p_blkio;
    int64_t                 part_offset;
    char                    volume_id[33];
    uint32_t                vol_sectors;
    uint32_t                partition_len;
    uint32_t                bitmap_lba;
    uint32_t                bitmap_blocks;
    uint32_t                layout_next_lba;
    uint32_t                fsd_lba;
    uint32_t                root_fe_lba;
    uint32_t                root_data_lba;
    uint64_t                next_unique_id;
    uint8_t                 timestamp[12];
    pfs_udf_mk_node_t      *nodes;
    int                     n_nodes;
    int                     n_cap;
    uint8_t                *copy_buf;
    uint8_t                *block_buf;
} pfs_udf_mk_ctx_t;

static int pfs_udf_mk_map_io(int rc)
{
    return pfs_udf_map_blkio_rc(rc);
}

// MSVC: pfs_write_le* 形参为 uint8_t*, 结构体字段需经 void* 包装
static void pfs_udf_mk_wle16(void *p, uint16_t v)
{
    pfs_write_le16((uint8_t *)p, v);
}

static void pfs_udf_mk_wle32(void *p, uint32_t v)
{
    pfs_write_le32((uint8_t *)p, v);
}

static void pfs_udf_mk_wle64(void *p, uint64_t v)
{
    pfs_write_le64((uint8_t *)p, v);
}

static int pfs_udf_mk_pwrite(pfs_udf_mk_ctx_t *p_ctx, int64_t off, const void *buf, int size)
{

    int rc = pfs_blkio_pwrite(p_ctx->p_blkio, buf, size, p_ctx->part_offset + off);
    if (0 > rc)
    {
        return pfs_udf_mk_map_io(rc);
    }
    else if (rc != size)
    {
        return PFS_UDF_RET_EIO;
    }
    return PFS_UDF_RET_OK;
}

static int pfs_udf_mk_write_vol_sector(pfs_udf_mk_ctx_t *p_ctx, uint32_t vol_sector,
        const void *buf)
{
    return pfs_udf_mk_pwrite(p_ctx, (int64_t)vol_sector * (int64_t)PFS_UDF_MK_SECTOR_SIZE, buf,
            (int)PFS_UDF_MK_SECTOR_SIZE);
}

static int pfs_udf_mk_write_part_lba(pfs_udf_mk_ctx_t *p_ctx, uint32_t part_lba,
        const void *buf)
{
    int64_t off = (int64_t)(PFS_UDF_MK_PARTITION_START + part_lba) * (int64_t)PFS_UDF_MK_BLOCK_SIZE;

    return pfs_udf_mk_pwrite(p_ctx, off, buf, (int)PFS_UDF_MK_BLOCK_SIZE);
}

static int pfs_udf_mk_zero_vol_sectors(pfs_udf_mk_ctx_t *p_ctx, uint32_t start,
        uint32_t count)
{
    uint8_t z[PFS_UDF_MK_SECTOR_SIZE];

    memset(z, 0, sizeof(z));
    for (uint32_t i = 0; i < count; i++)
    {
        int rc = pfs_udf_mk_write_vol_sector(p_ctx, start + i, z);
        if (PFS_UDF_RET_OK != rc)
        {
            return rc;
        }
    }
    return PFS_UDF_RET_OK;
}

static void pfs_udf_mk_fill_timestamp(uint8_t ts[12])
{
    memset(ts, 0, 12);
    time_t now = time(NULL);
    struct tm tmb;
#ifdef _WIN32
    if (0 != localtime_s(&tmb, &now))
    {
        return;
    }
    struct tm *p_tm = &tmb;
#else
    struct tm *p_tm = localtime_r(&now, &tmb);
    if (NULL == p_tm)
    {
        return;
    }
#endif
    int year = p_tm->tm_year + 1900;
    int cent = year / 100;
    year = year % 100;
    ts[0] = (uint8_t)cent;
    ts[1] = (uint8_t)year;
    ts[2] = (uint8_t)(p_tm->tm_mon + 1);
    ts[3] = (uint8_t)p_tm->tm_mday;
    ts[4] = (uint8_t)p_tm->tm_hour;
    ts[5] = (uint8_t)p_tm->tm_min;
    ts[6] = (uint8_t)p_tm->tm_sec;
    ts[7] = 0;
    ts[8] = 0;
    ts[9] = 0;
    ts[10] = 0;
    ts[11] = 0;
}

static void pfs_udf_mk_pad_dstring(uint8_t *dst, int dst_len, const char *src)
{
    int n;
    int i;

    if (NULL == src)
    {
        src = "";
    }
    n = (int)strlen(src);
    if (n > dst_len)
    {
        n = dst_len;
    }
    for (i = 0; i < n; i++)
    {
        dst[i] = (uint8_t)src[i];
    }
    for (; i < dst_len; i++)
    {
        dst[i] = (uint8_t)' ';
    }
}

static void pfs_udf_mk_set_tag(pfs_udf_tag_t *p_tag, uint16_t ident, uint32_t tag_loc,
        void *p_body, int body_off, int body_crc_len)
{
    uint8_t *p_crc_base;

    memset(p_tag, 0, sizeof(*p_tag));
    pfs_udf_mk_wle16(&p_tag->tagIdent, ident);
    pfs_udf_mk_wle16(&p_tag->descVersion, PFS_UDF_MK_DESC_VERSION);
    p_tag->reserved = 0;
    pfs_udf_mk_wle16(&p_tag->tagSerialNum, 1);
    pfs_udf_mk_wle32(&p_tag->tagLocation, tag_loc);
    if (body_crc_len > 0 && NULL != p_body)
    {
        p_crc_base = (uint8_t *)p_body + (size_t)body_off;
        pfs_udf_mk_wle16(&p_tag->descCRCLength, (uint16_t)body_crc_len);
        pfs_udf_mk_wle16(&p_tag->descCRC, pfs_udf_crc16(p_crc_base, body_crc_len));
    }
    else
    {
        pfs_udf_mk_wle16(&p_tag->descCRCLength, 0);
        pfs_udf_mk_wle16(&p_tag->descCRC, 0);
    }
    p_tag->tagChecksum = pfs_udf_tag_checksum(p_tag);
}

static void pfs_udf_mk_set_regid(pfs_udf_regid_t *p_id, const char *ident_str)
{
    int n;

    memset(p_id, 0, sizeof(*p_id));
    if (NULL == ident_str)
    {
        return;
    }
    n = (int)strlen(ident_str);
    if (n > 23)
    {
        n = 23;
    }
    memcpy(p_id->ident, ident_str, (size_t)n);
}

static void pfs_udf_mk_set_charspec(pfs_udf_charspec_t *p_cs)
{
    memset(p_cs, 0, sizeof(*p_cs));
    p_cs->charSetType = 0;
}

static void pfs_udf_mk_set_long_ad(pfs_udf_long_ad_t *p_la, uint32_t part_lba, uint32_t len_bytes)
{
    memset(p_la, 0, sizeof(*p_la));
    pfs_udf_mk_wle32(&p_la->extLength, len_bytes | PFS_UDF_EXT_RECORDED_ALLOC);
    pfs_udf_mk_wle32(&p_la->extLocation.logicalBlockNum, part_lba);
    pfs_udf_mk_wle16(&p_la->extLocation.partitionReferenceNum, PFS_UDF_MK_PARTITION_NUM);
}

static void pfs_udf_mk_set_short_ad(pfs_udf_short_ad_t *p_sa, uint32_t part_lba,
        uint32_t len_bytes)
{
    memset(p_sa, 0, sizeof(*p_sa));
    pfs_udf_mk_wle32(&p_sa->extLength, len_bytes | PFS_UDF_EXT_RECORDED_ALLOC);
    pfs_udf_mk_wle32(&p_sa->extPosition, part_lba);
}

static int pfs_udf_mk_utf8_next_cp(const char *s, int *p_i, uint32_t *p_cp)
{
    const unsigned char *p;
    unsigned char c0;
    int i;

    if (NULL == s || NULL == p_i || NULL == p_cp)
    {
        return PFS_UDF_RET_EINVAL;
    }
    i = *p_i;
    if ('\0' == s[i])
    {
        return PFS_UDF_RET_EBADMSG;
    }
    p = (const unsigned char *)s;
    c0 = p[i];
    if (c0 < 0x80u)
    {
        *p_cp = (uint32_t)c0;
        *p_i = i + 1;
        return PFS_UDF_RET_OK;
    }
    if ((c0 & 0xE0u) == 0xC0u)
    {
        if (0 == p[i + 1])
        {
            return PFS_UDF_RET_EILSEQ;
        }
        *p_cp = ((uint32_t)(c0 & 0x1Fu) << 6) | (uint32_t)(p[i + 1] & 0x3Fu);
        *p_i = i + 2;
        return PFS_UDF_RET_OK;
    }
    if ((c0 & 0xF0u) == 0xE0u)
    {
        if (0 == p[i + 1] || 0 == p[i + 2])
        {
            return PFS_UDF_RET_EILSEQ;
        }
        *p_cp = ((uint32_t)(c0 & 0x0Fu) << 12) |
                ((uint32_t)(p[i + 1] & 0x3Fu) << 6) |
                (uint32_t)(p[i + 2] & 0x3Fu);
        *p_i = i + 3;
        return PFS_UDF_RET_OK;
    }
    if ((c0 & 0xF8u) == 0xF0u)
    {
        if (0 == p[i + 1] || 0 == p[i + 2] || 0 == p[i + 3])
        {
            return PFS_UDF_RET_EILSEQ;
        }
        *p_cp = ((uint32_t)(c0 & 0x07u) << 18) |
                ((uint32_t)(p[i + 1] & 0x3Fu) << 12) |
                ((uint32_t)(p[i + 2] & 0x3Fu) << 6) |
                (uint32_t)(p[i + 3] & 0x3Fu);
        *p_i = i + 4;
        return PFS_UDF_RET_OK;
    }
    return PFS_UDF_RET_EILSEQ;
}

static int pfs_udf_mk_utf8_to_cps(const char *utf8, uint32_t *cps, int cps_cap, int *p_n)
{
    int i;
    int n;

    if (NULL == utf8 || NULL == cps || cps_cap <= 0 || NULL == p_n)
    {
        return PFS_UDF_RET_EINVAL;
    }
    i = 0;
    n = 0;
    if (0 == utf8[0])
    {
        *p_n = 0;
        return PFS_UDF_RET_OK;
    }
    if ((unsigned char)utf8[0] == 0xEFu &&
            (unsigned char)utf8[1] == 0xBBu &&
            (unsigned char)utf8[2] == 0xBFu)
    {
        i = 3;
    }
    while ('\0' != utf8[i])
    {
        uint32_t cp;
        if (n >= cps_cap)
        {
            return PFS_UDF_RET_ENAMETOOLONG;
        }
        int rc = pfs_udf_mk_utf8_next_cp(utf8, &i, &cp);
        if (PFS_UDF_RET_OK != rc)
        {
            return rc;
        }
        if (0xFEFFu == cp)
        {
            continue;
        }
        cps[n] = cp;
        n++;
    }
    *p_n = n;
    return PFS_UDF_RET_OK;
}

static int pfs_udf_mk_cs0_encode(const char *utf8, uint8_t *out, int out_cap, int *p_len)
{
    uint32_t cps[128];
    int n_cp;
    int use16;
    int o;

    if (NULL == utf8 || NULL == out || out_cap <= 1 || NULL == p_len)
    {
        return PFS_UDF_RET_EINVAL;
    }
    int rc = pfs_udf_mk_utf8_to_cps(utf8, cps, (int)(sizeof(cps) / sizeof(cps[0])), &n_cp);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    use16 = 0;
    for (int i = 0; i < n_cp; i++)
    {
        if (cps[i] >= 0x100u)
        {
            use16 = 1;
            break;
        }
    }
    if (0 == use16)
    {
        if (1 + n_cp > out_cap)
        {
            return PFS_UDF_RET_ENAMETOOLONG;
        }
        out[0] = 8;
        for (int i = 0; i < n_cp; i++)
        {
            out[1 + i] = (uint8_t)cps[i];
        }
        *p_len = 1 + n_cp;
        return PFS_UDF_RET_OK;
    }
    if (1 + n_cp * 2 > out_cap)
    {
        return PFS_UDF_RET_ENAMETOOLONG;
    }
    out[0] = 16;
    o = 1;
    for (int i = 0; i < n_cp; i++)
    {
        pfs_write_be16(out + o, (uint16_t)cps[i]);
        o += 2;
    }
    *p_len = o;
    return PFS_UDF_RET_OK;
}

static int pfs_udf_mk_fid_round_up(int len)
{
    return (len + 3) & ~3;
}

static int pfs_udf_mk_fid_bytes(const char *name_utf8, int is_dir, int is_parent)
{
    uint8_t cs0[PFS_UDF_MK_CS0_MAX];
    int cs0_len;
    int chars;

    cs0_len = 0;
    if (is_parent)
    {
        cs0[0] = 8;
        cs0[1] = '.';
        cs0_len = 2;
    }
    else
    {
        int rc = pfs_udf_mk_cs0_encode(name_utf8, cs0, (int)sizeof(cs0), &cs0_len);
        if (PFS_UDF_RET_OK != rc)
        {
            return rc;
        }
    }
    chars = (int)sizeof(pfs_udf_file_ident_desc_t) + cs0_len;
    if (chars > PFS_UDF_MK_BLOCK_SIZE)
    {
        return PFS_UDF_RET_ENAMETOOLONG;
    }
    (void)is_dir;
    return pfs_udf_mk_fid_round_up(chars);
}

static int pfs_udf_mk_check_file_size(uint64_t bytes)
{
    if (bytes > 0xFFFFFFFFull)
    {
        return PFS_UDF_RET_ERANGE;
    }
    return PFS_UDF_RET_OK;
}

static uint32_t pfs_udf_mk_blocks_for_bytes(uint32_t bytes)
{
    if (0 == bytes)
    {
        return 0;
    }
    return (bytes + PFS_UDF_MK_BLOCK_SIZE - 1u) / PFS_UDF_MK_BLOCK_SIZE;
}

static int pfs_udf_mk_path_join(char *out, int out_cap, const char *dir, const char *name)
{
    int n;

    if (NULL == out || out_cap <= 1 || NULL == dir || NULL == name)
    {
        return PFS_UDF_RET_EINVAL;
    }
    n = (int)strlen(dir);
    if (n <= 0)
    {
        return PFS_UDF_RET_EINVAL;
    }
#ifdef _WIN32
    if (n + 1 + (int)strlen(name) + 1 > out_cap)
    {
        return PFS_UDF_RET_ENAMETOOLONG;
    }
    if ('\\' == dir[n - 1] || '/' == dir[n - 1])
    {
        (void)snprintf(out, (size_t)out_cap, "%s%s", dir, name);
    }
    else
    {
        (void)snprintf(out, (size_t)out_cap, "%s\\%s", dir, name);
    }
#else
    if (n + 1 + (int)strlen(name) + 1 > out_cap)
    {
        return PFS_UDF_RET_ENAMETOOLONG;
    }
    if ('/' == dir[n - 1])
    {
        (void)snprintf(out, (size_t)out_cap, "%s%s", dir, name);
    }
    else
    {
        (void)snprintf(out, (size_t)out_cap, "%s/%s", dir, name);
    }
#endif
    return PFS_UDF_RET_OK;
}

static int pfs_udf_mk_add_node(pfs_udf_mk_ctx_t *p_ctx, int parent_idx, const char *host_path,
        const char *name, int is_dir, uint64_t file_size)
{
    pfs_udf_mk_node_t *p_n;
    int idx;

    if (0 == is_dir)
    {
        int rc = pfs_udf_mk_check_file_size(file_size);
        if (PFS_UDF_RET_OK != rc)
        {
            return rc;
        }
    }
    if (p_ctx->n_nodes >= PFS_UDF_MK_MKVOL_MAX_NODES)
    {
        return PFS_UDF_RET_ENOSPC;
    }
    if (p_ctx->n_nodes >= p_ctx->n_cap)
    {
        int ncap = (p_ctx->n_cap > 0) ? (p_ctx->n_cap * 2) : 64;
        pfs_udf_mk_node_t *p_new;

        if (ncap > PFS_UDF_MK_MKVOL_MAX_NODES)
        {
            ncap = PFS_UDF_MK_MKVOL_MAX_NODES;
        }
        p_new = (pfs_udf_mk_node_t *)pfs_realloc(p_ctx->nodes,
                (size_t)ncap * sizeof(pfs_udf_mk_node_t));
        if (NULL == p_new)
        {
            return PFS_UDF_RET_ENOMEM;
        }
        p_ctx->nodes = p_new;
        p_ctx->n_cap = ncap;
    }

    idx = p_ctx->n_nodes;
    p_n = &p_ctx->nodes[idx];
    memset(p_n, 0, sizeof(*p_n));
    (void)strncpy(p_n->host_path, host_path, sizeof(p_n->host_path) - 1);
    if (NULL != name)
    {
        (void)strncpy(p_n->name, name, sizeof(p_n->name) - 1);
    }
    p_n->is_dir = is_dir ? 1 : 0;
    p_n->file_size = file_size;
    p_n->parent_idx = parent_idx;
    p_n->child_head = -1;
    p_n->sibling = -1;

    if (parent_idx >= 0)
    {
        pfs_udf_mk_node_t *p_par = &p_ctx->nodes[parent_idx];
        p_n->sibling = p_par->child_head;
        p_par->child_head = idx;
    }
    p_ctx->n_nodes++;
    return idx;
}

static int pfs_udf_mk_scan_dir(pfs_udf_mk_ctx_t *p_ctx, int dir_idx);

#ifdef _WIN32
static int pfs_udf_mk_utf8_to_wide(const char *utf8, wchar_t **pp_w)
{
    int n;
    wchar_t *p_w;

    *pp_w = NULL;
    n = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    if (n <= 0)
    {
        return PFS_UDF_RET_EILSEQ;
    }
    p_w = (wchar_t *)pfs_malloc((size_t)n * sizeof(wchar_t));
    if (NULL == p_w)
    {
        return PFS_UDF_RET_ENOMEM;
    }
    if (0 == MultiByteToWideChar(CP_UTF8, 0, utf8, -1, p_w, n))
    {
        pfs_free(p_w);
        return PFS_UDF_RET_EILSEQ;
    }
    *pp_w = p_w;
    return PFS_UDF_RET_OK;
}

static int pfs_udf_mk_wide_to_utf8(const wchar_t *wide, char *out, int out_cap)
{
    int n;

    n = WideCharToMultiByte(CP_UTF8, 0, wide, -1, out, out_cap, NULL, NULL);
    if (n <= 0)
    {
        return PFS_UDF_RET_EILSEQ;
    }
    return PFS_UDF_RET_OK;
}

static int pfs_udf_mk_scan_dir(pfs_udf_mk_ctx_t *p_ctx, int dir_idx)
{
    wchar_t *p_wdir;
    wchar_t pattern[PFS_UDF_MK_MKVOL_PATH_MAX + 8];
    WIN32_FIND_DATAW fd;
    HANDLE h;

    int rc = pfs_udf_mk_utf8_to_wide(p_ctx->nodes[dir_idx].host_path, &p_wdir);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    (void)_snwprintf(pattern, (sizeof(pattern) / sizeof(pattern[0])) - 1, L"%s\\*", p_wdir);
    pattern[(sizeof(pattern) / sizeof(pattern[0])) - 1] = L'\0';
    pfs_free(p_wdir);

    h = FindFirstFileW(pattern, &fd);
    if (INVALID_HANDLE_VALUE == h)
    {
        return PFS_UDF_RET_ENOENT;
    }
    do {
        char name_utf8[PFS_NAME_MAX + 1];
        char child_path[PFS_UDF_MK_MKVOL_PATH_MAX];
        int is_dir;
        uint64_t fsz;
        int child_idx;

        if (0 == wcscmp(fd.cFileName, L".") || 0 == wcscmp(fd.cFileName, L".."))
        {
            continue;
        }
        rc = pfs_udf_mk_wide_to_utf8(fd.cFileName, name_utf8, (int)sizeof(name_utf8));
        if (PFS_UDF_RET_OK != rc)
        {
            FindClose(h);
            return rc;
        }
        is_dir = (0 != (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) ? 1 : 0;
        if (0 != (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
        {
            continue;
        }
        fsz = 0;
        if (0 == is_dir)
        {
            fsz = ((uint64_t)fd.nFileSizeHigh << 32) | (uint64_t)fd.nFileSizeLow;
        }
        rc = pfs_udf_mk_path_join(child_path, (int)sizeof(child_path),
                p_ctx->nodes[dir_idx].host_path, name_utf8);
        if (PFS_UDF_RET_OK != rc)
        {
            FindClose(h);
            return rc;
        }
        child_idx = pfs_udf_mk_add_node(p_ctx, dir_idx, child_path, name_utf8, is_dir, fsz);
        if (child_idx < 0)
        {
            FindClose(h);
            return child_idx;
        }
        if (is_dir)
        {
            rc = pfs_udf_mk_scan_dir(p_ctx, child_idx);
            if (PFS_UDF_RET_OK != rc)
            {
                FindClose(h);
                return rc;
            }
        }
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    return PFS_UDF_RET_OK;
}
#else
static int pfs_udf_mk_scan_dir(pfs_udf_mk_ctx_t *p_ctx, int dir_idx)
{
    DIR *p_d;
    struct dirent *p_de;

    p_d = opendir(p_ctx->nodes[dir_idx].host_path);
    if (NULL == p_d)
    {
        return PFS_UDF_RET_ENOENT;
    }
    while (NULL != (p_de = readdir(p_d)))
    {
        char child_path[PFS_UDF_MK_MKVOL_PATH_MAX];
        struct stat st;
        int is_dir;
        int child_idx;

        if (0 == strcmp(p_de->d_name, ".") || 0 == strcmp(p_de->d_name, ".."))
        {
            continue;
        }
        int rc = pfs_udf_mk_path_join(child_path, (int)sizeof(child_path),
                p_ctx->nodes[dir_idx].host_path, p_de->d_name);
        if (PFS_UDF_RET_OK != rc)
        {
            closedir(p_d);
            return rc;
        }
        if (0 != lstat(child_path, &st))
        {
            closedir(p_d);
            return PFS_UDF_RET_EIO;
        }
        if (0 != S_ISLNK(st.st_mode))
        {
            continue;
        }
        is_dir = S_ISDIR(st.st_mode) ? 1 : 0;
        if (!is_dir && !S_ISREG(st.st_mode))
        {
            continue;
        }
        child_idx = pfs_udf_mk_add_node(p_ctx, dir_idx, child_path, p_de->d_name, is_dir,
                is_dir ? 0 : (uint64_t)st.st_size);
        if (child_idx < 0)
        {
            closedir(p_d);
            return child_idx;
        }
        if (is_dir)
        {
            rc = pfs_udf_mk_scan_dir(p_ctx, child_idx);
            if (PFS_UDF_RET_OK != rc)
            {
                closedir(p_d);
                return rc;
            }
        }
    }
    closedir(p_d);
    return PFS_UDF_RET_OK;
}
#endif

static int pfs_udf_mk_dir_stream_size(pfs_udf_mk_ctx_t *p_ctx, int dir_idx, uint32_t *p_size)
{
    int total;

    int rc = pfs_udf_mk_fid_bytes("", 1, 1);
    if (rc < 0)
    {
        return rc;
    }
    total = rc;
    for (int ci = p_ctx->nodes[dir_idx].child_head; ci >= 0; ci = p_ctx->nodes[ci].sibling)
    {
        rc = pfs_udf_mk_fid_bytes(p_ctx->nodes[ci].name, p_ctx->nodes[ci].is_dir, 0);
        if (rc < 0)
        {
            return rc;
        }
        total += rc;
    }
    if (0 == total)
    {
        total = PFS_UDF_MK_BLOCK_SIZE;
    }
    *p_size = (uint32_t)total;
    return PFS_UDF_RET_OK;
}

static int pfs_udf_mk_assign_layout(pfs_udf_mk_ctx_t *p_ctx)
{
    uint32_t next_lba;

    p_ctx->fsd_lba = 0;
    next_lba = 1;
    for (int i = 0; i < p_ctx->n_nodes; i++)
    {
        pfs_udf_mk_node_t *p_n = &p_ctx->nodes[i];
        p_n->fe_lba = next_lba++;
        if (p_n->is_dir)
        {
            int rc = pfs_udf_mk_dir_stream_size(p_ctx, i, &p_n->data_len);
            if (PFS_UDF_RET_OK != rc)
            {
                return rc;
            }
            p_n->data_blocks = pfs_udf_mk_blocks_for_bytes(p_n->data_len);
            if (0 == p_n->data_blocks)
            {
                p_n->data_blocks = 1;
            }
            p_n->data_lba = next_lba;
            next_lba += p_n->data_blocks;
        }
        else
        {
            p_n->data_len = (uint32_t)p_n->file_size;
            p_n->data_blocks = pfs_udf_mk_blocks_for_bytes(p_n->data_len);
            if (p_n->data_len > 0 && 0 == p_n->data_blocks)
            {
                p_n->data_blocks = 1;
            }
            p_n->data_lba = next_lba;
            next_lba += p_n->data_blocks;
        }
    }
    p_ctx->partition_len = p_ctx->vol_sectors - PFS_UDF_MK_PARTITION_START;
    p_ctx->root_fe_lba = p_ctx->nodes[0].fe_lba;
    p_ctx->root_data_lba = p_ctx->nodes[0].data_lba;
    p_ctx->layout_next_lba = next_lba;
    {
        uint32_t num_bytes;
        uint32_t sbd_bytes;

        num_bytes = (p_ctx->partition_len + 7u) / 8u;
        sbd_bytes = (uint32_t)sizeof(pfs_udf_space_bitmap_desc_t) + num_bytes;
        p_ctx->bitmap_blocks = (sbd_bytes + PFS_UDF_MK_BLOCK_SIZE - 1u) / PFS_UDF_MK_BLOCK_SIZE;
        p_ctx->bitmap_lba = p_ctx->partition_len - p_ctx->bitmap_blocks;
    }
    if (next_lba > p_ctx->bitmap_lba)
    {
        return PFS_UDF_RET_ENOSPC;
    }
    return PFS_UDF_RET_OK;
}

static int pfs_udf_mk_resolve_vol_bytes(pfs_blkio_t *p_blkio, int64_t part_offset,
        uint64_t vol_bytes, uint32_t *p_vol_sectors)
{
    int64_t dev_size;
    uint64_t avail;
    uint32_t sectors;

    if (NULL == p_blkio || part_offset < 0 || NULL == p_vol_sectors)
    {
        return PFS_UDF_RET_EINVAL;
    }
    dev_size = pfs_blkio_get_size(p_blkio);
    if (0 == vol_bytes)
    {
        if (dev_size <= part_offset)
        {
            return PFS_UDF_RET_EINVAL;
        }
        avail = (uint64_t)(dev_size - part_offset);
    }
    else
    {
        avail = vol_bytes;
        if (dev_size > 0 && part_offset + (int64_t)avail > dev_size)
        {
            avail = (uint64_t)(dev_size - part_offset);
        }
    }
    if (avail < (uint64_t)PFS_UDF_MK_MIN_VOL_SECTORS * PFS_UDF_MK_SECTOR_SIZE)
    {
        return PFS_UDF_RET_ENOSPC;
    }
    sectors = (uint32_t)(avail / PFS_UDF_MK_SECTOR_SIZE);
    if ((uint64_t)sectors * PFS_UDF_MK_SECTOR_SIZE > PFS_UDF_VOL_BYTES_MAX)
    {
        sectors = (uint32_t)(PFS_UDF_VOL_BYTES_MAX / PFS_UDF_MK_SECTOR_SIZE);
    }
    *p_vol_sectors = sectors;
    return PFS_UDF_RET_OK;
}

static int pfs_udf_mk_write_vrs(pfs_udf_mk_ctx_t *p_ctx)
{
    uint8_t sec[PFS_UDF_MK_SECTOR_SIZE];

    memset(sec, 0, sizeof(sec));
    memcpy(sec, "BEA01", 5);
    memcpy(sec + 6, "CD001", 5);
    int rc = pfs_udf_mk_write_vol_sector(p_ctx, PFS_UDF_MK_VRS_BEA, sec);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }

    memset(sec, 0, sizeof(sec));
    memcpy(sec, "NSR02", 5);
    memcpy(sec + 6, "UDF 02.01", 9);
    rc = pfs_udf_mk_write_vol_sector(p_ctx, PFS_UDF_MK_VRS_NSR, sec);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }

    memset(sec, 0, sizeof(sec));
    memcpy(sec, "TEA01", 5);
    return pfs_udf_mk_write_vol_sector(p_ctx, PFS_UDF_MK_VRS_TEA, sec);
}

static int pfs_udf_mk_write_pvd(pfs_udf_mk_ctx_t *p_ctx, uint32_t vol_block)
{
    uint8_t blk[PFS_UDF_MK_BLOCK_SIZE];
    pfs_udf_primary_vol_desc_t *p_pvd;
    int crc_len;

    memset(blk, 0, sizeof(blk));
    p_pvd = (pfs_udf_primary_vol_desc_t *)blk;
    crc_len = (int)sizeof(*p_pvd) - (int)sizeof(pfs_udf_tag_t);
    pfs_udf_mk_wle32(&p_pvd->volDescSeqNum, 1);
    pfs_udf_mk_wle32(&p_pvd->primaryVolDescNum, 0);
    pfs_udf_mk_pad_dstring(p_pvd->volIdent, 32, p_ctx->volume_id);
    pfs_udf_mk_wle16(&p_pvd->volSeqNum, PFS_UDF_MK_VOL_SEQ_NUM);
    pfs_udf_mk_wle16(&p_pvd->maxVolSeqNum, PFS_UDF_MK_VOL_SEQ_NUM);
    pfs_udf_mk_wle16(&p_pvd->interchangeLvl, 2);
    pfs_udf_mk_wle16(&p_pvd->maxInterchangeLvl, 3);
    pfs_udf_mk_set_charspec(&p_pvd->descCharSet);
    pfs_udf_mk_set_charspec(&p_pvd->explanatoryCharSet);
    pfs_udf_mk_set_regid(&p_pvd->appIdent, "*PFS UDF");
    memcpy(p_pvd->recordingDateAndTime, p_ctx->timestamp, 12);
    // tag/CRC 须在 body 填完后计算
    pfs_udf_mk_set_tag(&p_pvd->descTag, PFS_UDF_TAG_IDENT_PVD, vol_block, blk,
            (int)sizeof(pfs_udf_tag_t), crc_len);
    return pfs_udf_mk_write_vol_sector(p_ctx, vol_block, blk);
}

static int pfs_udf_mk_write_pd(pfs_udf_mk_ctx_t *p_ctx, uint32_t vol_block)
{
    uint8_t blk[PFS_UDF_MK_BLOCK_SIZE];
    pfs_udf_partition_desc_t *p_pd;
    pfs_udf_partition_header_desc_t *p_phd;
    int crc_len;

    memset(blk, 0, sizeof(blk));
    p_pd = (pfs_udf_partition_desc_t *)blk;
    crc_len = (int)sizeof(*p_pd) - (int)sizeof(pfs_udf_tag_t);
    pfs_udf_mk_wle32(&p_pd->volDescSeqNum, 2);
    pfs_udf_mk_wle16(&p_pd->partitionFlags, 1);
    pfs_udf_mk_wle16(&p_pd->partitionNumber, PFS_UDF_MK_PARTITION_NUM);
    pfs_udf_mk_set_regid(&p_pd->partitionContents, "+NSR02");
    memset(p_pd->partitionContentsUse, 0, sizeof(p_pd->partitionContentsUse));
    p_phd = (pfs_udf_partition_header_desc_t *)p_pd->partitionContentsUse;
    pfs_udf_mk_set_short_ad(&p_phd->unallocSpaceBitmap, p_ctx->bitmap_lba,
            p_ctx->bitmap_blocks * PFS_UDF_MK_BLOCK_SIZE);
    pfs_udf_mk_wle32(&p_pd->accessType, PFS_UDF_PD_ACCESS_REWRITABLE);
    pfs_udf_mk_wle32(&p_pd->partitionStartingLocation, PFS_UDF_MK_PARTITION_START);
    pfs_udf_mk_wle32(&p_pd->partitionLength, p_ctx->partition_len);
    // tag/CRC 须在 body 填完后计算
    pfs_udf_mk_set_tag(&p_pd->descTag, PFS_UDF_TAG_IDENT_PD, vol_block, blk,
            (int)sizeof(pfs_udf_tag_t), crc_len);
    return pfs_udf_mk_write_vol_sector(p_ctx, vol_block, blk);
}

static int pfs_udf_mk_write_lvd(pfs_udf_mk_ctx_t *p_ctx, uint32_t vol_block)
{
    uint8_t blk[PFS_UDF_MK_BLOCK_SIZE];
    pfs_udf_logical_vol_desc_t *p_lvd;
    pfs_udf_partmap1_t *p_m1;
    pfs_udf_long_ad_t *p_fsd_la;
    int base;
    int crc_len;

    memset(blk, 0, sizeof(blk));
    p_lvd = (pfs_udf_logical_vol_desc_t *)blk;
    base = (int)sizeof(pfs_udf_logical_vol_desc_t);
    p_m1 = (pfs_udf_partmap1_t *)(blk + base);
    p_m1->partitionMapType = PFS_UDF_GP_PARTMAP_TYPE1;
    p_m1->partitionMapLength = (uint8_t)sizeof(pfs_udf_partmap1_t);
    pfs_udf_mk_wle16(&p_m1->volSeqNum, PFS_UDF_MK_VOL_SEQ_NUM);
    pfs_udf_mk_wle16(&p_m1->partitionNum, PFS_UDF_MK_PARTITION_NUM);
    crc_len = base + (int)sizeof(pfs_udf_partmap1_t) - (int)sizeof(pfs_udf_tag_t);
    pfs_udf_mk_wle32(&p_lvd->volDescSeqNum, 3);
    pfs_udf_mk_set_charspec(&p_lvd->descCharSet);
    pfs_udf_mk_pad_dstring(p_lvd->logicalVolIdent, 128, p_ctx->volume_id);
    pfs_udf_mk_wle32(&p_lvd->logicalBlockSize, PFS_UDF_MK_BLOCK_SIZE);
    pfs_udf_mk_set_regid(&p_lvd->domainIdent, "*OSTA UDF Compliant");
    p_fsd_la = (pfs_udf_long_ad_t *)p_lvd->logicalVolContentsUse;
    pfs_udf_mk_set_long_ad(p_fsd_la, p_ctx->fsd_lba, PFS_UDF_MK_BLOCK_SIZE);
    pfs_udf_mk_wle32(&p_lvd->mapTableLength, (uint32_t)sizeof(pfs_udf_partmap1_t));
    pfs_udf_mk_wle32(&p_lvd->numPartitionMaps, 1);
    pfs_udf_mk_set_regid(&p_lvd->impIdent, "*PFS UDF");
    // tag/CRC 须在 LVD body + partition map 填完后计算
    pfs_udf_mk_set_tag(&p_lvd->descTag, PFS_UDF_TAG_IDENT_LVD, vol_block, blk,
            (int)sizeof(pfs_udf_tag_t), crc_len);
    return pfs_udf_mk_write_vol_sector(p_ctx, vol_block, blk);
}

static int pfs_udf_mk_write_td(pfs_udf_mk_ctx_t *p_ctx, uint32_t vol_block)
{
    uint8_t blk[PFS_UDF_MK_BLOCK_SIZE];
    pfs_udf_terminating_desc_t *p_td;
    int crc_len;

    memset(blk, 0, sizeof(blk));
    p_td = (pfs_udf_terminating_desc_t *)blk;
    crc_len = (int)sizeof(*p_td) - (int)sizeof(pfs_udf_tag_t);
    pfs_udf_mk_set_tag(&p_td->descTag, PFS_UDF_TAG_IDENT_TD, vol_block, blk,
            (int)sizeof(pfs_udf_tag_t), crc_len);
    return pfs_udf_mk_write_vol_sector(p_ctx, vol_block, blk);
}

static int pfs_udf_mk_write_avdp(pfs_udf_mk_ctx_t *p_ctx)
{
    uint8_t blk[PFS_UDF_MK_BLOCK_SIZE];
    pfs_udf_anchor_vol_desc_ptr_t *p_avdp;
    int crc_len;
    uint32_t main_len;

    memset(blk, 0, sizeof(blk));
    p_avdp = (pfs_udf_anchor_vol_desc_ptr_t *)blk;
    main_len = PFS_UDF_MK_VDS_BLOCKS * PFS_UDF_MK_BLOCK_SIZE;
    crc_len = (int)sizeof(*p_avdp) - (int)sizeof(pfs_udf_tag_t);
    pfs_udf_mk_wle32(&p_avdp->mainVolDescSeqExt.extLocation, PFS_UDF_MK_VDS_START);
    pfs_udf_mk_wle32(&p_avdp->mainVolDescSeqExt.extLength, main_len);
    pfs_udf_mk_wle32(&p_avdp->reserveVolDescSeqExt.extLocation, PFS_UDF_MK_VDS_START);
    pfs_udf_mk_wle32(&p_avdp->reserveVolDescSeqExt.extLength, main_len);
    // tag/CRC 须在 extent 填完后计算
    pfs_udf_mk_set_tag(&p_avdp->descTag, PFS_UDF_TAG_IDENT_AVDP, PFS_UDF_MK_AVDP_SECTOR, blk,
            (int)sizeof(pfs_udf_tag_t), crc_len);
    return pfs_udf_mk_write_vol_sector(p_ctx, PFS_UDF_MK_AVDP_SECTOR, blk);
}

static int pfs_udf_mk_write_fsd(pfs_udf_mk_ctx_t *p_ctx)
{
    uint8_t blk[PFS_UDF_MK_BLOCK_SIZE];
    pfs_udf_file_set_desc_t *p_fsd;
    int crc_len;

    memset(blk, 0, sizeof(blk));
    p_fsd = (pfs_udf_file_set_desc_t *)blk;
    crc_len = (int)sizeof(*p_fsd) - (int)sizeof(pfs_udf_tag_t);
    memcpy(p_fsd->recordingDateAndTime, p_ctx->timestamp, 12);
    pfs_udf_mk_wle16(&p_fsd->interchangeLvl, 2);
    pfs_udf_mk_wle16(&p_fsd->maxInterchangeLvl, 3);
    pfs_udf_mk_wle32(&p_fsd->fileSetNum, 0);
    pfs_udf_mk_wle32(&p_fsd->fileSetDescNum, 0);
    pfs_udf_mk_set_charspec(&p_fsd->logicalVolIdentCharSet);
    pfs_udf_mk_pad_dstring(p_fsd->logicalVolIdent, 128, p_ctx->volume_id);
    pfs_udf_mk_set_charspec(&p_fsd->fileSetCharSet);
    pfs_udf_mk_pad_dstring(p_fsd->fileSetIdent, 32, p_ctx->volume_id);
    pfs_udf_mk_set_long_ad(&p_fsd->rootDirectoryICB, p_ctx->root_fe_lba, PFS_UDF_MK_BLOCK_SIZE);
    pfs_udf_mk_set_regid(&p_fsd->domainIdent, "*OSTA UDF Compliant");
    // tag/CRC 须在 body 填完后计算
    pfs_udf_mk_set_tag(&p_fsd->descTag, PFS_UDF_TAG_IDENT_FSD, p_ctx->fsd_lba, blk,
            (int)sizeof(pfs_udf_tag_t), crc_len);
    return pfs_udf_mk_write_part_lba(p_ctx, p_ctx->fsd_lba, blk);
}

static int pfs_udf_mk_write_fe(pfs_udf_mk_ctx_t *p_ctx, int node_idx)
{
    pfs_udf_mk_node_t *p_n = &p_ctx->nodes[node_idx];
    uint8_t blk[PFS_UDF_MK_BLOCK_SIZE];
    pfs_udf_file_entry_t *p_fe;
    pfs_udf_short_ad_t *p_sa;
    int hdr_sz;
    int crc_len;
    uint64_t uid;

    memset(blk, 0, sizeof(blk));
    p_fe = (pfs_udf_file_entry_t *)blk;
    hdr_sz = (int)sizeof(pfs_udf_file_entry_t);
    crc_len = hdr_sz - (int)sizeof(pfs_udf_tag_t);
    if (p_n->is_dir || p_n->data_len > 0)
    {
        crc_len += (int)sizeof(pfs_udf_short_ad_t);
    }

    pfs_udf_mk_wle16(&p_fe->icbTag.strategyType, 4);
    pfs_udf_mk_wle16(&p_fe->icbTag.numEntries, 1);
    if (p_n->is_dir)
    {
        p_fe->icbTag.fileType = PFS_UDF_ICBTAG_FILE_DIRECTORY;
    }
    else
    {
        p_fe->icbTag.fileType = PFS_UDF_ICBTAG_FILE_REGULAR;
    }
    if (p_n->parent_idx >= 0)
    {
        pfs_udf_mk_wle32(&p_fe->icbTag.parentICBLocation.logicalBlockNum,
                p_ctx->nodes[p_n->parent_idx].fe_lba);
        pfs_udf_mk_wle16(&p_fe->icbTag.parentICBLocation.partitionReferenceNum,
                PFS_UDF_MK_PARTITION_NUM);
    }
    pfs_udf_mk_wle16(&p_fe->icbTag.flags, PFS_UDF_ICBTAG_AD_SHORT);
    pfs_udf_mk_wle32(&p_fe->permissions, 0x1ed);
    pfs_udf_mk_wle16(&p_fe->fileLinkCount, 1);
    pfs_udf_mk_wle64(&p_fe->informationLength, p_n->is_dir ? p_n->data_len : p_n->file_size);
    memcpy(p_fe->accessTime, p_ctx->timestamp, 12);
    memcpy(p_fe->modificationTime, p_ctx->timestamp, 12);
    memcpy(p_fe->attrTime, p_ctx->timestamp, 12);
    uid = p_ctx->next_unique_id++;
    pfs_udf_mk_wle64(&p_fe->uniqueID, uid);

    if (p_n->is_dir || p_n->data_len > 0)
    {
        p_sa = (pfs_udf_short_ad_t *)(blk + hdr_sz);
        pfs_udf_mk_set_short_ad(p_sa, p_n->data_lba, p_n->data_len);
        pfs_udf_mk_wle32(&p_fe->lengthAllocDescs, (uint32_t)sizeof(pfs_udf_short_ad_t));
    }
    else
    {
        pfs_udf_mk_wle32(&p_fe->lengthAllocDescs, 0);
    }
    // tag/CRC 须在 FE body + alloc desc 填完后计算
    pfs_udf_mk_set_tag(&p_fe->descTag, PFS_UDF_TAG_IDENT_FE, p_n->fe_lba, blk,
            (int)sizeof(pfs_udf_tag_t), crc_len);
    return pfs_udf_mk_write_part_lba(p_ctx, p_n->fe_lba, blk);
}

static int pfs_udf_mk_write_fid(pfs_udf_mk_ctx_t *p_ctx, uint8_t *buf, int buf_cap, int *p_off,
        uint32_t data_lba, const char *name_utf8, int is_dir, int is_parent,
        uint32_t icb_lba)
{
    pfs_udf_file_ident_desc_t *p_fid;
    uint8_t cs0[PFS_UDF_MK_CS0_MAX];
    int cs0_len;
    int imp_len;
    int total;
    int off;
    uint8_t chars;

    if (NULL == buf || NULL == p_off)
    {
        return PFS_UDF_RET_EINVAL;
    }
    off = *p_off;
    cs0_len = 0;
    if (is_parent)
    {
        cs0[0] = 8;
        cs0[1] = '.';
        cs0_len = 2;
    }
    else
    {
        int rc = pfs_udf_mk_cs0_encode(name_utf8, cs0, (int)sizeof(cs0), &cs0_len);
        if (PFS_UDF_RET_OK != rc)
        {
            return rc;
        }
    }
    imp_len = 0;
    total = (int)sizeof(pfs_udf_file_ident_desc_t) + imp_len + cs0_len;
    total = pfs_udf_mk_fid_round_up(total);
    if (off + total > buf_cap)
    {
        return PFS_UDF_RET_ENAMETOOLONG;
    }

    p_fid = (pfs_udf_file_ident_desc_t *)(buf + off);
    memset(p_fid, 0, (size_t)total);
    pfs_udf_mk_wle16(&p_fid->fileVersionNum, 1);
    chars = 0;
    if (is_dir)
    {
        chars = (uint8_t)(chars | PFS_UDF_FID_CHAR_DIRECTORY);
    }
    if (is_parent)
    {
        chars = (uint8_t)(chars | PFS_UDF_FID_CHAR_PARENT);
    }
    p_fid->fileCharacteristics = chars;
    p_fid->lengthFileIdent = (uint8_t)cs0_len;
    pfs_udf_mk_set_long_ad(&p_fid->icb, icb_lba, PFS_UDF_MK_BLOCK_SIZE);
    pfs_udf_mk_wle16(&p_fid->lengthOfImpUse, 0);
    if (cs0_len > 0)
    {
        memcpy(buf + off + (int)sizeof(pfs_udf_file_ident_desc_t), cs0, (size_t)cs0_len);
    }
    {
        uint32_t tag_lba = data_lba + (uint32_t)((uint32_t)off / PFS_UDF_MK_BLOCK_SIZE);
        // tag/CRC 须在 FID body + fileIdent 填完后计算
        pfs_udf_mk_set_tag(&p_fid->descTag, PFS_UDF_TAG_IDENT_FID, tag_lba, buf + off,
                (int)sizeof(pfs_udf_tag_t),
                total - (int)sizeof(pfs_udf_tag_t));
    }
    off += total;
    *p_off = off;
    return PFS_UDF_RET_OK;
}

static int pfs_udf_mk_write_dir_data(pfs_udf_mk_ctx_t *p_ctx, int dir_idx)
{
    pfs_udf_mk_node_t *p_n = &p_ctx->nodes[dir_idx];
    uint8_t *buf;
    int off;
    uint32_t parent_fe;

    buf = (uint8_t *)pfs_malloc((size_t)p_n->data_len);
    if (NULL == buf)
    {
        return PFS_UDF_RET_ENOMEM;
    }
    memset(buf, 0, (size_t)p_n->data_len);
    off = 0;
    if (dir_idx > 0 && p_n->parent_idx >= 0)
    {
        parent_fe = p_ctx->nodes[p_n->parent_idx].fe_lba;
    }
    else
    {
        parent_fe = p_n->fe_lba;
    }
    int rc = pfs_udf_mk_write_fid(p_ctx, buf, (int)p_n->data_len, &off, p_n->data_lba, "",
            1, 1, parent_fe);
    if (PFS_UDF_RET_OK != rc)
    {
        pfs_free(buf);
        return rc;
    }
    for (int ci = p_n->child_head; ci >= 0; ci = p_ctx->nodes[ci].sibling)
    {
        pfs_udf_mk_node_t *p_ch = &p_ctx->nodes[ci];
        rc = pfs_udf_mk_write_fid(p_ctx, buf, (int)p_n->data_len, &off, p_n->data_lba,
                p_ch->name, p_ch->is_dir, 0, p_ch->fe_lba);
        if (PFS_UDF_RET_OK != rc)
        {
            pfs_free(buf);
            return rc;
        }
    }
    for (uint32_t b = 0; b < p_n->data_blocks; b++)
    {
        uint32_t chunk;
        uint32_t blk_off;
        chunk = PFS_UDF_MK_BLOCK_SIZE;
        blk_off = b * PFS_UDF_MK_BLOCK_SIZE;
        if (blk_off + chunk > p_n->data_len)
        {
            chunk = p_n->data_len - blk_off;
        }
        memset(p_ctx->block_buf, 0, (size_t)PFS_UDF_MK_BLOCK_SIZE);
        if (chunk > 0)
        {
            memcpy(p_ctx->block_buf, buf + blk_off, (size_t)chunk);
        }
        rc = pfs_udf_mk_write_part_lba(p_ctx, p_n->data_lba + b, p_ctx->block_buf);
        if (PFS_UDF_RET_OK != rc)
        {
            pfs_free(buf);
            return rc;
        }
    }
    pfs_free(buf);
    return PFS_UDF_RET_OK;
}

#ifdef _WIN32
static int pfs_udf_mk_copy_file(pfs_udf_mk_ctx_t *p_ctx, int file_idx)
{
    pfs_udf_mk_node_t *p_n = &p_ctx->nodes[file_idx];
    HANDLE h;
    wchar_t *p_wpath;
    int64_t remain;
    int64_t off;
    DWORD nrd;

    if (0 == p_n->file_size)
    {
        return PFS_UDF_RET_OK;
    }
    int rc = pfs_udf_mk_utf8_to_wide(p_n->host_path, &p_wpath);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    h = CreateFileW(p_wpath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL, NULL);
    pfs_free(p_wpath);
    if (INVALID_HANDLE_VALUE == h)
    {
        return PFS_UDF_RET_EIO;
    }
    remain = (int64_t)p_n->file_size;
    off = 0;
    while (remain > 0)
    {
        int chunk;
        int64_t part_off;
        if (remain > PFS_UDF_MK_MKVOL_COPY_CHUNK)
        {
            chunk = PFS_UDF_MK_MKVOL_COPY_CHUNK;
        }
        else
        {
            chunk = (int)remain;
        }
        if (0 == ReadFile(h, p_ctx->copy_buf, (DWORD)chunk, &nrd, NULL) ||
                (int)nrd != chunk)
        {
            CloseHandle(h);
            return PFS_UDF_RET_EIO;
        }
        part_off = (int64_t)(PFS_UDF_MK_PARTITION_START + p_n->data_lba) * (int64_t)PFS_UDF_MK_BLOCK_SIZE
                + off;
        rc = pfs_udf_mk_pwrite(p_ctx, part_off, p_ctx->copy_buf, chunk);
        if (PFS_UDF_RET_OK != rc)
        {
            CloseHandle(h);
            return rc;
        }
        off += chunk;
        remain -= chunk;
    }
    CloseHandle(h);
    return PFS_UDF_RET_OK;
}
#else
static int pfs_udf_mk_copy_file(pfs_udf_mk_ctx_t *p_ctx, int file_idx)
{
    pfs_udf_mk_node_t *p_n = &p_ctx->nodes[file_idx];
    int fd;
    int64_t remain;
    int64_t off;

    if (0 == p_n->file_size)
    {
        return PFS_UDF_RET_OK;
    }
    fd = open(p_n->host_path, O_RDONLY);
    if (fd < 0)
    {
        return PFS_UDF_RET_EIO;
    }
    remain = (int64_t)p_n->file_size;
    off = 0;
    while (remain > 0)
    {
        int chunk;
        int64_t part_off;
        ssize_t nrd;
        if (remain > PFS_UDF_MK_MKVOL_COPY_CHUNK)
        {
            chunk = PFS_UDF_MK_MKVOL_COPY_CHUNK;
        }
        else
        {
            chunk = (int)remain;
        }
        nrd = read(fd, p_ctx->copy_buf, (size_t)chunk);
        if (nrd != chunk)
        {
            close(fd);
            return PFS_UDF_RET_EIO;
        }
        part_off = (int64_t)(PFS_UDF_MK_PARTITION_START + p_n->data_lba) * (int64_t)PFS_UDF_MK_BLOCK_SIZE
                + off;
        int rc = pfs_udf_mk_pwrite(p_ctx, part_off, p_ctx->copy_buf, chunk);
        if (PFS_UDF_RET_OK != rc)
        {
            close(fd);
            return rc;
        }
        off += chunk;
        remain -= chunk;
    }
    close(fd);
    return PFS_UDF_RET_OK;
}
#endif

static void pfs_udf_mk_sbd_set_bit(uint8_t *buf, uint32_t part_lba, int free_bit)
{
    int bit;
    int byte;
    uint8_t mask;

    bit = (int)part_lba + (int)((int)sizeof(pfs_udf_space_bitmap_desc_t) << 3);
    byte = bit >> 3;
    mask = (uint8_t)(1u << (bit & 7));
    if (free_bit)
    {
        buf[byte] = (uint8_t)(buf[byte] | mask);
    }
    else
    {
        buf[byte] = (uint8_t)(buf[byte] & (uint8_t)~mask);
    }
}

static int pfs_udf_mk_write_sbd(pfs_udf_mk_ctx_t *p_ctx)
{
    pfs_udf_space_bitmap_desc_t *p_sbd;
    uint32_t num_bytes;
    uint32_t buf_size;
    uint8_t *buf;
    uint32_t lba;

    num_bytes = (p_ctx->partition_len + 7u) / 8u;
    buf_size = p_ctx->bitmap_blocks * PFS_UDF_MK_BLOCK_SIZE;
    buf = (uint8_t *)pfs_malloc((size_t)buf_size);
    if (NULL == buf)
    {
        return PFS_UDF_RET_ENOMEM;
    }
    memset(buf, 0, (size_t)buf_size);

    for (lba = 0; lba < p_ctx->layout_next_lba; lba++)
    {
        pfs_udf_mk_sbd_set_bit(buf, lba, 0);
    }
    for (lba = p_ctx->bitmap_lba; lba < p_ctx->partition_len; lba++)
    {
        pfs_udf_mk_sbd_set_bit(buf, lba, 0);
    }
    for (lba = p_ctx->layout_next_lba; lba < p_ctx->bitmap_lba; lba++)
    {
        pfs_udf_mk_sbd_set_bit(buf, lba, 1);
    }

    p_sbd = (pfs_udf_space_bitmap_desc_t *)buf;
    pfs_udf_mk_wle32(&p_sbd->numOfBits, p_ctx->partition_len);
    pfs_udf_mk_wle32(&p_sbd->numOfBytes, num_bytes);
    pfs_udf_mk_set_tag(&p_sbd->descTag, PFS_UDF_TAG_IDENT_SBD, p_ctx->bitmap_lba, buf,
            (int)sizeof(pfs_udf_tag_t),
            (int)sizeof(*p_sbd) - (int)sizeof(pfs_udf_tag_t) + (int)num_bytes);

    for (uint32_t b = 0; b < p_ctx->bitmap_blocks; b++)
    {
        memset(p_ctx->block_buf, 0, (size_t)PFS_UDF_MK_BLOCK_SIZE);
        memcpy(p_ctx->block_buf, buf + (size_t)b * (size_t)PFS_UDF_MK_BLOCK_SIZE,
                (size_t)PFS_UDF_MK_BLOCK_SIZE);
        int rc = pfs_udf_mk_write_part_lba(p_ctx, p_ctx->bitmap_lba + b, p_ctx->block_buf);
        if (PFS_UDF_RET_OK != rc)
        {
            pfs_free(buf);
            return rc;
        }
    }
    pfs_free(buf);
    return PFS_UDF_RET_OK;
}

static int pfs_udf_mk_build(pfs_udf_mk_ctx_t *p_ctx)
{

    int rc = pfs_udf_mk_assign_layout(p_ctx);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }

    rc = pfs_udf_mk_zero_vol_sectors(p_ctx, 0, PFS_UDF_MK_PARTITION_START);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_udf_mk_write_vrs(p_ctx);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_udf_mk_write_pvd(p_ctx, PFS_UDF_MK_VDS_START + 0);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_udf_mk_write_pd(p_ctx, PFS_UDF_MK_VDS_START + 1);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_udf_mk_write_lvd(p_ctx, PFS_UDF_MK_VDS_START + 2);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_udf_mk_write_td(p_ctx, PFS_UDF_MK_VDS_START + 3);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_udf_mk_write_avdp(p_ctx);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_udf_mk_write_fsd(p_ctx);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    for (int i = 0; i < p_ctx->n_nodes; i++)
    {
        rc = pfs_udf_mk_write_fe(p_ctx, i);
        if (PFS_UDF_RET_OK != rc)
        {
            return rc;
        }
    }
    for (int i = 0; i < p_ctx->n_nodes; i++)
    {
        if (p_ctx->nodes[i].is_dir)
        {
            rc = pfs_udf_mk_write_dir_data(p_ctx, i);
            if (PFS_UDF_RET_OK != rc)
            {
                return rc;
            }
        }
    }
    for (int i = 0; i < p_ctx->n_nodes; i++)
    {
        if (0 == p_ctx->nodes[i].is_dir)
        {
            rc = pfs_udf_mk_copy_file(p_ctx, i);
            if (PFS_UDF_RET_OK != rc)
            {
                return rc;
            }
        }
    }
    rc = pfs_udf_mk_write_sbd(p_ctx);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_blkio_fsync(p_ctx->p_blkio);
    if (rc < 0)
    {
        return pfs_udf_mk_map_io(rc);
    }
    return PFS_UDF_RET_OK;
}

static void pfs_udf_mk_ctx_free(pfs_udf_mk_ctx_t *p_ctx)
{
    if (NULL == p_ctx)
    {
        return;
    }
    if (NULL != p_ctx->nodes)
    {
        pfs_free(p_ctx->nodes);
        p_ctx->nodes = NULL;
    }
    if (NULL != p_ctx->copy_buf)
    {
        pfs_free(p_ctx->copy_buf);
        p_ctx->copy_buf = NULL;
    }
    if (NULL != p_ctx->block_buf)
    {
        pfs_free(p_ctx->block_buf);
        p_ctx->block_buf = NULL;
    }
}

static void pfs_udf_mk_init_ctx(pfs_udf_mk_ctx_t *p_ctx, pfs_blkio_t *p_blkio,
        int64_t part_offset, const char *volume_id)
{
    memset(p_ctx, 0, sizeof(*p_ctx));
    p_ctx->p_blkio = p_blkio;
    p_ctx->part_offset = part_offset;
    pfs_udf_mk_fill_timestamp(p_ctx->timestamp);
    if (NULL != volume_id && '\0' != volume_id[0])
    {
        (void)strncpy(p_ctx->volume_id, volume_id, sizeof(p_ctx->volume_id) - 1);
    }
    else
    {
        (void)strncpy(p_ctx->volume_id, "PFS_UDF", sizeof(p_ctx->volume_id) - 1);
    }
}

int pfs_udf_mkvol(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes, const char *src_dir,
        const char *volume_id)
{
    pfs_udf_mk_ctx_t ctx;
    int root_idx;

    if (NULL == p_blkio || part_offset < 0 || NULL == src_dir || '\0' == src_dir[0])
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (!pfs_blkio_is_open(p_blkio))
    {
        return PFS_UDF_RET_EINVAL;
    }

    memset(&ctx, 0, sizeof(ctx));
    pfs_udf_mk_init_ctx(&ctx, p_blkio, part_offset, volume_id);
    int rc = pfs_udf_mk_resolve_vol_bytes(p_blkio, part_offset, vol_bytes, &ctx.vol_sectors);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }

    ctx.copy_buf = (uint8_t *)pfs_malloc(PFS_UDF_MK_MKVOL_COPY_CHUNK);
    ctx.block_buf = (uint8_t *)pfs_malloc(PFS_UDF_MK_BLOCK_SIZE);
    if (NULL == ctx.copy_buf || NULL == ctx.block_buf)
    {
        pfs_udf_mk_ctx_free(&ctx);
        return PFS_UDF_RET_ENOMEM;
    }

    root_idx = pfs_udf_mk_add_node(&ctx, -1, src_dir, "", 1, 0);
    if (root_idx < 0)
    {
        pfs_udf_mk_ctx_free(&ctx);
        return root_idx;
    }
    rc = pfs_udf_mk_scan_dir(&ctx, root_idx);
    if (PFS_UDF_RET_OK != rc)
    {
        pfs_udf_mk_ctx_free(&ctx);
        return rc;
    }
    rc = pfs_udf_mk_build(&ctx);
    pfs_udf_mk_ctx_free(&ctx);
    return rc;
}

int pfs_udf_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes)
{
    pfs_udf_mk_ctx_t ctx;
    int root_idx;
    char dot_root[4];

    if (NULL == p_blkio || part_offset < 0)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (!pfs_blkio_is_open(p_blkio))
    {
        return PFS_UDF_RET_EINVAL;
    }

    memset(&ctx, 0, sizeof(ctx));
    pfs_udf_mk_init_ctx(&ctx, p_blkio, part_offset, NULL);
    int rc = pfs_udf_mk_resolve_vol_bytes(p_blkio, part_offset, vol_bytes, &ctx.vol_sectors);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }

    ctx.block_buf = (uint8_t *)pfs_malloc(PFS_UDF_MK_BLOCK_SIZE);
    if (NULL == ctx.block_buf)
    {
        return PFS_UDF_RET_ENOMEM;
    }

    (void)strncpy(dot_root, ".", sizeof(dot_root) - 1);
    root_idx = pfs_udf_mk_add_node(&ctx, -1, dot_root, "", 1, 0);
    if (root_idx < 0)
    {
        pfs_udf_mk_ctx_free(&ctx);
        return root_idx;
    }
    rc = pfs_udf_mk_build(&ctx);
    pfs_udf_mk_ctx_free(&ctx);
    return rc;
}

// end
