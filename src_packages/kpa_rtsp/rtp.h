///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    rtp.h
/// @brief   rtp
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __RTP_H__
#define __RTP_H__


#include <stdint.h>


#if defined(__cplusplus)
extern "C" {
#endif

// RFC 3550_ RTP_ A Transport Protocol for Real-Time Applications.html
// 结构体定义来源于 RFC 3550

/*
* Current protocol version.
*/
#define RTP_VERSION     2

#define RTP_SEQ_MOD     (1<<16)
#define RTP_MAX_SDES    255      /* maximum text length for SDES */

typedef enum rtcp_type_e_
{
    RTCP_SR     = 200,
    RTCP_RR     = 201,
    RTCP_SDES   = 202,
    RTCP_BYE    = 203,
    RTCP_APP    = 204
} rtcp_type_e;

typedef enum rtcp_sdes_type_e_
{
    RTCP_SDES_END   = 0,
    RTCP_SDES_CNAME = 1,
    RTCP_SDES_NAME  = 2,
    RTCP_SDES_EMAIL = 3,
    RTCP_SDES_PHONE = 4,
    RTCP_SDES_LOC   = 5,
    RTCP_SDES_TOOL  = 6,
    RTCP_SDES_NOTE  = 7,
    RTCP_SDES_PRIV  = 8
} rtcp_sdes_type_e;

/*
* RFC-3550 5.1 RTP Fixed Header Fields
* RTP data header
*  0                   1                   2                   3
*  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
* |V=2|P|X|   CC  |M|      PT     |      sequence number          |
* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
* | timestamp                                                     |
* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
* | synchronization source(SSRC) identifier                       |
* +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
* | contributing source(CSRC) identifiers                         |
* | ....                                                          |
* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
typedef struct rtp_hdr_t_
{
    uint32_t version : 2;   /* protocol version */
    uint32_t p : 1;         /* padding flag */
    uint32_t x : 1;         /* header extension flag */
    uint32_t cc : 4;        /* CSRC count */
    uint32_t m : 1;         /* marker bit */
    uint32_t pt : 7;        /* payload type */
    uint32_t seq : 16;      /* sequence number */
    uint32_t ts;            /* timestamp */
    uint32_t ssrc;          /* synchronization source */
    uint32_t csrc[1];       /* optional CSRC list */
} rtp_hdr_t;

/*
* RTCP common header word
*/
typedef struct rtcp_common_t_
{
    uint16_t version : 2;   /* protocol version */
    uint16_t p : 1;         /* padding flag */
    uint16_t count : 5;     /* varies by packet type */
    uint16_t pt : 8;        /* RTCP packet type */
    uint16_t length;        /* pkt len in words, w/o this word */
} rtcp_common_t;

/*
* Big-endian mask for version, padding bit and packet type pair
*/
#define RTCP_VALID_MASK     (0xc000 | 0x2000 | 0xfe)
#define RTCP_VALID_VALUE    ((RTP_VERSION << 14) | RTCP_SR)

/*
* Reception report block
*/
typedef struct rtcp_rr_t_
{
    uint32_t ssrc;          /* data source being reported */
    uint32_t fraction : 8;  /* fraction lost since last SR/RR */
    uint32_t lost : 24;     /* cumul. no. pkts lost (signed!) */
    uint32_t last_seq;      /* extended last seq. no. received */
    uint32_t jitter;        /* interarrival jitter */
    uint32_t lsr;           /* last SR packet from this source */
    uint32_t dlsr;          /* delay since last SR packet */
} rtcp_rr_t;

/*
* SDES item
*/
typedef struct rtcp_sdes_item_t_
{
    uint8_t type;           /* type of item (rtcp_sdes_type_t) */
    uint8_t length;         /* length of item (in octets) */
    char    data[1];        /* text, not null-terminated */
} rtcp_sdes_item_t;


/*
* One RTCP packet
*/
typedef struct rtcp_t_
{
    rtcp_common_t       common;     /* common header */
    union {
        /* sender report (SR) */
        struct {
            uint32_t    ssrc;       /* sender generating this report */
            uint32_t    ntp_sec;    /* NTP timestamp */
            uint32_t    ntp_frac;   
            uint32_t    rtp_ts;     /* RTP timestamp */
            uint32_t    psent;      /* packets sent */
            uint32_t    osent;      /* octets sent */
            rtcp_rr_t   rr[1];      /* variable-length list */
        } sr;

        /* reception report (RR) */
        struct {
            uint32_t    ssrc;       /* receiver generating this report */
            rtcp_rr_t   rr[1];      /* variable-length list */
        } rr;

        /* source description (SDES) */
        struct rtcp_sdes {
            uint32_t    src;        /* first SSRC/CSRC */
            rtcp_sdes_item_t item[1];   /* list of SDES items */
        } sdes;

        /* BYE */
        struct {
            uint32_t    src[1];     /* list of sources */
            /* can't express trailing text for reason */
        } bye;
    } r;
} rtcp_t;

typedef struct rtcp_sdes_t rtcp_sdes_t;

/*
* Per-source state information
*/
typedef struct rtcp_source_t_
{
    uint16_t max_seq;        /* highest seq. number seen */
    uint32_t cycles;         /* shifted count of seq. number cycles */
    uint32_t base_seq;       /* base seq number */
    uint32_t bad_seq;        /* last 'bad' seq number + 1 */
    uint32_t probation;      /* sequ. packets till source is valid */
    uint32_t received;       /* packets received */
    uint32_t expected_prior; /* packet expected at last interval */
    uint32_t received_prior; /* packet received at last interval */
    uint32_t transit;        /* relative trans time for prev pkt */
    uint32_t jitter;         /* estimated jitter */
    /* ... */
}rtcp_source_t;


#ifdef __cplusplus
}
#endif

#endif // __RTP_H__
//end
