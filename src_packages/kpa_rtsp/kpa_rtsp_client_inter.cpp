#include "kpa_rtsp/kpa_rtsp_client_inter.h"
#include "klbmem/klb_mem.h"
#include "klbutil/CMap.hpp"
#include "klbutil/klb_log.h"
#include <assert.h>


#define KPA_RTSP_CLIENT_USER_AGENT  "KlbClient/1.0.0"

static int get_seq_kpa_rtsp_client_inter(kpa_rtsp_client_inter_t* p_inter)
{
    int id = p_inter->seq_next;

    p_inter->seq_next++;
    if (0x7FFF <= p_inter->seq_next)
    {
        p_inter->seq_next = 2;
    }

    return id;
}

static void send_options_kpa_rtsp_client_inter(kpa_rtsp_client_inter_t* p_inter)
{ 
    int seq = get_seq_kpa_rtsp_client_inter(p_inter);

    sds str = sdsnew("");
    str = sdscatfmt(str, "OPTIONS %s RTSP/1.0\r\n", p_inter->url); // eg. "OPTIONS rtsp://127.0.0.1:554/ RTSP/1.0"
    str = sdscatfmt(str, "CSeq: %i\r\n", seq);
    str = sdscatfmt(str, "User-Agent: %s\r\n", KPA_RTSP_CLIENT_USER_AGENT);
    str = sdscat(str, "\r\n");

    kpa_rtsp_tcp_send(p_inter->p_tcp, str, sdslen(str));
    KLB_FREE_BY(str, sdsfree);

    p_inter->last_seq = seq;
    p_inter->last_cmd = KPA_RTSP_CLIENT_OPTIONS;
}

static void send_describe_kpa_rtsp_client_inter(kpa_rtsp_client_inter_t* p_inter)
{
    int seq = get_seq_kpa_rtsp_client_inter(p_inter);

    sds str = sdsnew("");
    str = sdscatfmt(str, "DESCRIBE %s RTSP/1.0\r\n", p_inter->url); // eg. "DESCRIBE rtsp://127.0.0.1:554/ RTSP/1.0"
    str = sdscatfmt(str, "CSeq: %i\r\n", seq);
    str = sdscatfmt(str, "User-Agent: %s\r\n", KPA_RTSP_CLIENT_USER_AGENT);
    str = sdscat(str, "Accept: application/sdp\r\n");
    str = sdscat(str, "\r\n");

    kpa_rtsp_tcp_send(p_inter->p_tcp, str, sdslen(str));
    KLB_FREE_BY(str, sdsfree);

    p_inter->last_seq = seq;
    p_inter->last_cmd = KPA_RTSP_CLIENT_DESCRIBE;
}

static void send_setup_tcp_kpa_rtsp_client_inter(kpa_rtsp_client_inter_t* p_inter, const char* p_url, bool video)
{
    // 使用TCP方式负载音视频
    int seq = get_seq_kpa_rtsp_client_inter(p_inter);

    sds str = sdsnew("");
    str = sdscatfmt(str, "SETUP %s RTSP/1.0\r\n", p_url); // eg. "SETUP rtsp://127.0.0.1:554/video RTSP/1.0"
    str = sdscatfmt(str, "CSeq: %i\r\n", seq);
    str = sdscatfmt(str, "User-Agent: %s\r\n", KPA_RTSP_CLIENT_USER_AGENT);
    str = sdscat(str, "Transport: RTP/AVP/TCP;unicast\r\n");
    if (0 < sdslen(p_inter->session)) { str = sdscatfmt(str, "Session: %s\r\n", p_inter->session); }
    str = sdscat(str, "\r\n");

    kpa_rtsp_tcp_send(p_inter->p_tcp, str, sdslen(str));
    KLB_FREE_BY(str, sdsfree);

    p_inter->last_seq = seq;
    p_inter->last_cmd = video ? KPA_RTSP_CLIENT_SETUP_VIDEO : KPA_RTSP_CLIENT_SETUP_AUDIO;
}

static void send_setup_udp_kpa_rtsp_client_inter(kpa_rtsp_client_inter_t* p_inter, const char* p_url, bool video, int port1, int port2)
{
    // 使用UDP方式负载音视频
    // 需要使用2个UDP端口; port1号传输音视频, port2号传输rtcp数据包
    int seq = get_seq_kpa_rtsp_client_inter(p_inter);

    sds str = sdsnew("");
    str = sdscatfmt(str, "SETUP %s RTSP/1.0\r\n", p_url); // eg. "SETUP rtsp://127.0.0.1:554/video RTSP/1.0"
    str = sdscatfmt(str, "CSeq: %i\r\n", seq);
    str = sdscatfmt(str, "User-Agent: %s\r\n", KPA_RTSP_CLIENT_USER_AGENT);
    str = sdscatfmt(str, "Transport: RTP/AVP;unicast;client_port=%i-%i\r\n", port1, port2);
    if (0 < sdslen(p_inter->session)) { str = sdscatfmt(str, "Session: %s\r\n", p_inter->session); }
    str = sdscat(str, "\r\n");

    kpa_rtsp_tcp_send(p_inter->p_tcp, str, sdslen(str));
    KLB_FREE_BY(str, sdsfree);

    p_inter->last_seq = seq;
    p_inter->last_cmd = video ? KPA_RTSP_CLIENT_SETUP_VIDEO : KPA_RTSP_CLIENT_SETUP_AUDIO;
}

static void send_play_udp_kpa_rtsp_client_inter(kpa_rtsp_client_inter_t* p_inter)
{
    int seq = get_seq_kpa_rtsp_client_inter(p_inter);

    sds str = sdsnew("");
    str = sdscatfmt(str, "PLAY %s RTSP/1.0\r\n", p_inter->url); // eg. "PLAY rtsp://127.0.0.1:554/ RTSP/1.0"
    str = sdscatfmt(str, "CSeq: %i\r\n", seq);
    str = sdscatfmt(str, "User-Agent: %s\r\n", KPA_RTSP_CLIENT_USER_AGENT);
    str = sdscat(str, "Range: npt=0.000-\r\n");
    if (0 < sdslen(p_inter->session)) { str = sdscatfmt(str, "Session: %s\r\n", p_inter->session); }
    str = sdscat(str, "\r\n");

    kpa_rtsp_tcp_send(p_inter->p_tcp, str, sdslen(str));
    KLB_FREE_BY(str, sdsfree);

    p_inter->last_seq = seq;
    p_inter->last_cmd = KPA_RTSP_CLIENT_PLAY;
}

static void send_get_parameter_udp_kpa_rtsp_client_inter(kpa_rtsp_client_inter_t* p_inter)
{
    int seq = get_seq_kpa_rtsp_client_inter(p_inter);

    sds str = sdsnew("");
    str = sdscatfmt(str, "GET_PARAMETER %s RTSP/1.0\r\n", p_inter->url); // eg. "GET_PARAMETER rtsp://127.0.0.1:554/ RTSP/1.0"
    str = sdscatfmt(str, "CSeq: %i\r\n", seq);
    str = sdscatfmt(str, "User-Agent: %s\r\n", KPA_RTSP_CLIENT_USER_AGENT);
    if (0 < sdslen(p_inter->session)) { str = sdscatfmt(str, "Session: %s\r\n", p_inter->session); }
    str = sdscat(str, "\r\n");

    kpa_rtsp_tcp_send(p_inter->p_tcp, str, sdslen(str));
    KLB_FREE_BY(str, sdsfree);

    p_inter->last_seq = seq;
    p_inter->last_cmd = KPA_RTSP_CLIENT_GET_PARAMETER;
}

static void parse_public_kpa_rtsp_client_inter(klb::CMap& param)
{
    // eg. Public: OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, GET_PARAMETER
    std::string str = param["Public"].ToString();

    klb::CMap r;

    int count = 0;
    sds* split = sdssplitlen(str.c_str(), str.size(), ",", 1, &count);
    if (NULL != split && 0 < count)
    {
        for (int i = 0; i < count; i++)
        {
            split[i] = sdstrim(split[i], " ");
            r.Append(split[i]);
        }

        sdsfreesplitres(split, count);
    }

    param["Public"] = r;
}

static void parse_transport_kpa_rtsp_client_inter(klb::CMap& param)
{
    // eg. Transport: RTP/AVP/UDP;unicast;destination=127.0.0.1;client_port=58594-58595;server_port=3458-3459
    std::string str = param["Transport"].ToString();

    klb::CMap r;

    int count = 0;
    sds* split = sdssplitlen(str.c_str(), str.size(), ";", 1, &count);
    if (NULL != split && 0 < count)
    {
        for (int i = 0; i < count; i++)
        {
            int kv_count = 0;
            sds* kv = sdssplitlen(split[i], sdslen(split[i]), "=", 1, &kv_count);
            if (NULL != kv && 0 < kv_count)
            {
                if (1 == kv_count)
                {
                    kv[0] = sdstrim(kv[0], " ");

                    r.Append(kv[0]);
                }
                else if(1 < kv_count)
                {
                    kv[0] = sdstrim(kv[0], " ");
                    kv[1] = sdstrim(kv[1], " ");

                    r.Set(kv[0], kv[1]);
                }

                sdsfreesplitres(kv, kv_count);
            }
        }

        sdsfreesplitres(split, count);
    }

    param["Transport"] = r;
}

static char s_kpa_rtsp_client_rn[] = "\r\n";

static bool parse_rtsp_kpa_rtsp_client_inter(kpa_rtsp_client_inter_t* p_inter, sds head, sds body, int* p_code, klb::CMap& param)
{
    // 第一阶段, rtsp文本协议通信: tcp rtsp
    // eg. 
    // RTSP/1.0 200 OK
    // CSeq: 2
    // Public : OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, GET_PARAMETER
    head = sdstrim(head, "\r\n\r\n");

    bool ret = false;

    int count = 0;
    sds* split = sdssplitlen(head, sdslen(head), s_kpa_rtsp_client_rn, sizeof(s_kpa_rtsp_client_rn) - 1, &count);
    if (NULL != split && 0 < count)
    {
        // 首行: eg. RTSP/1.0 200 OK
        int code = 0;
        sscanf(split[0], "%*[^ ] %d", &code);

        *p_code = code;
        ret = true;

        // 参数: eg. CSeq: 2
        for (int i = 1; i < count; i++)
        {
            int kv_count = 0;
            sds* kv = sdssplitlen(split[i], sdslen(split[i]), ":", 1, &kv_count);
            if (NULL != kv && 0 < kv_count)
            {
                if (2 <= kv_count)
                {
                    kv[0] = sdstrim(kv[0], " ");
                    kv[1] = sdstrim(kv[1], " ");

                    param[kv[0]] = kv[1];
                }

                sdsfreesplitres(kv, kv_count);
            }
        }

        sdsfreesplitres(split, count);
    }

    // 解析 "Public"
    if (KLB_ADT_string == param["Public"].Type())
    {
        parse_public_kpa_rtsp_client_inter(param);
    }

    // 解析 "Transport"
    if (KLB_ADT_string == param["Transport"].Type())
    {
        parse_transport_kpa_rtsp_client_inter(param);
    }

    return ret;
}

bool pase_transport_kpa_rtsp_client_inter(kpa_rtsp_client_inter_t* p_inter, klb::CMap& param, bool* p_tcp, int* p_client_port1, int* p_client_port2, int* p_server_port1, int* p_server_port2)
{
    if (KLB_ADT_map != param["Transport"].Type())
    {
        return false;
    }

    klb::CMap transport;
    param.Get("Transport", transport);

    int client_port1 = 0, client_port2 = 0;
    sscanf(transport["client_port"].ToString().c_str(), "%d-%d", &client_port1, &client_port2);

    int server_port1 = 0, server_port2 = 0;
    sscanf(transport["server_port"].ToString().c_str(), "%d-%d", &server_port1, &server_port2);


    bool tcp = false;
    for (int i = 0; i < transport.ArraySize(); i++)
    {
        // RTP/AVP/UDP => udp
        // RTP/AVP/TCP => tcp
        if (std::string::npos != transport[i].ToString().find("TCP"))
        {
            tcp = true;
            break;
        }
    }

    if (NULL != p_tcp) { *p_tcp = tcp; }
    if (NULL != p_client_port1) { *p_client_port1 = client_port1; }
    if (NULL != p_client_port2) { *p_client_port2 = client_port2; }
    if (NULL != p_server_port1) { *p_server_port1 = server_port1; }
    if (NULL != p_server_port2) { *p_server_port2 = server_port2; }

    return true;
}

static int on_replay_tcp_kpa_rtsp_client_inter(kpa_rtsp_client_inter_t* p_inter, sds head, sds body, int code, klb::CMap& param)
{
    KLB_LOG("kpa_rtsp_client replay cmd:[%d],code:[%d],param=%s\n", p_inter->last_cmd, code, param.PrintJson().c_str());

    int seq = atoi(param["CSeq"].ToString().c_str());

    if (seq == p_inter->last_seq)
    {
        if (KPA_RTSP_CLIENT_OPTIONS == p_inter->last_cmd)
        {
            send_describe_kpa_rtsp_client_inter(p_inter);
        }
        else if(KPA_RTSP_CLIENT_DESCRIBE == p_inter->last_cmd)
        {
            // 解析 sdp

            std::string url_video = p_inter->url;
            url_video += "/track1";

            p_inter->video_client_port1 = 58592;
            p_inter->video_client_port2 = 58593;

            send_setup_udp_kpa_rtsp_client_inter(p_inter, url_video.c_str(), true, p_inter->video_client_port1, p_inter->video_client_port2);
        }
        else if(KPA_RTSP_CLIENT_SETUP_VIDEO == p_inter->last_cmd)
        {
            if (sdslen(p_inter->session) <= 0)
            {
                p_inter->session = sdscpy(p_inter->session, param["Session"].ToString().c_str());
            }

            pase_transport_kpa_rtsp_client_inter(p_inter, param, NULL, NULL, NULL, &p_inter->video_server_port1, &p_inter->video_server_port2);

            std::string url_video = p_inter->url;
            url_video += "/track2";

            p_inter->audio_client_port1 = 58594;
            p_inter->audio_client_port2 = 58595;

            send_setup_udp_kpa_rtsp_client_inter(p_inter, url_video.c_str(), false, p_inter->audio_client_port1, p_inter->audio_client_port2);
        }
        else if(KPA_RTSP_CLIENT_SETUP_AUDIO == p_inter->last_cmd)
        {
            if (sdslen(p_inter->session) <= 0)
            {
                p_inter->session = sdscpy(p_inter->session, param["Session"].ToString().c_str());
            }

            pase_transport_kpa_rtsp_client_inter(p_inter, param, NULL, NULL, NULL, &p_inter->audio_server_port1, &p_inter->audio_server_port2);

            //
            p_inter->p_video_rtp = kpa_rtsp_udp_rtp_create(p_inter->p_multi, p_inter->video_client_port1, p_inter->video_server_port1);
            p_inter->p_video_rtcp = kpa_rtsp_udp_rtcp_create(p_inter->p_multi, p_inter->video_client_port2, p_inter->video_server_port2);
            p_inter->p_audio_rtp = kpa_rtsp_udp_rtp_create(p_inter->p_multi, p_inter->audio_client_port1, p_inter->audio_server_port1);
            p_inter->p_audio_rtcp = kpa_rtsp_udp_rtcp_create(p_inter->p_multi, p_inter->audio_client_port2, p_inter->audio_server_port2);

            send_play_udp_kpa_rtsp_client_inter(p_inter);
        }
        else if(KPA_RTSP_CLIENT_PLAY == p_inter->last_cmd)
        {
            send_get_parameter_udp_kpa_rtsp_client_inter(p_inter);
        }
        else if(KPA_RTSP_CLIENT_GET_PARAMETER == p_inter->last_cmd)
        {

        }
    }
    else
    {

    }

    return 0;
}

static int on_recv_tcp_kpa_rtsp_client_inter(void* p_obj, int type, const char* p_head, int head_len, const char* p_body, int body_len)
{
    kpa_rtsp_client_inter_t* p_inter = (kpa_rtsp_client_inter_t*)p_obj;

    sds head = sdsnewlen(p_head, head_len);
    sds body = sdsnewlen(p_body, body_len);

    int code = 0;
    klb::CMap param;

    if (parse_rtsp_kpa_rtsp_client_inter(p_inter, head, body, &code, param))
    {
        // 成功解析
        on_replay_tcp_kpa_rtsp_client_inter(p_inter, head, body, code, param);
    }

    KLB_FREE_BY(head, sdsfree);
    KLB_FREE_BY(body, sdsfree);
    return 0;
}

kpa_rtsp_client_inter_t* kpa_rtsp_client_inter_create(klb_multiplex_t* p_multi, const char* p_host, int port, const char* p_path)
{
    klb_socket_fd fd = klb_socket_connect(p_host, port, 0);
    assert(INVALID_SOCKET != fd);

    kpa_rtsp_client_inter_t* p_inter = KLB_MALLOCZ(kpa_rtsp_client_inter_t, 1, 0);

    p_inter->seq_next = 2;
    p_inter->p_multi = p_multi;
    p_inter->p_tcp = kpa_rtsp_tcp_create(p_multi, fd, on_recv_tcp_kpa_rtsp_client_inter, p_inter);

    p_inter->host = sdsnew(p_host);
    p_inter->port = port;
    p_inter->path = sdsnew(p_path);
    p_inter->url = sdsnew("");
    p_inter->session = sdsnew("");

    // 基础url
    p_inter->url = sdscatfmt(p_inter->url, "rtsp://%s:%i%s", p_host, port, p_path);

    // send 
    send_options_kpa_rtsp_client_inter(p_inter);

    return p_inter;
}

void kpa_rtsp_client_inter_destroy(kpa_rtsp_client_inter_t* p_inter)
{

}
