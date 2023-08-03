// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbutil/klb_nvector.h"
#include "klbmem/klb_mem.h"
#include <assert.h>


/// @def   KLB_NVECTOR_MIN
/// @brief 容量/掩码表序号的最小值
#define KLB_NVECTOR_MIN  6       // 2^6 = 64


/// @def   KLB_NVECTOR_MAX
/// @brief 容量/掩码表序号的最大值
#define KLB_NVECTOR_MAX  28      // 2^28 * sizeof(char*) <= 2^32


/// @struct klb_nvector_talbe_t
/// @brief  容量/掩码表
typedef struct klb_nvector_talbe_t_
{
    uint32_t    max;            ///< 容量最大值
    uint32_t    mask;           ///< 掩码值
}klb_nvector_talbe_t;


///< 容量/掩码表
static klb_nvector_talbe_t s_klb_nvector_talbe[32] = {
    { 0x00000001, 0x00000001 - 1 },{ 0x00000002, 0x00000002 - 1 },{ 0x00000004, 0x00000004 - 1 },{ 0x00000008, 0x00000008 - 1 },
    { 0x00000010, 0x00000010 - 1 },{ 0x00000020, 0x00000020 - 1 },{ 0x00000040, 0x00000040 - 1 },{ 0x00000080, 0x00000080 - 1 },
    { 0x00000100, 0x00000100 - 1 },{ 0x00000200, 0x00000200 - 1 },{ 0x00000400, 0x00000400 - 1 },{ 0x00000800, 0x00000800 - 1 },
    { 0x00001000, 0x00001000 - 1 },{ 0x00002000, 0x00002000 - 1 },{ 0x00004000, 0x00004000 - 1 },{ 0x00008000, 0x00008000 - 1 },
    { 0x00010000, 0x00010000 - 1 },{ 0x00020000, 0x00020000 - 1 },{ 0x00040000, 0x00040000 - 1 },{ 0x00080000, 0x00080000 - 1 },
    { 0x00100000, 0x00100000 - 1 },{ 0x00200000, 0x00200000 - 1 },{ 0x00400000, 0x00400000 - 1 },{ 0x00800000, 0x00800000 - 1 },
    { 0x01000000, 0x01000000 - 1 },{ 0x02000000, 0x02000000 - 1 },{ 0x04000000, 0x04000000 - 1 },{ 0x08000000, 0x08000000 - 1 },
    { 0x10000000, 0x10000000 - 1 },{ 0x20000000, 0x20000000 - 1 },{ 0x40000000, 0x40000000 - 1 },{ 0x80000000, 0x80000000 - 1 },
};


/// @struct klb_nvector_t
/// @brief  vector对象
typedef struct klb_nvector_t_
{
    char**              p_idx;          ///< 数据

    uint32_t            size;           ///< 节点成员数目
    uint32_t            idx_talbe;
}klb_nvector_t;


klb_nvector_t* klb_nvector_create()
{
    klb_nvector_t* p_vector = KLB_MALLOCZ(klb_nvector_t, 1, 0);

    p_vector->idx_talbe = KLB_NVECTOR_MIN;

    // 常驻内存量
    p_vector->p_idx = KLB_MALLOC(char*, s_klb_nvector_talbe[p_vector->idx_talbe].max, 0);

    return p_vector;
}

void klb_nvector_destroy(klb_nvector_t* p_vector)
{
    assert(NULL != p_vector);
    assert(0 == p_vector->size);

    KLB_FREE(p_vector->p_idx);
    KLB_FREE(p_vector);
}

static void klb_nvector_check_resize_inc(klb_nvector_t* p_vector)
{
    // 检查扩大容量
    // 条件: 当前容量的1/2 <= 当前节点数目; 没到最大值KLB_VECTOR_MAX
    if (KLB_NVECTOR_MAX <= p_vector->idx_talbe)
    {
        return; // 已经最大容量了
    }

    assert(KLB_NVECTOR_MIN <= p_vector->idx_talbe);
    if (p_vector->size < s_klb_nvector_talbe[p_vector->idx_talbe - 1].max)
    {
        return; // 在适合的范围内, 无需扩容
    }

    int old_idx = p_vector->idx_talbe;
    p_vector->idx_talbe += 1;

    char** ptr = KLB_REALLOC(p_vector->p_idx, char*, s_klb_nvector_talbe[p_vector->idx_talbe].max, 0);
    assert(NULL != ptr);

    p_vector->p_idx = ptr;
}

static void klb_nvector_check_resize_dec(klb_nvector_t* p_vector)
{
    // 检查缩小容量
    // 条件: 当前容量的1/4 >= 当前节点数目; 没有到最小值KLB_VECTOR_MIN
    if (p_vector->idx_talbe <= KLB_NVECTOR_MIN)
    {
        return; // 已经最小了
    }

    if (s_klb_nvector_talbe[p_vector->idx_talbe - 2].max < p_vector->size)
    {
        return; // 在适合的范围内, 无需缩小容量
    }

    // 缩小容量
    int old_idx = p_vector->idx_talbe;
    p_vector->idx_talbe -= 1;

    char** ptr = KLB_REALLOC(p_vector->p_idx, char*, s_klb_nvector_talbe[p_vector->idx_talbe].max, 0);
    assert(NULL != ptr);

    p_vector->p_idx = ptr;
}

void klb_nvector_clear(klb_nvector_t* p_vector, klb_nvector_clear_cb cb_clear, void* p_obj)
{
    if (cb_clear)
    {
        for (uint32_t i = 0; i < p_vector->size; i++)
        {
            cb_clear(p_obj, p_vector->p_idx[i]);
        }
    }

    p_vector->size = 0;
}

//void klb_nvector_push_head(klb_nvector_t* p_vector, void* p_data)
//{
//    klb_nvector_check_resize_inc(p_vector);
//
//    // move
//}

int klb_nvector_push_tail(klb_nvector_t* p_vector, void* p_data)
{
    klb_nvector_check_resize_inc(p_vector);

    int idx = p_vector->size;
    p_vector->p_idx[idx] = (char*)p_data;
    p_vector->size++;

    return idx;
}

void* klb_nvector_pop_head(klb_nvector_t* p_vector)
{
    if (p_vector->size <= 0)
    {
        return NULL;
    }

    void* ptr = p_vector->p_idx[0];
    p_vector->size--;

    // move
    int idx = 0;
    if (idx < p_vector->size)
    {
        memmove(p_vector->p_idx + idx, p_vector->p_idx + idx + 1, sizeof(char*) * (p_vector->size - idx));
    }

    klb_nvector_check_resize_dec(p_vector);
    return ptr;
}

void* klb_nvector_pop_tail(klb_nvector_t* p_vector)
{
    if (p_vector->size <= 0)
    {
        return NULL;
    }

    void* ptr = p_vector->p_idx[p_vector->size - 1];
    p_vector->size--;

    klb_nvector_check_resize_dec(p_vector);
    return ptr;
}

void* klb_nvector_head(klb_nvector_t* p_vector)
{
    if (0 < p_vector->size)
    {
        return p_vector->p_idx[0];
    }

    return NULL;
}

void* klb_nvector_tail(klb_nvector_t* p_vector)
{
    if (0 < p_vector->size)
    {
        p_vector->p_idx[p_vector->size - 1];
    }

    return NULL;
}


void* klb_nvector_get(klb_nvector_t* p_vector, int index)
{
    if (0 <= index && index < p_vector->size)
    {
        return p_vector->p_idx[index];
    }

    return NULL;
}

void* klb_nvector_update(klb_nvector_t* p_vector, int index, void* p_data)
{
    if (0 <= index && index < p_vector->size)
    {
        void* ptr = p_vector->p_idx[index];
        p_vector->p_idx[index] = (char*)p_data;

        return ptr;
    }

    return NULL;
}

void* klb_nvector_remove(klb_nvector_t* p_vector, int index)
{
    if (0 <= index && index < p_vector->size)
    {
        void* ptr = p_vector->p_idx[index];
        p_vector->size--;

        // move
        int idx = index;
        if (idx < p_vector->size)
        {
            memmove(p_vector->p_idx + idx, p_vector->p_idx + idx + 1, sizeof(char*) * (p_vector->size - idx));
        }

        klb_nvector_check_resize_dec(p_vector);
        return ptr;
    }

    return NULL;
}

int klb_nvector_size(klb_nvector_t* p_vector)
{
    return (int)p_vector->size;
}
