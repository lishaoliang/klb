#ifndef __KLB_RTSPPARSER_H__
#define __KLB_RTSPPARSER_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef enum klb_rtsptype_e_
{
    KLB_RTSPTYPE_null = 0,
    KLB_RTSPTYPE_rtsp,
    KLB_RTSPTYPE_rtp,
    KLB_RTSPTYPE_rtcp,
}klb_rtsptype_e;


typedef struct klb_rtspparser_t_
{
    int         rtsptype;           ///< rtsp 包类型(klb_rtsptype_e); eg. KLB_RTSPTYPE_rtp
    int         pack_len;           ///< packet length; 数据包长度
}klb_rtspparser_t;



int klb_rtspparser_parse(klb_rtspparser_t* p_parser, char* p_data, int data_len);




#if defined(__cplusplus)
}
#endif

#endif // __KLB_RTSPPARSER_H__
//end
