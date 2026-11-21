///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_mnp_codec.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   MNP/SMP TEXT(BINARY) 载荷编解码; 含可选 ZIP(zlib wrap) 压缩
/// @version 0.1
/// @history 修改历史
///  \n [2026-10] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_MNP_CODEC_H__
#define __KLB_MNP_CODEC_H__

#include "klb_type.h"
#include "klbmem/klb_buf.h"
#include "klbbase/klb_mnp.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 编解码错误码
typedef enum klb_mnp_codec_err_e_
{
    KLB_MNP_CODEC_OK        = 0,    ///< 成功
    KLB_MNP_CODEC_EPROTO    = 1,    ///< 协议错误(布局/风格/解压失败)
    KLB_MNP_CODEC_ENOSUP    = 2,    ///< 本端不支持压缩或未知 format
}klb_mnp_codec_err_e;


/// @brief TEXT 压缩连接策略(C 连接层维护; Lua 无感)
typedef struct klb_mnp_codec_text_policy_t_
{
    bool        is_server;          ///< true=服务端; false=客户端
    bool        use_compress;       ///< 客户端: 是否对 TEXT 使用压缩
    bool        peer_locked;        ///< 服务端: 是否已按首条客户端 TEXT 锁定
    bool        peer_compress;      ///< 服务端: 对端是否使用压缩
    uint32_t    peer_format;        ///< 服务端: 对端压缩 format(eg. ZIP)
}klb_mnp_codec_text_policy_t;


/// @brief 本端是否支持 TEXT 压缩(无 zlib 裁剪时为 false)
/// @return bool
KLB_API bool klb_mnp_codec_compress_supported(void);


/// @brief 窥探载荷 encode 字段
/// @param [in] *p_in       TEXT/BINARY 载荷(含 klb_mnp_text_t)
/// @return int  klb_mnp_encode_e; 非法时返回 KLB_MNP_ENCODE_NULL
KLB_API int klb_mnp_codec_tb_encode(const klb_buf_t* p_in);


/// @brief 窥探压缩包首段 format(encode=COMPRESS 时)
/// @param [in] *p_in       载荷
/// @return uint32_t  format; 非法为 0
KLB_API uint32_t klb_mnp_codec_tb_format(const klb_buf_t* p_in);


/// @brief 组装 TEXT/BINARY 载荷(布局同 klb_mnp_text_t)
/// @param [in] sequence    序号
/// @param [in] encode      KLB_MNP_ENCODE_NULL / COMPRESS
/// @param [in] *p_head     明文头; 可为 NULL
/// @param [in] head_len    明文头长度
/// @param [in] *p_body     明文体; 可为 NULL
/// @param [in] body_len    明文体长度
/// @return klb_buf_t* 成功非 NULL(调用者 klb_buf_unref); 失败 NULL
KLB_API klb_buf_t* klb_mnp_codec_pack_tb(int sequence, int encode,
    const uint8_t* p_head, int head_len,
    const uint8_t* p_body, int body_len);


/// @brief 解包为裸载荷(encode=NULL)
/// @param [in]  *p_in      输入载荷
/// @param [out] **pp_out   输出; 若本已裸包则 *pp_out==p_in(所有权仍归调用者)
///                         若解压则 *pp_out 为新缓冲(调用者负责; p_in 不释放)
/// @return int  klb_mnp_codec_err_e
KLB_API int klb_mnp_codec_unpack_tb(klb_buf_t* p_in, klb_buf_t** pp_out);


/// @brief 发送侧决定 TEXT encode
/// @param [in] *p_pol      策略
/// @return int  klb_mnp_encode_e
KLB_API int klb_mnp_codec_text_send_encode(const klb_mnp_codec_text_policy_t* p_pol);


/// @brief 接收 TEXT: 更新策略并解压为裸包
/// @param [in,out] *p_pol      策略
/// @param [in,out] **pp_data   入站载荷; 成功后为裸包(若替换则旧缓冲已 unref)
/// @return int  klb_mnp_codec_err_e; 非 0 时 *pp_data 仍由调用者释放, 并应断连
KLB_API int klb_mnp_codec_text_on_recv(klb_mnp_codec_text_policy_t* p_pol, klb_buf_t** pp_data);


#if defined(__cplusplus)
}
#endif

#endif // __KLB_MNP_CODEC_H__

// end
