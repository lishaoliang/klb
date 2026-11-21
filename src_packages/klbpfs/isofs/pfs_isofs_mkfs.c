// Doc Encode : UTF-8 BOM, Unix(LF)
// ISO 9660 mkfs / mkvol (Level 2 Primary; 可选 Joliet SVD + Rock Ridge SUSP)
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "isofs/pfs_isofs_internal.h"
#include "base/pfs_mem.h"
#include "pfs_endian.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#define PFS_ISOFS_MKVOL_MAX_NODES       8192
#define PFS_ISOFS_MKVOL_PATH_MAX        1024
#define PFS_ISOFS_MKVOL_ISO_NAME_MAX    31
#define PFS_ISOFS_MKVOL_COPY_CHUNK      (64 * 1024)
#define PFS_ISOFS_MKVOL_HOST_NAME_MAX   256
#define PFS_ISOFS_MKVOL_JOLIET_CHARS    64
#define PFS_ISOFS_MKVOL_JOLIET_NAME_MAX   (PFS_ISOFS_MKVOL_JOLIET_CHARS * 2)
#define PFS_ISOFS_MKVOL_SUSP_SP_LEN     7
#define PFS_ISOFS_MKVOL_SUSP_ER_ID      "RRIP_1991A"
// ER: sig(2)+len+ver+len_id+len_des+len_src+ext_ver + id (IEEE P1281)
#define PFS_ISOFS_MKVOL_SUSP_ER_ID_LEN  ((int)sizeof(PFS_ISOFS_MKVOL_SUSP_ER_ID) - 1)
#define PFS_ISOFS_MKVOL_SUSP_ER_LEN     (8 + PFS_ISOFS_MKVOL_SUSP_ER_ID_LEN)
#define PFS_ISOFS_MKVOL_SUSP_ROOT_LEN   (PFS_ISOFS_MKVOL_SUSP_SP_LEN + PFS_ISOFS_MKVOL_SUSP_ER_LEN)

typedef struct _pfs_isofs_mk_node_t {
    char                    host_path[PFS_ISOFS_MKVOL_PATH_MAX];
    char                    host_name[PFS_ISOFS_MKVOL_HOST_NAME_MAX];
    char                    iso_name[PFS_ISOFS_MKVOL_ISO_NAME_MAX + 1];
    int                     is_dir;
    uint64_t                file_size;                  // 目录=Primary 记录区字节; 文件=数据字节
    uint64_t                joliet_dir_size;            // 目录=Joliet 记录区字节
    uint32_t                extent;                     // Primary 目录或文件数据 LBA
    uint32_t                joliet_extent;              // Joliet 目录 LBA
    int                     parent_idx;                 // -1=根
    int                     child_head;                 // 子链表头 idx; -1 无
    int                     sibling;                    // 下一兄弟 idx; -1 无
} pfs_isofs_mk_node_t;

typedef struct _pfs_isofs_mk_ctx_t {
    pfs_blkio_t            *p_blkio;
    int64_t                 part_offset;
    char                    volume_id[33];              // 卷标 C 串 (写入时再垫空格)
    int                     joliet_level;               // 0=无; 1..3
    int                     rock_ridge;                 // 0/1
    pfs_isofs_mk_node_t    *nodes;
    int                     n_nodes;
    int                     n_cap;
    uint8_t                 date7[7];
    uint8_t                *copy_buf;
    uint32_t                pri_pt_lba;                 // Primary Type L path table LBA
    uint32_t                pri_pt_size;                // Primary path table 字节数
    uint32_t                jol_pt_lba;                 // Joliet Type L path table LBA
    uint32_t                jol_pt_size;                // Joliet path table 字节数
} pfs_isofs_mk_ctx_t;

static void pfs_isofs_mk_put_711(uint8_t *p, uint8_t v)
{
    p[0] = v;
}

static void pfs_isofs_mk_put_723(uint8_t *p, uint16_t v)
{
    pfs_write_le16(p, v);
    pfs_write_be16(p + 2, v);
}

static void pfs_isofs_mk_put_733(uint8_t *p, uint32_t v)
{
    pfs_write_le32(p, v);
    pfs_write_be32(p + 4, v);
}

// ISO 721 / 731: Path Table Type L 仅小端
static void pfs_isofs_mk_put_721(uint8_t *p, uint16_t v)
{
    pfs_write_le16(p, v);
}

static void pfs_isofs_mk_put_731(uint8_t *p, uint32_t v)
{
    pfs_write_le32(p, v);
}

static int pfs_isofs_mk_pt_entry_len(int name_len)
{
    if (0 > name_len)
    {
        name_len = 0;
    }
    int n = 8 + name_len;
    if (0 != (name_len & 1))
    {
        n++;
    }
    return n;
}

static int pfs_isofs_mk_map_io(int rc)
{
    return pfs_isofs_map_blkio_rc(rc);
}

static int pfs_isofs_mk_pwrite(pfs_isofs_mk_ctx_t *p_ctx, int64_t off, const void *buf, int size)
{

    int rc = pfs_blkio_pwrite(p_ctx->p_blkio, buf, size, p_ctx->part_offset + off);
    if (0 > rc)
    {
        return pfs_isofs_mk_map_io(rc);
    }
    else if (rc != size)
    {
        return PFS_ISOFS_RET_EIO;
    }
    return PFS_ISOFS_RET_OK;
}

static int pfs_isofs_mk_zero_lba(pfs_isofs_mk_ctx_t *p_ctx, uint32_t lba, uint32_t count)
{
    uint8_t z[PFS_ISOFS_BLOCK_SIZE];

    memset(z, 0, sizeof(z));
    for (uint32_t i = 0; i < count; i++)
    {
        int rc = pfs_isofs_mk_pwrite(p_ctx, (int64_t)(lba + i) * PFS_ISOFS_BLOCK_SIZE, z,
                PFS_ISOFS_BLOCK_SIZE);
        if (0 > rc)
        {
            return rc;
        }
    }
    return PFS_ISOFS_RET_OK;
}

static void pfs_isofs_mk_fill_date7(uint8_t date7[7])
{
    time_t now;
    struct tm *p_tm;
    struct tm tmb;

    now = time(NULL);
#ifdef _WIN32
    if (0 != localtime_s(&tmb, &now))
    {
        memset(date7, 0, 7);
        return;
    }
    p_tm = &tmb;
#else
    p_tm = localtime_r(&now, &tmb);
    if (NULL == p_tm)
    {
        memset(date7, 0, 7);
        return;
    }
#endif
    date7[0] = (uint8_t)(p_tm->tm_year);                // since 1900
    date7[1] = (uint8_t)(p_tm->tm_mon + 1);
    date7[2] = (uint8_t)(p_tm->tm_mday);
    date7[3] = (uint8_t)(p_tm->tm_hour);
    date7[4] = (uint8_t)(p_tm->tm_min);
    date7[5] = (uint8_t)(p_tm->tm_sec);
    date7[6] = 0;                                       // GMT offset 15min units
}

static void pfs_isofs_mk_pad_str(char *dst, int dst_len, const char *src, char pad)
{
    if (NULL == src)
    {
        src = "";
    }
    int n = (int)strlen(src);
    if (n > dst_len)
    {
        n = dst_len;
    }
    for (int i = 0; i < n; i++)
    {
        char c = src[i];
        if (c >= 'a' && c <= 'z')
        {
            c = (char)(c - 'a' + 'A');
        }
        dst[i] = c;
    }
    for (int i = n; i < dst_len; i++)
    {
        dst[i] = pad;
    }
}

static int pfs_isofs_mk_is_dchar(unsigned char c)
{
    if (c >= 'A' && c <= 'Z')
    {
        return 1;
    }
    if (c >= '0' && c <= '9')
    {
        return 1;
    }
    if ('_' == c)
    {
        return 1;
    }
    return 0;
}

// Level 2 名: 大写 + 数字 + _; 最长 31; 冲突由调用方处理
static void pfs_isofs_mk_make_iso_name(const char *host_name, char *iso_out, int iso_cap)
{
    char tmp[PFS_ISOFS_MKVOL_ISO_NAME_MAX + 1];

    if (1 >= iso_cap)
    {
        return;
    }
    int o = 0;
    int dot = -1;
    for (int i = 0; host_name[i] != '\0' && o < PFS_ISOFS_MKVOL_ISO_NAME_MAX; i++)
    {
        unsigned char c = (unsigned char)host_name[i];
        if (c >= 'a' && c <= 'z')
        {
            c = (unsigned char)(c - 'a' + 'A');
        }
        if ('.' == c)
        {
            if (dot >= 0)
            {
                continue;
            }
            dot = o;
            tmp[o] = '.';
            o++;
            continue;
        }
        if (!pfs_isofs_mk_is_dchar(c))
        {
            c = '_';
        }
        tmp[o] = (char)c;
        o++;
    }
    tmp[o] = '\0';
    if (0 == o)
    {
        (void)strncpy(iso_out, "FILE", (size_t)iso_cap - 1);
        iso_out[iso_cap - 1] = '\0';
        return;
    }
    (void)strncpy(iso_out, tmp, (size_t)iso_cap - 1);
    iso_out[iso_cap - 1] = '\0';
}

static int pfs_isofs_mk_path_join(char *out, int out_cap, const char *dir, const char *name)
{
    if (NULL == out || 1 >= out_cap || NULL == dir || NULL == name)
    {
        return PFS_ISOFS_RET_EINVAL;
    }
    int n = (int)strlen(dir);
    if (n <= 0)
    {
        return PFS_ISOFS_RET_EINVAL;
    }
#ifdef _WIN32
    if (n + 1 + (int)strlen(name) + 1 > out_cap)
    {
        return PFS_ISOFS_RET_ENAMETOOLONG;
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
        return PFS_ISOFS_RET_ENAMETOOLONG;
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
    return PFS_ISOFS_RET_OK;
}

static int pfs_isofs_mk_clamp_joliet(int level)
{
    if (level >= 1 && level <= 3)
    {
        return level;
    }
    if (0 == level)
    {
        return 0;
    }
    return PFS_ISOFS_MKVOL_JOLIET_DEFAULT;
}

static int pfs_isofs_mk_clamp_rock(int rock)
{
    return (0 != rock) ? 1 : 0;
}

// ISO9660 DE size 为 32 位; 超限拒绝 (勿静默截断)
static int pfs_isofs_mk_check_iso_size(uint64_t bytes)
{
    if (bytes > 0xFFFFFFFFull)
    {
        return PFS_ISOFS_RET_ERANGE;
    }
    return PFS_ISOFS_RET_OK;
}

static int pfs_isofs_mk_add_node(pfs_isofs_mk_ctx_t *p_ctx, int parent_idx, const char *host_path,
        const char *host_name, const char *iso_name, int is_dir, uint64_t file_size)
{
    pfs_isofs_mk_node_t *p_n;
    int idx;

    if (0 == is_dir)
    {
        int rc = pfs_isofs_mk_check_iso_size(file_size);
        if (0 > rc)
        {
            return rc;
        }
    }
    if (p_ctx->n_nodes >= PFS_ISOFS_MKVOL_MAX_NODES)
    {
        return PFS_ISOFS_RET_ENOSPC;
    }
    if (p_ctx->n_nodes >= p_ctx->n_cap)
    {
        int ncap = (p_ctx->n_cap > 0) ? (p_ctx->n_cap * 2) : 64;
        pfs_isofs_mk_node_t *p_new;

        if (ncap > PFS_ISOFS_MKVOL_MAX_NODES)
        {
            ncap = PFS_ISOFS_MKVOL_MAX_NODES;
        }
        p_new = (pfs_isofs_mk_node_t *)pfs_realloc(p_ctx->nodes,
                (size_t)ncap * sizeof(pfs_isofs_mk_node_t));
        if (NULL == p_new)
        {
            return PFS_ISOFS_RET_ENOMEM;
        }
        p_ctx->nodes = p_new;
        p_ctx->n_cap = ncap;
    }

    idx = p_ctx->n_nodes;
    p_n = &p_ctx->nodes[idx];
    memset(p_n, 0, sizeof(*p_n));
    (void)strncpy(p_n->host_path, host_path, sizeof(p_n->host_path) - 1);
    if (NULL != host_name)
    {
        (void)strncpy(p_n->host_name, host_name, sizeof(p_n->host_name) - 1);
    }
    (void)strncpy(p_n->iso_name, iso_name, sizeof(p_n->iso_name) - 1);
    p_n->is_dir = is_dir ? 1 : 0;
    p_n->file_size = file_size;
    p_n->parent_idx = parent_idx;
    p_n->child_head = -1;
    p_n->sibling = -1;

    if (parent_idx >= 0)
    {
        pfs_isofs_mk_node_t *p_par = &p_ctx->nodes[parent_idx];
        // 插到子链表头
        p_n->sibling = p_par->child_head;
        p_par->child_head = idx;
    }
    p_ctx->n_nodes++;
    return idx;
}

static int pfs_isofs_mk_unique_iso_name(pfs_isofs_mk_ctx_t *p_ctx, int parent_idx,
        char *iso_name, int iso_cap)
{
    char base[PFS_ISOFS_MKVOL_ISO_NAME_MAX + 1];

    (void)strncpy(base, iso_name, sizeof(base) - 1);
    base[sizeof(base) - 1] = '\0';
    for (int try_i = 0; try_i < 1000; try_i++)
    {
        int clash = 0;
        if (try_i > 0)
        {
            char suffix[8];
            int blen;
            int slen;
            (void)snprintf(suffix, sizeof(suffix), "_%d", try_i);
            slen = (int)strlen(suffix);
            blen = (int)strlen(base);
            if (blen + slen > PFS_ISOFS_MKVOL_ISO_NAME_MAX)
            {
                blen = PFS_ISOFS_MKVOL_ISO_NAME_MAX - slen;
                if (blen < 1)
                {
                    blen = 1;
                }
            }
            (void)snprintf(iso_name, (size_t)iso_cap, "%.*s%s", blen, base, suffix);
        }
        for (int ci = p_ctx->nodes[parent_idx].child_head; ci >= 0; ci = p_ctx->nodes[ci].sibling)
        {
            if (0 == strcmp(p_ctx->nodes[ci].iso_name, iso_name))
            {
                clash = 1;
                break;
            }
        }
        if (0 == clash)
        {
            return PFS_ISOFS_RET_OK;
        }
    }
    return PFS_ISOFS_RET_EEXIST;
}

#ifdef _WIN32
static int pfs_isofs_mk_utf8_to_wide(const char *utf8, wchar_t **pp_w)
{
    int n;
    wchar_t *p_w;

    *pp_w = NULL;
    n = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    if (n <= 0)
    {
        return PFS_ISOFS_RET_EILSEQ;
    }
    p_w = (wchar_t *)pfs_malloc((size_t)n * sizeof(wchar_t));
    if (NULL == p_w)
    {
        return PFS_ISOFS_RET_ENOMEM;
    }
    if (0 == MultiByteToWideChar(CP_UTF8, 0, utf8, -1, p_w, n))
    {
        pfs_free(p_w);
        return PFS_ISOFS_RET_EILSEQ;
    }
    *pp_w = p_w;
    return PFS_ISOFS_RET_OK;
}

static int pfs_isofs_mk_wide_to_utf8(const wchar_t *wide, char *out, int out_cap)
{
    int n;

    n = WideCharToMultiByte(CP_UTF8, 0, wide, -1, out, out_cap, NULL, NULL);
    if (n <= 0)
    {
        return PFS_ISOFS_RET_EILSEQ;
    }
    return PFS_ISOFS_RET_OK;
}
#endif

static int pfs_isofs_mk_scan_dir(pfs_isofs_mk_ctx_t *p_ctx, int dir_idx);

#ifdef _WIN32
static int pfs_isofs_mk_scan_dir(pfs_isofs_mk_ctx_t *p_ctx, int dir_idx)
{
    wchar_t *p_wdir;
    wchar_t pattern[PFS_ISOFS_MKVOL_PATH_MAX + 8];
    WIN32_FIND_DATAW fd;
    HANDLE h;

    int rc = pfs_isofs_mk_utf8_to_wide(p_ctx->nodes[dir_idx].host_path, &p_wdir);
    if (rc < 0)
    {
        return rc;
    }
    (void)_snwprintf(pattern, (sizeof(pattern) / sizeof(pattern[0])) - 1, L"%s\\*", p_wdir);
    pattern[(sizeof(pattern) / sizeof(pattern[0])) - 1] = L'\0';
    pfs_free(p_wdir);

    h = FindFirstFileW(pattern, &fd);
    if (INVALID_HANDLE_VALUE == h)
    {
        return PFS_ISOFS_RET_ENOENT;
    }
    do {
        char name_utf8[PFS_NAME_MAX + 1];
        char child_path[PFS_ISOFS_MKVOL_PATH_MAX];
        char iso_name[PFS_ISOFS_MKVOL_ISO_NAME_MAX + 1];
        int is_dir;
        uint64_t fsz;
        int child_idx;

        if (0 == wcscmp(fd.cFileName, L".") || 0 == wcscmp(fd.cFileName, L".."))
        {
            continue;
        }
        rc = pfs_isofs_mk_wide_to_utf8(fd.cFileName, name_utf8, (int)sizeof(name_utf8));
        if (rc < 0)
        {
            FindClose(h);
            return rc;
        }
        is_dir = (0 != (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) ? 1 : 0;
        if (0 != (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
        {
            continue;                                   // 跳过链接
        }
        fsz = 0;
        if (0 == is_dir)
        {
            fsz = ((uint64_t)fd.nFileSizeHigh << 32) | (uint64_t)fd.nFileSizeLow;
        }
        rc = pfs_isofs_mk_path_join(child_path, (int)sizeof(child_path),
                p_ctx->nodes[dir_idx].host_path, name_utf8);
        if (rc < 0)
        {
            FindClose(h);
            return rc;
        }
        pfs_isofs_mk_make_iso_name(name_utf8, iso_name, (int)sizeof(iso_name));
        rc = pfs_isofs_mk_unique_iso_name(p_ctx, dir_idx, iso_name, (int)sizeof(iso_name));
        if (rc < 0)
        {
            FindClose(h);
            return rc;
        }
        child_idx = pfs_isofs_mk_add_node(p_ctx, dir_idx, child_path, name_utf8, iso_name, is_dir, fsz);
        if (child_idx < 0)
        {
            FindClose(h);
            return child_idx;
        }
        if (is_dir)
        {
            rc = pfs_isofs_mk_scan_dir(p_ctx, child_idx);
            if (rc < 0)
            {
                FindClose(h);
                return rc;
            }
        }
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    return PFS_ISOFS_RET_OK;
}
#else
static int pfs_isofs_mk_scan_dir(pfs_isofs_mk_ctx_t *p_ctx, int dir_idx)
{
    DIR *p_d;
    struct dirent *p_de;

    p_d = opendir(p_ctx->nodes[dir_idx].host_path);
    if (NULL == p_d)
    {
        return PFS_ISOFS_RET_ENOENT;
    }
    while (NULL != (p_de = readdir(p_d)))
    {
        char child_path[PFS_ISOFS_MKVOL_PATH_MAX];
        char iso_name[PFS_ISOFS_MKVOL_ISO_NAME_MAX + 1];
        struct stat st;
        int is_dir;
        int child_idx;

        if (0 == strcmp(p_de->d_name, ".") || 0 == strcmp(p_de->d_name, ".."))
        {
            continue;
        }
        int rc = pfs_isofs_mk_path_join(child_path, (int)sizeof(child_path),
                p_ctx->nodes[dir_idx].host_path, p_de->d_name);
        if (rc < 0)
        {
            closedir(p_d);
            return rc;
        }
        if (0 != lstat(child_path, &st))
        {
            closedir(p_d);
            return PFS_ISOFS_RET_EIO;
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
        pfs_isofs_mk_make_iso_name(p_de->d_name, iso_name, (int)sizeof(iso_name));
        rc = pfs_isofs_mk_unique_iso_name(p_ctx, dir_idx, iso_name, (int)sizeof(iso_name));
        if (rc < 0)
        {
            closedir(p_d);
            return rc;
        }
        child_idx = pfs_isofs_mk_add_node(p_ctx, dir_idx, child_path, p_de->d_name, iso_name, is_dir,
                is_dir ? 0 : (uint64_t)st.st_size);
        if (child_idx < 0)
        {
            closedir(p_d);
            return child_idx;
        }
        if (is_dir)
        {
            rc = pfs_isofs_mk_scan_dir(p_ctx, child_idx);
            if (rc < 0)
            {
                closedir(p_d);
                return rc;
            }
        }
    }
    closedir(p_d);
    return PFS_ISOFS_RET_OK;
}
#endif

static int pfs_isofs_mk_rec_len(int name_len, int susp_len)
{
    int len = 33 + name_len;
    if (0 != (len & 1))
    {
        len++;
    }
    if (susp_len > 0)
    {
        len += susp_len;
        if (0 != (len & 1))
        {
            len++;
        }
    }
    return len;
}

static void pfs_isofs_mk_write_susp_sp(uint8_t *p)
{
    // SUSP SP: ver + check(0xBE,0xEF) + LEN_SKP (对照 IEEE P1281 / linux rock.c)
    p[0] = 'S';
    p[1] = 'P';
    p[2] = (uint8_t)PFS_ISOFS_MKVOL_SUSP_SP_LEN;
    p[3] = 1;                                          // version
    p[4] = 0xBE;                                       // check1
    p[5] = 0xEF;                                       // check2
    p[6] = 0;                                          // LEN_SKP
}

static void pfs_isofs_mk_write_susp_er(uint8_t *p)
{
    p[0] = 'E';
    p[1] = 'R';
    p[2] = (uint8_t)PFS_ISOFS_MKVOL_SUSP_ER_LEN;
    p[3] = 1;                                          // version
    p[4] = (uint8_t)PFS_ISOFS_MKVOL_SUSP_ER_ID_LEN;     // LEN_ID
    p[5] = 0;                                          // LEN_DES
    p[6] = 0;                                          // LEN_SRC
    p[7] = 1;                                          // Extension Version
    memcpy(p + 8, PFS_ISOFS_MKVOL_SUSP_ER_ID, (size_t)PFS_ISOFS_MKVOL_SUSP_ER_ID_LEN);
}

static int pfs_isofs_mk_susp_nm_len(const char *host_name)
{
    int n;

    if (NULL == host_name || '\0' == host_name[0])
    {
        return 0;
    }
    n = (int)strlen(host_name);
    return 5 + n;
}

static void pfs_isofs_mk_write_susp_nm(uint8_t *p, const char *host_name)
{
    int n;

    n = (int)strlen(host_name);
    p[0] = 'N';
    p[1] = 'M';
    p[2] = (uint8_t)(5 + n);
    p[3] = 1;                                          // version
    p[4] = 0;                                          // flags
    if (n > 0)
    {
        memcpy(p + 5, host_name, (size_t)n);
    }
}

static int pfs_isofs_mk_utf8_to_utf16be(const char *utf8, uint8_t *out, int out_cap, int max_chars)
{
    const unsigned char *s;
    int o;
    int nchars;

    if (NULL == utf8 || NULL == out || out_cap < 2 || max_chars <= 0)
    {
        return 0;
    }
    s = (const unsigned char *)utf8;
    o = 0;
    nchars = 0;
    while ('\0' != *s && nchars < max_chars)
    {
        unsigned char c = s[0];
        uint32_t cp;

        if (c < 0x80u)
        {
            cp = c;
            s++;
        }
        else if ((c & 0xE0u) == 0xC0u && '\0' != s[1])
        {
            cp = ((uint32_t)(c & 0x1Fu) << 6) | (uint32_t)(s[1] & 0x3Fu);
            s += 2;
        }
        else if ((c & 0xF0u) == 0xE0u && '\0' != s[1] && '\0' != s[2])
        {
            cp = ((uint32_t)(c & 0x0Fu) << 12) |
                    ((uint32_t)(s[1] & 0x3Fu) << 6) | (uint32_t)(s[2] & 0x3Fu);
            s += 3;
        }
        else if ((c & 0xF8u) == 0xF0u && '\0' != s[1] && '\0' != s[2] && '\0' != s[3])
        {
            cp = ((uint32_t)(c & 0x07u) << 18) |
                    ((uint32_t)(s[1] & 0x3Fu) << 12) |
                    ((uint32_t)(s[2] & 0x3Fu) << 6) | (uint32_t)(s[3] & 0x3Fu);
            s += 4;
        }
        else
        {
            cp = '?';
            s++;
        }
        if (cp > 0xFFFFu)
        {
            cp = '?';
        }
        if (o + 2 > out_cap)
        {
            break;
        }
        out[o] = (uint8_t)(cp >> 8);
        out[o + 1] = (uint8_t)(cp & 0xFFu);
        o += 2;
        nchars++;
    }
    return o;
}

static int pfs_isofs_mk_joliet_name_bytes(const char *host_name, uint8_t *out, int out_cap)
{
    return pfs_isofs_mk_utf8_to_utf16be(host_name, out, out_cap, PFS_ISOFS_MKVOL_JOLIET_CHARS);
}

static void pfs_isofs_mk_pad_volume_id_ucs2(uint8_t out[32], const char *volume_id)
{
    uint8_t tmp[64];
    int n;

    n = pfs_isofs_mk_utf8_to_utf16be((NULL != volume_id) ? volume_id : "", tmp, (int)sizeof(tmp), 16);
    for (int i = 0; i < 32; i++)
    {
        out[i] = 0x20;
    }
    if (n > 32)
    {
        n = 32;
    }
    for (int i = 0; i < n; i++)
    {
        out[i] = tmp[i];
    }
}

static int pfs_isofs_mk_dir_add_entry(int *p_size, int name_len, int susp_len)
{
    int size = *p_size;
    int need = pfs_isofs_mk_rec_len(name_len, susp_len);
    int in_blk = size % PFS_ISOFS_BLOCK_SIZE;

    if (0 != in_blk && in_blk + need > PFS_ISOFS_BLOCK_SIZE)
    {
        size += (PFS_ISOFS_BLOCK_SIZE - in_blk);
    }
    size += need;
    *p_size = size;
    return PFS_ISOFS_RET_OK;
}

static int pfs_isofs_mk_finish_dir_size(uint64_t *p_size, int size)
{
    uint64_t bytes;

    if (size < 0)
    {
        return PFS_ISOFS_RET_ERANGE;
    }
    if (0 != (size % PFS_ISOFS_BLOCK_SIZE))
    {
        size += (PFS_ISOFS_BLOCK_SIZE - (size % PFS_ISOFS_BLOCK_SIZE));
    }
    bytes = (uint64_t)size;
    if (pfs_isofs_mk_check_iso_size(bytes) < 0)
    {
        return PFS_ISOFS_RET_ERANGE;
    }
    *p_size = bytes;
    return PFS_ISOFS_RET_OK;
}

static int pfs_isofs_mk_calc_dir_size(pfs_isofs_mk_ctx_t *p_ctx, int dir_idx)
{
    int size;
    int root_dot_susp;

    root_dot_susp = (0 != p_ctx->rock_ridge && 0 == dir_idx) ? PFS_ISOFS_MKVOL_SUSP_ROOT_LEN : 0;
    size = 0;
    (void)pfs_isofs_mk_dir_add_entry(&size, 1, root_dot_susp);
    (void)pfs_isofs_mk_dir_add_entry(&size, 1, 0);
    for (int ci = p_ctx->nodes[dir_idx].child_head; ci >= 0; ci = p_ctx->nodes[ci].sibling)
    {
        int nlen = (int)strlen(p_ctx->nodes[ci].iso_name);
        int susp = 0;
        if (0 != p_ctx->rock_ridge)
        {
            susp = pfs_isofs_mk_susp_nm_len(p_ctx->nodes[ci].host_name);
        }
        (void)pfs_isofs_mk_dir_add_entry(&size, nlen, susp);
    }
    return pfs_isofs_mk_finish_dir_size(&p_ctx->nodes[dir_idx].file_size, size);
}

static int pfs_isofs_mk_calc_joliet_dir_size(pfs_isofs_mk_ctx_t *p_ctx, int dir_idx)
{
    int size;
    uint8_t jname[PFS_ISOFS_MKVOL_JOLIET_NAME_MAX];

    size = 0;
    (void)pfs_isofs_mk_dir_add_entry(&size, 1, 0);
    (void)pfs_isofs_mk_dir_add_entry(&size, 1, 0);
    for (int ci = p_ctx->nodes[dir_idx].child_head; ci >= 0; ci = p_ctx->nodes[ci].sibling)
    {
        int nlen = pfs_isofs_mk_joliet_name_bytes(p_ctx->nodes[ci].host_name, jname, (int)sizeof(jname));
        (void)pfs_isofs_mk_dir_add_entry(&size, nlen, 0);
    }
    return pfs_isofs_mk_finish_dir_size(&p_ctx->nodes[dir_idx].joliet_dir_size, size);
}

static uint32_t pfs_isofs_mk_terminator_lba(const pfs_isofs_mk_ctx_t *p_ctx)
{
    return (p_ctx->joliet_level > 0) ? 18u : 17u;
}

static uint32_t pfs_isofs_mk_bytes_to_lba(uint64_t bytes)
{
    if (0 == bytes)
    {
        return 0;
    }
    return (uint32_t)((bytes + PFS_ISOFS_BLOCK_SIZE - 1) / PFS_ISOFS_BLOCK_SIZE);
}

// 收集某目录下的子目录 idx, 按 iso_name / Joliet 名排序 (Path Table 同层序)
static int pfs_isofs_mk_collect_sorted_subdir(pfs_isofs_mk_ctx_t *p_ctx, int dir_idx,
        int joliet, int *kids, int kids_cap)
{
    int n;

    n = 0;
    for (int ci = p_ctx->nodes[dir_idx].child_head; ci >= 0; ci = p_ctx->nodes[ci].sibling)
    {
        if (0 == p_ctx->nodes[ci].is_dir)
        {
            continue;
        }
        if (n >= kids_cap)
        {
            return PFS_ISOFS_RET_ERANGE;
        }
        kids[n++] = ci;
    }
    for (int a = 0; a < n; a++)
    {
        for (int b = a + 1; b < n; b++)
        {
            int cmp;

            if (0 != joliet)
            {
                uint8_t na[PFS_ISOFS_MKVOL_JOLIET_NAME_MAX];
                uint8_t nb[PFS_ISOFS_MKVOL_JOLIET_NAME_MAX];
                int la = pfs_isofs_mk_joliet_name_bytes(p_ctx->nodes[kids[a]].host_name, na,
                        (int)sizeof(na));
                int lb = pfs_isofs_mk_joliet_name_bytes(p_ctx->nodes[kids[b]].host_name, nb,
                        (int)sizeof(nb));
                int m = (la < lb) ? la : lb;
                cmp = memcmp(na, nb, (size_t)m);
                if (0 == cmp)
                {
                    cmp = la - lb;
                }
            }
            else
            {
                cmp = strcmp(p_ctx->nodes[kids[a]].iso_name, p_ctx->nodes[kids[b]].iso_name);
            }
            if (cmp > 0)
            {
                int t = kids[a];
                kids[a] = kids[b];
                kids[b] = t;
            }
        }
    }
    return n;
}

// BFS 目录序 + path_num[1..]; 成功返回目录数; order/path_num 由调用方提供 (n_nodes 槽)
static int pfs_isofs_mk_build_pt_order(pfs_isofs_mk_ctx_t *p_ctx, int joliet, int *order,
        uint16_t *path_num)
{
    int *queue;
    int *kids;
    int qh;
    int qt;
    int n_dirs;
    uint16_t next;

    if (NULL == p_ctx || NULL == order || NULL == path_num || p_ctx->n_nodes <= 0)
    {
        return PFS_ISOFS_RET_EINVAL;
    }
    if (0 == p_ctx->nodes[0].is_dir)
    {
        return PFS_ISOFS_RET_EINVAL;
    }

    queue = (int *)pfs_malloc((size_t)p_ctx->n_nodes * sizeof(int));
    kids = (int *)pfs_malloc((size_t)p_ctx->n_nodes * sizeof(int));
    if (NULL == queue || NULL == kids)
    {
        if (NULL != queue)
        {
            pfs_free(queue);
        }
        if (NULL != kids)
        {
            pfs_free(kids);
        }
        return PFS_ISOFS_RET_ENOMEM;
    }

    for (int i = 0; i < p_ctx->n_nodes; i++)
    {
        path_num[i] = 0;
    }
    qh = 0;
    qt = 0;
    n_dirs = 0;
    next = 1;
    queue[qt++] = 0;
    path_num[0] = next++;
    order[n_dirs++] = 0;

    while (qh < qt)
    {
        int di = queue[qh++];
        int nk = pfs_isofs_mk_collect_sorted_subdir(p_ctx, di, joliet, kids, p_ctx->n_nodes);

        if (nk < 0)
        {
            pfs_free(queue);
            pfs_free(kids);
            return nk;
        }
        for (int k = 0; k < nk; k++)
        {
            int ci = kids[k];
            if (0 != path_num[ci])
            {
                continue;
            }
            if (next == 0)
            {
                pfs_free(queue);
                pfs_free(kids);
                return PFS_ISOFS_RET_ERANGE;
            }
            path_num[ci] = next++;
            order[n_dirs++] = ci;
            queue[qt++] = ci;
        }
    }

    pfs_free(queue);
    pfs_free(kids);
    return n_dirs;
}

static int pfs_isofs_mk_calc_pt_size(pfs_isofs_mk_ctx_t *p_ctx, int joliet, uint32_t *p_size)
{
    int *order;
    uint16_t *path_num;
    int n_dirs;
    uint32_t size;

    if (NULL == p_size)
    {
        return PFS_ISOFS_RET_EINVAL;
    }
    order = (int *)pfs_malloc((size_t)p_ctx->n_nodes * sizeof(int));
    path_num = (uint16_t *)pfs_malloc((size_t)p_ctx->n_nodes * sizeof(uint16_t));
    if (NULL == order || NULL == path_num)
    {
        if (NULL != order)
        {
            pfs_free(order);
        }
        if (NULL != path_num)
        {
            pfs_free(path_num);
        }
        return PFS_ISOFS_RET_ENOMEM;
    }
    n_dirs = pfs_isofs_mk_build_pt_order(p_ctx, joliet, order, path_num);
    if (n_dirs < 0)
    {
        pfs_free(order);
        pfs_free(path_num);
        return n_dirs;
    }
    size = 0;
    for (int i = 0; i < n_dirs; i++)
    {
        int idx = order[i];
        int nlen;

        if (0 == idx)
        {
            nlen = 1;
        }
        else if (0 != joliet)
        {
            uint8_t jname[PFS_ISOFS_MKVOL_JOLIET_NAME_MAX];
            nlen = pfs_isofs_mk_joliet_name_bytes(p_ctx->nodes[idx].host_name, jname,
                    (int)sizeof(jname));
        }
        else
        {
            nlen = (int)strlen(p_ctx->nodes[idx].iso_name);
        }
        size += (uint32_t)pfs_isofs_mk_pt_entry_len(nlen);
    }
    *p_size = size;
    pfs_free(order);
    pfs_free(path_num);
    return PFS_ISOFS_RET_OK;
}

static int pfs_isofs_mk_write_path_table(pfs_isofs_mk_ctx_t *p_ctx, int joliet, uint32_t pt_lba,
        uint32_t pt_size)
{
    int *order;
    uint16_t *path_num;
    uint8_t *buf;
    uint32_t buf_lba;
    int n_dirs;
    int off;

    if (0 == pt_size || 0 == pt_lba)
    {
        return PFS_ISOFS_RET_EINVAL;
    }
    order = (int *)pfs_malloc((size_t)p_ctx->n_nodes * sizeof(int));
    path_num = (uint16_t *)pfs_malloc((size_t)p_ctx->n_nodes * sizeof(uint16_t));
    if (NULL == order || NULL == path_num)
    {
        if (NULL != order)
        {
            pfs_free(order);
        }
        if (NULL != path_num)
        {
            pfs_free(path_num);
        }
        return PFS_ISOFS_RET_ENOMEM;
    }
    n_dirs = pfs_isofs_mk_build_pt_order(p_ctx, joliet, order, path_num);
    if (n_dirs < 0)
    {
        pfs_free(order);
        pfs_free(path_num);
        return n_dirs;
    }

    buf_lba = pfs_isofs_mk_bytes_to_lba(pt_size);
    if (0 == buf_lba)
    {
        buf_lba = 1;
    }
    buf = (uint8_t *)pfs_mallocz((size_t)buf_lba * PFS_ISOFS_BLOCK_SIZE);
    if (NULL == buf)
    {
        pfs_free(order);
        pfs_free(path_num);
        return PFS_ISOFS_RET_ENOMEM;
    }

    off = 0;
    for (int i = 0; i < n_dirs; i++)
    {
        int idx = order[i];
        pfs_isofs_mk_node_t *p_n = &p_ctx->nodes[idx];
        uint8_t name_buf[PFS_ISOFS_MKVOL_JOLIET_NAME_MAX];
        const uint8_t *p_name;
        int nlen;
        uint16_t parent;
        uint32_t extent;
        int ent;

        if (0 == idx)
        {
            name_buf[0] = 0;
            nlen = 1;
            p_name = name_buf;
            parent = 1;
        }
        else if (0 != joliet)
        {
            nlen = pfs_isofs_mk_joliet_name_bytes(p_n->host_name, name_buf, (int)sizeof(name_buf));
            p_name = name_buf;
            parent = path_num[p_n->parent_idx];
        }
        else
        {
            nlen = (int)strlen(p_n->iso_name);
            p_name = (const uint8_t *)p_n->iso_name;
            parent = path_num[p_n->parent_idx];
        }
        extent = (0 != joliet) ? p_n->joliet_extent : p_n->extent;
        ent = pfs_isofs_mk_pt_entry_len(nlen);
        if (off + ent > (int)pt_size)
        {
            pfs_free(buf);
            pfs_free(order);
            pfs_free(path_num);
            return PFS_ISOFS_RET_EBADMSG;
        }
        pfs_isofs_mk_put_711(buf + off + 0, (uint8_t)nlen);
        pfs_isofs_mk_put_711(buf + off + 1, 0);
        pfs_isofs_mk_put_731(buf + off + 2, extent);
        pfs_isofs_mk_put_721(buf + off + 6, parent);
        if (nlen > 0)
        {
            memcpy(buf + off + 8, p_name, (size_t)nlen);
        }
        off += ent;
    }

    int rc = pfs_isofs_mk_pwrite(p_ctx, (int64_t)pt_lba * PFS_ISOFS_BLOCK_SIZE, buf,
            (int)(buf_lba * PFS_ISOFS_BLOCK_SIZE));
    pfs_free(buf);
    pfs_free(order);
    pfs_free(path_num);
    return rc;
}

static int pfs_isofs_mk_assign_extents(pfs_isofs_mk_ctx_t *p_ctx, uint32_t *p_total_lba)
{
    uint32_t lba;

    int rc = pfs_isofs_mk_calc_pt_size(p_ctx, 0, &p_ctx->pri_pt_size);
    if (rc < 0)
    {
        return rc;
    }
    p_ctx->jol_pt_size = 0;
    if (p_ctx->joliet_level > 0)
    {
        rc = pfs_isofs_mk_calc_pt_size(p_ctx, 1, &p_ctx->jol_pt_size);
        if (0 > rc)
        {
            return rc;
        }
    }

    // Terminator 之后: Primary PT [, Joliet PT], 再目录/文件
    lba = pfs_isofs_mk_terminator_lba(p_ctx) + 1u;
    p_ctx->pri_pt_lba = lba;
    lba += pfs_isofs_mk_bytes_to_lba(p_ctx->pri_pt_size);
    if (0 == p_ctx->pri_pt_size)
    {
        return PFS_ISOFS_RET_EBADMSG;
    }
    p_ctx->jol_pt_lba = 0;
    if (p_ctx->joliet_level > 0)
    {
        p_ctx->jol_pt_lba = lba;
        lba += pfs_isofs_mk_bytes_to_lba(p_ctx->jol_pt_size);
    }

    for (int i = 0; i < p_ctx->n_nodes; i++)
    {
        if (p_ctx->nodes[i].is_dir)
        {
            rc = pfs_isofs_mk_calc_dir_size(p_ctx, i);
            if (rc < 0)
            {
                return rc;
            }
            p_ctx->nodes[i].extent = lba;
            lba += pfs_isofs_mk_bytes_to_lba(p_ctx->nodes[i].file_size);
        }
    }
    if (p_ctx->joliet_level > 0)
    {
        for (int i = 0; i < p_ctx->n_nodes; i++)
        {
            if (p_ctx->nodes[i].is_dir)
            {
                rc = pfs_isofs_mk_calc_joliet_dir_size(p_ctx, i);
                if (rc < 0)
                {
                    return rc;
                }
                p_ctx->nodes[i].joliet_extent = lba;
                lba += pfs_isofs_mk_bytes_to_lba(p_ctx->nodes[i].joliet_dir_size);
            }
        }
    }
    for (int i = 0; i < p_ctx->n_nodes; i++)
    {
        if (0 == p_ctx->nodes[i].is_dir)
        {
            p_ctx->nodes[i].extent = lba;
            if (0 != p_ctx->nodes[i].file_size)
            {
                lba += pfs_isofs_mk_bytes_to_lba(p_ctx->nodes[i].file_size);
            }
        }
    }
    *p_total_lba = lba;
    return PFS_ISOFS_RET_OK;
}

static void pfs_isofs_mk_write_dirent(uint8_t *p, int *p_off, uint32_t extent, uint32_t size,
        uint8_t flags, const void *name, int name_len, const uint8_t date7[7],
        const uint8_t *susp, int susp_len)
{
    int rec = pfs_isofs_mk_rec_len(name_len, susp_len);
    uint8_t *d = p + *p_off;
    int base;
    int pad;

    memset(d, 0, (size_t)rec);
    pfs_isofs_mk_put_711(d + 0, (uint8_t)rec);
    pfs_isofs_mk_put_711(d + 1, 0);
    pfs_isofs_mk_put_733(d + 2, extent);
    pfs_isofs_mk_put_733(d + 10, size);
    memcpy(d + 18, date7, 7);
    pfs_isofs_mk_put_711(d + 25, flags);
    pfs_isofs_mk_put_711(d + 26, 0);
    pfs_isofs_mk_put_711(d + 27, 0);
    pfs_isofs_mk_put_723(d + 28, 1);
    pfs_isofs_mk_put_711(d + 32, (uint8_t)name_len);
    if (name_len > 0)
    {
        memcpy(d + 33, name, (size_t)name_len);
    }
    base = 33 + name_len;
    if (0 != (base & 1))
    {
        base++;
    }
    if (susp_len > 0 && NULL != susp)
    {
        memcpy(d + base, susp, (size_t)susp_len);
        pad = base + susp_len;
        if (0 != (pad & 1))
        {
            d[pad] = 0;
        }
    }
    *p_off += rec;
}

static int pfs_isofs_mk_write_directory(pfs_isofs_mk_ctx_t *p_ctx, int dir_idx)
{
    pfs_isofs_mk_node_t *p_dir = &p_ctx->nodes[dir_idx];
    uint8_t *buf;
    uint8_t susp_root[PFS_ISOFS_MKVOL_SUSP_ROOT_LEN];
    uint8_t susp_nm[PFS_ISOFS_MKVOL_HOST_NAME_MAX + 8];
    int off;
    int parent_idx;
    uint32_t parent_extent;
    uint32_t parent_size;
    char dot;

    buf = (uint8_t *)pfs_mallocz((size_t)p_dir->file_size);
    if (NULL == buf)
    {
        return PFS_ISOFS_RET_ENOMEM;
    }
    off = 0;
    parent_idx = p_dir->parent_idx;
    if (parent_idx < 0)
    {
        parent_extent = p_dir->extent;
        parent_size = (uint32_t)p_dir->file_size;
    }
    else
    {
        parent_extent = p_ctx->nodes[parent_idx].extent;
        parent_size = (uint32_t)p_ctx->nodes[parent_idx].file_size;
    }

    dot = 0;
    if (0 != p_ctx->rock_ridge && 0 == dir_idx)
    {
        pfs_isofs_mk_write_susp_sp(susp_root);
        pfs_isofs_mk_write_susp_er(susp_root + PFS_ISOFS_MKVOL_SUSP_SP_LEN);
        pfs_isofs_mk_write_dirent(buf, &off, p_dir->extent, (uint32_t)p_dir->file_size,
                PFS_ISO_FILE_DIRECTORY, &dot, 1, p_ctx->date7, susp_root,
                PFS_ISOFS_MKVOL_SUSP_ROOT_LEN);
    }
    else
    {
        pfs_isofs_mk_write_dirent(buf, &off, p_dir->extent, (uint32_t)p_dir->file_size,
                PFS_ISO_FILE_DIRECTORY, &dot, 1, p_ctx->date7, NULL, 0);
    }
    dot = 1;
    pfs_isofs_mk_write_dirent(buf, &off, parent_extent, parent_size, PFS_ISO_FILE_DIRECTORY,
            &dot, 1, p_ctx->date7, NULL, 0);

    for (int ci = p_dir->child_head; ci >= 0; ci = p_ctx->nodes[ci].sibling)
    {
        pfs_isofs_mk_node_t *p_ch = &p_ctx->nodes[ci];
        int nlen = (int)strlen(p_ch->iso_name);
        int need = pfs_isofs_mk_rec_len(nlen, 0 != p_ctx->rock_ridge ?
                pfs_isofs_mk_susp_nm_len(p_ch->host_name) : 0);
        uint8_t flags = p_ch->is_dir ? PFS_ISO_FILE_DIRECTORY : 0;
        uint32_t sz = p_ch->is_dir ? (uint32_t)p_ch->file_size : (uint32_t)p_ch->file_size;
        const uint8_t *ps = NULL;
        int sl = 0;

        if (0 != (off % PFS_ISOFS_BLOCK_SIZE) &&
                (off % PFS_ISOFS_BLOCK_SIZE) + need > PFS_ISOFS_BLOCK_SIZE)
        {
            off += (PFS_ISOFS_BLOCK_SIZE - (off % PFS_ISOFS_BLOCK_SIZE));
        }
        if (0 != p_ctx->rock_ridge && '\0' != p_ch->host_name[0])
        {
            pfs_isofs_mk_write_susp_nm(susp_nm, p_ch->host_name);
            ps = susp_nm;
            sl = pfs_isofs_mk_susp_nm_len(p_ch->host_name);
        }
        pfs_isofs_mk_write_dirent(buf, &off, p_ch->extent, sz, flags, p_ch->iso_name, nlen,
                p_ctx->date7, ps, sl);
    }

    int rc = pfs_isofs_mk_pwrite(p_ctx, (int64_t)p_dir->extent * PFS_ISOFS_BLOCK_SIZE, buf,
            (int)p_dir->file_size);
    pfs_free(buf);
    return rc;
}

static int pfs_isofs_mk_write_joliet_directory(pfs_isofs_mk_ctx_t *p_ctx, int dir_idx)
{
    pfs_isofs_mk_node_t *p_dir = &p_ctx->nodes[dir_idx];
    uint8_t *buf;
    uint8_t jname[PFS_ISOFS_MKVOL_JOLIET_NAME_MAX];
    int off;
    int parent_idx;
    uint32_t parent_extent;
    uint32_t parent_size;
    char dot;

    buf = (uint8_t *)pfs_mallocz((size_t)p_dir->joliet_dir_size);
    if (NULL == buf)
    {
        return PFS_ISOFS_RET_ENOMEM;
    }
    off = 0;
    parent_idx = p_dir->parent_idx;
    if (parent_idx < 0)
    {
        parent_extent = p_dir->joliet_extent;
        parent_size = (uint32_t)p_dir->joliet_dir_size;
    }
    else
    {
        parent_extent = p_ctx->nodes[parent_idx].joliet_extent;
        parent_size = (uint32_t)p_ctx->nodes[parent_idx].joliet_dir_size;
    }

    dot = 0;
    pfs_isofs_mk_write_dirent(buf, &off, p_dir->joliet_extent, (uint32_t)p_dir->joliet_dir_size,
            PFS_ISO_FILE_DIRECTORY, &dot, 1, p_ctx->date7, NULL, 0);
    dot = 1;
    pfs_isofs_mk_write_dirent(buf, &off, parent_extent, parent_size, PFS_ISO_FILE_DIRECTORY,
            &dot, 1, p_ctx->date7, NULL, 0);

    for (int ci = p_dir->child_head; ci >= 0; ci = p_ctx->nodes[ci].sibling)
    {
        pfs_isofs_mk_node_t *p_ch = &p_ctx->nodes[ci];
        int nlen;
        int need;
        uint8_t flags = p_ch->is_dir ? PFS_ISO_FILE_DIRECTORY : 0;
        uint32_t ext;
        uint32_t sz;

        nlen = pfs_isofs_mk_joliet_name_bytes(p_ch->host_name, jname, (int)sizeof(jname));
        need = pfs_isofs_mk_rec_len(nlen, 0);
        if (0 != (off % PFS_ISOFS_BLOCK_SIZE) &&
                (off % PFS_ISOFS_BLOCK_SIZE) + need > PFS_ISOFS_BLOCK_SIZE)
        {
            off += (PFS_ISOFS_BLOCK_SIZE - (off % PFS_ISOFS_BLOCK_SIZE));
        }
        if (p_ch->is_dir)
        {
            ext = p_ch->joliet_extent;
            sz = (uint32_t)p_ch->joliet_dir_size;
        }
        else
        {
            ext = p_ch->extent;
            sz = (uint32_t)p_ch->file_size;
        }
        pfs_isofs_mk_write_dirent(buf, &off, ext, sz, flags, jname, nlen, p_ctx->date7, NULL, 0);
    }

    int rc = pfs_isofs_mk_pwrite(p_ctx, (int64_t)p_dir->joliet_extent * PFS_ISOFS_BLOCK_SIZE, buf,
            (int)p_dir->joliet_dir_size);
    pfs_free(buf);
    return rc;
}

static int pfs_isofs_mk_copy_file(pfs_isofs_mk_ctx_t *p_ctx, int file_idx)
{
    pfs_isofs_mk_node_t *p_f = &p_ctx->nodes[file_idx];
    uint64_t left;
    int64_t disk_off;

    if (0 == p_f->file_size)
    {
        return PFS_ISOFS_RET_OK;
    }
    int rc;
    left = p_f->file_size;
    disk_off = (int64_t)p_f->extent * PFS_ISOFS_BLOCK_SIZE;

#ifdef _WIN32
    {
        wchar_t *p_w;
        HANDLE h;
        rc = pfs_isofs_mk_utf8_to_wide(p_f->host_path, &p_w);
        if (0 > rc)
        {
            return rc;
        }
        h = CreateFileW(p_w, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL, NULL);
        pfs_free(p_w);
        if (INVALID_HANDLE_VALUE == h)
        {
            return PFS_ISOFS_RET_EIO;
        }
        while (left > 0)
        {
            DWORD nread;
            int chunk = (left > (uint64_t)PFS_ISOFS_MKVOL_COPY_CHUNK) ?
                    PFS_ISOFS_MKVOL_COPY_CHUNK : (int)left;
            if (0 == ReadFile(h, p_ctx->copy_buf, (DWORD)chunk, &nread, NULL) ||
                    0 == nread)
            {
                CloseHandle(h);
                return PFS_ISOFS_RET_EIO;
            }
            rc = pfs_isofs_mk_pwrite(p_ctx, disk_off, p_ctx->copy_buf, (int)nread);
            if (rc < 0)
            {
                CloseHandle(h);
                return rc;
            }
            disk_off += (int64_t)nread;
            left -= (uint64_t)nread;
        }
        CloseHandle(h);
    }
#else
    {
        int fd = open(p_f->host_path, O_RDONLY);
        if (fd < 0)
        {
            return PFS_ISOFS_RET_EIO;
        }
        while (left > 0)
        {
            ssize_t nread;
            int chunk = (left > (uint64_t)PFS_ISOFS_MKVOL_COPY_CHUNK) ?
                    PFS_ISOFS_MKVOL_COPY_CHUNK : (int)left;
            nread = read(fd, p_ctx->copy_buf, (size_t)chunk);
            if (nread <= 0)
            {
                close(fd);
                return PFS_ISOFS_RET_EIO;
            }
            rc = pfs_isofs_mk_pwrite(p_ctx, disk_off, p_ctx->copy_buf, (int)nread);
            if (rc < 0)
            {
                close(fd);
                return rc;
            }
            disk_off += (int64_t)nread;
            left -= (uint64_t)nread;
        }
        close(fd);
    }
#endif
    // 尾块补零
    {
        uint32_t used = pfs_isofs_mk_bytes_to_lba(p_f->file_size);
        uint64_t pad = (uint64_t)used * PFS_ISOFS_BLOCK_SIZE - p_f->file_size;
        if (pad > 0 && pad <= PFS_ISOFS_BLOCK_SIZE)
        {
            uint8_t z[PFS_ISOFS_BLOCK_SIZE];
            memset(z, 0, sizeof(z));
            rc = pfs_isofs_mk_pwrite(p_ctx, disk_off, z, (int)pad);
            if (rc < 0)
            {
                return rc;
            }
        }
    }
    return PFS_ISOFS_RET_OK;
}

static int pfs_isofs_mk_write_pvd(pfs_isofs_mk_ctx_t *p_ctx, uint32_t total_lba)
{
    uint8_t sec[PFS_ISOFS_BLOCK_SIZE];
    pfs_iso_primary_descriptor_t *p_pri;
    pfs_isofs_mk_node_t *p_root = &p_ctx->nodes[0];
    int off;
    char root_name;

    memset(sec, 0, sizeof(sec));
    p_pri = (pfs_iso_primary_descriptor_t *)sec;
    pfs_isofs_mk_put_711(p_pri->type, PFS_ISO_VD_PRIMARY);
    memcpy(p_pri->id, PFS_ISO_STANDARD_ID, PFS_ISO_STANDARD_ID_LEN);
    pfs_isofs_mk_put_711(p_pri->version, 1);
    pfs_isofs_mk_pad_str(p_pri->system_id, 32, "", ' ');
    pfs_isofs_mk_pad_str(p_pri->volume_id, 32, p_ctx->volume_id, ' ');
    pfs_isofs_mk_put_733(p_pri->volume_space_size, total_lba);
    pfs_isofs_mk_put_723(p_pri->volume_set_size, 1);
    pfs_isofs_mk_put_723(p_pri->volume_sequence_number, 1);
    pfs_isofs_mk_put_723(p_pri->logical_block_size, (uint16_t)PFS_ISOFS_BLOCK_SIZE);
    pfs_isofs_mk_put_733(p_pri->path_table_size, p_ctx->pri_pt_size);
    pfs_isofs_mk_put_731(p_pri->type_l_path_table, p_ctx->pri_pt_lba);
    memset(p_pri->opt_type_l_path_table, 0, 4);
    memset(p_pri->type_m_path_table, 0, 4);
    memset(p_pri->opt_type_m_path_table, 0, 4);

    off = 0;
    root_name = 0;
    pfs_isofs_mk_write_dirent(p_pri->root_directory_record, &off, p_root->extent,
            (uint32_t)p_root->file_size, PFS_ISO_FILE_DIRECTORY, &root_name, 1, p_ctx->date7,
            NULL, 0);

    pfs_isofs_mk_pad_str(p_pri->volume_set_id, 128, "", ' ');
    pfs_isofs_mk_pad_str(p_pri->publisher_id, 128, "", ' ');
    pfs_isofs_mk_pad_str(p_pri->preparer_id, 128, "PFS", ' ');
    pfs_isofs_mk_pad_str(p_pri->application_id, 128, "PFS ISOFS MKVOL", ' ');
    pfs_isofs_mk_pad_str(p_pri->copyright_file_id, 37, "", ' ');
    pfs_isofs_mk_pad_str(p_pri->abstract_file_id, 37, "", ' ');
    pfs_isofs_mk_pad_str(p_pri->bibliographic_file_id, 37, "", ' ');
    // 日期字段留 0
    pfs_isofs_mk_put_711(p_pri->file_structure_version, 1);

    return pfs_isofs_mk_pwrite(p_ctx, 16LL * PFS_ISOFS_BLOCK_SIZE, sec, PFS_ISOFS_BLOCK_SIZE);
}

static int pfs_isofs_mk_write_svd(pfs_isofs_mk_ctx_t *p_ctx, uint32_t total_lba)
{
    uint8_t sec[PFS_ISOFS_BLOCK_SIZE];
    pfs_iso_supplementary_descriptor_t *p_svd;
    pfs_isofs_mk_node_t *p_root = &p_ctx->nodes[0];
    int off;
    char root_name;
    uint8_t escape[32];

    if (p_ctx->joliet_level <= 0)
    {
        return PFS_ISOFS_RET_OK;
    }

    memset(sec, 0, sizeof(sec));
    memset(escape, 0, sizeof(escape));
    escape[0] = 0x25;
    escape[1] = 0x2F;
    if (1 == p_ctx->joliet_level)
    {
        escape[2] = 0x40;
    }
    else if (2 == p_ctx->joliet_level)
    {
        escape[2] = 0x43;
    }
    else
    {
        escape[2] = 0x45;
    }

    p_svd = (pfs_iso_supplementary_descriptor_t *)sec;
    pfs_isofs_mk_put_711(p_svd->type, PFS_ISO_VD_SUPPLEMENTARY);
    memcpy(p_svd->id, PFS_ISO_STANDARD_ID, PFS_ISO_STANDARD_ID_LEN);
    pfs_isofs_mk_put_711(p_svd->version, 1);
    pfs_isofs_mk_pad_str(p_svd->system_id, 32, "", ' ');
    pfs_isofs_mk_pad_volume_id_ucs2((uint8_t *)p_svd->volume_id, p_ctx->volume_id);
    pfs_isofs_mk_put_733(p_svd->volume_space_size, total_lba);
    memcpy(p_svd->escape, escape, sizeof(escape));
    pfs_isofs_mk_put_723(p_svd->volume_set_size, 1);
    pfs_isofs_mk_put_723(p_svd->volume_sequence_number, 1);
    pfs_isofs_mk_put_723(p_svd->logical_block_size, (uint16_t)PFS_ISOFS_BLOCK_SIZE);
    pfs_isofs_mk_put_733(p_svd->path_table_size, p_ctx->jol_pt_size);
    pfs_isofs_mk_put_731(p_svd->type_l_path_table, p_ctx->jol_pt_lba);
    memset(p_svd->opt_type_l_path_table, 0, 4);
    memset(p_svd->type_m_path_table, 0, 4);
    memset(p_svd->opt_type_m_path_table, 0, 4);

    off = 0;
    root_name = 0;
    pfs_isofs_mk_write_dirent(p_svd->root_directory_record, &off, p_root->joliet_extent,
            (uint32_t)p_root->joliet_dir_size, PFS_ISO_FILE_DIRECTORY, &root_name, 1, p_ctx->date7,
            NULL, 0);

    pfs_isofs_mk_pad_str(p_svd->volume_set_id, 128, "", ' ');
    pfs_isofs_mk_pad_str(p_svd->publisher_id, 128, "", ' ');
    pfs_isofs_mk_pad_str(p_svd->preparer_id, 128, "PFS", ' ');
    pfs_isofs_mk_pad_str(p_svd->application_id, 128, "PFS ISOFS MKVOL", ' ');
    pfs_isofs_mk_pad_str(p_svd->copyright_file_id, 37, "", ' ');
    pfs_isofs_mk_pad_str(p_svd->abstract_file_id, 37, "", ' ');
    pfs_isofs_mk_pad_str(p_svd->bibliographic_file_id, 37, "", ' ');
    pfs_isofs_mk_put_711(p_svd->file_structure_version, 1);

    return pfs_isofs_mk_pwrite(p_ctx, 17LL * PFS_ISOFS_BLOCK_SIZE, sec, PFS_ISOFS_BLOCK_SIZE);
}

static int pfs_isofs_mk_write_terminator(pfs_isofs_mk_ctx_t *p_ctx)
{
    uint8_t sec[PFS_ISOFS_BLOCK_SIZE];
    uint32_t lba;

    memset(sec, 0, sizeof(sec));
    sec[0] = PFS_ISO_VD_END;
    memcpy(sec + 1, PFS_ISO_STANDARD_ID, PFS_ISO_STANDARD_ID_LEN);
    sec[6] = 1;
    lba = pfs_isofs_mk_terminator_lba(p_ctx);
    return pfs_isofs_mk_pwrite(p_ctx, (int64_t)lba * PFS_ISOFS_BLOCK_SIZE, sec, PFS_ISOFS_BLOCK_SIZE);
}

static int pfs_isofs_mk_build(pfs_isofs_mk_ctx_t *p_ctx)
{
    uint32_t total_lba;
    int64_t need;
    int64_t cap;

    int rc = pfs_isofs_mk_assign_extents(p_ctx, &total_lba);
    if (rc < 0)
    {
        return rc;
    }
    need = (int64_t)total_lba * PFS_ISOFS_BLOCK_SIZE;
    cap = pfs_blkio_get_size(p_ctx->p_blkio);
    if (cap > 0 && p_ctx->part_offset + need > cap)
    {
        return PFS_ISOFS_RET_ENOSPC;
    }

    rc = pfs_isofs_mk_zero_lba(p_ctx, 0, 16);
    if (rc < 0)
    {
        return rc;
    }
    rc = pfs_isofs_mk_write_pvd(p_ctx, total_lba);
    if (rc < 0)
    {
        return rc;
    }
    rc = pfs_isofs_mk_write_svd(p_ctx, total_lba);
    if (rc < 0)
    {
        return rc;
    }
    rc = pfs_isofs_mk_write_terminator(p_ctx);
    if (rc < 0)
    {
        return rc;
    }
    // Windows CDFS 进子目录依赖 Path Table; pfs/Linux 只扫目录项
    rc = pfs_isofs_mk_write_path_table(p_ctx, 0, p_ctx->pri_pt_lba, p_ctx->pri_pt_size);
    if (rc < 0)
    {
        return rc;
    }
    if (p_ctx->joliet_level > 0)
    {
        rc = pfs_isofs_mk_write_path_table(p_ctx, 1, p_ctx->jol_pt_lba, p_ctx->jol_pt_size);
        if (0 > rc)
        {
            return rc;
        }
    }

    for (int i = 0; i < p_ctx->n_nodes; i++)
    {
        if (p_ctx->nodes[i].is_dir)
        {
            rc = pfs_isofs_mk_write_directory(p_ctx, i);
            if (rc < 0)
            {
                return rc;
            }
        }
    }
    if (p_ctx->joliet_level > 0)
    {
        for (int i = 0; i < p_ctx->n_nodes; i++)
        {
            if (p_ctx->nodes[i].is_dir)
            {
                rc = pfs_isofs_mk_write_joliet_directory(p_ctx, i);
                if (rc < 0)
                {
                    return rc;
                }
            }
        }
    }
    for (int i = 0; i < p_ctx->n_nodes; i++)
    {
        if (0 == p_ctx->nodes[i].is_dir)
        {
            rc = pfs_isofs_mk_copy_file(p_ctx, i);
            if (rc < 0)
            {
                return rc;
            }
        }
    }
    rc = pfs_blkio_fsync(p_ctx->p_blkio);
    if (rc < 0)
    {
        return pfs_isofs_mk_map_io(rc);
    }
    return PFS_ISOFS_RET_OK;
}

static void pfs_isofs_mk_ctx_free(pfs_isofs_mk_ctx_t *p_ctx)
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
}

int pfs_isofs_mkvol(pfs_blkio_t *p_blkio, int64_t part_offset, const char *src_dir,
        const char *volume_id, int joliet_level, int rock_ridge)
{
    pfs_isofs_mk_ctx_t ctx;
    int root_idx;

    if (NULL == p_blkio || part_offset < 0 || NULL == src_dir || '\0' == src_dir[0])
    {
        return PFS_ISOFS_RET_EINVAL;
    }
    if (!pfs_blkio_is_open(p_blkio))
    {
        return PFS_ISOFS_RET_EINVAL;
    }

    memset(&ctx, 0, sizeof(ctx));
    ctx.p_blkio = p_blkio;
    ctx.part_offset = part_offset;
    ctx.joliet_level = pfs_isofs_mk_clamp_joliet(joliet_level);
    ctx.rock_ridge = pfs_isofs_mk_clamp_rock(rock_ridge);
    pfs_isofs_mk_fill_date7(ctx.date7);
    memset(ctx.volume_id, 0, sizeof(ctx.volume_id));
    if (NULL != volume_id && '\0' != volume_id[0])
    {
        (void)strncpy(ctx.volume_id, volume_id, sizeof(ctx.volume_id) - 1);
    }
    else
    {
        (void)strncpy(ctx.volume_id, "PFS_ISOFS", sizeof(ctx.volume_id) - 1);
    }
    // 卷标仅保留可打印 ASCII, 转大写
    {
        for (int i = 0; '\0' != ctx.volume_id[i]; i++)
        {
            char c = ctx.volume_id[i];
            if (c >= 'a' && c <= 'z')
            {
                ctx.volume_id[i] = (char)(c - 'a' + 'A');
            }
        }
    }

    ctx.copy_buf = (uint8_t *)pfs_malloc(PFS_ISOFS_MKVOL_COPY_CHUNK);
    if (NULL == ctx.copy_buf)
    {
        return PFS_ISOFS_RET_ENOMEM;
    }

    root_idx = pfs_isofs_mk_add_node(&ctx, -1, src_dir, "", "", 1, 0);
    if (root_idx < 0)
    {
        pfs_isofs_mk_ctx_free(&ctx);
        return root_idx;
    }
    int rc = pfs_isofs_mk_scan_dir(&ctx, root_idx);
    if (rc < 0)
    {
        pfs_isofs_mk_ctx_free(&ctx);
        return rc;
    }
    rc = pfs_isofs_mk_build(&ctx);
    pfs_isofs_mk_ctx_free(&ctx);
    return rc;
}

int pfs_isofs_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes)
{
    pfs_isofs_mk_ctx_t ctx;
    int root_idx;
    char empty_root[8];

    (void)vol_bytes;
    if (NULL == p_blkio || part_offset < 0)
    {
        return PFS_ISOFS_RET_EINVAL;
    }
    if (!pfs_blkio_is_open(p_blkio))
    {
        return PFS_ISOFS_RET_EINVAL;
    }

    memset(&ctx, 0, sizeof(ctx));
    ctx.p_blkio = p_blkio;
    ctx.part_offset = part_offset;
    ctx.joliet_level = 0;
    ctx.rock_ridge = 0;
    pfs_isofs_mk_fill_date7(ctx.date7);
    memset(ctx.volume_id, 0, sizeof(ctx.volume_id));
    (void)strncpy(ctx.volume_id, "PFS_ISOFS", sizeof(ctx.volume_id) - 1);
    // 空卷: 根节点 host_path 占位, 不扫描
    (void)strncpy(empty_root, ".", sizeof(empty_root) - 1);
    root_idx = pfs_isofs_mk_add_node(&ctx, -1, empty_root, "", "", 1, 0);
    if (root_idx < 0)
    {
        pfs_isofs_mk_ctx_free(&ctx);
        return root_idx;
    }
    int rc = pfs_isofs_mk_build(&ctx);
    pfs_isofs_mk_ctx_free(&ctx);
    return rc;
}

// end
