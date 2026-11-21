// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbmnp/core/klb_mnprpcer.h"


const char* klb_mnprpcer_to_rpctype_string(int rpctype)
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

const char* klb_mnprpcer_to_method_string(klb_mnp_rpc_method_e method)
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
