// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbws/core/klb_wsrpcer.h"
#include "klbbase/klb_mnp.h"


/// @brief 转换为 RPCTYPE 字符串
const char* klb_wsrpcer_to_rpctype_string(int rpctype)
{
    switch (rpctype)
    {
    case KLB_MNP_RPC_LUA:
    {
        return "LUA";
    }
    case KLB_MNP_RPC_JSON:
    {
        return "JSON";
    }
    default:
    {
        break;
    }
    }

    return "";
}

/// @brief 转换为 method 字符串
const char* klb_wsrpcer_to_method_string(int method)
{
    switch (method)
    {
    case KLB_MNP_RPC_POST:
    {
        return "POST";
    }
    case KLB_MNP_RPC_NOTIFY:
    {
        return "NOTIFY";
    }
    case KLB_MNP_RPC_REQUEST:
    {
        return "REQUEST";
    }
    case KLB_MNP_RPC_RESPONSE:
    {
        return "RESPONSE";
    }
    default:
    {
        break;
    }
    }

    return "";
}

// end
