// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_webrtc_jitter.c
/// @brief   WebRTC RTP 抖动缓冲与重排序 (内部实现)
/// @version 0.1
/// @history 修改历史
///  \n [2026-10] 创建文件: 序号槽位缓冲 / 有序出队 / 缺口跳过 / 缺口查询 / 计数
/// @warning 单 SSRC; 槽位数须小于 32768
///////////////////////////////////////////////////////////////////////////
#include "klbwebrtc/core/klb_webrtc_jitter.h"
#include "klbnet/klb_netcode.h"
#include "klbmem/klb_mem.h"
#include <string.h>


/// @struct klb_webrtc_jb_slot_t
/// @brief  单个序号槽位
typedef struct klb_webrtc_jb_slot_t_
{
    uint8_t*    p_data;                 ///< 报文副本; 未占用为 NULL
    int         len;                    ///< 报文长度
    uint16_t    seq;                    ///< RTP 序号
    bool        used;                   ///< 槽位已占用
}klb_webrtc_jb_slot_t;


/// @struct klb_webrtc_jb_t_
/// @brief  抖动缓冲 (对外不透明)
struct klb_webrtc_jb_t_
{
    int                     slots;      ///< 槽位数
    klb_webrtc_jb_slot_t*   p_slot;     ///< 槽位数组
    bool                    has_base;   ///< 出队基点已初始化
    uint16_t                base;       ///< 出队基点序号
    int                     count;      ///< 已缓冲包数
    klb_webrtc_jb_stats_t   stats;      ///< 计数
};


/// @brief 序号相对基点的偏移 (16 位回绕)
/// @param [in] base    基点序号
/// @param [in] seq     待比较序号
/// @return int 大于等于 0.超前偏移; 小于 0.迟到
static int delta_klb_webrtc_jb(uint16_t base, uint16_t seq)
{
    return (int)(int16_t)(uint16_t)(seq - base);
}


/// @brief 清空一个槽位并释放报文
/// @param [in,out] *p_slot     槽位
/// @return 无
static void clear_slot_klb_webrtc_jb(klb_webrtc_jb_slot_t* p_slot)
{
    KLB_FREE(p_slot->p_data);

    p_slot->len = 0;
    p_slot->seq = 0;
    p_slot->used = false;
}


/// @brief 偏移映射到槽位下标
/// @param [in] *p_jb    缓冲
/// @param [in] delta   偏移
/// @return int 槽位下标
static int slot_of_klb_webrtc_jb(const klb_webrtc_jb_t* p_jb, int delta)
{
    return (int)(((uint32_t)p_jb->base + (uint32_t)delta) % (uint32_t)p_jb->slots);
}


klb_webrtc_jb_t* klb_webrtc_jb_create(int slots, int* p_code)
{
    if (0 >= slots)
    {
        slots = KLB_WEBRTC_JB_SLOTS;
    }

    if (32768 <= slots)
    {
        if (NULL != p_code)
        {
            *p_code = KLB_NETCODE_EINVAL;
        }

        return NULL;
    }

    klb_webrtc_jb_t* p_jb = KLB_MALLOCZ(klb_webrtc_jb_t, 1, 0);
    if (NULL == p_jb)
    {
        if (NULL != p_code)
        {
            *p_code = KLB_NETCODE_ENOMEM;
        }

        return NULL;
    }

    p_jb->p_slot = KLB_MALLOCZ(klb_webrtc_jb_slot_t, slots, 0);
    if (NULL == p_jb->p_slot)
    {
        KLB_FREE(p_jb);

        if (NULL != p_code)
        {
            *p_code = KLB_NETCODE_ENOMEM;
        }

        return NULL;
    }

    p_jb->slots = slots;

    if (NULL != p_code)
    {
        *p_code = 0;
    }

    return p_jb;
}


void klb_webrtc_jb_destroy(klb_webrtc_jb_t* p_jb)
{
    if (NULL == p_jb)
    {
        return;
    }

    if (NULL != p_jb->p_slot)
    {
        for (int i = 0; i < p_jb->slots; i++)
        {
            clear_slot_klb_webrtc_jb(&p_jb->p_slot[i]);
        }

        KLB_FREE(p_jb->p_slot);
    }

    KLB_FREE(p_jb);
}


void klb_webrtc_jb_reset(klb_webrtc_jb_t* p_jb)
{
    if (NULL == p_jb)
    {
        return;
    }

    for (int i = 0; i < p_jb->slots; i++)
    {
        clear_slot_klb_webrtc_jb(&p_jb->p_slot[i]);
    }

    p_jb->has_base = false;
    p_jb->base = 0;
    p_jb->count = 0;

    KLB_MEMSET(&p_jb->stats, 0, sizeof(p_jb->stats));
}


int klb_webrtc_jb_put(klb_webrtc_jb_t* p_jb, uint16_t seq, const uint8_t* p_data, int len)
{
    if ((NULL == p_jb) || (NULL == p_data) || (0 >= len))
    {
        return KLB_NETCODE_EINVAL;
    }

    if (!p_jb->has_base)
    {
        p_jb->has_base = true;
        p_jb->base = seq;
    }

    int delta = delta_klb_webrtc_jb(p_jb->base, seq);
    if (0 > delta)
    {
        p_jb->stats.late_pkts++;
        return 0;
    }

    if (p_jb->slots <= delta)
    {
        p_jb->stats.overflow_pkts++;
        return 0;
    }

    klb_webrtc_jb_slot_t* p_slot = &p_jb->p_slot[slot_of_klb_webrtc_jb(p_jb, delta)];
    if (p_slot->used)
    {
        p_jb->stats.dup_pkts++;
        return 1;
    }

    uint8_t* p_copy = KLB_MALLOC(uint8_t, len, 0);
    if (NULL == p_copy)
    {
        return KLB_NETCODE_ENOMEM;
    }

    memcpy(p_copy, p_data, (size_t)len);

    p_slot->p_data = p_copy;
    p_slot->len = len;
    p_slot->seq = seq;
    p_slot->used = true;

    p_jb->count++;
    p_jb->stats.recv_pkts++;

    if (0 < delta)
    {
        p_jb->stats.reorder_pkts++;
    }

    return 0;
}


int klb_webrtc_jb_get(klb_webrtc_jb_t* p_jb, uint8_t* p_buf, int cap, uint16_t* p_seq)
{
    if ((NULL == p_jb) || (NULL == p_buf) || (0 >= cap))
    {
        return 0;
    }

    if ((!p_jb->has_base) || (0 >= p_jb->count))
    {
        return 0;
    }

    klb_webrtc_jb_slot_t* p_slot = &p_jb->p_slot[slot_of_klb_webrtc_jb(p_jb, 0)];
    if (!p_slot->used)
    {
        return 0;
    }

    int n = (p_slot->len > cap) ? cap : p_slot->len;
    memcpy(p_buf, p_slot->p_data, (size_t)n);

    if (NULL != p_seq)
    {
        *p_seq = p_slot->seq;
    }

    clear_slot_klb_webrtc_jb(p_slot);

    p_jb->count--;
    p_jb->base = (uint16_t)(p_jb->base + 1);
    p_jb->stats.got_pkts++;

    return n;
}


int klb_webrtc_jb_skip(klb_webrtc_jb_t* p_jb)
{
    if ((NULL == p_jb) || (!p_jb->has_base) || (0 >= p_jb->count))
    {
        return 0;
    }

    int best = -1;
    for (int delta = 1; delta < p_jb->slots; delta++)
    {
        if (p_jb->p_slot[slot_of_klb_webrtc_jb(p_jb, delta)].used)
        {
            best = delta;
            break;
        }
    }

    if (0 > best)
    {
        return 0;
    }

    p_jb->base = (uint16_t)(p_jb->base + (uint16_t)best);
    p_jb->stats.lost_pkts += (uint64_t)best;

    return best;
}


int klb_webrtc_jb_missing(klb_webrtc_jb_t* p_jb, uint16_t* p_seqs, int max)
{
    if ((NULL == p_jb) || (!p_jb->has_base) || (0 >= p_jb->count))
    {
        return 0;
    }

    if ((NULL == p_seqs) || (0 >= max))
    {
        return 0;
    }

    int highest = -1;
    for (int delta = p_jb->slots - 1; delta >= 1; delta--)
    {
        if (p_jb->p_slot[slot_of_klb_webrtc_jb(p_jb, delta)].used)
        {
            highest = delta;
            break;
        }
    }

    if (0 > highest)
    {
        return 0;
    }

    int num = 0;
    for (int delta = 0; (delta < highest) && (num < max); delta++)
    {
        if (!p_jb->p_slot[slot_of_klb_webrtc_jb(p_jb, delta)].used)
        {
            p_seqs[num] = (uint16_t)(p_jb->base + (uint16_t)delta);
            num++;
        }
    }

    return num;
}


int klb_webrtc_jb_pending(klb_webrtc_jb_t* p_jb)
{
    if (NULL == p_jb)
    {
        return 0;
    }

    return p_jb->count;
}


int klb_webrtc_jb_stats(klb_webrtc_jb_t* p_jb, klb_webrtc_jb_stats_t* p_stats)
{
    if ((NULL == p_jb) || (NULL == p_stats))
    {
        return KLB_NETCODE_EINVAL;
    }

    *p_stats = p_jb->stats;

    return 0;
}


// end
