// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbnet/klbsmp/klb_smpparser.h"
#include <string.h>


/// @brief 解析 SMP 包
/// @return int 0.解析成功; -1. 协议错误; 1. 数据不足
int klb_smpparser_parse(klb_smpparser_t* p_parser, char* p_data, int data_len)
{
    // 初始化
    {
        memset(p_parser, 0, sizeof(klb_smpparser_t));
        p_parser->packtype = -1;
    }

    // 数据不足
    if (data_len < sizeof(klb_smp_t))
    {
        return 1;
    }

    int pos = 0;
    uint8_t* ptr = p_data;

    klb_smp_t* p_smp = (klb_smp_t*)ptr;
    if (KLB_SMP_MAGIC != p_smp->magic)
    {
        return -1; // 协议错误
    }

    if (0x80000000 <= p_smp->size)
    {
        return -1; // 协议错误
    }

    ptr += sizeof(klb_smp_t); // 偏移
    pos += sizeof(klb_smp_t); // 偏移

    switch (p_smp->packtype)
    {
    case KLB_MNP_PONG: ///< 心跳包(回应): 附加数据为0; 否则协议错误
    case KLB_MNP_PING: ///< 心跳包(发起): 附加数据为0; 否则协议错误
        {
            if (sizeof(klb_smp_t) != p_smp->size)
            {
                return -1; // 协议错误
            }
        }
        break;

    case KLB_MNP_TEXT: ///< 文本数据
        {

        }
        break;

    case KLB_MNP_BINARY: ///< 二进制数据
        {

        }
        break;

    case KLB_MNP_MEDIA: ///< 媒体数据
        {

        }
        break;

    case KLB_MNP_RPC_LUA: ///< RPC Lua包
    case KLB_MNP_RPC_JSON: ///< RPC Json包
        {
            if (data_len < sizeof(klb_smp_t) + sizeof(klb_mnp_rpc_t))
            {
                return 1; // 数据长度不足
            }

            klb_mnp_rpc_t* p_rpc = (klb_mnp_rpc_t*)ptr;

            memcpy(&p_parser->rpc, p_rpc, sizeof(klb_mnp_rpc_t));

            ptr += sizeof(klb_mnp_rpc_t); // 偏移
            pos += sizeof(klb_mnp_rpc_t); // 偏移
        }

    default:
        break;
    }

    memcpy(&p_parser->smp, p_smp, sizeof(klb_smp_t));

    p_parser->packtype = p_smp->packtype;
    p_parser->pack_len = p_smp->size;
    p_parser->head_len = pos;

    return 0;
}

//end
