#include "wsdl_video.h"
#include "klbmem/klb_mem.h"


wsdl_video_t* wsdl_video_create()
{
    wsdl_video_t* p_video = KLB_MALLOCZ(wsdl_video_t, 1, 0);



    return p_video;
}

void wsdl_video_destroy(wsdl_video_t* p_video)
{
    KLB_FREE_BY(p_video->p_dec, ffmpeg_dec_destroy);

    KLB_FREE(p_video);
}

static klb_buf_t* wsdl_video_join_buffer(klb_buf_t* p_media)
{
    int size = 0;

    klb_buf_t* p_next = p_media;
    while (NULL != p_next)
    {
        size = size + p_next->end - p_next->start;
        p_next = p_next->p_next;
    }

    klb_buf_t* ptr = klb_buf_malloc(size, false);

    p_next = p_media;
    while (NULL != p_next && p_next->start < p_next->end)
    {
        memcpy(ptr->p_buf + ptr->end, p_next->p_buf + p_next->start + sizeof(klb_mnp_t), p_next->end - p_next->start - sizeof(klb_mnp_t));
        ptr->end = ptr->end + p_next->end - p_next->start - sizeof(klb_mnp_t);

        p_next = p_next->p_next;
    }

    return ptr;
}

int wsdl_video_decode(wsdl_video_t* p_video, int chnn, int sidx, klb_buf_t* p_media, AVFrame** p_out)
{
    int ret = 1;

    if (0 == chnn && 0 == sidx && NULL != p_media)
    {
        // test
        if (NULL == p_video->p_dec)
        {
            p_video->p_dec = ffmpeg_dec_create(AV_CODEC_ID_H264);
        }

        klb_buf_t* ptr = wsdl_video_join_buffer(p_media);

        uint8_t* p_data = (uint8_t*)ptr->p_buf + sizeof(klb_mnp_media_t);
        int data_len = ptr->end - sizeof(klb_mnp_media_t);

        if (NULL != p_data && 0 < data_len)
        {
            AVFrame* p_frame = ffmpeg_dec_decode(p_video->p_dec, p_data, data_len, 0, 0);
            if (NULL != p_frame)
            {
                *p_out = p_frame;
                ret = 0;

                //if (NULL != p_ex->p_wnd)
                //{
                //    wsdl_wnd_video_update(p_ex->p_wnd, 0, p_frame);

                //    p_ex->refresh = true;
                //}
                //ffmpeg_dec_avframe_free(p_frame);
            }
        }

        KLB_FREE(ptr);
    }

    return ret;
}
