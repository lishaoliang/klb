///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_map_serialize.h
/// @brief   klb_map_t 序列化
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_MAP_SERIALIZE_H__
#define __KLB_MAP_SERIALIZE_H__

#include "klb_type.h"
#include "klbmem/klb_buf.h"
#include "klbutil/klb_adt.h"
#include "klbutil/klb_map.h"

#if defined(__cplusplus)
extern "C" {
#endif

#pragma pack(1) /// 1 字节对齐


/// @struct klbmapseri_head_t
/// @brief  序列化头
///  格式: klbmapseri_head_t + [size] + [data]
///  key = KLBMAPSERI_key && adt_type = KLB_ADT_null 表示map终止
typedef struct klbmapseri_head_t_
{
    uint8_t key : 1;            ///< KV键值对; 0.值; 1.关键字
#define KLBMAPSERI_value    0
#define KLBMAPSERI_key      1

    uint8_t adt_type : 3;       ///< 数据类型: klb_adt_type_e_

    uint8_t payload : 4;        ///< 数据大小占用的长度 [15, 14, 13, 12, 0, 1, 2, 4, 8]
#define KLBMAPSERI_payload_0        0   ///< 后续 无数据
#define KLBMAPSERI_payload_1        1   ///< 后续 1Byte数据
#define KLBMAPSERI_payload_2        2   ///< 后续 2Byte数据
#define KLBMAPSERI_payload_4        4   ///< 后续 4Byte数据
#define KLBMAPSERI_payload_8        8   ///< 后续 8Byte数据
#define KLBMAPSERI_payload_size_1B  12  ///< 后续 1字节长度 + [数据]
#define KLBMAPSERI_payload_size_2B  13  ///< 后续 2字节长度 + [数据]
#define KLBMAPSERI_payload_size_4B  14  ///< 后续 4字节长度 + [数据]
#define KLBMAPSERI_payload_size_8B  15  ///< 后续 8字节长度 + [数据]
}klbmapseri_head_t;

#pragma pack()


/// @brief 将 map 序列化成二进制数据
/// @param [in] *p_map      map
/// @return klb_buf_t* 缓存
KLB_API klb_buf_t* klb_map_seri_pack(const klb_map_t* p_map);


/// @brief 从二进制反序列化成 map
/// @param [in] *p_map      map
/// @param [in] *p_data     缓存
/// @param [in] data_len    数据长度
/// @return bool 是否成功
KLB_API bool klb_map_seri_unpack(klb_map_t* p_map, const char* p_data, int data_len);


//////////////////////////////////////////////////////////////////////////

typedef struct klbmapseri_reader_t_
{
    char*   p_data;
    int     data_len;

    char*   ptr;
    int     offset;

    klbmapseri_head_t   h;
}klbmapseri_reader_t;


KLB_API void klb_map_seri_read_init(klbmapseri_reader_t* p_reader, const char* p_data, int data_len);
KLB_API bool klb_map_seri_read_head(klbmapseri_reader_t* p_reader, klbmapseri_head_t* p_head);

KLB_API bool klb_map_seri_read_idx(klbmapseri_reader_t* p_reader, int* p_idx);
KLB_API bool klb_map_seri_read_key(klbmapseri_reader_t* p_reader, char** p_key, int* p_key_len);

KLB_API bool klb_map_seri_read_bool(klbmapseri_reader_t* p_reader, bool* p_b);
KLB_API bool klb_map_seri_read_int64(klbmapseri_reader_t* p_reader, int64_t* p_v);
KLB_API bool klb_map_seri_read_uint64(klbmapseri_reader_t* p_reader, uint64_t* p_v);
KLB_API bool klb_map_seri_read_double(klbmapseri_reader_t* p_reader, double* p_v);
KLB_API bool klb_map_seri_read_string(klbmapseri_reader_t* p_reader, char** p_str, int* p_str_len);


#ifdef __cplusplus
}
#endif

#endif // __KLB_MAP_SERIALIZE_H__
//end
