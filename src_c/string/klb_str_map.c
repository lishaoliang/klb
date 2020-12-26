#include "string/klb_str_map.h"
#include "mem/klb_mem.h"
#include <assert.h>


typedef struct klb_str_map_t_
{
    int a;
}klb_str_map_t;


klb_str_map_t* klb_str_map_create()
{
    klb_str_map_t* p_map = KLB_MALLOC(klb_str_map_t, 1, 0);
    KLB_MEMSET(p_map, 0, sizeof(klb_str_map_t));

    return p_map;
}

void klb_str_map_destroy(klb_str_map_t* p_map)
{

}

uint32_t klb_str_map_size(klb_str_map_t* p_map)
{
    return 0;
}

void klb_str_map_set(klb_str_map_t* p_map, const char* p_key, const char* p_value)
{

}

const char* klb_str_map_get(klb_str_map_t* p_map, const char* p_key)
{
    return NULL;
}

bool klb_str_map_remove(klb_str_map_t* p_map, const char* p_key)
{
    return false;
}

void klb_str_map_remove_all(klb_str_map_t* p_map)
{

}
