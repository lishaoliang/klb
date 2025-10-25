// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbnet/klbsmp/klb_smprpcer.h"


/// @brief 转换为RPCTYPE字符串
const char* klb_smprpcer_to_rpctype_string(int rpctype)
{
    switch (rpctype)
    {
    case KLB_MNP_RPC_LUA: { return "LUA"; break; }
    case KLB_MNP_RPC_JSON: { return "JSON"; break; }
    default: { break; }
    }
    return "";
}

/// @brief 转换为method字符串
const char* klb_smprpcer_to_method_string(klb_mnp_rpc_method_e method)
{
    switch (method)
    {
    case KLB_MNP_RPC_POST: { return "POST"; break; }
    case KLB_MNP_RPC_NOTIFY: { return "NOTIFY"; break; }
    case KLB_MNP_RPC_REQUEST: { return "REQUEST"; break; }
    default: { break; }
    }
    return "";
}

//end
