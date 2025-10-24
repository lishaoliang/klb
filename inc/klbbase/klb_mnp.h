///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_mnp.h
/// @brief   media net protocol, 媒体网络协议
/// @version 0.1
/// @history 修改历史
///  \n 2019 0.1 创建文件
///  \n 2022 0.2 a.修改心跳机制: 由ping发起, pong回应
///              b.添加 RPC Lua, RPC Json数据分包, 在底层协议直接支持RPC
///  \n 2023 0.3 a.为 klb_mnp_media_t/klb_mnp_common_t 结构体, 添加 padding 对齐字段, 方便做内存对齐处理
///              b.调整chnn/sidx, 从uint32_t调整为uint16_t, 并联合取名为 sid
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_MNP_H__
#define __KLB_MNP_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


#pragma pack(4)

/// @struct klb_mnp_t
/// @brief  网络封包头
///  \n 固定8字节, 封包头可以被写入文件, 需要精简大小
///  \n 小端序:
///  \n 媒体包缓存: [4K, 8K, 16K, 32K]
typedef struct klb_mnp_t_
{
    uint32_t magic;             ///< 魔数: KLB_MNP_MAGIC
#define KLB_MNP_MAGIC           0x504E4DEB  ///< "*MNP"
    //- 4 Byte

    uint16_t size ;             ///< 单个数据包大小(包含本结构体): [8,32K]
#define KLB_MNP_BLOCK_SIZE_MAX  (0x8000)

    uint8_t  opt : 2;           ///< 包组合方式: klb_mnp_opt_e
    uint8_t  packtype : 5;      ///< 包类型: klb_mnp_packtype_e
    uint8_t  resv1 : 1;         ///< 保留: 0
    uint8_t  resv2;             ///< 保留: 0
    //- 4 + 4 = 8 Byte
}klb_mnp_t;

/// @struct klb_mnp_media_t
/// @brief  media net protocol, media head
///  \n F包: [klb_mnp_t][klb_mnp_media_t][data...]
///  \n B包: [klb_mnp_t][klb_mnp_media_t][data...]
///  \n C包: [klb_mnp_t][data...]
///  \n E包: [klb_mnp_t][data...]
///  \n 数据包可以被存储, 只描述与数据有关部分
typedef struct klb_mnp_media_t_
{
    uint32_t         size;              ///< 完整数据长度(data size, 包含本结构体)
    uint16_t         padding;           ///< 末尾对齐数据
    uint16_t         dtype;             ///< 数据类型(data type): klb_mnp_dtype_e
    //- 4 + 4 = 8 Byte

    // 标识号
    union
    {
        uint32_t     sid;               ///< 编号

        struct
        {
            uint16_t chnn;              ///< 通道(channel)
            uint16_t sidx;              ///< 流序号(stream index): klb_mnp_sidx_e
        };
    };
    //- 8 + 4 = 12 Byte

    int64_t          time;              ///< 时间戳(基于1970年基准,毫秒)
    //- 12 + 8 = 20 Byte

    union
    {
        uint64_t     resv;              ///< 0

        // 视频参数
        struct
        {
            uint8_t  vtype;             ///< 视频类型(video type): klb_mnp_vtype_e;
            uint8_t  vtype2;            ///< 
        };

        // 音频参数
        struct
        {
            uint8_t  tracks;            ///< 音频声道数; 1, 2, 5.1;
            uint8_t  bits_per_sample;   ///< 音频编码数; 1(8比特), 2(16比特)
            uint16_t resv2;
            uint32_t samples;           ///< 音频采样率; 44100
        };
    };
    //- 20 + 8 = 28 Byte

    uint32_t         rsv;               ///< 保留: 0
    //- 28 + 4 = 32 Byte
}klb_mnp_media_t;

/// @struct klb_mnp_common_t
/// @brief  text/binary header
///  \n F包: [klb_mnp_t][klb_mnp_common_t][head...][data...][padding...]
///  \n B包: [klb_mnp_t][klb_mnp_common_t][head...][data...][padding...]
///  \n C包: [klb_mnp_t][head...][data...][padding...]
///  \n E包: [klb_mnp_t][head...][data...][padding...]
typedef struct klb_mnp_common_t_
{
    uint32_t    size;       ///< 完整数据长度(data size, 包含本结构体)
    uint32_t    head;       ///< 数据头部长度; 正式数据长度 = size - head - sizeof(klb_mnp_common_t) - padding
    uint32_t    sequence;   ///< 序列号
    uint32_t    uid;        ///< 用户自定义ID(user defined id)

    uint16_t    padding;    ///< 末尾对齐数据
    uint16_t    rsv;        ///< 保留: 0
    // - 4 + 4 + 4 + 4 + 4 = 20 Byte
}klb_mnp_common_t;

/// @struct klb_mnp_rpc_t
/// @brief  RPC header
typedef struct klb_mnp_rpc_t_
{
    uint32_t    size;       ///< 完整数据长度(data size, 包含本结构体)
    uint32_t    sequence;   ///< 序列号
    uint32_t    uid;        ///< 用户自定义ID(user defined id)

    uint32_t    rsv;        ///< 保留: 0
}klb_mnp_rpc_t;

#pragma pack()


/// @struct klb_mnp_opt_e
/// @brief  包组合方式
typedef enum klb_mnp_opt_e_
{
    KLB_MNP_BEGIN    = 0x0,     ///< Begin包(B包)
    KLB_MNP_CONTINUE = 0x1,     ///< Continue包(C包)
    KLB_MNP_END      = 0x2,     ///< End包(E包)
    KLB_MNP_FULL     = 0x3,     ///< Full包(F包)

    KLB_MNP_OPT_MAX  = 0x3      ///< MAX
}klb_mnp_opt_e;


/// @struct klb_mnp_packtype_e
/// @brief  包类型
/// @note 将心跳包修改参考websocket: 由ping发起, pong回应
typedef enum klb_mnp_packtype_e_
{
    KLB_MNP_PONG            = 0x0, ///< 心跳包(回应): 附加数据为0; 否则协议错误
    KLB_MNP_TEXT            = 0x1, ///< 文本数据
    KLB_MNP_BINARY          = 0x2, ///< 二进制数据
    KLB_MNP_MEDIA           = 0x3, ///< 媒体数据
    KLB_MNP_PING            = 0x4, ///< 心跳包(发起): 附加数据为0; 否则协议错误

    KLB_MNP_RPC_LUA         = 0x10,///< RPC Lua包;
    KLB_MNP_RPC_JSON        = 0x11,///< RPC Json包;

    KLB_MNP_PACKTYPE_MAX    = 0x1F ///< MAX
}klb_mnp_packtype_e;


/// @enum   klb_mnp_vtype_e
/// @brief  视频帧类型
typedef enum klb_mnp_vtype_e_
{
    KLB_MNP_VTYPE_P     = 0x00,     ///< P帧
    KLB_MNP_VTYPE_I     = 0x01,     ///< I帧
    KLB_MNP_VTYPE_B     = 0x02,     ///< B帧
    KLB_MNP_VTYPE_CFG   = 0x03,     ///< 视频配置(config)数据: vps, sps, pps, sei, eg.
    KLB_MNP_VTYPE_MAX   = 0xFF      ///< MAX
}klb_mnp_vtype_e;


/// @enum   klb_mnp_sidx_e
/// @brief  媒体流序号
typedef enum klb_mnp_sidx_e_
{
    KLB_MNP_SIDX_NULL   = 0x0000,    ///< NULL
    KLB_MNP_SIDX_V1     = 0x0001,    ///< Video 1
    KLB_MNP_SIDX_V2     = 0x0002,    ///< Video 2
    KLB_MNP_SIDX_V3     = 0x0003,    ///< Video 3

    KLB_MNP_SIDX_A1     = 0x0021,    ///< Audio 1
    KLB_MNP_SIDX_A2     = 0x0022,    ///< Audio 2
    KLB_MNP_SIDX_A3     = 0x0023,    ///< Audio 3

    KLB_MNP_SIDX_P1     = 0x0041,    ///< Picture 1
    KLB_MNP_SIDX_P2     = 0x0042,    ///< Picture 2
    KLB_MNP_SIDX_P3     = 0x0043,    ///< Picture 3

    KLB_MNP_SIDX_I1     = 0x0061,    ///< Image 1
    KLB_MNP_SIDX_I2     = 0x0062,    ///< Image 2
    KLB_MNP_SIDX_I3     = 0x0063,    ///< Image 3

    KLB_MNP_SIDX_MAX    = 0x7FFF,    ///< max sidx
}klb_mnp_sidx_e;


/// @enum  klb_mnp_dtype_e
/// @brief 媒体数据类型
/// @note 原打算使用ffmpeg的AVCodecID, 但其版本更新过程中值会变更,
///   所以这里重新定义
typedef enum klb_mnp_dtype_e_
{
    KLB_MNP_DTYPE_NULL  = 0x0000,
    KLB_MNP_DTYPE_H264  = 0x0001,   ///< AV_CODEC_ID_H264
    KLB_MNP_DTYPE_H265  = 0x0002,   ///< AV_CODEC_ID_H265

    KLB_MNP_DTYPE_AAC   = 0x1001,   ///< AV_CODEC_ID_AAC

    KLB_MNP_DTYPE_JPEG  = 0x2001,   ///< JPEG

    KLB_MNP_DTYPE_MAX   = 0x7FFF,   ///< max data type
}klb_mnp_dtype_e;


#ifdef __cplusplus
}
#endif

#endif // __KLB_MNP_H__
//end
