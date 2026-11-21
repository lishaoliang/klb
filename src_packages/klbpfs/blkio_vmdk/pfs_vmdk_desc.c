// Doc Encode : UTF-8 BOM, Unix(LF)
// VMDK 文本描述符解析 + 路径拼接
#include "blkio_vmdk/pfs_vmdk_internal.h"
#include "base/pfs_blkio_raw.h"
#include "base/pfs_mem.h"

#include <stdint.h>
#include <string.h>

int pfs_vmdk_path_is_abs(const char *path)
{
    if (NULL == path || '\0' == path[0])
    {
        return 0;
    }
    if ('/' == path[0] || '\\' == path[0])
    {
        return 1;
    }
    // Windows: "C:\..." / "C:/..."
    if (((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z')) &&
            ':' == path[1] && ('/' == path[2] || '\\' == path[2]))
    {
        return 1;
    }
    return 0;
}

int pfs_vmdk_path_dirname(const char *path, char *out, size_t out_sz)
{
    if (NULL == path || NULL == out || 0 == out_sz)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    size_t len = strlen(path);
    if (0 == len)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    size_t last = len;
    while (last > 0 && ('/' == path[last - 1] || '\\' == path[last - 1]))
    {
        last--;
    }
    size_t i = last;
    while (i > 0 && '/' != path[i - 1] && '\\' != path[i - 1])
    {
        i--;
    }
    if (0 == i)
    {
        if (out_sz < 2)
        {
            return PFS_BLKIO_RET_PARAM;
        }
        out[0] = '.';
        out[1] = '\0';
        return PFS_BLKIO_RET_OK;
    }
    // 保留分隔符前的目录 (含根 "/")
    if (i >= out_sz)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    memcpy(out, path, i);
    out[i] = '\0';
    // 去掉末尾多余分隔 (根 "/" 保留)
    while (i > 1 && ('/' == out[i - 1] || '\\' == out[i - 1]))
    {
        i--;
        out[i] = '\0';
    }
    return PFS_BLKIO_RET_OK;
}

int pfs_vmdk_path_join(const char *dir, const char *name, char *out, size_t out_sz)
{
    if (NULL == dir || NULL == name || NULL == out || 0 == out_sz)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (pfs_vmdk_path_is_abs(name))
    {
        size_t nlen = strlen(name);
        if (nlen + 1 > out_sz)
        {
            return PFS_BLKIO_RET_PARAM;
        }
        memcpy(out, name, nlen + 1);
        return PFS_BLKIO_RET_OK;
    }

    size_t dlen = strlen(dir);
    size_t nlen = strlen(name);
    int need_sep = 0;
    if (dlen > 0)
    {
        char last = dir[dlen - 1];
        if ('/' != last && '\\' != last)
        {
            need_sep = 1;
        }
    }
    if (dlen + (size_t)need_sep + nlen + 1 > out_sz)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    memcpy(out, dir, dlen);
    if (0 != need_sep)
    {
        out[dlen] = '/';
        dlen++;
    }
    memcpy(out + dlen, name, nlen + 1);
    return PFS_BLKIO_RET_OK;
}

static const char *pfs_vmdk_skip_ws(const char *p, const char *end)
{
    while (p < end && (' ' == *p || '\t' == *p || '\r' == *p))
    {
        p++;
    }
    return p;
}

static int pfs_vmdk_token_eq(const char *p, const char *end, const char *lit)
{
    size_t n = strlen(lit);
    if ((size_t)(end - p) < n)
    {
        return 0;
    }
    if (0 != strncmp(p, lit, n))
    {
        return 0;
    }
    if (p + n < end)
    {
        char c = p[n];
        if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') ||
                ('0' <= c && c <= '9') || '_' == c)
        {
            return 0;
        }
    }
    return 1;
}

static int pfs_vmdk_parse_u64(const char *p, const char *end, uint64_t *p_val, const char **p_next)
{
    uint64_t v = 0;
    int any = 0;

    while (p < end && *p >= '0' && *p <= '9')
    {
        uint64_t d = (uint64_t)(*p - '0');
        if (v > (UINT64_MAX - d) / 10ull)
        {
            return PFS_BLKIO_RET_STATE;
        }
        v = v * 10ull + d;
        p++;
        any = 1;
    }
    if (0 == any)
    {
        return PFS_BLKIO_RET_STATE;
    }
    *p_val = v;
    *p_next = p;
    return PFS_BLKIO_RET_OK;
}

static int pfs_vmdk_parse_extent_line(const char *line, size_t len, pfs_vmdk_desc_extent_t *p_ext)
{
    memset(p_ext, 0, sizeof(*p_ext));
    const char *p = line;
    const char *end = line + len;
    p = pfs_vmdk_skip_ws(p, end);
    if (p >= end)
    {
        return PFS_BLKIO_RET_PROBE;
    }

    // access: RW / RDONLY / NOACCESS
    if (pfs_vmdk_token_eq(p, end, "RW"))
    {
        p += 2;
    }
    else if (pfs_vmdk_token_eq(p, end, "RDONLY"))
    {
        p += 6;
    }
    else if (pfs_vmdk_token_eq(p, end, "NOACCESS"))
    {
        p += 8;
    }
    else
    {
        return PFS_BLKIO_RET_PROBE;
    }
    p = pfs_vmdk_skip_ws(p, end);

    uint64_t sectors;
    const char *q;
    int rc = pfs_vmdk_parse_u64(p, end, &sectors, &q);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return PFS_BLKIO_RET_STATE;
    }
    p = pfs_vmdk_skip_ws(q, end);

    int type;
    if (pfs_vmdk_token_eq(p, end, "FLAT"))
    {
        type = PFS_VMDK_EXT_FLAT;
        p += 4;
    }
    else if (pfs_vmdk_token_eq(p, end, "SPARSE"))
    {
        type = PFS_VMDK_EXT_SPARSE;
        p += 6;
    }
    else if (pfs_vmdk_token_eq(p, end, "ZERO"))
    {
        type = PFS_VMDK_EXT_ZERO;
        p += 4;
    }
    else
    {
        // VMFS / SESPARSE 等本阶段拒绝
        return PFS_BLKIO_RET_STATE;
    }
    p = pfs_vmdk_skip_ws(p, end);

    char path[PFS_VMDK_PATH_MAX];
    path[0] = '\0';
    uint64_t flat_off = 0;
    if (PFS_VMDK_EXT_ZERO != type)
    {
        if (p >= end || '"' != *p)
        {
            return PFS_BLKIO_RET_STATE;
        }
        p++;
        q = p;
        while (q < end && '"' != *q)
        {
            q++;
        }
        if (q >= end)
        {
            return PFS_BLKIO_RET_STATE;
        }
        size_t plen = (size_t)(q - p);
        if (0 == plen || plen + 1 > sizeof(path))
        {
            return PFS_BLKIO_RET_STATE;
        }
        memcpy(path, p, plen);
        path[plen] = '\0';
        p = pfs_vmdk_skip_ws(q + 1, end);

        if (PFS_VMDK_EXT_FLAT == type && p < end && *p >= '0' && *p <= '9')
        {
            rc = pfs_vmdk_parse_u64(p, end, &flat_off, &q);
            if (PFS_BLKIO_RET_OK != rc)
            {
                return PFS_BLKIO_RET_STATE;
            }
        }
    }

    p_ext->type = type;
    p_ext->sectors = sectors;
    p_ext->flat_offset_sectors = flat_off;
    memcpy(p_ext->path, path, sizeof(path));
    return PFS_BLKIO_RET_OK;
}

int pfs_vmdk_desc_parse(const char *text, size_t len, pfs_vmdk_desc_t *p_out)
{
    if (NULL == text || NULL == p_out)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    memset(p_out, 0, sizeof(*p_out));

    size_t i = 0;
    size_t line_start = 0;
    while (i <= len)
    {
        if (i == len || '\n' == text[i])
        {
            size_t line_len = i - line_start;
            const char *line = text + line_start;
            // 去掉 \r
            if (line_len > 0 && '\r' == line[line_len - 1])
            {
                line_len--;
            }
            if (line_len > 0 && '#' != line[0])
            {
                pfs_vmdk_desc_extent_t ext;
                const char *p = pfs_vmdk_skip_ws(line, line + line_len);
                if (p < line + line_len &&
                        (pfs_vmdk_token_eq(p, line + line_len, "RW") ||
                         pfs_vmdk_token_eq(p, line + line_len, "RDONLY") ||
                         pfs_vmdk_token_eq(p, line + line_len, "NOACCESS")))
                {
                    int rc = pfs_vmdk_parse_extent_line(line, line_len, &ext);
                    if (PFS_BLKIO_RET_OK != rc)
                    {
                        if (PFS_BLKIO_RET_PROBE == rc)
                        {
                            // 非 extent 行
                        }
                        else
                        {
                            return rc;
                        }
                    }
                    else
                    {
                        if (p_out->extent_count >= PFS_VMDK_EXTENT_MAX)
                        {
                            return PFS_BLKIO_RET_STATE;
                        }
                        p_out->extents[p_out->extent_count] = ext;
                        p_out->extent_count++;
                    }
                }
            }
            line_start = i + 1;
        }
        i++;
    }

    if (0 == p_out->extent_count)
    {
        return PFS_BLKIO_RET_STATE;
    }
    return PFS_BLKIO_RET_OK;
}

int pfs_vmdk_desc_load(pfs_blkio_t *p_host, int64_t byte_off, uint64_t byte_len,
        pfs_vmdk_desc_t *p_out)
{
    if (NULL == p_host || NULL == p_out || 0 > byte_off)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 == byte_len)
    {
        return PFS_BLKIO_RET_STATE;
    }
    uint64_t n = byte_len;
    if (n > (uint64_t)PFS_VMDK_DESC_MAX)
    {
        n = (uint64_t)PFS_VMDK_DESC_MAX;
    }
    if (n > (uint64_t)PFS_BLKIO_IO_SIZE_MAX)
    {
        return PFS_BLKIO_RET_STATE;
    }

    char *buf = (char *)pfs_malloc((size_t)n + 1u);
    if (NULL == buf)
    {
        return PFS_BLKIO_RET_IO;
    }
    int rc = pfs_blkio_raw_pread(p_host, buf, (int)n, byte_off);
    if (0 > rc)
    {
        pfs_free(buf);
        return rc;
    }
    else if (0 >= rc)
    {
        pfs_free(buf);
        return PFS_BLKIO_RET_IO;
    }
    buf[rc] = '\0';
    rc = pfs_vmdk_desc_parse(buf, (size_t)rc, p_out);
    pfs_free(buf);
    return rc;
}

// end
