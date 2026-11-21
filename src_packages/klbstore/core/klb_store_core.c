// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbstore/klbstore.h"
#include "klbstore/core/klb_store_core.h"
#include "klbmem/klb_mem.h"

#include <stdio.h>
#include <string.h>


//////////////////////////////////////////////////////////////////////////
// 校验

static bool is_hex_name(const char* p, int n)
{
    if (n <= 0)
    {
        return false;
    }

    for (int i = 0; i < n; ++i)
    {
        char c = p[i];

        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
        {
            return false;
        }
    }

    return true;
}

static bool is_seg_name(const char* p, int n)
{
    if (n <= 0)
    {
        return false;
    }

    for (int i = 0; i < n; ++i)
    {
        char c = p[i];

        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
            || '_' == c || '-' == c || '.' == c))
        {
            return false;
        }
    }

    return true;
}


bool klb_store_core_valid_id(const char* p_id)
{
    const char* p = p_id;
    int seg = 0;

    if (NULL == p_id || '\0' == p_id[0])
    {
        return false;
    }

    // step1. 按 '/' 切段: group / M00 / xx / yy / name
    while ('\0' != p[0])
    {
        const char* p_end = strchr(p, '/');
        int len = 0;

        if (NULL == p_end)
        {
            len = (int)strlen(p);
        }
        else
        {
            len = (int)(p_end - p);
        }

        if (0 == seg)
        {
            if (!is_seg_name(p, len))
            {
                return false;
            }
        }
        else if (1 == seg)
        {
            if (len != 3)
            {
                return false;
            }
        }
        else if (2 == seg || 3 == seg)
        {
            if (2 != len || !is_hex_name(p, len))
            {
                return false;
            }
        }
        else if (4 == seg)
        {
            if (len > KLB_STORE_ID_MAX || !is_seg_name(p, len))
            {
                return false;
            }

            return NULL == p_end;
        }
        else
        {
            return false;
        }

        seg += 1;

        if (NULL == p_end)
        {
            // 段数不足: 缺 M00 / xx / yy / name
            return false;
        }

        p = p_end + 1;
    }

    return false;
}


int klb_store_core_rel_path(const char* p_id, char* p_out, int out_max)
{
    if (!klb_store_core_valid_id(p_id))
    {
        return KLB_STORE_E_ARG;
    }

    if (NULL == p_out || out_max <= 0)
    {
        return KLB_STORE_E_ARG;
    }

    if ((int)strlen(p_id) >= out_max)
    {
        return KLB_STORE_E_ARG;
    }

    snprintf(p_out, (size_t)out_max, "%s", p_id);

    return KLB_STORE_OK;
}


int klb_store_core_abs_path(const klb_store_t* p_store, const char* p_id, char* p_out, int out_max)
{
    if (NULL == p_store || NULL == p_out || out_max <= 0)
    {
        return KLB_STORE_E_ARG;
    }

    if (!klb_store_core_valid_id(p_id))
    {
        return KLB_STORE_E_ARG;
    }

    int root_len = (int)strlen(p_store->root);
    int id_len = (int)strlen(p_id);

    if (root_len + 1 + id_len >= out_max)
    {
        return KLB_STORE_E_ARG;
    }

    snprintf(p_out, (size_t)out_max, "%s/%s", p_store->root, p_id);

    return KLB_STORE_OK;
}


int klb_store_core_mkdir_p(const char* p_dir)
{
    if (NULL == p_dir || '\0' == p_dir[0])
    {
        return KLB_STORE_E_ARG;
    }

    return KLB_STORE_E_NOTIMPL;
}


//////////////////////////////////////////////////////////////////////////
// 对外

klb_store_t* klb_store_create(const char* p_root)
{
    if (NULL == p_root || '\0' == p_root[0])
    {
        return NULL;
    }

    if ((int)strlen(p_root) >= KLB_STORE_PATH_MAX)
    {
        return NULL;
    }

    klb_store_t* p_store = KLB_MALLOCZ(klb_store_t, 1, 0);
    snprintf(p_store->root, sizeof(p_store->root), "%s", p_root);

    return p_store;
}


void klb_store_destroy(klb_store_t* p_store)
{
    if (NULL == p_store)
    {
        return;
    }

    KLB_FREE(p_store);
}


int klb_store_write(klb_store_t* p_store, const char* p_id, const uint8_t* p_data, int len, int64_t offset)
{
    if (NULL == p_store || NULL == p_data || len <= 0 || offset < 0)
    {
        return KLB_STORE_E_ARG;
    }

    if (!klb_store_core_valid_id(p_id))
    {
        return KLB_STORE_E_ARG;
    }

    return KLB_STORE_E_NOTIMPL;
}


int klb_store_read(klb_store_t* p_store, const char* p_id, int64_t offset, uint8_t* p_out, int len)
{
    if (NULL == p_store || NULL == p_out || len <= 0 || offset < 0)
    {
        return KLB_STORE_E_ARG;
    }

    if (!klb_store_core_valid_id(p_id))
    {
        return KLB_STORE_E_ARG;
    }

    return KLB_STORE_E_NOTIMPL;
}


int klb_store_stat(klb_store_t* p_store, const char* p_id, int64_t* p_size, int64_t* p_mtime)
{
    if (NULL == p_store)
    {
        return KLB_STORE_E_ARG;
    }

    if (!klb_store_core_valid_id(p_id))
    {
        return KLB_STORE_E_ARG;
    }

    return KLB_STORE_E_NOTIMPL;
}


int klb_store_remove(klb_store_t* p_store, const char* p_id)
{
    if (NULL == p_store)
    {
        return KLB_STORE_E_ARG;
    }

    if (!klb_store_core_valid_id(p_id))
    {
        return KLB_STORE_E_ARG;
    }

    return KLB_STORE_E_NOTIMPL;
}


bool klb_store_valid_id(const char* p_id)
{
    return klb_store_core_valid_id(p_id);
}

// end
