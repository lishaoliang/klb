// Doc Encode : UTF-8 BOM, Unix(LF)
// romfs mkfs (空卷 . / ..) / mkvol (从 src_dir 打包; BE + 16 对齐)
#include "romfs/pfs_romfs_internal.h"
#include "base/pfsmtd_dirwalk.h"
#include "base/pfsmtd_mem.h"

#include <string.h>
#include <stdbool.h>

static void pfs_romfs_put_be32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)((v >> 24) & 0xffu);
    p[1] = (uint8_t)((v >> 16) & 0xffu);
    p[2] = (uint8_t)((v >> 8) & 0xffu);
    p[3] = (uint8_t)(v & 0xffu);
}

static uint32_t pfs_romfs_fix_checksum(const uint8_t *buf, uint32_t nbytes)
{
    uint32_t words = nbytes / 4u;
    uint32_t sum = 0;
    for (uint32_t i = 0; i < words; i++)
    {
        sum += pfs_romfs_be32(buf + i * 4u);
    }
    return sum;
}

static int pfs_romfs_write_all(pfs_blkio_t *p_blkio, int64_t abs_off, const void *buf, int len)
{
    int n = pfs_blkio_pwrite(p_blkio, buf, len, abs_off);
    if (n != len)
    {
        if (0 > n)
        {
            return pfs_romfs_map_blkio_rc(n);
        }
        return PFS_EIO;
    }
    return PFS_OK;
}

static int pfs_romfs_emit_empty(pfs_blkio_t *p_blkio, int64_t part_offset, const char *volume_id)
{
    uint8_t img[256];
    uint32_t vlen;
    uint32_t root_pos;
    uint32_t dot_pos;
    uint32_t dotdot_pos;
    uint32_t img_size;
    uint32_t csum;
    uint32_t words;

    memset(img, 0, sizeof(img));
    memcpy(img, PFS_ROMFS_MAGIC_WORD, 8);

    vlen = 0;
    if (NULL != volume_id)
    {
        vlen = (uint32_t)strlen(volume_id);
        if (vlen > PFS_ROMFS_MAXFN)
        {
            return PFS_ENAMETOOLONG;
        }
        memcpy(img + 16, volume_id, (size_t)vlen);
    }
    img[16 + vlen] = '\0';
    root_pos = pfs_romfs_align16(16u + vlen + 1u);

    // root DIR "." @ root_pos; children . and .. as HRD
    dot_pos = root_pos + 32u;
    dotdot_pos = dot_pos + 32u;
    img_size = dotdot_pos + 32u;

    // root
    pfs_romfs_put_be32(img + root_pos + 0, (dot_pos & PFS_ROMFS_FH_MASK) | PFS_ROMFH_DIR);
    pfs_romfs_put_be32(img + root_pos + 4, dot_pos);
    pfs_romfs_put_be32(img + root_pos + 8, 0);
    pfs_romfs_put_be32(img + root_pos + 12, 0);
    img[root_pos + 16] = '.';
    img[root_pos + 17] = '\0';

    // .
    pfs_romfs_put_be32(img + dot_pos + 0, (dotdot_pos & PFS_ROMFS_FH_MASK) | PFS_ROMFH_HRD);
    pfs_romfs_put_be32(img + dot_pos + 4, root_pos);
    pfs_romfs_put_be32(img + dot_pos + 8, 0);
    pfs_romfs_put_be32(img + dot_pos + 12, 0);
    img[dot_pos + 16] = '.';
    img[dot_pos + 17] = '\0';

    // ..
    pfs_romfs_put_be32(img + dotdot_pos + 0, PFS_ROMFH_HRD);
    pfs_romfs_put_be32(img + dotdot_pos + 4, root_pos);
    pfs_romfs_put_be32(img + dotdot_pos + 8, 0);
    pfs_romfs_put_be32(img + dotdot_pos + 12, 0);
    img[dotdot_pos + 16] = '.';
    img[dotdot_pos + 17] = '.';
    img[dotdot_pos + 18] = '\0';

    pfs_romfs_put_be32(img + 8, img_size);
    pfs_romfs_put_be32(img + 12, 0);
    words = (img_size < PFS_ROMFS_CHECKSUM_BYTES) ? img_size : PFS_ROMFS_CHECKSUM_BYTES;
    csum = pfs_romfs_fix_checksum(img, words);
    pfs_romfs_put_be32(img + 12, (uint32_t)(0u - csum));

    int rc = pfs_romfs_write_all(p_blkio, part_offset, img, (int)img_size);
    if (PFS_OK != rc)
    {
        return rc;
    }
    {
        int n = pfs_blkio_fsync(p_blkio);
        if (n < 0)
        {
            return pfs_romfs_map_blkio_rc(n);
        }
    }
    return PFS_OK;
}

int pfs_romfs_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes)
{
    (void)vol_bytes;
    if (NULL == p_blkio || part_offset < 0)
    {
        return PFS_EINVAL;
    }
    return pfs_romfs_emit_empty(p_blkio, part_offset, "");
}

// mkvol: 收集节点后线性布局 (子集: DIR/REG; 无 SYM)
typedef struct _pfs_romfs_mk_node_t {
    int                     parent;
    bool                    is_dir;
    char                    name[PFS_ROMFS_MAXFN + 1];
    char                    host[PFSMTD_DIRWALK_PATH_MAX];
    uint32_t                size;
    uint32_t                pos;
    uint32_t                meta_sz;
    uint32_t                data_off;
    uint32_t                first_child_pos;
    uint32_t                next_sib_pos;
} pfs_romfs_mk_node_t;

typedef struct _pfs_romfs_mk_ctx_t {
    pfs_romfs_mk_node_t    *nodes;
    int                     n_nodes;
    int                     cap;
} pfs_romfs_mk_ctx_t;

static int pfs_romfs_mk_add(pfs_romfs_mk_ctx_t *p_ctx, int parent, const char *name, const char *host, bool is_dir, uint32_t size)
{
    pfs_romfs_mk_node_t *p_n;
    pfs_romfs_mk_node_t *neu;
    int nlen;

    nlen = (int)strlen(name);
    if (nlen <= 0 || nlen > PFS_ROMFS_MAXFN)
    {
        return PFS_ENAMETOOLONG;
    }
    if (p_ctx->n_nodes >= p_ctx->cap)
    {
        int ncap = (p_ctx->cap < 8) ? 16 : p_ctx->cap * 2;
        neu = (pfs_romfs_mk_node_t *)pfsmtd_realloc(p_ctx->nodes, (size_t)ncap * sizeof(*neu));
        if (NULL == neu)
        {
            return PFS_ENOMEM;
        }
        p_ctx->nodes = neu;
        p_ctx->cap = ncap;
    }
    p_n = &p_ctx->nodes[p_ctx->n_nodes];
    memset(p_n, 0, sizeof(*p_n));
    p_n->parent = parent;
    p_n->is_dir = is_dir;
    memcpy(p_n->name, name, (size_t)nlen + 1);
    if (NULL != host)
    {
        strncpy(p_n->host, host, sizeof(p_n->host) - 1);
    }
    p_n->size = size;
    p_n->meta_sz = pfs_romfs_align16(PFS_ROMFS_FH_SIZE + (uint32_t)nlen + 1u);
    return p_ctx->n_nodes++;
}

typedef struct _pfs_romfs_walk_ctx_t {
    pfs_romfs_mk_ctx_t      mk;
} pfs_romfs_walk_ctx_t;

static int pfs_romfs_mk_parent_idx(pfs_romfs_mk_ctx_t *p_mk, const char *vfs_rel, char *leaf, int leaf_cap)
{
    char parent_path[PFSMTD_DIRWALK_PATH_MAX];
    const char *slash;
    const char *p;
    char comp[PFS_ROMFS_MAXFN + 1];
    int plen;
    int nlen;
    int cur;

    slash = strrchr(vfs_rel, '/');
    if (NULL == slash || '\0' == *(slash + 1))
    {
        return PFS_EINVAL;
    }
    if (slash == vfs_rel)
    {
        strncpy(leaf, slash + 1, (size_t)leaf_cap - 1);
        leaf[leaf_cap - 1] = '\0';
        return 0;
    }
    plen = (int)(slash - vfs_rel);
    if (plen >= (int)sizeof(parent_path))
    {
        return PFS_ENAMETOOLONG;
    }
    memcpy(parent_path, vfs_rel, (size_t)plen);
    parent_path[plen] = '\0';
    strncpy(leaf, slash + 1, (size_t)leaf_cap - 1);
    leaf[leaf_cap - 1] = '\0';

    int i;

    cur = 0;
    p = parent_path;
    while ('/' == *p)
    {
        p++;
    }
    while ('\0' != *p)
    {
        nlen = 0;
        while ('\0' != *p && '/' != *p)
        {
            if (nlen + 1 >= (int)sizeof(comp))
            {
                return PFS_ENAMETOOLONG;
            }
            comp[nlen++] = *p++;
        }
        comp[nlen] = '\0';
        while ('/' == *p)
        {
            p++;
        }
        for (i = 0; i < p_mk->n_nodes; i++)
        {
            if (p_mk->nodes[i].parent == cur && 0 == strcmp(p_mk->nodes[i].name, comp))
            {
                cur = i;
                break;
            }
        }
        if (i >= p_mk->n_nodes)
        {
            return PFS_ENOENT;
        }
    }
    return cur;
}

static int pfs_romfs_mkvol_on_entry(void *ctx, const char *vfs_rel, const char *host_path, bool is_dir)
{
    pfs_romfs_walk_ctx_t *p_w;
    char leaf[PFS_ROMFS_MAXFN + 1];
    int parent;
    uint32_t fsz;
    uint8_t *tmp;
    int idx;

    p_w = (pfs_romfs_walk_ctx_t *)ctx;
    parent = pfs_romfs_mk_parent_idx(&p_w->mk, vfs_rel, leaf, (int)sizeof(leaf));
    if (parent < 0)
    {
        return parent;
    }
    fsz = 0;
    if (!is_dir)
    {
        int rc = pfsmtd_host_read_file(host_path, &tmp, &fsz);
        if (PFS_OK != rc)
        {
            return rc;
        }
        pfsmtd_free(tmp);
    }
    idx = pfs_romfs_mk_add(&p_w->mk, parent, leaf, host_path, is_dir, fsz);
    return (idx < 0) ? idx : PFS_OK;
}

static int pfs_romfs_mk_layout_and_write(pfs_blkio_t *p_blkio, int64_t part_offset, pfs_romfs_mk_ctx_t *p_mk, const char *volume_id)
{
    uint8_t *img;
    uint32_t vlen;
    uint32_t root_pos;
    uint32_t cursor;
    uint32_t img_size;
    uint32_t csum;
    uint32_t words;
    int first;

    vlen = 0;
    int rc;
    if (NULL != volume_id)
    {
        vlen = (uint32_t)strlen(volume_id);
        if (vlen > PFS_ROMFS_MAXFN)
        {
            return PFS_ENAMETOOLONG;
        }
    }
    root_pos = pfs_romfs_align16(16u + vlen + 1u);

    // 为每个目录插入合成 . / .. 占位: 简化 -- 仅 root 写 . ..; 子目录不写 . .. (lookup 对子目录 . 可能缺)
    // 读侧 namei 对 . / .. 在非根会失败; 子目录仍可按名打开. 为兼容 dir_readdir, 每个 DIR 加 . / .. HRD

    // 重新计算: 先给每个节点分配 pos (含每目录额外 2 个 HRD 不进 nodes 表, 写时内联)
    cursor = root_pos;
    for (int i = 0; i < p_mk->n_nodes; i++)
    {
        p_mk->nodes[i].pos = cursor;
        cursor += p_mk->nodes[i].meta_sz;
        if (p_mk->nodes[i].is_dir)
        {
            // . and .. headers
            cursor += 32u + 32u;
        }
        else
        {
            p_mk->nodes[i].data_off = cursor;
            cursor = pfs_romfs_align16(cursor + p_mk->nodes[i].size);
        }
    }
    img_size = cursor;
    img = (uint8_t *)pfsmtd_mallocz((size_t)img_size + PFS_ROMFS_CHECKSUM_BYTES);
    if (NULL == img)
    {
        return PFS_ENOMEM;
    }
    memcpy(img, PFS_ROMFS_MAGIC_WORD, 8);
    if (vlen > 0)
    {
        memcpy(img + 16, volume_id, (size_t)vlen);
    }

    for (int i = 0; i < p_mk->n_nodes; i++)
    {
        pfs_romfs_mk_node_t *p_n = &p_mk->nodes[i];
        uint32_t next_pos;
        uint32_t type;
        uint32_t spec;
        uint32_t hpos;
        int nlen;

        // next sibling among same parent
        next_pos = 0;
        for (int j = i + 1; j < p_mk->n_nodes; j++)
        {
            if (p_mk->nodes[j].parent == p_n->parent)
            {
                next_pos = p_mk->nodes[j].pos;
                break;
            }
        }
        type = p_n->is_dir ? PFS_ROMFH_DIR : PFS_ROMFH_REG;
        spec = 0;
        if (p_n->is_dir)
        {
            // first child header is synthetic "." at pos+meta_sz
            spec = p_n->pos + p_n->meta_sz;
        }
        else
        {
            spec = 0;
        }
        hpos = p_n->pos;
        pfs_romfs_put_be32(img + hpos + 0, (next_pos & PFS_ROMFS_FH_MASK) | type);
        pfs_romfs_put_be32(img + hpos + 4, spec);
        pfs_romfs_put_be32(img + hpos + 8, p_n->is_dir ? 0 : p_n->size);
        pfs_romfs_put_be32(img + hpos + 12, 0);
        nlen = (int)strlen(p_n->name);
        memcpy(img + hpos + 16, p_n->name, (size_t)nlen + 1);

        if (p_n->is_dir)
        {
            uint32_t dpos = p_n->pos + p_n->meta_sz;
            uint32_t ddpos = dpos + 32u;
            // first real child after ..
            first = -1;
            for (int j = 0; j < p_mk->n_nodes; j++)
            {
                if (p_mk->nodes[j].parent == i)
                {
                    first = j;
                    break;
                }
            }
            // .
            pfs_romfs_put_be32(img + dpos + 0, (ddpos & PFS_ROMFS_FH_MASK) | PFS_ROMFH_HRD);
            pfs_romfs_put_be32(img + dpos + 4, p_n->pos);
            img[dpos + 16] = '.';
            img[dpos + 17] = '\0';
            // ..
            {
                uint32_t next_after = 0;
                uint32_t parent_pos = (p_n->parent >= 0) ? p_mk->nodes[p_n->parent].pos : p_n->pos;
                if (first >= 0)
                {
                    next_after = p_mk->nodes[first].pos;
                }
                pfs_romfs_put_be32(img + ddpos + 0, (next_after & PFS_ROMFS_FH_MASK) | PFS_ROMFH_HRD);
                pfs_romfs_put_be32(img + ddpos + 4, parent_pos);
                img[ddpos + 16] = '.';
                img[ddpos + 17] = '.';
                img[ddpos + 18] = '\0';
            }
            // fix DIR next of last? children linked via their own next among siblings; first child is after ..
            // Patch: first child's predecessor is .. which already points to first
            (void)first;
        }
        else if (p_n->size > 0)
        {
            uint8_t *data;
            uint32_t got;
            rc = pfsmtd_host_read_file(p_n->host, &data, &got);
            if (PFS_OK != rc || got != p_n->size)
            {
                pfsmtd_free(data);
                pfsmtd_free(img);
                return (PFS_OK != rc) ? rc : PFS_EIO;
            }
            memcpy(img + p_n->data_off, data, (size_t)p_n->size);
            pfsmtd_free(data);
        }
    }

    // root's first child should be . at root+meta; already set via is_dir branch for node 0
    // Fix sibling chain: entries under same parent -- the DIR's children list starts at ., then .., then real.
    // Real nodes' next already among siblings. But first real must be linked from .. -- done.
    // Problem: real nodes are also at their pos which may be AFTER the . .. block of parent. Good.

    // For sibling next among real children: currently next points to next node with same parent -- correct.
    // However root DIR next field points to next sibling of root (none) -- root is nodes[0], next among parent==-1? parent of root is -1 special.

    // Fix root node parent: we used parent=-1 for root; add root first as node 0 with parent=-1

    pfs_romfs_put_be32(img + 8, img_size);
    pfs_romfs_put_be32(img + 12, 0);
    words = (img_size < PFS_ROMFS_CHECKSUM_BYTES) ? img_size : PFS_ROMFS_CHECKSUM_BYTES;
    csum = pfs_romfs_fix_checksum(img, words);
    pfs_romfs_put_be32(img + 12, (uint32_t)(0u - csum));

    rc = pfs_romfs_write_all(p_blkio, part_offset, img, (int)img_size);
    pfsmtd_free(img);
    if (PFS_OK != rc)
    {
        return rc;
    }
    {
        int n = pfs_blkio_fsync(p_blkio);
        if (n < 0)
        {
            return pfs_romfs_map_blkio_rc(n);
        }
    }
    return PFS_OK;
}

int pfs_romfs_mkvol(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes, const char *src_dir, const char *volume_id)
{
    (void)vol_bytes;
    if (NULL == p_blkio || part_offset < 0 || NULL == src_dir || '\0' == src_dir[0])
    {
        return PFS_EINVAL;
    }

    pfs_romfs_walk_ctx_t w;

    memset(&w, 0, sizeof(w));
    // root node
    int rc = pfs_romfs_mk_add(&w.mk, -1, ".", NULL, true, 0);
    if (rc < 0)
    {
        return rc;
    }
    rc = pfsmtd_dirwalk(src_dir, pfs_romfs_mkvol_on_entry, &w);
    if (PFS_OK != rc)
    {
        pfsmtd_free(w.mk.nodes);
        return rc;
    }
    if (1 == w.mk.n_nodes)
    {
        pfsmtd_free(w.mk.nodes);
        return pfs_romfs_emit_empty(p_blkio, part_offset, volume_id ? volume_id : "");
    }
    rc = pfs_romfs_mk_layout_and_write(p_blkio, part_offset, &w.mk, volume_id ? volume_id : "");
    pfsmtd_free(w.mk.nodes);
    return rc;
}

// end
