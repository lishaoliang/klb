// Doc Encode : UTF-8 BOM, Unix(LF)
// squashfs mkfs / mkvol (v4; 数据/fragment 可 gzip 或未压缩; 元数据表首版未压缩; volume_id 忽略)
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "squashfs/pfs_squashfs_internal.h"
#include "base/pfsmtd_mem.h"
#include "base/pfsmtd_util.h"

#include "zlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#define PFS_SQUASHFS_MK_BLOCK_SIZE      (64u * 1024u)
#define PFS_SQUASHFS_MK_BLOCK_LOG       16
#define PFS_SQUASHFS_MK_MAX_NODES       4096
#define PFS_SQUASHFS_MK_PATH_MAX        1024
#define PFS_SQUASHFS_MK_NAME_MAX        255
#define PFS_SQUASHFS_MK_COPY_CHUNK      (64 * 1024)
#define PFS_SQUASHFS_MK_COMP_OVERHEAD   4096

typedef struct _pfs_squashfs_mk_node_t {
    char                    host_path[PFS_SQUASHFS_MK_PATH_MAX];
    char                    name[PFS_SQUASHFS_MK_NAME_MAX + 1];
    bool                    is_dir;
    int                     parent_idx;
    int                     child_head;
    int                     sibling;
    uint64_t                file_size;
    uint32_t                mode;
    uint32_t                mtime;
    uint32_t                inode_number;
    uint64_t                inode_ref;
    uint64_t                data_start;
    uint32_t               *block_list;
    uint32_t                nblocks;
    uint32_t                fragment;
    uint32_t                frag_offset;
    uint32_t                dir_start_block;
    uint16_t                dir_offset;
    uint32_t                dir_size;
    int                     blob_off;                   // inode 在 inode_blob 内偏移
} pfs_squashfs_mk_node_t;

typedef struct _pfs_squashfs_mk_frag_t {
    uint64_t                start_block;
    uint32_t                size_code;
} pfs_squashfs_mk_frag_t;

typedef struct _pfs_squashfs_mk_ctx_t {
    pfs_blkio_t            *p_blkio;
    int64_t                 part_offset;
    uint64_t                cursor;
    uint32_t                block_size;
    uint16_t                block_log;
    uint32_t                mkfs_time;
    int                     compress;                   // PFS_SQUASHFS_COMPRESS_NONE / GZIP
    pfs_squashfs_mk_node_t *nodes;
    int                     n_nodes;
    int                     n_cap;
    uint32_t                next_ino;
    uint8_t                *copy_buf;
    uint8_t                *comp_buf;
    int                     comp_buf_cap;
    uint8_t                *frag_buf;
    uint32_t                frag_used;
    pfs_squashfs_mk_frag_t *frags;
    uint32_t                n_frags;
    uint32_t                n_frags_cap;
    uint8_t                *inode_blob;
    int                     inode_blob_len;
    int                     inode_blob_cap;
    uint8_t                *dir_blob;
    int                     dir_blob_len;
    int                     dir_blob_cap;
    uint64_t                inode_table;
    uint64_t                directory_table;
    uint64_t                fragment_table_start;
    uint64_t                id_table_start;
    uint64_t                root_inode;
    uint64_t                bytes_used;
} pfs_squashfs_mk_ctx_t;

static int pfs_squashfs_mk_pwrite(pfs_squashfs_mk_ctx_t *p_ctx, uint64_t rel, const void *buf, int size);

// DEFAULT/0 -> GZIP; 仅接受 NONE/GZIP; 成功写入 *p_out
static int pfs_squashfs_mk_resolve_compress(int compress, int *p_out)
{
    if (NULL == p_out)
    {
        return PFS_EINVAL;
    }
    if (PFS_SQUASHFS_COMPRESS_DEFAULT == compress || PFS_SQUASHFS_COMPRESS_GZIP == compress)
    {
        *p_out = PFS_SQUASHFS_COMPRESS_GZIP;
        return PFS_OK;
    }
    if (PFS_SQUASHFS_COMPRESS_NONE == compress)
    {
        *p_out = PFS_SQUASHFS_COMPRESS_NONE;
        return PFS_OK;
    }
    return PFS_EINVAL;
}

// zlib 包装流 (与本栈 inflateInit 对齐); 压不过或失败返回非 OK, 调用方存未压缩
static int pfs_squashfs_mk_zlib_deflate(const uint8_t *src, int src_len, uint8_t *dst, int dst_cap,
        int *p_out_len)
{
    if (NULL == src || NULL == dst || NULL == p_out_len || src_len <= 0 || dst_cap <= 0)
    {
        return PFS_EINVAL;
    }

    z_stream strm;

    memset(&strm, 0, sizeof(strm));
    int zrc = deflateInit(&strm, Z_DEFAULT_COMPRESSION);
    if (Z_OK != zrc)
    {
        return PFS_ENOMEM;
    }

    strm.next_in = (Bytef *)src;
    strm.avail_in = (uInt)src_len;
    strm.next_out = (Bytef *)dst;
    strm.avail_out = (uInt)dst_cap;

    zrc = deflate(&strm, Z_FINISH);
    int out_len = (int)strm.total_out;
    deflateEnd(&strm);

    if (Z_STREAM_END != zrc)
    {
        return PFS_EIO;
    }
    if (out_len <= 0 || out_len >= src_len || out_len > dst_cap)
    {
        return PFS_EIO;
    }

    *p_out_len = out_len;
    return PFS_OK;
}

// 写数据/fragment 块: gzip 时尝试压缩, 否则或压不过则标未压缩位写出
static int pfs_squashfs_mk_store_data_block(pfs_squashfs_mk_ctx_t *p_ctx, const uint8_t *src, int src_len,
        uint32_t *p_size_code)
{
    if (NULL == p_ctx || NULL == src || NULL == p_size_code || src_len <= 0)
    {
        return PFS_EINVAL;
    }

    const uint8_t *out_ptr = src;
    int out_bytes = src_len;
    uint32_t size_code = (uint32_t)src_len | PFS_SQUASHFS_COMPRESSED_BIT_BLOCK;

    if (PFS_SQUASHFS_COMPRESS_GZIP == p_ctx->compress)
    {
        int out_len;
        int rc = pfs_squashfs_mk_zlib_deflate(src, src_len, p_ctx->comp_buf, p_ctx->comp_buf_cap, &out_len);
        if (PFS_OK == rc)
        {
            out_ptr = p_ctx->comp_buf;
            out_bytes = out_len;
            size_code = (uint32_t)out_len; // 无 COMPRESSED_BIT_BLOCK => 压缩块
        }
    }

    int rc = pfs_squashfs_mk_pwrite(p_ctx, p_ctx->cursor, out_ptr, out_bytes);
    if (PFS_OK != rc)
    {
        return rc;
    }
    p_ctx->cursor += (uint64_t)out_bytes;
    *p_size_code = size_code;
    return PFS_OK;
}

static void pfs_squashfs_mk_put_le16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xffu);
    p[1] = (uint8_t)((v >> 8) & 0xffu);
}

static void pfs_squashfs_mk_put_le32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xffu);
    p[1] = (uint8_t)((v >> 8) & 0xffu);
    p[2] = (uint8_t)((v >> 16) & 0xffu);
    p[3] = (uint8_t)((v >> 24) & 0xffu);
}

static void pfs_squashfs_mk_put_le64(uint8_t *p, uint64_t v)
{
    pfs_squashfs_mk_put_le32(p, (uint32_t)(v & 0xffffffffu));
    pfs_squashfs_mk_put_le32(p + 4, (uint32_t)(v >> 32));
}

static int pfs_squashfs_mk_pwrite(pfs_squashfs_mk_ctx_t *p_ctx, uint64_t rel, const void *buf, int size)
{
    int n;

    if (size <= 0)
    {
        return PFS_OK;
    }
    n = pfs_blkio_pwrite(p_ctx->p_blkio, buf, size, p_ctx->part_offset + (int64_t)rel);
    if (n < 0)
    {
        return pfs_squashfs_map_blkio_rc(n);
    }
    if (n != size)
    {
        return PFS_EIO;
    }
    return PFS_OK;
}

static int pfs_squashfs_mk_blob_grow(uint8_t **p_blob, int *p_len, int *p_cap, int add)
{
    int need;
    uint8_t *p_new;
    int ncap;

    if (add < 0)
    {
        return PFS_EINVAL;
    }
    need = *p_len + add;
    if (need <= *p_cap)
    {
        return PFS_OK;
    }
    ncap = (*p_cap > 0) ? *p_cap : 4096;
    while (ncap < need)
    {
        ncap *= 2;
    }
    p_new = (uint8_t *)pfsmtd_realloc(*p_blob, (size_t)ncap);
    if (NULL == p_new)
    {
        return PFS_ENOMEM;
    }
    *p_blob = p_new;
    *p_cap = ncap;
    return PFS_OK;
}

static int pfs_squashfs_mk_blob_append(uint8_t **p_blob, int *p_len, int *p_cap, const void *data, int n)
{

    int rc = pfs_squashfs_mk_blob_grow(p_blob, p_len, p_cap, n);
    if (PFS_OK != rc)
    {
        return rc;
    }
    if (n > 0 && NULL != data)
    {
        memcpy(*p_blob + *p_len, data, (size_t)n);
    }
    *p_len += n;
    return PFS_OK;
}

static void pfs_squashfs_mk_ctx_free(pfs_squashfs_mk_ctx_t *p_ctx)
{
    if (NULL == p_ctx)
    {
        return;
    }
    if (NULL != p_ctx->nodes)
    {
        for (int i = 0; i < p_ctx->n_nodes; i++)
        {
            pfsmtd_free(p_ctx->nodes[i].block_list);
            p_ctx->nodes[i].block_list = NULL;
        }
        pfsmtd_free(p_ctx->nodes);
        p_ctx->nodes = NULL;
    }
    pfsmtd_free(p_ctx->copy_buf);
    pfsmtd_free(p_ctx->comp_buf);
    pfsmtd_free(p_ctx->frag_buf);
    pfsmtd_free(p_ctx->frags);
    pfsmtd_free(p_ctx->inode_blob);
    pfsmtd_free(p_ctx->dir_blob);
    p_ctx->copy_buf = NULL;
    p_ctx->comp_buf = NULL;
    p_ctx->frag_buf = NULL;
    p_ctx->frags = NULL;
    p_ctx->inode_blob = NULL;
    p_ctx->dir_blob = NULL;
}

static int pfs_squashfs_mk_path_join(char *out, int out_cap, const char *dir, const char *name)
{
    int n;

    if (NULL == out || out_cap <= 0 || NULL == dir || NULL == name)
    {
        return PFS_EINVAL;
    }
#ifdef _WIN32
    n = snprintf(out, (size_t)out_cap, "%s\\%s", dir, name);
#else
    n = snprintf(out, (size_t)out_cap, "%s/%s", dir, name);
#endif
    if (n < 0 || n >= out_cap)
    {
        return PFS_ENAMETOOLONG;
    }
    return PFS_OK;
}

static int pfs_squashfs_mk_add_node(pfs_squashfs_mk_ctx_t *p_ctx, int parent_idx, const char *host_path,
        const char *name, bool is_dir, uint64_t file_size, uint32_t mode, uint32_t mtime)
{
    pfs_squashfs_mk_node_t *p_node;
    pfs_squashfs_mk_node_t *p_new;
    int idx;

    if (p_ctx->n_nodes >= PFS_SQUASHFS_MK_MAX_NODES)
    {
        return PFS_ERANGE;
    }
    if (p_ctx->n_nodes >= p_ctx->n_cap)
    {
        int ncap = (p_ctx->n_cap > 0) ? (p_ctx->n_cap * 2) : 64;
        p_new = (pfs_squashfs_mk_node_t *)pfsmtd_realloc(p_ctx->nodes, (size_t)ncap * sizeof(*p_new));
        if (NULL == p_new)
        {
            return PFS_ENOMEM;
        }
        p_ctx->nodes = p_new;
        p_ctx->n_cap = ncap;
    }

    idx = p_ctx->n_nodes;
    p_node = &p_ctx->nodes[idx];
    memset(p_node, 0, sizeof(*p_node));
    (void)strncpy(p_node->host_path, host_path, sizeof(p_node->host_path) - 1u);
    (void)strncpy(p_node->name, name, sizeof(p_node->name) - 1u);
    p_node->is_dir = is_dir;
    p_node->parent_idx = parent_idx;
    p_node->child_head = -1;
    p_node->sibling = -1;
    p_node->file_size = file_size;
    p_node->mode = mode;
    p_node->mtime = mtime;
    p_node->fragment = PFS_SQUASHFS_INVALID_FRAG;
    p_node->blob_off = -1;

    p_ctx->next_ino++;
    p_node->inode_number = p_ctx->next_ino;

    if (parent_idx >= 0)
    {
        p_node->sibling = p_ctx->nodes[parent_idx].child_head;
        p_ctx->nodes[parent_idx].child_head = idx;
    }

    p_ctx->n_nodes++;
    return idx;
}

static int pfs_squashfs_mk_cmp_name(const void *a, const void *b)
{
    const pfs_squashfs_mk_node_t *pa = (const pfs_squashfs_mk_node_t *)a;
    const pfs_squashfs_mk_node_t *pb = (const pfs_squashfs_mk_node_t *)b;
    return strcmp(pa->name, pb->name);
}

static int pfs_squashfs_mk_sort_children(pfs_squashfs_mk_ctx_t *p_ctx, int dir_idx)
{
    int kids[PFS_SQUASHFS_MK_MAX_NODES];
    int n = 0;
    int idx;

    idx = p_ctx->nodes[dir_idx].child_head;
    while (idx >= 0)
    {
        kids[n] = idx;
        n++;
        idx = p_ctx->nodes[idx].sibling;
    }
    if (n <= 1)
    {
        return PFS_OK;
    }

    // 简单插入排序 (按 name)
    for (int i = 1; i < n; i++)
    {
        int key = kids[i];
        int j = i - 1;
        while (j >= 0 && strcmp(p_ctx->nodes[kids[j]].name, p_ctx->nodes[key].name) > 0)
        {
            kids[j + 1] = kids[j];
            j--;
        }
        kids[j + 1] = key;
    }

    p_ctx->nodes[dir_idx].child_head = kids[0];
    for (int i = 0; i < n - 1; i++)
    {
        p_ctx->nodes[kids[i]].sibling = kids[i + 1];
    }
    p_ctx->nodes[kids[n - 1]].sibling = -1;
    (void)pfs_squashfs_mk_cmp_name;
    return PFS_OK;
}

#ifdef _WIN32
static int pfs_squashfs_mk_utf8_to_wide(const char *utf8, wchar_t **p_out)
{
    int n;
    wchar_t *w;

    n = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    if (n <= 0)
    {
        return PFS_EINVAL;
    }
    w = (wchar_t *)pfsmtd_malloc((size_t)n * sizeof(wchar_t));
    if (NULL == w)
    {
        return PFS_ENOMEM;
    }
    if (MultiByteToWideChar(CP_UTF8, 0, utf8, -1, w, n) <= 0)
    {
        pfsmtd_free(w);
        return PFS_EINVAL;
    }
    *p_out = w;
    return PFS_OK;
}

static int pfs_squashfs_mk_wide_to_utf8(const wchar_t *w, char *out, int out_cap)
{
    int n;

    n = WideCharToMultiByte(CP_UTF8, 0, w, -1, out, out_cap, NULL, NULL);
    if (n <= 0 || n > out_cap)
    {
        return PFS_EINVAL;
    }
    return PFS_OK;
}

static int pfs_squashfs_mk_scan_dir(pfs_squashfs_mk_ctx_t *p_ctx, int dir_idx);

static int pfs_squashfs_mk_scan_dir(pfs_squashfs_mk_ctx_t *p_ctx, int dir_idx)
{
    wchar_t *p_wdir;
    wchar_t pattern[PFS_SQUASHFS_MK_PATH_MAX + 8];
    WIN32_FIND_DATAW fd;
    HANDLE h;

    int rc = pfs_squashfs_mk_utf8_to_wide(p_ctx->nodes[dir_idx].host_path, &p_wdir);
    if (PFS_OK != rc)
    {
        return rc;
    }
    (void)_snwprintf(pattern, (sizeof(pattern) / sizeof(pattern[0])) - 1, L"%s\\*", p_wdir);
    pattern[(sizeof(pattern) / sizeof(pattern[0])) - 1] = L'\0';
    pfsmtd_free(p_wdir);

    h = FindFirstFileW(pattern, &fd);
    if (INVALID_HANDLE_VALUE == h)
    {
        return PFS_ENOENT;
    }
    do
    {
        char name_utf8[PFS_SQUASHFS_MK_NAME_MAX + 1];
        char child_path[PFS_SQUASHFS_MK_PATH_MAX];
        bool is_dir;
        uint64_t fsz;
        int child_idx;
        uint32_t mode;
        uint32_t mtime;

        if (0 == wcscmp(fd.cFileName, L".") || 0 == wcscmp(fd.cFileName, L".."))
        {
            continue;
        }
        if (0 != (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
        {
            continue;
        }
        rc = pfs_squashfs_mk_wide_to_utf8(fd.cFileName, name_utf8, (int)sizeof(name_utf8));
        if (PFS_OK != rc)
        {
            FindClose(h);
            return rc;
        }
        if ((int)strlen(name_utf8) > PFS_SQUASHFS_NAME_LEN)
        {
            FindClose(h);
            return PFS_ENAMETOOLONG;
        }
        is_dir = (0 != (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
        fsz = 0;
        if (!is_dir)
        {
            fsz = ((uint64_t)fd.nFileSizeHigh << 32) | (uint64_t)fd.nFileSizeLow;
        }
        rc = pfs_squashfs_mk_path_join(child_path, (int)sizeof(child_path),
                p_ctx->nodes[dir_idx].host_path, name_utf8);
        if (PFS_OK != rc)
        {
            FindClose(h);
            return rc;
        }
        mode = is_dir ? 0755u : 0644u;
        mtime = p_ctx->mkfs_time;
        child_idx = pfs_squashfs_mk_add_node(p_ctx, dir_idx, child_path, name_utf8, is_dir, fsz, mode, mtime);
        if (child_idx < 0)
        {
            FindClose(h);
            return child_idx;
        }
        if (is_dir)
        {
            rc = pfs_squashfs_mk_scan_dir(p_ctx, child_idx);
            if (PFS_OK != rc)
            {
                FindClose(h);
                return rc;
            }
        }
    } while (FindNextFileW(h, &fd));
    FindClose(h);

    return pfs_squashfs_mk_sort_children(p_ctx, dir_idx);
}
#else
static int pfs_squashfs_mk_scan_dir(pfs_squashfs_mk_ctx_t *p_ctx, int dir_idx)
{
    DIR *p_d;
    struct dirent *p_ent;

    p_d = opendir(p_ctx->nodes[dir_idx].host_path);
    if (NULL == p_d)
    {
        return PFS_ENOENT;
    }
    while (NULL != (p_ent = readdir(p_d)))
    {
        char child_path[PFS_SQUASHFS_MK_PATH_MAX];
        struct stat st;
        bool is_dir;
        int child_idx;
        uint32_t mode;

        if (0 == strcmp(p_ent->d_name, ".") || 0 == strcmp(p_ent->d_name, ".."))
        {
            continue;
        }
        if ((int)strlen(p_ent->d_name) > PFS_SQUASHFS_NAME_LEN)
        {
            closedir(p_d);
            return PFS_ENAMETOOLONG;
        }
        int rc = pfs_squashfs_mk_path_join(child_path, (int)sizeof(child_path),
                p_ctx->nodes[dir_idx].host_path, p_ent->d_name);
        if (PFS_OK != rc)
        {
            closedir(p_d);
            return rc;
        }
        if (0 != lstat(child_path, &st))
        {
            closedir(p_d);
            return PFS_EIO;
        }
        if (S_ISLNK(st.st_mode))
        {
            continue;
        }
        is_dir = S_ISDIR(st.st_mode);
        if (!is_dir && !S_ISREG(st.st_mode))
        {
            continue;
        }
        mode = (uint32_t)(st.st_mode & 0777);
        if (0 == mode)
        {
            mode = is_dir ? 0755u : 0644u;
        }
        child_idx = pfs_squashfs_mk_add_node(p_ctx, dir_idx, child_path, p_ent->d_name, is_dir,
                is_dir ? 0 : (uint64_t)st.st_size, mode, (uint32_t)st.st_mtime);
        if (child_idx < 0)
        {
            closedir(p_d);
            return child_idx;
        }
        if (is_dir)
        {
            rc = pfs_squashfs_mk_scan_dir(p_ctx, child_idx);
            if (PFS_OK != rc)
            {
                closedir(p_d);
                return rc;
            }
        }
    }
    closedir(p_d);
    return pfs_squashfs_mk_sort_children(p_ctx, dir_idx);
}
#endif

static int pfs_squashfs_mk_flush_frag(pfs_squashfs_mk_ctx_t *p_ctx)
{
    uint32_t size_code;
    uint64_t start_block;
    pfs_squashfs_mk_frag_t *p_new;

    if (0u == p_ctx->frag_used)
    {
        return PFS_OK;
    }
    if (p_ctx->n_frags >= p_ctx->n_frags_cap)
    {
        uint32_t ncap = (p_ctx->n_frags_cap > 0u) ? (p_ctx->n_frags_cap * 2u) : 16u;
        p_new = (pfs_squashfs_mk_frag_t *)pfsmtd_realloc(p_ctx->frags, (size_t)ncap * sizeof(*p_new));
        if (NULL == p_new)
        {
            return PFS_ENOMEM;
        }
        p_ctx->frags = p_new;
        p_ctx->n_frags_cap = ncap;
    }

    start_block = p_ctx->cursor;
    size_code = 0;
    int rc = pfs_squashfs_mk_store_data_block(p_ctx, p_ctx->frag_buf, (int)p_ctx->frag_used, &size_code);
    if (PFS_OK != rc)
    {
        return rc;
    }
    p_ctx->frags[p_ctx->n_frags].start_block = start_block;
    p_ctx->frags[p_ctx->n_frags].size_code = size_code;
    p_ctx->n_frags++;
    p_ctx->frag_used = 0;
    return PFS_OK;
}

static int pfs_squashfs_mk_write_file_data(pfs_squashfs_mk_ctx_t *p_ctx, int node_idx)
{
    pfs_squashfs_mk_node_t *p_node;
    FILE *fp;
    uint64_t left;
    uint32_t nblocks;

    p_node = &p_ctx->nodes[node_idx];
    int rc;
    if (p_node->is_dir || 0u == p_node->file_size)
    {
        p_node->nblocks = 0;
        p_node->fragment = PFS_SQUASHFS_INVALID_FRAG;
        return PFS_OK;
    }

    nblocks = (uint32_t)(p_node->file_size / p_ctx->block_size);
    if ((uint64_t)nblocks != (p_node->file_size / p_ctx->block_size))
    {
        return PFS_ERANGE; // block_list 项数超出 u32
    }
    p_node->nblocks = nblocks;
    if (nblocks > 0u)
    {
        p_node->block_list = (uint32_t *)pfsmtd_mallocz((size_t)nblocks * sizeof(uint32_t));
        if (NULL == p_node->block_list)
        {
            return PFS_ENOMEM;
        }
    }

    fp = fopen(p_node->host_path, "rb");
    if (NULL == fp)
    {
        return PFS_ENOENT;
    }

    p_node->data_start = p_ctx->cursor;
    left = p_node->file_size;

    for (uint32_t i = 0; i < nblocks; i++)
    {
        size_t nr = fread(p_ctx->copy_buf, 1, (size_t)p_ctx->block_size, fp);
        uint32_t size_code;

        if (nr != (size_t)p_ctx->block_size)
        {
            fclose(fp);
            return PFS_EIO;
        }
        rc = pfs_squashfs_mk_store_data_block(p_ctx, p_ctx->copy_buf, (int)nr, &size_code);
        if (PFS_OK != rc)
        {
            fclose(fp);
            return rc;
        }
        p_node->block_list[i] = size_code;
        left -= (uint64_t)nr;
    }

    if (left > 0u)
    {
        size_t nr = fread(p_ctx->copy_buf, 1, (size_t)left, fp);
        if (nr != (size_t)left)
        {
            fclose(fp);
            return PFS_EIO;
        }
        if (p_ctx->frag_used + (uint32_t)nr > p_ctx->block_size)
        {
            rc = pfs_squashfs_mk_flush_frag(p_ctx);
            if (PFS_OK != rc)
            {
                fclose(fp);
                return rc;
            }
        }
        p_node->frag_offset = p_ctx->frag_used;
        p_node->fragment = p_ctx->n_frags;
        memcpy(p_ctx->frag_buf + p_ctx->frag_used, p_ctx->copy_buf, nr);
        p_ctx->frag_used += (uint32_t)nr;
        if (p_ctx->frag_used >= p_ctx->block_size)
        {
            rc = pfs_squashfs_mk_flush_frag(p_ctx);
            if (PFS_OK != rc)
            {
                fclose(fp);
                return rc;
            }
        }
    }
    else
    {
        p_node->fragment = PFS_SQUASHFS_INVALID_FRAG;
    }

    fclose(fp);
    return PFS_OK;
}

static int pfs_squashfs_mk_emit_metablocks(pfs_squashfs_mk_ctx_t *p_ctx, const uint8_t *blob, int blob_len,
        uint64_t *p_table_start)
{
    int pos;
    uint8_t hdr[2];

    *p_table_start = p_ctx->cursor;
    pos = 0;
    while (pos < blob_len)
    {
        int chunk = blob_len - pos;
        if (chunk > PFS_SQUASHFS_METADATA_SIZE)
        {
            chunk = PFS_SQUASHFS_METADATA_SIZE;
        }
        pfs_squashfs_mk_put_le16(hdr, (uint16_t)((unsigned int)chunk | PFS_SQUASHFS_COMPRESSED_BIT));
        int rc = pfs_squashfs_mk_pwrite(p_ctx, p_ctx->cursor, hdr, 2);
        if (PFS_OK != rc)
        {
            return rc;
        }
        p_ctx->cursor += 2;
        rc = pfs_squashfs_mk_pwrite(p_ctx, p_ctx->cursor, blob + pos, chunk);
        if (PFS_OK != rc)
        {
            return rc;
        }
        p_ctx->cursor += (uint64_t)chunk;
        pos += chunk;
    }
    if (0 == blob_len)
    {
        // 空表仍占位: 写一个空 metablock 头+0 不合法; 保持 cursor, table 指向当前位置
    }
    return PFS_OK;
}

// 短型 REG: start/size 均为 u32; 超限改 LREG (本栈 reader 已支持)
static int pfs_squashfs_mk_need_lreg(const pfs_squashfs_mk_node_t *p_node)
{
    if (NULL == p_node || p_node->is_dir)
    {
        return 0;
    }
    if (p_node->file_size > 0xffffffffu)
    {
        return 1;
    }
    if (p_node->data_start > 0xffffffffu)
    {
        return 1;
    }
    return 0;
}

// 短型 DIR: file_size 为 u16; 超限改 LDIR (无目录索引; i_count=0)
static int pfs_squashfs_mk_need_ldir(const pfs_squashfs_mk_node_t *p_node)
{
    if (NULL == p_node || !p_node->is_dir)
    {
        return 0;
    }
    return (p_node->dir_size > 0xffffu) ? 1 : 0;
}

static uint32_t pfs_squashfs_mk_parent_ino(const pfs_squashfs_mk_ctx_t *p_ctx, const pfs_squashfs_mk_node_t *p_node)
{
    if (p_node->parent_idx >= 0)
    {
        return p_ctx->nodes[p_node->parent_idx].inode_number;
    }
    return 1;
}

static int pfs_squashfs_mk_append_block_list(pfs_squashfs_mk_ctx_t *p_ctx, const pfs_squashfs_mk_node_t *p_node)
{
    uint32_t b;

    for (b = 0; b < p_node->nblocks; b++)
    {
        uint8_t be[4];

        pfs_squashfs_mk_put_le32(be, p_node->block_list[b]);
        int rc = pfs_squashfs_mk_blob_append(&p_ctx->inode_blob, &p_ctx->inode_blob_len,
                &p_ctx->inode_blob_cap, be, 4);
        if (PFS_OK != rc)
        {
            return rc;
        }
    }
    return PFS_OK;
}

static int pfs_squashfs_mk_build_inodes(pfs_squashfs_mk_ctx_t *p_ctx)
{
    uint8_t raw[64];

    for (int i = 0; i < p_ctx->n_nodes; i++)
    {
        pfs_squashfs_mk_node_t *p_node = &p_ctx->nodes[i];
        int off = p_ctx->inode_blob_len;

        p_node->blob_off = off;
        int rc;
        if (p_node->is_dir)
        {
            uint32_t parent = pfs_squashfs_mk_parent_ino(p_ctx, p_node);

            memset(raw, 0, sizeof(raw));
            if (pfs_squashfs_mk_need_ldir(p_node))
            {
                // LDIR: file_size 为 u32; 无 index (i_count=0)
                pfs_squashfs_mk_put_le16(raw + 0, PFS_SQUASHFS_LDIR_TYPE);
                pfs_squashfs_mk_put_le16(raw + 2, (uint16_t)(p_node->mode));
                pfs_squashfs_mk_put_le16(raw + 4, 0);
                pfs_squashfs_mk_put_le16(raw + 6, 0);
                pfs_squashfs_mk_put_le32(raw + 8, p_node->mtime);
                pfs_squashfs_mk_put_le32(raw + 12, p_node->inode_number);
                pfs_squashfs_mk_put_le32(raw + 16, 2); // nlink
                pfs_squashfs_mk_put_le32(raw + 20, p_node->dir_size);
                pfs_squashfs_mk_put_le32(raw + 24, p_node->dir_start_block);
                pfs_squashfs_mk_put_le32(raw + 28, parent);
                pfs_squashfs_mk_put_le16(raw + 32, 0); // i_count
                pfs_squashfs_mk_put_le16(raw + 34, p_node->dir_offset);
                pfs_squashfs_mk_put_le32(raw + 36, 0xffffffffu); // xattr = INVALID_XATTR
                rc = pfs_squashfs_mk_blob_append(&p_ctx->inode_blob, &p_ctx->inode_blob_len,
                        &p_ctx->inode_blob_cap, raw, (int)sizeof(pfs_squashfs_ldir_inode_t));
            }
            else
            {
                pfs_squashfs_mk_put_le16(raw + 0, PFS_SQUASHFS_DIR_TYPE);
                pfs_squashfs_mk_put_le16(raw + 2, (uint16_t)(p_node->mode));
                pfs_squashfs_mk_put_le16(raw + 4, 0);
                pfs_squashfs_mk_put_le16(raw + 6, 0);
                pfs_squashfs_mk_put_le32(raw + 8, p_node->mtime);
                pfs_squashfs_mk_put_le32(raw + 12, p_node->inode_number);
                pfs_squashfs_mk_put_le32(raw + 16, p_node->dir_start_block);
                pfs_squashfs_mk_put_le32(raw + 20, 2); // nlink
                pfs_squashfs_mk_put_le16(raw + 24, (uint16_t)p_node->dir_size);
                pfs_squashfs_mk_put_le16(raw + 26, p_node->dir_offset);
                pfs_squashfs_mk_put_le32(raw + 28, parent);
                rc = pfs_squashfs_mk_blob_append(&p_ctx->inode_blob, &p_ctx->inode_blob_len,
                        &p_ctx->inode_blob_cap, raw, (int)sizeof(pfs_squashfs_dir_inode_t));
            }
            if (PFS_OK != rc)
            {
                return rc;
            }
        }
        else if (pfs_squashfs_mk_need_lreg(p_node))
        {
            memset(raw, 0, sizeof(raw));
            pfs_squashfs_mk_put_le16(raw + 0, PFS_SQUASHFS_LREG_TYPE);
            pfs_squashfs_mk_put_le16(raw + 2, (uint16_t)(p_node->mode));
            pfs_squashfs_mk_put_le16(raw + 4, 0);
            pfs_squashfs_mk_put_le16(raw + 6, 0);
            pfs_squashfs_mk_put_le32(raw + 8, p_node->mtime);
            pfs_squashfs_mk_put_le32(raw + 12, p_node->inode_number);
            pfs_squashfs_mk_put_le64(raw + 16, p_node->data_start);
            pfs_squashfs_mk_put_le64(raw + 24, p_node->file_size);
            pfs_squashfs_mk_put_le64(raw + 32, 0); // sparse
            pfs_squashfs_mk_put_le32(raw + 40, 1); // nlink
            pfs_squashfs_mk_put_le32(raw + 44, p_node->fragment);
            pfs_squashfs_mk_put_le32(raw + 48, p_node->frag_offset);
            pfs_squashfs_mk_put_le32(raw + 52, 0xffffffffu); // xattr = INVALID_XATTR
            rc = pfs_squashfs_mk_blob_append(&p_ctx->inode_blob, &p_ctx->inode_blob_len,
                    &p_ctx->inode_blob_cap, raw, (int)sizeof(pfs_squashfs_lreg_inode_t));
            if (PFS_OK != rc)
            {
                return rc;
            }
            rc = pfs_squashfs_mk_append_block_list(p_ctx, p_node);
            if (PFS_OK != rc)
            {
                return rc;
            }
        }
        else
        {
            memset(raw, 0, sizeof(raw));
            pfs_squashfs_mk_put_le16(raw + 0, PFS_SQUASHFS_REG_TYPE);
            pfs_squashfs_mk_put_le16(raw + 2, (uint16_t)(p_node->mode));
            pfs_squashfs_mk_put_le16(raw + 4, 0);
            pfs_squashfs_mk_put_le16(raw + 6, 0);
            pfs_squashfs_mk_put_le32(raw + 8, p_node->mtime);
            pfs_squashfs_mk_put_le32(raw + 12, p_node->inode_number);
            pfs_squashfs_mk_put_le32(raw + 16, (uint32_t)p_node->data_start);
            pfs_squashfs_mk_put_le32(raw + 20, p_node->fragment);
            pfs_squashfs_mk_put_le32(raw + 24, p_node->frag_offset);
            pfs_squashfs_mk_put_le32(raw + 28, (uint32_t)p_node->file_size);
            rc = pfs_squashfs_mk_blob_append(&p_ctx->inode_blob, &p_ctx->inode_blob_len,
                    &p_ctx->inode_blob_cap, raw, (int)sizeof(pfs_squashfs_reg_inode_t));
            if (PFS_OK != rc)
            {
                return rc;
            }
            rc = pfs_squashfs_mk_append_block_list(p_ctx, p_node);
            if (PFS_OK != rc)
            {
                return rc;
            }
        }
    }
    return PFS_OK;
}

static void pfs_squashfs_mk_meta_loc(int blob_off, int blob_len, unsigned int *p_blk_rel, unsigned int *p_off)
{
    int pos = 0;
    int abs_fake = 0;

    (void)blob_len;
    while (pos + PFS_SQUASHFS_METADATA_SIZE <= blob_off)
    {
        abs_fake += 2 + PFS_SQUASHFS_METADATA_SIZE;
        pos += PFS_SQUASHFS_METADATA_SIZE;
    }
    *p_blk_rel = (unsigned int)abs_fake;
    *p_off = (unsigned int)(blob_off - pos);
}

static void pfs_squashfs_mk_assign_inode_refs(pfs_squashfs_mk_ctx_t *p_ctx)
{
    for (int i = 0; i < p_ctx->n_nodes; i++)
    {
        unsigned int blk;
        unsigned int off;

        pfs_squashfs_mk_meta_loc(p_ctx->nodes[i].blob_off, p_ctx->inode_blob_len, &blk, &off);
        p_ctx->nodes[i].inode_ref = (uint64_t)PFS_SQUASHFS_MKINODE(blk, off);
    }
    p_ctx->root_inode = p_ctx->nodes[0].inode_ref;
}

static int pfs_squashfs_mk_build_dirs(pfs_squashfs_mk_ctx_t *p_ctx)
{
    // inode_ref 已知后写目录项 (iblk/ioff 用 inode 表相对 metablock 位置)

    pfsmtd_free(p_ctx->dir_blob);
    p_ctx->dir_blob = NULL;
    p_ctx->dir_blob_len = 0;
    p_ctx->dir_blob_cap = 0;

    for (int i = 0; i < p_ctx->n_nodes; i++)
    {
        pfs_squashfs_mk_node_t *p_dir = &p_ctx->nodes[i];
        int child;
        int start_off;
        unsigned int dblk;
        unsigned int doff;

        if (!p_dir->is_dir)
        {
            continue;
        }
        start_off = p_ctx->dir_blob_len;
        child = p_dir->child_head;
        while (child >= 0)
        {
            pfs_squashfs_mk_node_t *p_ch = &p_ctx->nodes[child];
            uint8_t hdr[12];
            uint8_t ent[8];
            uint16_t namelen;
            uint16_t dtype;
            unsigned int iblk;
            unsigned int ioff;

            namelen = (uint16_t)strlen(p_ch->name);
            if (0u == namelen || namelen > PFS_SQUASHFS_MK_NAME_MAX)
            {
                return PFS_EINVAL;
            }
            dtype = p_ch->is_dir ? PFS_SQUASHFS_DIR_TYPE : PFS_SQUASHFS_REG_TYPE;
            iblk = (unsigned int)PFS_SQUASHFS_INODE_BLK(p_ch->inode_ref);
            ioff = (unsigned int)PFS_SQUASHFS_INODE_OFFSET(p_ch->inode_ref);

            memset(hdr, 0, sizeof(hdr));
            pfs_squashfs_mk_put_le32(hdr + 0, 0); // count=0 => 1 entry
            pfs_squashfs_mk_put_le32(hdr + 4, iblk);
            pfs_squashfs_mk_put_le32(hdr + 8, p_ch->inode_number);
            int rc = pfs_squashfs_mk_blob_append(&p_ctx->dir_blob, &p_ctx->dir_blob_len, &p_ctx->dir_blob_cap,
                    hdr, 12);
            if (PFS_OK != rc)
            {
                return rc;
            }
            memset(ent, 0, sizeof(ent));
            pfs_squashfs_mk_put_le16(ent + 0, (uint16_t)ioff);
            pfs_squashfs_mk_put_le16(ent + 2, 0);
            pfs_squashfs_mk_put_le16(ent + 4, dtype);
            pfs_squashfs_mk_put_le16(ent + 6, (uint16_t)(namelen - 1u));
            rc = pfs_squashfs_mk_blob_append(&p_ctx->dir_blob, &p_ctx->dir_blob_len, &p_ctx->dir_blob_cap,
                    ent, 8);
            if (PFS_OK != rc)
            {
                return rc;
            }
            rc = pfs_squashfs_mk_blob_append(&p_ctx->dir_blob, &p_ctx->dir_blob_len, &p_ctx->dir_blob_cap,
                    p_ch->name, (int)namelen);
            if (PFS_OK != rc)
            {
                return rc;
            }
            child = p_ch->sibling;
        }
        // 与本栈 reader 对齐: file_size = 目录表字节数 (不含内核 . / .. 的 +3)
        p_dir->dir_size = (uint32_t)(p_ctx->dir_blob_len - start_off);
        pfs_squashfs_mk_meta_loc(start_off, p_ctx->dir_blob_len, &dblk, &doff);
        p_dir->dir_start_block = dblk;
        p_dir->dir_offset = (uint16_t)doff;
    }
    return PFS_OK;
}

static int pfs_squashfs_mk_write_super(pfs_squashfs_mk_ctx_t *p_ctx)
{
    uint8_t sb[sizeof(pfs_squashfs_super_block_t)];
    uint16_t flags;

    memset(sb, 0, sizeof(sb));
    if (PFS_SQUASHFS_COMPRESS_NONE == p_ctx->compress)
    {
        flags = (1u << PFS_SQUASHFS_NOI) | (1u << PFS_SQUASHFS_NOD) | (1u << PFS_SQUASHFS_NOF);
    }
    else
    {
        // gzip: 数据/fragment 可压缩; 元数据表首版仍未压缩 (NOI)
        flags = (1u << PFS_SQUASHFS_NOI);
    }

    pfs_squashfs_mk_put_le32(sb + 0, PFS_SQUASHFS_MAGIC);
    pfs_squashfs_mk_put_le32(sb + 4, (uint32_t)p_ctx->n_nodes);
    pfs_squashfs_mk_put_le32(sb + 8, p_ctx->mkfs_time);
    pfs_squashfs_mk_put_le32(sb + 12, p_ctx->block_size);
    pfs_squashfs_mk_put_le32(sb + 16, p_ctx->n_frags);
    pfs_squashfs_mk_put_le16(sb + 20, PFS_SQUASHFS_COMP_ZLIB);
    pfs_squashfs_mk_put_le16(sb + 22, p_ctx->block_log);
    pfs_squashfs_mk_put_le16(sb + 24, flags);
    pfs_squashfs_mk_put_le16(sb + 26, 1); // no_ids
    pfs_squashfs_mk_put_le16(sb + 28, PFS_SQUASHFS_MAJOR);
    pfs_squashfs_mk_put_le16(sb + 30, PFS_SQUASHFS_MINOR);
    pfs_squashfs_mk_put_le64(sb + 32, p_ctx->root_inode);
    pfs_squashfs_mk_put_le64(sb + 40, p_ctx->bytes_used);
    pfs_squashfs_mk_put_le64(sb + 48, p_ctx->id_table_start);
    pfs_squashfs_mk_put_le64(sb + 56, (uint64_t)PFS_SQUASHFS_INVALID_BLK); // xattr
    pfs_squashfs_mk_put_le64(sb + 64, p_ctx->inode_table);
    pfs_squashfs_mk_put_le64(sb + 72, p_ctx->directory_table);
    pfs_squashfs_mk_put_le64(sb + 80, p_ctx->fragment_table_start);
    pfs_squashfs_mk_put_le64(sb + 88, (uint64_t)PFS_SQUASHFS_INVALID_BLK); // lookup

    return pfs_squashfs_mk_pwrite(p_ctx, 0, sb, (int)sizeof(sb));
}

static int pfs_squashfs_mk_build(pfs_squashfs_mk_ctx_t *p_ctx)
{
    uint32_t fi;
    uint8_t *frag_meta = NULL;
    int frag_meta_len = 0;
    int frag_meta_cap = 0;
    uint64_t *frag_index = NULL;
    unsigned int frag_indexes;
    unsigned int frag_index_bytes;

    // 数据区从 super 之后开始
    p_ctx->cursor = sizeof(pfs_squashfs_super_block_t);

    int rc;
    for (int i = 0; i < p_ctx->n_nodes; i++)
    {
        if (!p_ctx->nodes[i].is_dir)
        {
            rc = pfs_squashfs_mk_write_file_data(p_ctx, i);
            if (PFS_OK != rc)
            {
                return rc;
            }
        }
    }
    rc = pfs_squashfs_mk_flush_frag(p_ctx);
    if (PFS_OK != rc)
    {
        return rc;
    }

    // 流程: 占位 inode(dir 字段 0) -> inode_ref -> 目录表 -> 补齐 dir 后重建 inode
    // 二次 inode 可能升级为 LDIR (尺寸变), 须再刷目录项 iblk/ioff; 目录在 blob 内布局不变故无需三刷 inode
    for (int i = 0; i < p_ctx->n_nodes; i++)
    {
        if (p_ctx->nodes[i].is_dir)
        {
            p_ctx->nodes[i].dir_size = 0;
            p_ctx->nodes[i].dir_start_block = 0;
            p_ctx->nodes[i].dir_offset = 0;
        }
    }

    rc = pfs_squashfs_mk_build_inodes(p_ctx);
    if (PFS_OK != rc)
    {
        return rc;
    }
    pfs_squashfs_mk_assign_inode_refs(p_ctx);

    rc = pfs_squashfs_mk_build_dirs(p_ctx);
    if (PFS_OK != rc)
    {
        return rc;
    }

    pfsmtd_free(p_ctx->inode_blob);
    p_ctx->inode_blob = NULL;
    p_ctx->inode_blob_len = 0;
    p_ctx->inode_blob_cap = 0;
    rc = pfs_squashfs_mk_build_inodes(p_ctx);
    if (PFS_OK != rc)
    {
        return rc;
    }
    pfs_squashfs_mk_assign_inode_refs(p_ctx);

    rc = pfs_squashfs_mk_build_dirs(p_ctx);
    if (PFS_OK != rc)
    {
        return rc;
    }

    // 写 inode table
    rc = pfs_squashfs_mk_emit_metablocks(p_ctx, p_ctx->inode_blob, p_ctx->inode_blob_len, &p_ctx->inode_table);
    if (PFS_OK != rc)
    {
        return rc;
    }

    // 写 directory table
    rc = pfs_squashfs_mk_emit_metablocks(p_ctx, p_ctx->dir_blob, p_ctx->dir_blob_len, &p_ctx->directory_table);
    if (PFS_OK != rc)
    {
        return rc;
    }

    // fragment table
    if (p_ctx->n_frags > 0u)
    {
        for (fi = 0; fi < p_ctx->n_frags; fi++)
        {
            uint8_t ent[16];
            pfs_squashfs_mk_put_le64(ent + 0, p_ctx->frags[fi].start_block);
            pfs_squashfs_mk_put_le32(ent + 8, p_ctx->frags[fi].size_code);
            pfs_squashfs_mk_put_le32(ent + 12, 0);
            rc = pfs_squashfs_mk_blob_append(&frag_meta, &frag_meta_len, &frag_meta_cap, ent, 16);
            if (PFS_OK != rc)
            {
                pfsmtd_free(frag_meta);
                return rc;
            }
        }
        // 写 fragment metadata, 记录每个 metablock 起点到 index
        frag_indexes = PFS_SQUASHFS_FRAGMENT_INDEXES(p_ctx->n_frags);
        frag_index_bytes = PFS_SQUASHFS_FRAGMENT_INDEX_BYTES(p_ctx->n_frags);
        frag_index = (uint64_t *)pfsmtd_mallocz(frag_index_bytes);
        if (NULL == frag_index)
        {
            pfsmtd_free(frag_meta);
            return PFS_ENOMEM;
        }
        {
            int pos = 0;
            unsigned int idx = 0;
            uint64_t meta_start = p_ctx->cursor;
            while (pos < frag_meta_len)
            {
                int chunk = frag_meta_len - pos;
                uint8_t hdr[2];
                if (chunk > PFS_SQUASHFS_METADATA_SIZE)
                {
                    chunk = PFS_SQUASHFS_METADATA_SIZE;
                }
                if (idx < frag_indexes)
                {
                    frag_index[idx] = p_ctx->cursor;
                    idx++;
                }
                pfs_squashfs_mk_put_le16(hdr, (uint16_t)((unsigned int)chunk | PFS_SQUASHFS_COMPRESSED_BIT));
                rc = pfs_squashfs_mk_pwrite(p_ctx, p_ctx->cursor, hdr, 2);
                if (PFS_OK != rc)
                {
                    pfsmtd_free(frag_meta);
                    pfsmtd_free(frag_index);
                    return rc;
                }
                p_ctx->cursor += 2;
                rc = pfs_squashfs_mk_pwrite(p_ctx, p_ctx->cursor, frag_meta + pos, chunk);
                if (PFS_OK != rc)
                {
                    pfsmtd_free(frag_meta);
                    pfsmtd_free(frag_index);
                    return rc;
                }
                p_ctx->cursor += (uint64_t)chunk;
                pos += chunk;
            }
            (void)meta_start;
        }
        p_ctx->fragment_table_start = p_ctx->cursor;
        for (fi = 0; fi < frag_indexes; fi++)
        {
            uint8_t be[8];
            pfs_squashfs_mk_put_le64(be, frag_index[fi]);
            rc = pfs_squashfs_mk_pwrite(p_ctx, p_ctx->cursor, be, 8);
            if (PFS_OK != rc)
            {
                pfsmtd_free(frag_meta);
                pfsmtd_free(frag_index);
                return rc;
            }
            p_ctx->cursor += 8;
        }
        pfsmtd_free(frag_meta);
        pfsmtd_free(frag_index);
    }
    else
    {
        p_ctx->fragment_table_start = p_ctx->cursor;
    }

    // id table: metadata(uid=0) + index u64
    {
        uint8_t id_meta[4];
        uint8_t hdr[2];
        uint8_t idx8[8];
        uint64_t id_meta_off;

        id_meta_off = p_ctx->cursor;
        pfs_squashfs_mk_put_le32(id_meta, 0);
        pfs_squashfs_mk_put_le16(hdr, (uint16_t)(4u | PFS_SQUASHFS_COMPRESSED_BIT));
        rc = pfs_squashfs_mk_pwrite(p_ctx, p_ctx->cursor, hdr, 2);
        if (PFS_OK != rc)
        {
            return rc;
        }
        p_ctx->cursor += 2;
        rc = pfs_squashfs_mk_pwrite(p_ctx, p_ctx->cursor, id_meta, 4);
        if (PFS_OK != rc)
        {
            return rc;
        }
        p_ctx->cursor += 4;
        p_ctx->id_table_start = p_ctx->cursor;
        pfs_squashfs_mk_put_le64(idx8, id_meta_off);
        rc = pfs_squashfs_mk_pwrite(p_ctx, p_ctx->cursor, idx8, 8);
        if (PFS_OK != rc)
        {
            return rc;
        }
        p_ctx->cursor += 8;
    }

    p_ctx->bytes_used = p_ctx->cursor;
    return pfs_squashfs_mk_write_super(p_ctx);
}

static int pfs_squashfs_mk_prepare(pfs_squashfs_mk_ctx_t *p_ctx, pfs_blkio_t *p_blkio, int64_t part_offset,
        int compress)
{
    int resolved;

    int rc = pfs_squashfs_mk_resolve_compress(compress, &resolved);
    if (PFS_OK != rc)
    {
        return rc;
    }

    memset(p_ctx, 0, sizeof(*p_ctx));
    p_ctx->p_blkio = p_blkio;
    p_ctx->part_offset = part_offset;
    p_ctx->block_size = PFS_SQUASHFS_MK_BLOCK_SIZE;
    p_ctx->block_log = PFS_SQUASHFS_MK_BLOCK_LOG;
    p_ctx->mkfs_time = (uint32_t)time(NULL);
    p_ctx->compress = resolved;
    p_ctx->next_ino = 0;
    p_ctx->comp_buf_cap = (int)p_ctx->block_size + PFS_SQUASHFS_MK_COMP_OVERHEAD;
    p_ctx->copy_buf = (uint8_t *)pfsmtd_malloc(PFS_SQUASHFS_MK_COPY_CHUNK);
    p_ctx->comp_buf = (uint8_t *)pfsmtd_malloc((size_t)p_ctx->comp_buf_cap);
    p_ctx->frag_buf = (uint8_t *)pfsmtd_malloc(p_ctx->block_size);
    if (NULL == p_ctx->copy_buf || NULL == p_ctx->comp_buf || NULL == p_ctx->frag_buf)
    {
        pfs_squashfs_mk_ctx_free(p_ctx);
        return PFS_ENOMEM;
    }
    return PFS_OK;
}

int pfs_squashfs_mkvol_ex(pfs_blkio_t *p_blkio, int64_t part_offset, const char *src_dir, const char *volume_id,
        int compress)
{
    (void)volume_id;
    if (NULL == p_blkio || part_offset < 0 || NULL == src_dir || '\0' == src_dir[0])
    {
        return PFS_EINVAL;
    }
    if (!pfs_blkio_is_open(p_blkio))
    {
        return PFS_EINVAL;
    }

    pfs_squashfs_mk_ctx_t ctx;
    int root_idx;

    int rc = pfs_squashfs_mk_prepare(&ctx, p_blkio, part_offset, compress);
    if (PFS_OK != rc)
    {
        return rc;
    }

    root_idx = pfs_squashfs_mk_add_node(&ctx, -1, src_dir, "", true, 0, 0755u, ctx.mkfs_time);
    if (root_idx < 0)
    {
        pfs_squashfs_mk_ctx_free(&ctx);
        return root_idx;
    }
    rc = pfs_squashfs_mk_scan_dir(&ctx, root_idx);
    if (PFS_OK != rc)
    {
        pfs_squashfs_mk_ctx_free(&ctx);
        return rc;
    }
    rc = pfs_squashfs_mk_build(&ctx);
    pfs_squashfs_mk_ctx_free(&ctx);
    return rc;
}

int pfs_squashfs_mkvol(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes, const char *src_dir, const char *volume_id)
{
    (void)vol_bytes;
    return pfs_squashfs_mkvol_ex(p_blkio, part_offset, src_dir, volume_id, PFS_SQUASHFS_COMPRESS_DEFAULT);
}

int pfs_squashfs_mkfs_ex(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes, int compress)
{
    (void)vol_bytes;
    if (NULL == p_blkio || part_offset < 0)
    {
        return PFS_EINVAL;
    }
    if (!pfs_blkio_is_open(p_blkio))
    {
        return PFS_EINVAL;
    }

    pfs_squashfs_mk_ctx_t ctx;
    int root_idx;

    int rc = pfs_squashfs_mk_prepare(&ctx, p_blkio, part_offset, compress);
    if (PFS_OK != rc)
    {
        return rc;
    }

    root_idx = pfs_squashfs_mk_add_node(&ctx, -1, ".", "", true, 0, 0755u, ctx.mkfs_time);
    if (root_idx < 0)
    {
        pfs_squashfs_mk_ctx_free(&ctx);
        return root_idx;
    }
    rc = pfs_squashfs_mk_build(&ctx);
    pfs_squashfs_mk_ctx_free(&ctx);
    return rc;
}

int pfs_squashfs_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes)
{
    return pfs_squashfs_mkfs_ex(p_blkio, part_offset, vol_bytes, PFS_SQUASHFS_COMPRESS_DEFAULT);
}

// end
