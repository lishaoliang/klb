// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_webrtc_jitter.h
/// @brief   WebRTC RTP 抖动缓冲与重排序 (内部接口, 仅供 core 使用)
/// @version 0.1
/// @history 修改历史
///  \n [2026-10] 创建文件: 序号槽位缓冲 / 有序出队 / 缺口跳过 / 缺口查询 / 计数
/// @warning 单 SSRC; 槽位满的包与迟到包直接丢弃并计数
///  \n 序号比较用 16 位回绕差值, 缓冲窗口须小于 32768
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_WEBRTC_JITTER_H__
#define __KLB_WEBRTC_JITTER_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @def   KLB_WEBRTC_JB_SLOTS
/// @brief 抖动缓冲默认槽位数
#define KLB_WEBRTC_JB_SLOTS            64


/// @struct klb_webrtc_jb_stats_t
/// @brief  抖动缓冲计数
typedef struct klb_webrtc_jb_stats_t_
{
    uint64_t    recv_pkts;              ///< 入缓冲包数
    uint64_t    got_pkts;               ///< 有序出队包数
    uint64_t    dup_pkts;               ///< 重复包数
    uint64_t    late_pkts;              ///< 迟到包数 (序号落后于出队基点)
    uint64_t    overflow_pkts;          ///< 越窗丢弃包数
    uint64_t    reorder_pkts;           ///< 乱序到达包数
    uint64_t    lost_pkts;              ///< 跳过缺口丢弃的序号数
}klb_webrtc_jb_stats_t;


/// @brief 抖动缓冲 (不透明)
typedef struct klb_webrtc_jb_t_ klb_webrtc_jb_t;


/// @brief 创建抖动缓冲
/// @param [in]  slots          槽位数; 小于等于 0 用 KLB_WEBRTC_JB_SLOTS
/// @param [out] *p_code        可选; 失败时 klb_netcode_e
/// @return klb_webrtc_jb_t* NULL.失败; 非 NULL.缓冲
klb_webrtc_jb_t* klb_webrtc_jb_create(int slots, int* p_code);


/// @brief 销毁抖动缓冲
/// @param [in] *p_jb           缓冲; 可为 NULL
/// @return 无
void klb_webrtc_jb_destroy(klb_webrtc_jb_t* p_jb);


/// @brief 重置缓冲 (释放全部包, 清空计数, 基点回到未初始化)
/// @param [in] *p_jb           缓冲
/// @return 无
void klb_webrtc_jb_reset(klb_webrtc_jb_t* p_jb);


/// @brief 写入一个包 (按序号落槽)
/// @param [in] *p_jb           缓冲
/// @param [in] seq             RTP 序号
/// @param [in] *p_data         报文
/// @param [in] len             报文长度
/// @return int 0.成功; 1.重复; 小于 0.klb_netcode_e
/// @note 迟到与越窗包丢弃并计数, 仍返回 0
int klb_webrtc_jb_put(klb_webrtc_jb_t* p_jb, uint16_t seq, const uint8_t* p_data, int len);


/// @brief 取出基点处的一个有序包, 基点前进 1
/// @param [in]  *p_jb          缓冲
/// @param [out] *p_buf         输出缓冲
/// @param [in]  cap            输出容量
/// @param [out] *p_seq         可选; 输出序号
/// @return int 大于 0.字节数; 0.基点处无包 (存在缺口或缓冲空)
int klb_webrtc_jb_get(klb_webrtc_jb_t* p_jb, uint8_t* p_buf, int cap, uint16_t* p_seq);


/// @brief 跳过缺口: 基点前进到最小正偏移的已收包处
/// @param [in] *p_jb           缓冲
/// @return int 大于等于 0.跳过的序号数; 0.无缺口或缓冲空
int klb_webrtc_jb_skip(klb_webrtc_jb_t* p_jb);


/// @brief 查询基点之后的缺口序号
/// @param [in]  *p_jb          缓冲
/// @param [out] *p_seqs        输出序号数组
/// @param [in]  max            数组容量
/// @return int 大于等于 0.缺口个数; 0.无缺口
int klb_webrtc_jb_missing(klb_webrtc_jb_t* p_jb, uint16_t* p_seqs, int max);


/// @brief 当前缓冲中的包数
/// @param [in] *p_jb           缓冲
/// @return int 包数; 缓冲为 NULL 回 0
int klb_webrtc_jb_pending(klb_webrtc_jb_t* p_jb);


/// @brief 取计数
/// @param [in]  *p_jb          缓冲
/// @param [out] *p_stats       计数结果
/// @return int 0.成功; 小于 0.klb_netcode_e
int klb_webrtc_jb_stats(klb_webrtc_jb_t* p_jb, klb_webrtc_jb_stats_t* p_stats);


#if defined(__cplusplus)
}
#endif

#endif // __KLB_WEBRTC_JITTER_H__

// end
