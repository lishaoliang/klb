#include "string/klb_str_map.h"
#include "mem/klb_mem.h"
#include "hash/klb_hlist.h"
#include "string/sds.h"
#include <assert.h>


typedef struct klb_str_map_t_
{
    klb_hlist_t*    p_hlist;
}klb_str_map_t;


klb_str_map_t* klb_str_map_create()
{
    klb_str_map_t* p_map = KLB_MALLOC(klb_str_map_t, 1, 0);
    KLB_MEMSET(p_map, 0, sizeof(klb_str_map_t));

    p_map->p_hlist = klb_hlist_create(0);

    return p_map;
}

void klb_str_map_destroy(klb_str_map_t* p_map)
{
    assert(NULL != p_map);

    klb_str_map_remove_all(p_map);

    KLB_FREE_BY(p_map->p_hlist, klb_hlist_destroy);
    KLB_FREE(p_map);
}

uint32_t klb_str_map_size(klb_str_map_t* p_map)
{
    assert(NULL != p_map);

    return klb_hlist_size(p_map->p_hlist);
}

void klb_str_map_set(klb_str_map_t* p_map, const char* p_key, const char* p_value)
{
    assert(NULL != p_map);
    assert(NULL != p_key);
    assert(NULL != p_value);

    int key_len = strlen(p_key);
    sds p_str = sdsnew(p_value);
    if (NULL == klb_hlist_push_tail(p_map->p_hlist, p_key, key_len, p_str))
    {
        sds p_old = klb_hlist_update(p_map->p_hlist, p_key, key_len, p_str);
        assert(NULL != p_old);

        KLB_FREE_BY(p_old, sdsfree);
    }
}

const sds klb_str_map_get(klb_str_map_t* p_map, const char* p_key)
{
    assert(NULL != p_map);

    return (sds)klb_hlist_find(p_map->p_hlist, p_key, strlen(p_key));
}

bool klb_str_map_remove(klb_str_map_t* p_map, const char* p_key)
{
    assert(NULL != p_map);

    sds p_old = klb_hlist_remove_bykey(p_map->p_hlist, p_key, strlen(p_key));
    if (NULL != p_old)
    {
        sdsfree(p_old);
        return true;
    }

    return false;
}

void klb_str_map_remove_all(klb_str_map_t* p_map)
{
    assert(NULL != p_map);

    while (0 < klb_hlist_size(p_map->p_hlist))
    {
        sds p_old = klb_hlist_pop_head(p_map->p_hlist);
        KLB_FREE_BY(p_old, sdsfree);
    }
}
