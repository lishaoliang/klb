// Doc Encode : UTF-8 BOM, Unix(LF)
// yaffs2 介质扫描 (packed chunk+spare; 对照 HowYaffsWorks YAFFS2 反向/高 seq 胜)
#include "yaffs/pfs_yaffs_internal.h"
#include "base/pfsmtd_mem.h"

#include <string.h>
#include <stdbool.h>

typedef struct _pfs_yaffs_geom_t {
    int                     chunk_size;
    int                     spare_size;
} pfs_yaffs_geom_t;

static const pfs_yaffs_geom_t g_pfs_yaffs_geoms[] = {
    { 2048, 64 },
    { 2048, 16 },
    { 1024, 32 },
    { 1024, 16 },
    { 512, 16 },
    { 4096, 128 },
    { 4096, 64 },
    { 4096, 16 },
};

static bool pfs_yaffs_seq_ok(uint32_t seq)
{
    if (0 == seq || 0xffffffffu == seq)
    {
        return false;
    }
    if (seq < PFS_YAFFS_LOWEST_SEQUENCE_NUMBER)
    {
        return false;
    }
    if (seq > PFS_YAFFS_HIGHEST_SEQUENCE_NUMBER)
    {
        return false;
    }
    return true;
}

static bool pfs_yaffs_tags_ok(const pfs_yaffs_packed_tags2_t *p_t)
{
    if (NULL == p_t)
    {
        return false;
    }
    if (!pfs_yaffs_seq_ok(p_t->seq_number))
    {
        return false;
    }
    if (0 == p_t->obj_id || 0xffffffffu == p_t->obj_id)
    {
        return false;
    }
    return true;
}

static bool pfs_yaffs_type_ok(uint32_t type)
{
    return (type >= PFS_YAFFS_OBJECT_TYPE_FILE && type <= PFS_YAFFS_OBJECT_TYPE_SPECIAL);
}

void pfs_yaffs_scan_free(pfs_yaffs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return;
    }

    pfs_yaffs_obj_t *p_obj = p_sbi->obj_list;
    p_sbi->obj_list = NULL;
    while (NULL != p_obj)
    {
        pfs_yaffs_obj_t *p_obj_n = p_obj->next;
        pfs_yaffs_dchunk_t *p_ch = p_obj->chunks;
        while (NULL != p_ch)
        {
            pfs_yaffs_dchunk_t *p_ch_n = p_ch->next;
            pfsmtd_free(p_ch);
            p_ch = p_ch_n;
        }
        pfsmtd_free(p_obj);
        p_obj = p_obj_n;
    }
}

pfs_yaffs_obj_t *pfs_yaffs_obj_find(pfs_yaffs_sbi_t *p_sbi, uint32_t obj_id)
{
    pfs_yaffs_obj_t *p_obj;

    if (NULL == p_sbi || 0 == obj_id)
    {
        return NULL;
    }
    for (p_obj = p_sbi->obj_list; NULL != p_obj; p_obj = p_obj->next)
    {
        if (p_obj->obj_id == obj_id)
        {
            return p_obj;
        }
    }
    return NULL;
}

bool pfs_yaffs_obj_is_dir(const pfs_yaffs_obj_t *p_obj)
{
    if (NULL == p_obj || !p_obj->has_hdr)
    {
        return false;
    }
    return (PFS_YAFFS_OBJECT_TYPE_DIRECTORY == p_obj->type);
}

int pfs_yaffs_obj_visible(const pfs_yaffs_obj_t *p_obj)
{
    if (NULL == p_obj || !p_obj->has_hdr)
    {
        return 0;
    }
    if (PFS_YAFFS_OBJECTID_UNLINKED == p_obj->parent_id
            || PFS_YAFFS_OBJECTID_DELETED == p_obj->parent_id)
    {
        return 0;
    }
    if (PFS_YAFFS_OBJECTID_ROOT == p_obj->obj_id
            || PFS_YAFFS_OBJECTID_LOSTNFOUND == p_obj->obj_id
            || PFS_YAFFS_OBJECTID_UNLINKED == p_obj->obj_id
            || PFS_YAFFS_OBJECTID_DELETED == p_obj->obj_id)
    {
        // 特殊目录本身可挂; 列表时由调用方决定是否展示名
        return 1;
    }
    return 1;
}

static pfs_yaffs_obj_t *pfs_yaffs_obj_get(pfs_yaffs_sbi_t *p_sbi, uint32_t obj_id)
{
    pfs_yaffs_obj_t *p_obj;

    p_obj = pfs_yaffs_obj_find(p_sbi, obj_id);
    if (NULL != p_obj)
    {
        return p_obj;
    }
    p_obj = (pfs_yaffs_obj_t *)pfsmtd_mallocz(sizeof(*p_obj));
    if (NULL == p_obj)
    {
        return NULL;
    }
    p_obj->obj_id = obj_id;
    p_obj->next = p_sbi->obj_list;
    p_sbi->obj_list = p_obj;
    return p_obj;
}

static int pfs_yaffs_read_page_tags(pfs_blkio_t *p_blkio, int64_t part_offset, int chunk_size, int spare_size,
        int64_t page, pfs_yaffs_packed_tags2_t *p_tags, uint8_t *hdr_opt)
{
    if (spare_size < (int)sizeof(*p_tags) || spare_size > 256)
    {
        return PFS_EINVAL;
    }

    uint8_t spare[256];
    int page_size;
    int n;
    int64_t abs_off;

    page_size = chunk_size + spare_size;
    abs_off = part_offset + page * (int64_t)page_size + (int64_t)chunk_size;
    n = pfs_blkio_pread(p_blkio, spare, spare_size, abs_off);
    if (n != spare_size)
    {
        if (n < 0)
        {
            return pfs_yaffs_map_blkio_rc(n);
        }
        return PFS_EIO;
    }
    memcpy(p_tags, spare, sizeof(*p_tags));

    if (NULL != hdr_opt && 0 == p_tags->chunk_id && pfs_yaffs_tags_ok(p_tags))
    {
        n = pfs_blkio_pread(p_blkio, hdr_opt, (int)sizeof(pfs_yaffs_obj_hdr_t),
                part_offset + page * (int64_t)page_size);
        if (n != (int)sizeof(pfs_yaffs_obj_hdr_t))
        {
            if (n < 0)
            {
                return pfs_yaffs_map_blkio_rc(n);
            }
            return PFS_EIO;
        }
    }
    return PFS_OK;
}

// 评分; *p_has_root=1 表示采样窗内见到 ROOT 目录对象头 (防块设备 FS 误认)
static int pfs_yaffs_score_geom(pfs_blkio_t *p_blkio, int64_t part_offset, int64_t flash_size,
        int chunk_size, int spare_size, int *p_has_root)
{
    if (NULL != p_has_root)
    {
        *p_has_root = 0;
    }

    int64_t page_size;

    page_size = (int64_t)chunk_size + (int64_t)spare_size;
    if (page_size <= 0 || flash_size < page_size)
    {
        return -1;
    }

    int64_t page_count;
    int64_t limit;
    int64_t page;
    int score;
    int has_root;
    pfs_yaffs_packed_tags2_t tags;
    pfs_yaffs_obj_hdr_t hdr;

    page_count = flash_size / page_size;
    if (page_count <= 0)
    {
        return -1;
    }

    limit = page_count;
    if (limit > 256)
    {
        limit = 256;
    }

    score = 0;
    has_root = 0;
    for (page = 0; page < limit; page++)
    {
        int rc = pfs_yaffs_read_page_tags(p_blkio, part_offset, chunk_size, spare_size, page, &tags, NULL);
        if (PFS_OK != rc)
        {
            return -1;
        }
        if (!pfs_yaffs_tags_ok(&tags))
        {
            continue;
        }
        if (0 != tags.chunk_id)
        {
            score += 1;
            continue;
        }
        rc = pfs_yaffs_read_page_tags(p_blkio, part_offset, chunk_size, spare_size, page, &tags, (uint8_t *)&hdr);
        if (PFS_OK != rc)
        {
            continue;
        }
        if (pfs_yaffs_type_ok(hdr.type))
        {
            score += 10;
            if (PFS_YAFFS_OBJECTID_ROOT == tags.obj_id
                    && PFS_YAFFS_OBJECT_TYPE_DIRECTORY == hdr.type)
            {
                has_root = 1;
            }
        }
    }
    if (NULL != p_has_root)
    {
        *p_has_root = has_root;
    }
    return score;
}

int pfs_yaffs_detect_geom(pfs_blkio_t *p_blkio, int64_t part_offset, int64_t flash_size, int *p_chunk, int *p_spare)
{
    int best_i;
    int best_score;
    int score;
    int has_root;

    if (NULL == p_blkio || NULL == p_chunk || NULL == p_spare || part_offset < 0 || flash_size <= 0)
    {
        return PFS_ENOEXEC;
    }

    best_i = -1;
    best_score = 0;
    for (int i = 0; i < (int)(sizeof(g_pfs_yaffs_geoms) / sizeof(g_pfs_yaffs_geoms[0])); i++)
    {
        has_root = 0;
        score = pfs_yaffs_score_geom(p_blkio, part_offset, flash_size,
                g_pfs_yaffs_geoms[i].chunk_size, g_pfs_yaffs_geoms[i].spare_size, &has_root);
        // 须有 ROOT 目录头; 单靠随机 type 命中不得认作 yaffs
        if (has_root && score > best_score)
        {
            best_score = score;
            best_i = i;
        }
    }
    if (best_i < 0 || best_score < 10)
    {
        return PFS_ENOEXEC;
    }
    *p_chunk = g_pfs_yaffs_geoms[best_i].chunk_size;
    *p_spare = g_pfs_yaffs_geoms[best_i].spare_size;
    return PFS_OK;
}

static int pfs_yaffs_add_header(pfs_yaffs_sbi_t *p_sbi, uint32_t page, const pfs_yaffs_packed_tags2_t *p_t,
        const pfs_yaffs_obj_hdr_t *p_hdr)
{
    pfs_yaffs_obj_t *p_obj;
    uint64_t fsize;

    if (!pfs_yaffs_type_ok(p_hdr->type))
    {
        return PFS_OK;
    }

    p_obj = pfs_yaffs_obj_get(p_sbi, p_t->obj_id);
    if (NULL == p_obj)
    {
        return PFS_ENOMEM;
    }
    if (p_obj->has_hdr && p_t->seq_number < p_obj->hdr_seq)
    {
        return PFS_OK;
    }

    p_obj->has_hdr = 1;
    p_obj->hdr_seq = p_t->seq_number;
    p_obj->hdr_page = page;
    p_obj->type = p_hdr->type;
    p_obj->parent_id = p_hdr->parent_obj_id;
    p_obj->mode = p_hdr->yst_mode;
    fsize = ((uint64_t)p_hdr->file_size_high << 32) | (uint64_t)p_hdr->file_size_low;
    if (fsize > 0xffffffffu)
    {
        fsize = 0xffffffffu;
    }
    p_obj->file_size = (uint32_t)fsize;
    memcpy(p_obj->name, p_hdr->name, PFS_YAFFS_MAX_NAME_LENGTH);
    p_obj->name[PFS_YAFFS_MAX_NAME_LENGTH] = '\0';
    return PFS_OK;
}

static int pfs_yaffs_add_data(pfs_yaffs_sbi_t *p_sbi, uint32_t page, const pfs_yaffs_packed_tags2_t *p_t)
{
    pfs_yaffs_obj_t *p_obj;
    pfs_yaffs_dchunk_t *p_ch;
    pfs_yaffs_dchunk_t *p_prev;

    p_obj = pfs_yaffs_obj_get(p_sbi, p_t->obj_id);
    if (NULL == p_obj)
    {
        return PFS_ENOMEM;
    }

    p_prev = NULL;
    for (p_ch = p_obj->chunks; NULL != p_ch; p_prev = p_ch, p_ch = p_ch->next)
    {
        if (p_ch->chunk_id == p_t->chunk_id)
        {
            if (p_t->seq_number < p_ch->seq)
            {
                return PFS_OK;
            }
            p_ch->seq = p_t->seq_number;
            p_ch->page = page;
            p_ch->n_bytes = p_t->n_bytes;
            return PFS_OK;
        }
    }

    p_ch = (pfs_yaffs_dchunk_t *)pfsmtd_mallocz(sizeof(*p_ch));
    if (NULL == p_ch)
    {
        return PFS_ENOMEM;
    }
    p_ch->chunk_id = p_t->chunk_id;
    p_ch->seq = p_t->seq_number;
    p_ch->page = page;
    p_ch->n_bytes = p_t->n_bytes;
    p_ch->next = NULL;
    if (NULL == p_prev)
    {
        p_obj->chunks = p_ch;
    }
    else
    {
        p_prev->next = p_ch;
    }
    return PFS_OK;
}

static int pfs_yaffs_ensure_root(pfs_yaffs_sbi_t *p_sbi)
{
    pfs_yaffs_obj_t *p_root;

    p_root = pfs_yaffs_obj_get(p_sbi, PFS_YAFFS_OBJECTID_ROOT);
    if (NULL == p_root)
    {
        return PFS_ENOMEM;
    }
    if (!p_root->has_hdr)
    {
        p_root->has_hdr = 1;
        p_root->type = PFS_YAFFS_OBJECT_TYPE_DIRECTORY;
        p_root->parent_id = PFS_YAFFS_OBJECTID_ROOT;
        p_root->name[0] = '\0';
        p_root->mode = 0755;
    }
    return PFS_OK;
}

int pfs_yaffs_scan_medium(pfs_yaffs_sbi_t *p_sbi)
{
    if (NULL == p_sbi || NULL == p_sbi->p_blkio || p_sbi->page_size <= 0 || p_sbi->page_count <= 0)
    {
        return PFS_EINVAL;
    }

    int64_t page;
    int64_t max_used;
    uint32_t max_seq;
    uint32_t max_obj;
    pfs_yaffs_packed_tags2_t tags;
    pfs_yaffs_obj_hdr_t hdr;

    pfs_yaffs_scan_free(p_sbi);
    max_used = -1;
    max_seq = PFS_YAFFS_LOWEST_SEQUENCE_NUMBER - 1u;
    max_obj = PFS_YAFFS_OBJECTID_DELETED;

    for (page = 0; page < p_sbi->page_count; page++)
    {
        int rc = pfs_yaffs_read_page_tags(p_sbi->p_blkio, p_sbi->part_offset, p_sbi->chunk_size, p_sbi->spare_size,
                page, &tags, NULL);
        if (PFS_OK != rc)
        {
            return rc;
        }
        if (!pfs_yaffs_tags_ok(&tags))
        {
            continue;
        }

        if (page > max_used)
        {
            max_used = page;
        }
        if (tags.seq_number > max_seq)
        {
            max_seq = tags.seq_number;
        }
        if (tags.obj_id > max_obj)
        {
            max_obj = tags.obj_id;
        }

        if (0 == tags.chunk_id)
        {
            rc = pfs_yaffs_read_page_tags(p_sbi->p_blkio, p_sbi->part_offset, p_sbi->chunk_size, p_sbi->spare_size,
                    page, &tags, (uint8_t *)&hdr);
            if (PFS_OK != rc)
            {
                return rc;
            }
            rc = pfs_yaffs_add_header(p_sbi, (uint32_t)page, &tags, &hdr);
            if (PFS_OK != rc)
            {
                return rc;
            }
        }
        else
        {
            if (tags.n_bytes > (uint32_t)p_sbi->chunk_size)
            {
                continue;
            }
            rc = pfs_yaffs_add_data(p_sbi, (uint32_t)page, &tags);
            if (PFS_OK != rc)
            {
                return rc;
            }
        }
    }

    p_sbi->free_page = max_used + 1;
    p_sbi->next_seq = max_seq + 1u;
    if (p_sbi->next_seq < PFS_YAFFS_LOWEST_SEQUENCE_NUMBER)
    {
        p_sbi->next_seq = PFS_YAFFS_LOWEST_SEQUENCE_NUMBER;
    }
    p_sbi->next_obj_id = max_obj + 1u;
    if (p_sbi->next_obj_id <= PFS_YAFFS_OBJECTID_DELETED)
    {
        p_sbi->next_obj_id = PFS_YAFFS_OBJECTID_DELETED + 1u;
    }
    return pfs_yaffs_ensure_root(p_sbi);
}

int pfs_yaffs_emit_page(pfs_yaffs_sbi_t *p_sbi, uint32_t obj_id, uint32_t chunk_id, uint32_t n_bytes, const void *data)
{
    if (NULL == p_sbi || NULL == p_sbi->p_blkio || 0 == obj_id)
    {
        return PFS_EINVAL;
    }
    if (PFS_YAFFS_MOUNT_RDWR != p_sbi->mount_state)
    {
        return PFS_EPERM;
    }
    if (n_bytes > (uint32_t)p_sbi->chunk_size)
    {
        return PFS_EINVAL;
    }
    if (n_bytes > 0 && NULL == data)
    {
        return PFS_EINVAL;
    }
    if (p_sbi->free_page < 0 || p_sbi->free_page >= p_sbi->page_count)
    {
        return PFS_ENOSPC;
    }
    if (p_sbi->next_seq < PFS_YAFFS_LOWEST_SEQUENCE_NUMBER
            || p_sbi->next_seq > PFS_YAFFS_HIGHEST_SEQUENCE_NUMBER)
    {
        return PFS_ENOSPC;
    }

    uint8_t *pagebuf;
    pfs_yaffs_packed_tags2_t tags;
    uint32_t seq;
    uint32_t page;
    int64_t abs_off;
    int n;
    int copy_n;
    int rc;

    pagebuf = (uint8_t *)pfsmtd_malloc((size_t)p_sbi->page_size);
    if (NULL == pagebuf)
    {
        return PFS_ENOMEM;
    }
    memset(pagebuf, 0xff, (size_t)p_sbi->page_size);
    copy_n = (int)n_bytes;
    if (copy_n > 0)
    {
        memcpy(pagebuf, data, (size_t)copy_n);
    }

    seq = p_sbi->next_seq++;
    page = (uint32_t)p_sbi->free_page;
    memset(&tags, 0, sizeof(tags));
    tags.seq_number = seq;
    tags.obj_id = obj_id;
    tags.chunk_id = chunk_id;
    tags.n_bytes = n_bytes;
    memcpy(pagebuf + p_sbi->chunk_size, &tags, sizeof(tags));

    abs_off = p_sbi->part_offset + (int64_t)page * (int64_t)p_sbi->page_size;
    n = pfs_blkio_pwrite(p_sbi->p_blkio, pagebuf, p_sbi->page_size, abs_off);
    pfsmtd_free(pagebuf);
    if (n != p_sbi->page_size)
    {
        if (n < 0)
        {
            return pfs_yaffs_map_blkio_rc(n);
        }
        return PFS_EIO;
    }
    p_sbi->free_page++;

    if (0 == chunk_id)
    {
        // 对象头: 调用方已保证 data 为 obj_hdr; 再读入缓存
        rc = pfs_yaffs_add_header(p_sbi, page, &tags, (const pfs_yaffs_obj_hdr_t *)data);
    }
    else
    {
        rc = pfs_yaffs_add_data(p_sbi, page, &tags);
    }
    return rc;
}

int pfs_yaffs_emit_obj_hdr(pfs_yaffs_sbi_t *p_sbi, uint32_t obj_id, uint32_t type, uint32_t parent_id, uint32_t mode, uint32_t file_size, const char *name)
{
    if (NULL == p_sbi || 0 == obj_id || !pfs_yaffs_type_ok(type))
    {
        return PFS_EINVAL;
    }

    pfs_yaffs_obj_hdr_t hdr;

    memset(&hdr, 0, sizeof(hdr));
    hdr.type = type;
    hdr.parent_obj_id = parent_id;
    hdr.yst_mode = mode;
    hdr.file_size_low = file_size;
    hdr.file_size_high = 0;
    if (NULL != name)
    {
        int nlen;

        nlen = (int)strlen(name);
        if (nlen > PFS_YAFFS_MAX_NAME_LENGTH)
        {
            return PFS_ENAMETOOLONG;
        }
        memcpy(hdr.name, name, (size_t)nlen);
        hdr.name[nlen] = '\0';
    }
    return pfs_yaffs_emit_page(p_sbi, obj_id, 0, (uint32_t)sizeof(hdr), &hdr);
}

int pfs_yaffs_sync_media(pfs_yaffs_sbi_t *p_sbi)
{
    if (NULL == p_sbi || NULL == p_sbi->p_blkio)
    {
        return PFS_EINVAL;
    }


    int rc = pfs_blkio_fsync(p_sbi->p_blkio);
    if (rc < 0)
    {
        return pfs_yaffs_map_blkio_rc(rc);
    }
    return PFS_OK;
}

// end
