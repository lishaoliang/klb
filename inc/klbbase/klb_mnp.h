///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_mnp.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   media net protocol, 媒体网络协议
///          目的: 约定媒体数据等 在网络上传输 方式
/// @version 0.4
/// @history 修改历史
///  \n 2019 0.1 创建文件
///  \n 2022 0.2 a.修改心跳机制: 由ping发起, pong回应
///              b.添加 RPC Lua, RPC Json数据分包, 在底层协议直接支持RPC
///  \n 2023 0.3 a.为 klb_mnp_media_t 结构体, 添加 padding 对齐字段, 方便做内存对齐处理
///  \n [2025-10] 0.4 重新设计 调整 文本/二进制/RPC包/媒体包等 头部定义; 变更较大, 不兼容前面设计, 但愿本次坚挺时间长点 :)
///                   a. 添加OPTIONS/用户自定义包/保留包等定义
///                   b. 添加编码/压缩等定义
///                   c. 添加RPC扩展定义
///                   d. 调整媒体包附加数据方式 由 数据类型(data type)来自行定义,以免完全固定后, 后续添加媒体类型难以扩展
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_MNP_H__
#define __KLB_MNP_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


//////////////////////////////////////////////////////////////////////////
// MNP协议

#pragma pack(4)

//////////////////////////////////////////////////////////////////////////
// MNP 封包头

/// @struct klb_mnp_opt_e
/// @brief  包组合方式
typedef enum klb_mnp_opt_e_
{
    KLB_MNP_BEGIN       = 0x0,      ///< Begin包(B包)
    KLB_MNP_CONTINUE    = 0x1,      ///< Continue包(C包)
    KLB_MNP_END         = 0x2,      ///< End包(E包)
    KLB_MNP_FULL        = 0x3,      ///< Full包(F包)

    KLB_MNP_OPT_MAX     = 0x3,      ///< MAX = 2^2 = 4
}klb_mnp_opt_e;

/// @struct klb_mnp_packtype_e
/// @brief  包类型
/// @note 将心跳包修改参考websocket: 由ping发起, pong回应
typedef enum klb_mnp_packtype_e_
{
    KLB_MNP_PONG        = 0x0,      ///< 心跳包(回应): 附加数据为0; 否则协议错误
    KLB_MNP_TEXT        = 0x1,      ///< 文本数据
    KLB_MNP_BINARY      = 0x2,      ///< 二进制数据
    KLB_MNP_MEDIA       = 0x3,      ///< 媒体数据
    KLB_MNP_PING        = 0x4,      ///< 心跳包(发起): 附加数据为0; 否则协议错误

    KLB_MNP_RPC_LUA     = 0x10,     ///< RPC Lua包;
    KLB_MNP_RPC_JSON    = 0x11,     ///< RPC Json包;
    KLB_MNP_RPC_EX      = 0x12,     ///< RPC 扩展(extension); 许可扩展RPC实现

    KLB_MNP_OPTIONS     = 0x13,     ///< 查询,配置,协商加密 等

    KLB_MNP_PACKTYPE_USER_B = 0x14, ///< 用户自定义扩展 Begin
    KLB_MNP_PACKTYPE_USER_E = 0x1B, ///< 用户自定义扩展 End

    KLB_MNP_PACKTYPE_1C     = 0x1C, ///< 保留, 勿使用
    KLB_MNP_PACKTYPE_1D     = 0x1D, ///< 保留, 勿使用
    KLB_MNP_PACKTYPE_1E     = 0x1E, ///< 保留, 勿使用
    KLB_MNP_PACKTYPE_MAX    = 0x1F, ///< 保留, 勿使用; MAX = 2^5 = 32
}klb_mnp_packtype_e;

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


//////////////////////////////////////////////////////////////////////////
// 文本/二进制

/// @enum  klb_mnp_encode_e
/// @brief 编码方式
typedef enum klb_mnp_encode_e_
{
    KLB_MNP_ENCODE_NULL     = 0,    ///< 裸数据
    KLB_MNP_ENCODE_COMPRESS = 1,    ///< 压缩格式, 参见: klb_mnp_compress_e/klb_mnp_compress_t
    KLB_MNP_ENCODE_MAX      = 7,    ///< max=2^3=8
}klb_mnp_encode_e;

/// @struct klb_mnp_text_t
/// @brief  text header
///   包类型: packtype = KLB_MNP_TEXT
/// @note 这里的 size 是 压缩之后的 大小
///   裸包格式(encode=NULL): [klb_mnp_text_t] + [数据头] + [数据体]
///   压缩包格式(encode=COMPRESS): [klb_mnp_text_t] + [[klb_mnp_compress_t] + [数据头]] + [[klb_mnp_compress_t] + [数据体]]
typedef struct klb_mnp_text_t_
{
    uint32_t    size : 27;      ///< 完整数据长度(data size, 包含本结构体); max=2^27=128M
    uint32_t    encode : 3;     ///< 编码方式(klb_mnp_encode_e)
    uint32_t    rsv1 : 2;       ///<

    uint32_t    sequence : 20;  ///< 序号
    uint32_t    rsv2 : 12;      ///< 

    uint32_t    head_size : 22; ///< 数据头部长度(max=2^22=4M): 正式数据body长度 = size - head_size - sizeof(klb_mnp_text_t)
    uint32_t    rsv3 : 10;
    // - 4 + 4 + 4 = 12 字节
}klb_mnp_text_t;

/// @struct klb_mnp_binary_t
/// @brief  binary header
///   包类型: packtype = KLB_MNP_BINARY
/// @note 这里的 size 是 压缩之后的 大小
///   裸包格式(encode=NULL): [klb_mnp_binary_t] + [数据头] + [数据体]
///   压缩包格式(encode=COMPRESS): [klb_mnp_binary_t] + [[klb_mnp_compress_t] + [数据头]] + [[klb_mnp_compress_t] + [数据体]]
typedef struct klb_mnp_binary_t_
{
    uint32_t    size : 27;      ///< 完整数据长度(data size, 包含本结构体); max=2^27=128M
    uint32_t    encode : 3;     ///< 编码方式(klb_mnp_encode_e)
    uint32_t    rsv1 : 2;       ///<

    uint32_t    sequence : 20;  ///< 序号
    uint32_t    rsv2 : 12;      ///< 

    uint32_t    head_size : 22; ///< 数据头部长度(max=2^22=4M): 正式数据body长度 = size - head_size - sizeof(klb_mnp_binary_t)
    uint32_t    rsv3 : 10;
    // - 4 + 4 + 4 = 12 字节
}klb_mnp_binary_t;


//////////////////////////////////////////////////////////////////////////
// RPC

/// @struct klb_mnp_rpc_method_e
/// @brief  RPC 方法
typedef enum klb_mnp_rpc_method_e_
{
    KLB_MNP_RPC_METHOD_NULL = 0x00,     ///< 空
    KLB_MNP_RPC_OPTIONS     = 0x01,     ///< 查询方法, 或查询支持的所有RPC方法
    KLB_MNP_RPC_GET         = 0x02,     ///< 获取
    KLB_MNP_RPC_POST        = 0x03,     ///< 提交
    KLB_MNP_RPC_PUT         = 0x04,     ///< 替换
    KLB_MNP_RPC_DELETE      = 0x05,     ///< 删除

    KLB_MNP_RPC_NOTIFY      = 0x0A,     ///< 通知数据

    KLB_MNP_RPC_REQUEST     = 0x10,     ///< 请求, 客户端 向 服务端 请求
    KLB_MNP_RPC_RESPONSE    = 0x11,     ///< 回应, 服务端 向 客户端 回应请求

    KLB_MNP_RPC_METHOD_MAX  = 0x1F,     ///< MAX = 2^5 = 32
}klb_mnp_rpc_method_e;

/// @struct klb_mnp_rpc_t
/// @brief  RPC header
///   包类型: packtype = KLB_MNP_RPC_LUA / KLB_MNP_RPC_JSON
/// @note 这里的 size 是 压缩之后的 大小
///   裸包格式(encode=NULL): [klb_mnp_rpc_t] + [数据头] + [数据体]
///   压缩包格式(encode=COMPRESS): [klb_mnp_rpc_t] + [klb_mnp_compress_t] + [数据体]
typedef struct klb_mnp_rpc_t_
{
    uint32_t    size : 27;      ///< 完整数据长度(data size, 包含本结构体); max=2^27=128M
    uint32_t    rpctype : 5;    ///< RPC类型: KLB_MNP_RPC_LUA / KLB_MNP_RPC_JSON

    uint32_t    sequence : 20;  ///< 序号
    uint32_t    method : 5;     ///< RPC方法: klb_mnp_rpc_method_e
    uint32_t    encode : 3;     ///< 编码方式(klb_mnp_encode_e)
    uint32_t    rsv : 4;        ///< 保留
    // - 4 + 4 = 8 字节
}klb_mnp_rpc_t;

/// @struct klb_mnp_rpcex_t
/// @brief  RPC扩展 header
///   包类型: packtype = KLB_MNP_RPC_EX
typedef struct klb_mnp_rpcex_t_
{
    uint32_t    size : 27;      ///< 完整数据长度(data size, 包含本结构体); max=2^27=128M
    uint32_t    encode : 3;     ///< 编码方式(klb_mnp_encode_e)
    uint32_t    rsv1 : 2;       ///<

    // 待定
}klb_mnp_rpcex_t;


//////////////////////////////////////////////////////////////////////////
// 用户自定义扩展 [KLB_MNP_PACKTYPE_USER_B, KLB_MNP_PACKTYPE_USER_E]
// 用于扩展私有数据包


/// @enum  klb_mnp_userid_e
/// @brief 用户自定义格式 标记
typedef enum klb_mnp_userid_e_
{
    KLB_MNP_USERID_NULL     = 0,                                ///< 无压缩
    KLB_MNP_USERID_MNP      = KLB_FOURCC(0, 'M', 'N', 'P'),     ///< MNP
    KLB_MNP_USERID_SMP      = KLB_FOURCC(0, 'S', 'M', 'P'),     ///< SMP
}klb_mnp_userid_e;


/// @struct klb_mnp_ptuser_t
/// @brief  用户自定义类型包头(packet type user)
///   裸包格式(encode=NULL): [klb_mnp_ptuser_t] + [用户自定义格式数据]
typedef struct klb_mnp_ptuser_t_
{
    uint32_t    size : 27;      ///< 完整数据长度(data size, 包含本结构体); max=2^27=128M
    uint32_t    encode : 3;     ///< 编码方式(klb_mnp_encode_e)
    uint32_t    rsv1 : 2;       ///<

    uint32_t    userid;         ///< 用户自定义ID, 用户自定义格式数据
    // - 4 + 4 = 8 字节
}klb_mnp_ptuser_t;


//////////////////////////////////////////////////////////////////////////
// 编码 压缩 加密

/// @enum  klb_mnp_compress_e
/// @brief 压缩/加密 格式
typedef enum klb_mnp_compress_e_
{
    KLB_MNP_COMPRESS_NULL   = 0,                                ///< 无压缩
    KLB_MNP_COMPRESS_ZIP    = KLB_FOURCC(0, 'Z', 'I', 'P'),     ///< ZIP压缩
    KLB_MNP_COMPRESS_MAX    = 0xFFFFFFFF,                       ///< max=2^32
}klb_mnp_compress_e;

/// @struct klb_mnp_compress_t
/// @brief  编码压缩头 header
///   eg. [klb_mnp_rpc_t] + [klb_mnp_compress_t] + [压缩数据]
typedef struct klb_mnp_compress_t_
{
    uint32_t    src_size : 27;  ///< 原始完整数据长度(不包含本结构体); max=2^27=128M
    uint32_t    rsv : 5;        ///< 保留

    uint32_t    format;         ///< 压缩格式(klb_mnp_compress_e); eg. zip
    // - 4 + 4 = 8 字节
}klb_mnp_compress_t;


//////////////////////////////////////////////////////////////////////////
// 媒体

/// @enum  klb_mnp_dtype_e
/// @brief 媒体数据类型(media data type)
/// @note 原打算使用ffmpeg的AVCodecID, 但其版本更新过程中值会变更,
///   所以这里重新定义
///   媒体数据类型 包含 音频,视频,图片, 及与这些相关的 AI分析等
typedef enum klb_mnp_dtype_e_
{
    KLB_MNP_DTYPE_NULL  = 0x000,
    KLB_MNP_DTYPE_H264  = 0x001,   ///< AV_CODEC_ID_H264
    KLB_MNP_DTYPE_H265  = 0x002,   ///< AV_CODEC_ID_H265

    KLB_MNP_DTYPE_AAC   = 0x051,   ///< AV_CODEC_ID_AAC

    KLB_MNP_DTYPE_JPEG  = 0x0A1,   ///< JPEG

    KLB_MNP_DTYPE_MAX   = 0xFFF,   ///< MAX=2^12=4096
}klb_mnp_dtype_e;

/// @enum   klb_mnp_sidx_e
/// @brief  媒体流序号
typedef enum klb_mnp_sidx_e_
{
    KLB_MNP_SIDX_NULL   = 0x000,   ///< NULL
    KLB_MNP_SIDX_V1     = 0x001,   ///< Video 1
    KLB_MNP_SIDX_V2     = 0x002,   ///< Video 2
    KLB_MNP_SIDX_V3     = 0x003,   ///< Video 3

    KLB_MNP_SIDX_A1     = 0x021,   ///< Audio 1
    KLB_MNP_SIDX_A2     = 0x022,   ///< Audio 2
    KLB_MNP_SIDX_A3     = 0x023,   ///< Audio 3

    KLB_MNP_SIDX_P1     = 0x041,   ///< Picture 1
    KLB_MNP_SIDX_P2     = 0x042,   ///< Picture 2
    KLB_MNP_SIDX_P3     = 0x043,   ///< Picture 3

    KLB_MNP_SIDX_I1     = 0x061,   ///< Image 1
    KLB_MNP_SIDX_I2     = 0x062,   ///< Image 2
    KLB_MNP_SIDX_I3     = 0x063,   ///< Image 3

    KLB_MNP_SIDX_MAX    = 0xFFF,   ///< MAX=2^12=4096
}klb_mnp_sidx_e;

/// @enum   klb_mnp_vtype_e
/// @brief  视频帧类型(video type)
typedef enum klb_mnp_vtype_e_
{
    KLB_MNP_VTYPE_P     = 0x00,     ///< P帧
    KLB_MNP_VTYPE_I     = 0x01,     ///< I帧
    KLB_MNP_VTYPE_B     = 0x02,     ///< B帧
    KLB_MNP_VTYPE_CFG   = 0x03,     ///< 视频配置(config)数据: vps, sps, pps, sei, eg.
    KLB_MNP_VTYPE_MAX   = 0xFF      ///< MAX=2^8=256
}klb_mnp_vtype_e;

/// @struct klb_mnp_media_t
/// @brief  media net protocol, media head
///  \n F包: [klb_mnp_t][klb_mnp_media_t][data...]
///  \n B包: [klb_mnp_t][klb_mnp_media_t][data...]
///  \n C包: [klb_mnp_t][data...]
///  \n E包: [klb_mnp_t][data...]
///  \n 数据包可以被存储, 只描述与数据有关部分
///  \n H264视频包格式: [klb_mnp_media_t] + [H264数据]
///  \n 加密H264视频包格式: [klb_mnp_media_t] + [klb_mnp_compress_t] + [加密H264数据]
///  \n H265视频包格式: [klb_mnp_media_t] + [H265数据]
///  \n AAC音频格式: [klb_mnp_media_t] + [klb_mnp_audio_t] + [AAC音频数据]
typedef struct klb_mnp_media_t_
{
    uint32_t    size : 27;          ///< 完整数据长度(data size, 包含本结构体); max=2^27=128M
    uint32_t    padding : 5;        ///< 末尾对齐数据; max=2^5=32

    int64_t     time;               ///< 时间戳(基于1970年基准,毫秒)

    uint32_t    dtype : 12;         ///< 数据类型(data type): klb_mnp_dtype_e
    uint32_t    sidx : 12;          ///< 流序号(stream index): klb_mnp_sidx_e
    uint32_t    vtype : 8;          ///< 视频类型(video type): klb_mnp_vtype_e;

    uint32_t    chnn : 16;          ///< 通道(channel); max=2^16=65536
    uint32_t    encode : 3;         ///< 编码方式(klb_mnp_encode_e)
    uint32_t    rsv : 13;           ///< 保留
    // - 4 + 8 + 4 + 4 = 20 字节
}klb_mnp_media_t;

/// @struct klb_mnp_audio_t
/// @brief  音频头部
///   音频包格式: [klb_mnp_media_t] + [klb_mnp_audio_t] + [音频]
///   加密音频包格式: [klb_mnp_media_t] + [klb_mnp_audio_t] +[klb_mnp_compress_t] + [加密音频]
typedef struct klb_mnp_audio_t_
{
    uint32_t    samples;                ///< 音频采样率; eg. 44100

    uint32_t    tracks: 8;              ///< 音频声道数; 1, 2, 5.1;
    uint32_t    bits_per_sample : 8;    ///< 音频编码数; 1(8比特), 2(16比特)
    uint32_t    rsv : 16;
    // - 4 + 4 = 8 字节
}klb_mnp_audio_t;


//////////////////////////////////////////////////////////////////////////

#pragma pack()

// MNP协议 结束
//////////////////////////////////////////////////////////////////////////


#ifdef __cplusplus
}
#endif

#endif // __KLB_MNP_H__
//end
