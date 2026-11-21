// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbonvif/core/klb_onvif_client.h"


//////////////////////////////////////////////////////////////////////////


klb_onvif_client_t* klb_onvif_client_create(void)
{
    // stub: not implemented
    return NULL;
}


void klb_onvif_client_destroy(klb_onvif_client_t* p_client)
{
    (void)p_client;
}


int klb_onvif_client_call(klb_onvif_client_t* p_client, const char* p_action,
                          const char* p_req, char** pp_rsp)
{
    (void)p_client;
    (void)p_action;
    (void)p_req;

    if (NULL != pp_rsp)
    {
        *pp_rsp = NULL;
    }

    // stub: not implemented
    return -1;
}


// end
