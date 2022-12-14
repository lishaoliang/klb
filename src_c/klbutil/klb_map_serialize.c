// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbutil/klb_map_serialize.h"
#include "klbutil/klb_adt.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_buffer.h"
#include "klbmem/klb_rbuf.h"
#include <assert.h>


//////////////////////////////////////////////////////////////////////////

#define KLBMAPSERI_WRITE_RBUF(RBUF_, KEY_, ADT_TYPE_, PAYLOAD_, VALUE_) \
{ \
    klbmapseri_head_t h_ = { 0 }; \
    h_.key = (KEY_) & 0x1; \
    h_.adt_type = (ADT_TYPE_) & 0x7; \
    h_.payload = (PAYLOAD_) & 0xF; \
    klb_rbuf_write(p_rbuf, &h_, sizeof(h_)); \
    klb_rbuf_write(p_rbuf, &(VALUE_), sizeof(VALUE_)); \
}

static void klbmapseri_map(klb_buffer_t* p_buffer, klb_rbuf_t* p_rbuf, klb_map_t* p_map);

static void klbmapseri_head_init(klbmapseri_head_t* p_h, uint8_t key, uint8_t adt_type, uint8_t payload)
{
    p_h->key = key & 0x1;
    p_h->adt_type = adt_type & 0x7;
    p_h->payload = payload & 0xF;
}

static void klbmapseri_int(klb_rbuf_t* p_rbuf, uint8_t k, int64_t v)
{
    // 格式: klbmapseri_head_t + [数值]

    int64_t abs_v = ABS(v);
    if (abs_v < 0x80)
    {
        int8_t i8 = (int8_t)v;
        KLBMAPSERI_WRITE_RBUF(p_rbuf, k, KLB_ADT_int64, sizeof(int8_t), i8);
    }
    else if(abs_v < 0x8000)
    {
        int16_t i16 = (int16_t)v;
        KLBMAPSERI_WRITE_RBUF(p_rbuf, k, KLB_ADT_int64, sizeof(int16_t), i16);
    }
    else if (abs_v < 0x80000000)
    {
        int32_t i32 = (int32_t)v;
        KLBMAPSERI_WRITE_RBUF(p_rbuf, k, KLB_ADT_int64, sizeof(int32_t), i32);
    }
    else
    {
        int64_t i64 = (int64_t)v;
        KLBMAPSERI_WRITE_RBUF(p_rbuf, k, KLB_ADT_int64, sizeof(int64_t), i64);
    }
}

static void klbmapseri_uint(klb_rbuf_t* p_rbuf, uint64_t v)
{
    // 格式: klbmapseri_head_t + [数值]

    if (v < 0xFF)
    {
        uint8_t u8 = (uint8_t)v;
        KLBMAPSERI_WRITE_RBUF(p_rbuf, KLBMAPSERI_value, KLB_ADT_uint64, sizeof(uint8_t), u8);
    }
    else if (v < 0xFFFF)
    {
        uint16_t u16 = (uint16_t)v;
        KLBMAPSERI_WRITE_RBUF(p_rbuf, KLBMAPSERI_value, KLB_ADT_uint64, sizeof(uint16_t), u16);
    }
    else if (v < 0xFFFFFFFF)
    {
        uint32_t u32 = (uint32_t)v;
        KLBMAPSERI_WRITE_RBUF(p_rbuf, KLBMAPSERI_value, KLB_ADT_uint64, sizeof(uint32_t), u32);
    }
    else
    {
        uint64_t u64 = (uint64_t)v;
        KLBMAPSERI_WRITE_RBUF(p_rbuf, KLBMAPSERI_value, KLB_ADT_uint64, sizeof(uint64_t), u64);
    }
}

static void klbmapseri_double(klb_rbuf_t* p_rbuf, double v)
{
    // 格式: klbmapseri_head_t + [数值]

    KLBMAPSERI_WRITE_RBUF(p_rbuf, KLBMAPSERI_value, KLB_ADT_double, sizeof(double), v);
}

static void klbmapseri_string(klb_rbuf_t* p_rbuf, const char* p_str, int str_len)
{
    // 格式: klbmapseri_head_t + 字符串size + 字符串

    // step1. 写 klbmapseri_head_t 头 + 字符串size
    int32_t abs_v = ABS(str_len);
    if (abs_v < 0x80)
    {
        int8_t i8 = (int8_t)str_len;
        KLBMAPSERI_WRITE_RBUF(p_rbuf, KLBMAPSERI_value, KLB_ADT_string, KLBMAPSERI_payload_size_1B, i8);
    }
    else if (abs_v < 0x8000)
    {
        int16_t i16 = (int16_t)str_len;
        KLBMAPSERI_WRITE_RBUF(p_rbuf, KLBMAPSERI_value, KLB_ADT_string, KLBMAPSERI_payload_size_2B, i16);
    }
    else
    {
        int32_t i32 = (int32_t)str_len;
        KLBMAPSERI_WRITE_RBUF(p_rbuf, KLBMAPSERI_value, KLB_ADT_string, KLBMAPSERI_payload_size_4B, i32);
    }

    // step2. 写字符串
    klb_rbuf_write(p_rbuf, p_str, str_len);
}

static void klbmapseri_adt(klb_buffer_t* p_buffer, klb_rbuf_t* p_rbuf, klb_adt_t* p_adt)
{
    klb_rbuf_reset(p_rbuf);

    switch (klb_adt_type(p_adt))
    {       
    case KLB_ADT_bool: // 格式: klbmapseri_head_t + [数值]
        {
            int8_t b = klb_adt_to_bool(p_adt) ? 1 : 0;
            KLBMAPSERI_WRITE_RBUF(p_rbuf, KLBMAPSERI_value, KLB_ADT_bool, sizeof(int8_t), b);

            klb_buffer_write_rbuf(p_buffer, p_rbuf);
        }
        break;
    case KLB_ADT_string: // 格式: klbmapseri_head_t + 字符串size + 字符串
        {
            int str_len = 0;
            const char* p_str = klb_adt_to_lstring(p_adt, &str_len);
            klbmapseri_string(p_rbuf, p_str, str_len);

            klb_buffer_write_rbuf(p_buffer, p_rbuf);
        }
        break;
    case KLB_ADT_double: // 格式: klbmapseri_head_t + [数值]
        {
            double d = klb_adt_to_double(p_adt);
            KLBMAPSERI_WRITE_RBUF(p_rbuf, KLBMAPSERI_value, KLB_ADT_double, sizeof(double), d);

            klb_buffer_write_rbuf(p_buffer, p_rbuf);
        }
        break;
    case KLB_ADT_uint64: // 格式: klbmapseri_head_t + [数值]
        {
            uint64_t u64 = klb_adt_to_uint64(p_adt);
            klbmapseri_uint(p_rbuf, u64);

            klb_buffer_write_rbuf(p_buffer, p_rbuf);
        }
        break;
    case KLB_ADT_int64: // 格式: klbmapseri_head_t + [数值]
        {
            int64_t i64 = klb_adt_to_int64(p_adt);
            klbmapseri_int(p_rbuf, KLBMAPSERI_value, i64);

            klb_buffer_write_rbuf(p_buffer, p_rbuf);
         }
        break;
    case KLB_ADT_map: // 格式: klbmapseri_head_t + [...]
        {
            klbmapseri_head_t h = { 0 };
            klbmapseri_head_init(&h, KLBMAPSERI_value, KLB_ADT_map, 0);
            klb_rbuf_write(p_rbuf, &h, sizeof(h));

            klb_buffer_write_rbuf(p_buffer, p_rbuf);

            klbmapseri_map(p_buffer, p_rbuf, klb_adt_to_map(p_adt));
        }
        break;
    case KLB_ADT_ptr:  // 格式: klbmapseri_head_t
    case KLB_ADT_null: // 格式: klbmapseri_head_t
    default:
        {
            klbmapseri_head_t h = { 0 };
            klbmapseri_head_init(&h, KLBMAPSERI_value, KLB_ADT_null, 0);
            klb_rbuf_write(p_rbuf, &h, sizeof(h));

            klb_buffer_write_rbuf(p_buffer, p_rbuf);
        }
        break;
    }
}

static void klbmapseri_idx(klb_buffer_t* p_buffer, klb_rbuf_t* p_rbuf, int idx)
{
    klb_rbuf_reset(p_rbuf);

    klbmapseri_int(p_rbuf, KLBMAPSERI_key, idx);

    klb_buffer_write_rbuf(p_buffer, p_rbuf);
}

static void klbmapseri_key(klb_buffer_t* p_buffer, klb_rbuf_t* p_rbuf, const char* p_key)
{
    klb_rbuf_reset(p_rbuf);

    int key_len = strlen(p_key);

    // 格式: klbmapseri_head_t + 字符串size + 字符串

    // step1. 写 klbmapseri_head_t 头 + 字符串size
    int32_t abs_v = ABS(key_len);
    if (abs_v < 0x80)
    {
        int8_t i8 = (int8_t)key_len;
        KLBMAPSERI_WRITE_RBUF(p_rbuf, KLBMAPSERI_key, KLB_ADT_string, KLBMAPSERI_payload_size_1B, i8);
    }
    else if (abs_v < 0x8000)
    {
        int16_t i16 = (int16_t)key_len;
        KLBMAPSERI_WRITE_RBUF(p_rbuf, KLBMAPSERI_key, KLB_ADT_string, KLBMAPSERI_payload_size_2B, i16);
    }
    else
    {
        int32_t i32 = (int32_t)key_len;
        KLBMAPSERI_WRITE_RBUF(p_rbuf, KLBMAPSERI_key, KLB_ADT_string, KLBMAPSERI_payload_size_4B, i32);
    }

    // step2. 写字符串
    klb_rbuf_write(p_rbuf, p_key, key_len);

    klb_buffer_write_rbuf(p_buffer, p_rbuf);
}

static void klbmapseri_end(klb_buffer_t* p_buffer, klb_rbuf_t* p_rbuf)
{
    klb_rbuf_reset(p_rbuf);

    klbmapseri_head_t h = { 0 };
    klbmapseri_head_init(&h, KLBMAPSERI_key, KLB_ADT_null, 0);
    klb_rbuf_write(p_rbuf, &h, sizeof(h));

    klb_buffer_write_rbuf(p_buffer, p_rbuf);
}

static void klbmapseri_map(klb_buffer_t* p_buffer, klb_rbuf_t* p_rbuf, klb_map_t* p_map)
{
    assert(NULL != p_map);

    // array, 认为 idx 为数值 key
    for (int i = 0; i < klb_map_array_size(p_map); i++)
    {
        klbmapseri_idx(p_buffer, p_rbuf, i);
        klbmapseri_adt(p_buffer, p_rbuf, (klb_adt_t*)klb_map_idx_to_adt(p_map, i));
    }

    // object, key 为字符串
    klb_map_iter_t* p_iter = klb_map_begin(p_map);
    while (NULL != p_iter)
    {
        const char* p_key = klb_map_key(p_iter);
        klb_adt_t* p_value = klb_map_data(p_iter);

        klbmapseri_key(p_buffer, p_rbuf, p_key);
        klbmapseri_adt(p_buffer, p_rbuf, p_value);

        p_iter = klb_map_next(p_iter);
    }

    klbmapseri_end(p_buffer, p_rbuf);
}

klb_buf_t* klb_map_seri_pack(const klb_map_t* p_map)
{
    klb_buffer_t* p_buffer = klb_buffer_create(0);
    klb_rbuf_t rbuf = { 0 };

    klbmapseri_map(p_buffer, &rbuf, (klb_map_t*)p_map);

    klb_rbuf_quit(&rbuf);

    klb_buf_t* p_buf = klb_buffer_join(p_buffer, NULL, NULL);

    klb_buffer_destroy(p_buffer);
    return p_buf;
}

//////////////////////////////////////////////////////////////////////////
static int klbmapseri_read(klb_map_t* p_map, char* p_data, int data_len);

static int klbmapseri_read_head(klbmapseri_head_t* p_h, char* ptr, int len)
{
    memcpy(p_h, ptr, sizeof(klbmapseri_head_t));

    return sizeof(klbmapseri_head_t);
}

static int klbmapseri_read_bool(uint8_t payload, char* ptr, int len, bool* p_v)
{
    if (KLBMAPSERI_payload_1 == payload)
    {
        int8_t v = *((int8_t*)ptr);
        *p_v = (0 == v) ? false : true;
        return 1;
    }
    return 0;
}

static int klbmapseri_read_double(uint8_t payload, char* ptr, int len, klb_adt_t* p_adt)
{
    if (KLBMAPSERI_payload_8 == payload)
    {
        double d = *((double*)ptr);
        klb_adt_set_double(p_adt, d);

        assert(KLBMAPSERI_payload_8 == sizeof(double));
        return sizeof(double);
    }

    return 0;
}

static int klbmapseri_read_string(uint8_t payload, char* ptr, int len, klb_adt_t* p_adt)
{
    if (KLBMAPSERI_payload_size_1B == payload)
    {
        int8_t i8_len = *((int8_t*)ptr);
        ptr += sizeof(int8_t);

        klb_adt_set_lstring(p_adt, ptr, i8_len);

        return i8_len + sizeof(int8_t);
    }
    else if(KLBMAPSERI_payload_size_2B == payload)
    {
        int16_t i16_len = *((int16_t*)ptr);
        ptr += sizeof(int16_t);

        klb_adt_set_lstring(p_adt, ptr, i16_len);

        return i16_len + sizeof(int16_t);
    }
    else if (KLBMAPSERI_payload_size_4B == payload)
    {
        int32_t i32_len = *((int32_t*)ptr);
        ptr += sizeof(int32_t);

        klb_adt_set_lstring(p_adt, ptr, i32_len);

        return i32_len + sizeof(int32_t);
    }

    return 0;
}

static int klbmapseri_read_int(uint8_t payload, char* ptr, int len, int64_t* p_v)
{
    if (KLBMAPSERI_payload_1 == payload) 
    { 
        *p_v = *((int8_t*)ptr);
        return 1;
    }
    else if(KLBMAPSERI_payload_2 == payload) 
    { 
        *p_v = *((int16_t*)ptr);
        return 2;
    }
    else if (KLBMAPSERI_payload_4 == payload) 
    {
        *p_v = *((int32_t*)ptr);
        return 4;
    }
    else if (KLBMAPSERI_payload_8 == payload)
    { 
        *p_v = *((int64_t*)ptr);
        return 8;
    }

    return 0;
}

static int klbmapseri_read_uint(uint8_t payload, char* ptr, int len, uint64_t* p_v)
{
    if (KLBMAPSERI_payload_1 == payload)
    {
        *p_v = *((uint8_t*)ptr);
        return 1;
    }
    else if (KLBMAPSERI_payload_2 == payload)
    {
        *p_v = *((uint16_t*)ptr);
        return 2;
    }
    else if (KLBMAPSERI_payload_4 == payload)
    {
        *p_v = *((uint32_t*)ptr);
        return 4;
    }
    else if (KLBMAPSERI_payload_8 == payload)
    {
        *p_v = *((uint64_t*)ptr);
        return 8;
    }

    return 0;
}

static int klbmapseri_read_adt(klbmapseri_head_t* p_h, char* ptr, int len, klb_adt_t* p_adt)
{
    int off = 0;
    switch (p_h->adt_type)
    {
    case KLB_ADT_bool:
        {
            bool b = false;
            off = klbmapseri_read_bool(p_h->payload, ptr, len, &b);
            klb_adt_set_bool(p_adt, b);
        }
        break;
    case KLB_ADT_string:
        {
            off = klbmapseri_read_string(p_h->payload, ptr, len, p_adt);
        }
        break;
    case KLB_ADT_double:
        {
            off = klbmapseri_read_double(p_h->payload, ptr, len, p_adt);
        }
        break;
    case KLB_ADT_int64:
        {
            int64_t i64 = 0;
            off = klbmapseri_read_int(p_h->payload, ptr, len, &i64);

            klb_adt_set_int64(p_adt, i64);            
        }
        break;
    case KLB_ADT_uint64:
        {
            uint64_t u64 = 0;
            off = klbmapseri_read_uint(p_h->payload, ptr, len, &u64);

            klb_adt_set_uint64(p_adt, u64);
        }
        break;
    case KLB_ADT_map:
        {
            klb_map_t* p_map = klb_map_create();
            off = klbmapseri_read(p_map, ptr, len);
            klb_adt_set_map(p_adt, p_map);
        }
        break;
    default:
        break;
    }

    return off;
}

static int klbmapseri_read_idx(uint8_t payload, char* ptr, int len, int* p_idx)
{
    if (KLBMAPSERI_payload_1 == payload)
    {
        *p_idx = *((int8_t*)ptr);
        return 1;
    }
    else if(KLBMAPSERI_payload_2 == payload)
    {
        *p_idx = *((int16_t*)ptr);
        return 2;
    }
    else if (KLBMAPSERI_payload_4 == payload)
    {
        *p_idx = *((int32_t*)ptr);
        return 4;
    }

    return 0;
}

static int klbmapseri_read_key(uint8_t payload, char* ptr, int len, sds* key)
{
    if (KLBMAPSERI_payload_size_1B == payload)
    {
        int8_t i8_len = *((int8_t*)ptr);

        ptr += sizeof(int8_t);
        len -= sizeof(int8_t);

        *key = sdsnewlen(ptr, i8_len);

        return i8_len + sizeof(int8_t);
    }
    else if(KLBMAPSERI_payload_size_2B == payload)
    {
        int16_t i16_len = *((int16_t*)ptr);

        ptr += sizeof(int16_t);
        len -= sizeof(int16_t);

        *key = sdsnewlen(ptr, i16_len);

        return i16_len + sizeof(int16_t);
    }
    else if (KLBMAPSERI_payload_size_4B == payload)    
    {
        int32_t i32_len = *((int32_t*)ptr);

        ptr += sizeof(int32_t);
        len -= sizeof(int32_t);

        *key = sdsnewlen(ptr, i32_len);

        return i32_len + sizeof(int32_t);
    }

    return 0;
}

static int klbmapseri_read(klb_map_t* p_map, char* p_data, int data_len)
{
    char* ptr = p_data;
    int len = data_len;
    int offset = 0;

    while (0 < len)
    {
        // key
        klbmapseri_head_t h = { 0 };

        int off = klbmapseri_read_head(&h, ptr, len);
        ptr += off;
        len -= off;
        offset += off;

        int idx = 0;
        sds key = NULL;

        if (KLB_ADT_int64 == h.adt_type)
        {
            off = klbmapseri_read_idx(h.payload, ptr, len, &idx);
            ptr += off;
            len -= off;
            offset += off;
        }
        else if(KLB_ADT_string == h.adt_type)
        {
            off = klbmapseri_read_key(h.payload, ptr, len, &key);
            ptr += off;
            len -= off;
            offset += off;
        }
        else if(KLB_ADT_null == h.adt_type)
        {
            break;
        }

        // value
        klbmapseri_head_t h_v = { 0 };
        off = klbmapseri_read_head(&h_v, ptr, len);
        ptr += off;
        len -= off;
        offset += off;

        klb_adt_t* p_adt = klb_adt_create();

        if (KLB_ADT_null != h_v.adt_type)
        {
            off = klbmapseri_read_adt(&h_v, ptr, len, p_adt);
            ptr += off;
            len -= off;
            offset += off;
        }

        if (KLB_ADT_int64 == h.adt_type)
        {
            klb_map_set_idx_adt(p_map, idx, p_adt);
        }
        else if(KLB_ADT_string == h.adt_type)
        {
            klb_map_set_adt(p_map, key, p_adt);
        }

        KLB_FREE_BY(key, sdsfree);
    }

    return offset;
}

bool klb_map_seri_unpack(klb_map_t* p_map, const char* p_data, int data_len)
{
    assert(NULL != p_map);

    // TODO. 异常数据检查

    klbmapseri_read(p_map, (char*)p_data, data_len);

    return true;
}

//////////////////////////////////////////////////////////////////////////

void klb_map_seri_read_init(klbmapseri_reader_t* p_reader, const char* p_data, int data_len)
{
    p_reader->p_data = (char*)p_data;
    p_reader->data_len = data_len;

    p_reader->ptr = p_reader->p_data;
    p_reader->offset = 0;
}

bool klb_map_seri_read_head(klbmapseri_reader_t* p_reader, klbmapseri_head_t* p_head)
{
    memcpy(&p_reader->h, p_reader->ptr, sizeof(klbmapseri_head_t));
    memcpy(p_head, p_reader->ptr, sizeof(klbmapseri_head_t));
    
    p_reader->ptr += sizeof(klbmapseri_head_t);
    p_reader->offset += sizeof(klbmapseri_head_t);

    return true;
}

bool klb_map_seri_read_idx(klbmapseri_reader_t* p_reader, int* p_idx)
{
    uint8_t payload = p_reader->h.payload;
    char* ptr = p_reader->ptr;
    int offset = 0;

    if (KLBMAPSERI_payload_1 == payload)
    {
        *p_idx = *((int8_t*)ptr);   offset = sizeof(int8_t);
    }
    else if (KLBMAPSERI_payload_2 == payload)
    {
        *p_idx = *((int16_t*)ptr);  offset = sizeof(int16_t);
    }
    else if (KLBMAPSERI_payload_4 == payload)
    {
        *p_idx = *((int32_t*)ptr);  offset = sizeof(int32_t);
    }

    p_reader->ptr += offset;
    p_reader->offset += offset;

    return true;
}

bool klb_map_seri_read_key(klbmapseri_reader_t* p_reader, char** p_key, int* p_key_len)
{
    uint8_t payload = p_reader->h.payload;
    char* ptr = p_reader->ptr;
    int offset = 0;

    if (KLBMAPSERI_payload_size_1B == payload)
    {
        int8_t i8_len = *((int8_t*)ptr);
        ptr += sizeof(int8_t);

        *p_key = ptr;
        *p_key_len = i8_len;

        offset = i8_len + sizeof(int8_t);
    }
    else if (KLBMAPSERI_payload_size_2B == payload)
    {
        int16_t i16_len = *((int16_t*)ptr);
        ptr += sizeof(int16_t);

        *p_key = ptr;
        *p_key_len = i16_len;

        offset = i16_len + sizeof(int16_t);
    }
    else if (KLBMAPSERI_payload_size_4B == payload)
    {
        int32_t i32_len = *((int32_t*)ptr);
        ptr += sizeof(int32_t);

        *p_key = ptr;
        *p_key_len = i32_len;

        offset = i32_len + sizeof(int32_t);
    }

    p_reader->ptr += offset;
    p_reader->offset += offset;

    return true;
}

bool klb_map_seri_read_bool(klbmapseri_reader_t* p_reader, bool* p_b)
{
    uint8_t payload = p_reader->h.payload;
    char* ptr = p_reader->ptr;
    int offset = 0;

    if (KLBMAPSERI_payload_1 == payload)
    {
        int8_t v = *((int8_t*)ptr);
        *p_b = (0 == v) ? false : true;

        offset = sizeof(int8_t);
    }

    p_reader->ptr += offset;
    p_reader->offset += offset;

    return true;
}

bool klb_map_seri_read_int64(klbmapseri_reader_t* p_reader, int64_t* p_v)
{
    char* ptr = p_reader->ptr;
    int offset = 0;

    if (KLBMAPSERI_payload_1 == p_reader->h.payload)
    {
        *p_v = *((int8_t*)ptr);     offset = sizeof(int8_t);
    }
    else if (KLBMAPSERI_payload_2 == p_reader->h.payload)
    {
        *p_v = *((int16_t*)ptr);    offset = sizeof(int16_t);
    }
    else if (KLBMAPSERI_payload_4 == p_reader->h.payload)
    {
        *p_v = *((int32_t*)ptr);    offset = sizeof(int32_t);
    }
    else if (KLBMAPSERI_payload_8 == p_reader->h.payload)
    {
        *p_v = *((int64_t*)ptr);    offset = sizeof(int64_t);
    }

    p_reader->ptr += offset;
    p_reader->offset += offset;

    return true;
}

bool klb_map_seri_read_uint64(klbmapseri_reader_t* p_reader, uint64_t* p_v)
{
    char* ptr = p_reader->ptr;
    int offset = 0;

    if (KLBMAPSERI_payload_1 == p_reader->h.payload)
    {
        *p_v = *((uint8_t*)ptr);    offset = sizeof(uint8_t);
    }
    else if (KLBMAPSERI_payload_2 == p_reader->h.payload)
    {
        *p_v = *((uint16_t*)ptr);   offset = sizeof(uint16_t);
    }
    else if (KLBMAPSERI_payload_4 == p_reader->h.payload)
    {
        *p_v = *((uint32_t*)ptr);   offset = sizeof(uint32_t);
    }
    else if (KLBMAPSERI_payload_8 == p_reader->h.payload)
    {
        *p_v = *((uint64_t*)ptr);   offset = sizeof(uint64_t);
    }

    p_reader->ptr += offset;
    p_reader->offset += offset;

    return true;
}

bool klb_map_seri_read_double(klbmapseri_reader_t* p_reader, double* p_v)
{
    char* ptr = p_reader->ptr;
    int offset = 0;

    if (KLBMAPSERI_payload_8 == p_reader->h.payload)
    {
        assert(KLBMAPSERI_payload_8 == sizeof(double));
        *p_v = *((double*)ptr);     offset = sizeof(double);
    }

    p_reader->ptr += offset;
    p_reader->offset += offset;

    return true;
}

bool klb_map_seri_read_string(klbmapseri_reader_t* p_reader, char** p_str, int* p_str_len)
{
    char* ptr = p_reader->ptr;
    int offset = 0;

    if (KLBMAPSERI_payload_size_1B == p_reader->h.payload)
    {
        int8_t i8_len = *((int8_t*)ptr);
        ptr += sizeof(int8_t);

        *p_str = ptr;
        *p_str_len = i8_len;

        offset = i8_len + sizeof(int8_t);
    }
    else if (KLBMAPSERI_payload_size_2B == p_reader->h.payload)
    {
        int16_t i16_len = *((int16_t*)ptr);
        ptr += sizeof(int16_t);

        *p_str = ptr;
        *p_str_len = i16_len;

        offset = i16_len + sizeof(int16_t);
    }
    else if (KLBMAPSERI_payload_size_4B == p_reader->h.payload)
    {
        int32_t i32_len = *((int32_t*)ptr);
        ptr += sizeof(int32_t);

        *p_str = ptr;
        *p_str_len = i32_len;

        offset = i32_len + sizeof(int32_t);
    }

    p_reader->ptr += offset;
    p_reader->offset += offset;

    return true;
}
