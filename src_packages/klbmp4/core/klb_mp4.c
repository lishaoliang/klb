// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbmp4/klbmp4.h"
#include "klbmem/klb_mem.h"
#include "klbbase/klb_mnp.h"
#include "klbutil/klb_h26x.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>


#define MP4_MAX_MOOV        (32 * 1024 * 1024)
#define MP4_MAX_SAMPLES     (256 * 1024)
#define MP4_MAX_SAMPLE      (16 * 1024 * 1024)
#define MP4_MAX_PARAM       2048


typedef struct mp4_sample_t_
{
    int64_t     offset;
    int         size;
    int64_t     time_ms;
    bool        sync;
} mp4_sample_t;


typedef struct mp4_wsample_t_
{
    uint8_t*    p_data;
    int         size;
    int64_t     time_ms;
    int64_t     file_off;
    bool        sync;
} mp4_wsample_t;


typedef struct mp4_stsc_t_
{
    uint32_t    first_chunk;
    uint32_t    samples_per_chunk;
} mp4_stsc_t;


typedef struct mp4_wbuf_t_
{
    uint8_t*    p;
    int         len;
    int         cap;
} mp4_wbuf_t;


typedef struct mp4_bs_t_
{
    const uint8_t*  p;
    int             bit_len;
    int             bit;
    bool            err;
} mp4_bs_t;


struct klbmp4_t_
{
    FILE*           pf;
    bool            write;

    int             length_size;
    int64_t         timescale;
    int64_t         duration_ms;
    int             width;
    int             height;
    int             profile;
    int             level;

    uint8_t*        p_sps;
    int             sps_len;
    uint8_t*        p_pps;
    int             pps_len;

    mp4_sample_t*   p_samples;
    int             sample_count;
    int             sample_index;

    mp4_wsample_t*  p_wsamples;
    int             wsample_count;
    int             wsample_cap;

    int             err;
};


static const uint8_t s_mp4_sc[4] = { 0x00, 0x00, 0x00, 0x01 };


//////////////////////////////////////////////////////////////////////////
// 文件定位

static int seek_klb_mp4(FILE* pf, int64_t off)
{
#if defined(_WIN32)
    if (0 != _fseeki64(pf, (__int64)off, SEEK_SET))
    {
        return -1;
    }
#else
    if (0 != fseeko(pf, (off_t)off, SEEK_SET))
    {
        return -1;
    }
#endif

    return 0;
}

static int64_t size_klb_mp4(FILE* pf)
{
#if defined(_WIN32)
    if (0 != _fseeki64(pf, 0, SEEK_END))
    {
        return -1;
    }

    return (int64_t)_ftelli64(pf);
#else
    if (0 != fseeko(pf, 0, SEEK_END))
    {
        return -1;
    }

    return (int64_t)ftello(pf);
#endif
}

static bool read_at_klb_mp4(FILE* pf, int64_t off, void* p, int n)
{
    if (n <= 0)
    {
        return false;
    }

    if (0 != seek_klb_mp4(pf, off))
    {
        return false;
    }

    if (1 != fread(p, (size_t)n, 1, pf))
    {
        return false;
    }

    return true;
}

static bool tag_is_klb_mp4(const char* p_tag, const char* p_name)
{
    if (0 == memcmp(p_tag, p_name, 4))
    {
        return true;
    }

    return false;
}


//////////////////////////////////////////////////////////////////////////
// box

static int next_box_klb_mp4(const uint8_t* p, int size, int pos, int* p_body, int* p_body_len, char* p_type)
{
    int hdr = 8;
    int64_t box = 0;

    if (pos < 0 || size < pos + 8)
    {
        return -1;
    }

    box = KLB_RB32(p + pos);
    memcpy(p_type, p + pos + 4, 4);

    if (1 == box)
    {
        if (size < pos + 16)
        {
            return -1;
        }

        box = (int64_t)KLB_RB64(p + pos + 8);
        hdr = 16;
    }
    else if (0 == box)
    {
        box = (int64_t)size - pos;
    }

    if (box < hdr || box > (int64_t)size - pos)
    {
        return -1;
    }

    *p_body = pos + hdr;
    *p_body_len = (int)box - hdr;

    return pos + (int)box;
}

static bool find_child_klb_mp4(const uint8_t* p, int size, const char* p_name, const uint8_t** p_body, int* p_body_len)
{
    int pos = 0;

    while (pos + 8 <= size)
    {
        int body = 0;
        int body_len = 0;
        char type[4] = { 0 };
        int next = next_box_klb_mp4(p, size, pos, &body, &body_len, type);

        if (next < 0)
        {
            return false;
        }

        if (tag_is_klb_mp4(type, p_name))
        {
            *p_body = p + body;
            *p_body_len = body_len;
            return true;
        }

        pos = next;
    }

    return false;
}


//////////////////////////////////////////////////////////////////////////
// 样本表

static void clear_track_klb_mp4(klbmp4_t* p_mp4)
{
    KLB_FREE(p_mp4->p_sps);
    KLB_FREE(p_mp4->p_pps);
    KLB_FREE(p_mp4->p_samples);
    p_mp4->sps_len = 0;
    p_mp4->pps_len = 0;
    p_mp4->sample_count = 0;
    p_mp4->sample_index = 0;
    p_mp4->length_size = 4;
    p_mp4->timescale = 1000;
    p_mp4->duration_ms = 0;
    p_mp4->width = 0;
    p_mp4->height = 0;
    p_mp4->profile = 0;
    p_mp4->level = 0;
}

static bool take_param_klb_mp4(uint8_t** p_dst, int* p_dst_len, const uint8_t* p, int len)
{
    if (len <= 0 || len > MP4_MAX_PARAM)
    {
        return false;
    }

    if (NULL != *p_dst)
    {
        return true;
    }

    *p_dst = KLB_MALLOC(uint8_t, len, 0);
    memcpy(*p_dst, p, (size_t)len);
    *p_dst_len = len;

    return true;
}

static bool parse_avcc_klb_mp4(klbmp4_t* p_mp4, const uint8_t* p, int size)
{
    int pos = 0;
    int length_size = 0;
    int sps_count = 0;
    int pps_count = 0;

    if (size < 7)
    {
        return false;
    }

    length_size = (p[4] & 0x03) + 1;
    if (3 != length_size && 4 != length_size)
    {
        return false;
    }

    p_mp4->length_size = length_size;
    p_mp4->profile = p[1];
    p_mp4->level = p[3];
    sps_count = p[5] & 0x1F;
    pos = 6;

    for (int i = 0; i < sps_count; ++i)
    {
        int nal_len = 0;

        if (pos + 2 > size)
        {
            return false;
        }

        nal_len = KLB_RB16(p + pos);
        pos += 2;

        if (nal_len <= 0 || pos + nal_len > size)
        {
            return false;
        }

        if (!take_param_klb_mp4(&p_mp4->p_sps, &p_mp4->sps_len, p + pos, nal_len))
        {
            return false;
        }

        pos += nal_len;
    }

    if (pos + 1 > size)
    {
        return false;
    }

    pps_count = p[pos];
    pos += 1;

    for (int i = 0; i < pps_count; ++i)
    {
        int nal_len = 0;

        if (pos + 2 > size)
        {
            return false;
        }

        nal_len = KLB_RB16(p + pos);
        pos += 2;

        if (nal_len <= 0 || pos + nal_len > size)
        {
            return false;
        }

        if (!take_param_klb_mp4(&p_mp4->p_pps, &p_mp4->pps_len, p + pos, nal_len))
        {
            return false;
        }

        pos += nal_len;
    }

    if (NULL == p_mp4->p_sps || NULL == p_mp4->p_pps)
    {
        return false;
    }

    return true;
}

static bool parse_avc1_klb_mp4(klbmp4_t* p_mp4, const uint8_t* p, int size)
{
    int pos = 86;

    if (size < 86)
    {
        return false;
    }

    p_mp4->width = (int)KLB_RB16(p + 32);
    p_mp4->height = (int)KLB_RB16(p + 34);

    while (pos + 8 <= size)
    {
        int body = 0;
        int body_len = 0;
        char type[4] = { 0 };
        int next = next_box_klb_mp4(p, size, pos, &body, &body_len, type);

        if (next < 0)
        {
            return false;
        }

        if (tag_is_klb_mp4(type, "avcC"))
        {
            return parse_avcc_klb_mp4(p_mp4, p + body, body_len);
        }

        pos = next;
    }

    return false;
}

static bool parse_stsd_klb_mp4(klbmp4_t* p_mp4, const uint8_t* p, int size)
{
    int count = 0;
    int pos = 8;

    if (size < 8)
    {
        return false;
    }

    count = (int)KLB_RB32(p + 4);
    if (count <= 0)
    {
        return false;
    }

    while (pos + 8 <= size && count > 0)
    {
        uint32_t entry = KLB_RB32(p + pos);
        char type[4] = { 0 };

        if (entry < 8 || pos + (int)entry > size)
        {
            return false;
        }

        memcpy(type, p + pos + 4, 4);

        if (tag_is_klb_mp4(type, "avc1"))
        {
            return parse_avc1_klb_mp4(p_mp4, p + pos, (int)entry);
        }

        pos += (int)entry;
        count -= 1;
    }

    return false;
}

static bool build_samples_klb_mp4(klbmp4_t* p_mp4, const uint8_t* p_stbl, int stbl_len)
{
    const uint8_t* p_stts = NULL;
    const uint8_t* p_stsc = NULL;
    const uint8_t* p_stsz = NULL;
    const uint8_t* p_stco = NULL;
    const uint8_t* p_stss = NULL;
    int stts_len = 0;
    int stsc_len = 0;
    int stsz_len = 0;
    int stco_len = 0;
    int stss_len = 0;
    bool co64 = false;
    int sample_count = 0;
    uint32_t default_size = 0;
    int* p_sizes = NULL;
    int64_t* p_chunk_off = NULL;
    mp4_stsc_t* p_stsc_tab = NULL;
    int chunk_count = 0;
    int stsc_n = 0;
    int produced = 0;
    int stsc_i = 0;
    bool done = false;
    int64_t dts = 0;
    int timed = 0;

    if (!find_child_klb_mp4(p_stbl, stbl_len, "stts", &p_stts, &stts_len))
    {
        return false;
    }

    if (!find_child_klb_mp4(p_stbl, stbl_len, "stsc", &p_stsc, &stsc_len))
    {
        return false;
    }

    if (!find_child_klb_mp4(p_stbl, stbl_len, "stsz", &p_stsz, &stsz_len))
    {
        return false;
    }

    if (find_child_klb_mp4(p_stbl, stbl_len, "co64", &p_stco, &stco_len))
    {
        co64 = true;
    }
    else if (!find_child_klb_mp4(p_stbl, stbl_len, "stco", &p_stco, &stco_len))
    {
        return false;
    }

    find_child_klb_mp4(p_stbl, stbl_len, "stss", &p_stss, &stss_len);

    if (stsz_len < 12 || stco_len < 8 || stsc_len < 8 || stts_len < 8)
    {
        return false;
    }

    default_size = KLB_RB32(p_stsz + 4);
    sample_count = (int)KLB_RB32(p_stsz + 8);

    if (sample_count <= 0 || sample_count > MP4_MAX_SAMPLES)
    {
        p_mp4->err = (sample_count > MP4_MAX_SAMPLES) ? KLBMP4_ERR_LIMIT : KLBMP4_ERR_TRACK;
        return false;
    }

    if (0 == default_size && stsz_len < 12 + sample_count * 4)
    {
        return false;
    }

    chunk_count = (int)KLB_RB32(p_stco + 4);
    if (chunk_count <= 0 || chunk_count > MP4_MAX_SAMPLES)
    {
        p_mp4->err = (chunk_count > MP4_MAX_SAMPLES) ? KLBMP4_ERR_LIMIT : KLBMP4_ERR_TRACK;
        return false;
    }

    if (co64)
    {
        if (stco_len < 8 + chunk_count * 8)
        {
            return false;
        }
    }
    else if (stco_len < 8 + chunk_count * 4)
    {
        return false;
    }

    stsc_n = (int)KLB_RB32(p_stsc + 4);
    if (stsc_n <= 0 || stsc_n > MP4_MAX_SAMPLES || stsc_len < 8 + stsc_n * 12)
    {
        p_mp4->err = (stsc_n > MP4_MAX_SAMPLES) ? KLBMP4_ERR_LIMIT : KLBMP4_ERR_TRACK;
        return false;
    }

    p_sizes = KLB_MALLOC(int, sample_count, 0);
    p_chunk_off = KLB_MALLOC(int64_t, chunk_count, 0);
    p_stsc_tab = KLB_MALLOC(mp4_stsc_t, stsc_n, 0);
    p_mp4->p_samples = KLB_MALLOCZ(mp4_sample_t, sample_count, 0);
    p_mp4->sample_count = sample_count;

    for (int i = 0; i < sample_count; ++i)
    {
        int sz = 0;

        if (0 != default_size)
        {
            sz = (int)default_size;
        }
        else
        {
            sz = (int)KLB_RB32(p_stsz + 12 + i * 4);
        }

        if (sz <= 0 || sz > MP4_MAX_SAMPLE)
        {
            p_mp4->err = (sz > MP4_MAX_SAMPLE) ? KLBMP4_ERR_LIMIT : KLBMP4_ERR_TRACK;
            goto err;
        }

        p_sizes[i] = sz;
    }

    for (int i = 0; i < chunk_count; ++i)
    {
        if (co64)
        {
            p_chunk_off[i] = (int64_t)KLB_RB64(p_stco + 8 + i * 8);
        }
        else
        {
            p_chunk_off[i] = KLB_RB32(p_stco + 8 + i * 4);
        }
    }

    for (int i = 0; i < stsc_n; ++i)
    {
        p_stsc_tab[i].first_chunk = KLB_RB32(p_stsc + 8 + i * 12);
        p_stsc_tab[i].samples_per_chunk = KLB_RB32(p_stsc + 12 + i * 12);

        if (0 == p_stsc_tab[i].first_chunk || 0 == p_stsc_tab[i].samples_per_chunk)
        {
            goto err;
        }
    }

    // step1. chunk 偏移展开成逐样本偏移
    for (int chunk = 1; chunk <= chunk_count; ++chunk)
    {
        uint32_t per = 0;
        int64_t off = 0;

        while (stsc_i + 1 < stsc_n && (int)p_stsc_tab[stsc_i + 1].first_chunk <= chunk)
        {
            stsc_i += 1;
        }

        per = p_stsc_tab[stsc_i].samples_per_chunk;
        if (per > (uint32_t)(sample_count - produced))
        {
            per = (uint32_t)(sample_count - produced);
        }

        off = p_chunk_off[chunk - 1];

        for (uint32_t s = 0; s < per; ++s)
        {
            if (produced >= sample_count)
            {
                done = true;
                break;
            }

            p_mp4->p_samples[produced].offset = off;
            p_mp4->p_samples[produced].size = p_sizes[produced];
            off += p_sizes[produced];
            produced += 1;
        }

        if (done || produced >= sample_count)
        {
            break;
        }
    }

    if (produced != sample_count)
    {
        goto err;
    }

    // step2. stts 换成毫秒
    if (stts_len >= 8)
    {
        int entry_n = (int)KLB_RB32(p_stts + 4);

        if (entry_n < 0 || stts_len < 8 + entry_n * 8)
        {
            goto err;
        }

        for (int e = 0; e < entry_n; ++e)
        {
            uint32_t count = KLB_RB32(p_stts + 8 + e * 8);
            uint32_t delta = KLB_RB32(p_stts + 12 + e * 8);

            for (uint32_t c = 0; c < count; ++c)
            {
                if (timed >= sample_count)
                {
                    break;
                }

                if (0 == p_mp4->timescale)
                {
                    p_mp4->p_samples[timed].time_ms = dts;
                }
                else
                {
                    p_mp4->p_samples[timed].time_ms = dts * 1000 / p_mp4->timescale;
                }

                dts += delta;
                timed += 1;
            }
        }
    }

    // step3. 无 stss 时全部视为同步样本
    if (NULL == p_stss || stss_len < 8)
    {
        for (int i = 0; i < sample_count; ++i)
        {
            p_mp4->p_samples[i].sync = true;
        }
    }
    else
    {
        int sync_n = (int)KLB_RB32(p_stss + 4);

        if (sync_n < 0 || stss_len < 8 + sync_n * 4)
        {
            goto err;
        }

        for (int i = 0; i < sync_n; ++i)
        {
            uint32_t number = KLB_RB32(p_stss + 8 + i * 4);

            if (number >= 1 && (int)number <= sample_count)
            {
                p_mp4->p_samples[number - 1].sync = true;
            }
        }
    }

    KLB_FREE(p_sizes);
    KLB_FREE(p_chunk_off);
    KLB_FREE(p_stsc_tab);

    return true;

err:
    KLB_FREE(p_sizes);
    KLB_FREE(p_chunk_off);
    KLB_FREE(p_stsc_tab);
    clear_track_klb_mp4(p_mp4);

    if (KLBMP4_OK == p_mp4->err)
    {
        p_mp4->err = KLBMP4_ERR_TRACK;
    }

    return false;
}

static bool parse_mdhd_klb_mp4(klbmp4_t* p_mp4, const uint8_t* p, int size)
{
    uint8_t version = 0;
    int64_t duration = 0;

    if (size < 4)
    {
        return false;
    }

    version = p[0];

    if (0 == version)
    {
        if (size < 20)
        {
            return false;
        }

        p_mp4->timescale = KLB_RB32(p + 12);
        duration = (int64_t)KLB_RB32(p + 16);
    }
    else if (1 == version)
    {
        if (size < 32)
        {
            return false;
        }

        p_mp4->timescale = KLB_RB32(p + 20);
        duration = (int64_t)KLB_RB64(p + 24);
    }
    else
    {
        return false;
    }

    if (0 == p_mp4->timescale)
    {
        p_mp4->timescale = 1000;
    }

    if (duration > 0)
    {
        p_mp4->duration_ms = duration * 1000 / p_mp4->timescale;
    }
    else
    {
        p_mp4->duration_ms = 0;
    }

    return true;
}

static bool parse_trak_klb_mp4(klbmp4_t* p_mp4, const uint8_t* p, int size)
{
    const uint8_t* p_mdia = NULL;
    const uint8_t* p_mdhd = NULL;
    const uint8_t* p_hdlr = NULL;
    const uint8_t* p_minf = NULL;
    const uint8_t* p_stbl = NULL;
    const uint8_t* p_stsd = NULL;
    int mdia_len = 0;
    int mdhd_len = 0;
    int hdlr_len = 0;
    int minf_len = 0;
    int stbl_len = 0;
    int stsd_len = 0;

    clear_track_klb_mp4(p_mp4);

    if (!find_child_klb_mp4(p, size, "mdia", &p_mdia, &mdia_len))
    {
        return false;
    }

    if (!find_child_klb_mp4(p_mdia, mdia_len, "hdlr", &p_hdlr, &hdlr_len))
    {
        return false;
    }

    if (hdlr_len < 12 || !tag_is_klb_mp4((const char*)p_hdlr + 8, "vide"))
    {
        return false;
    }

    if (!find_child_klb_mp4(p_mdia, mdia_len, "mdhd", &p_mdhd, &mdhd_len))
    {
        return false;
    }

    if (!parse_mdhd_klb_mp4(p_mp4, p_mdhd, mdhd_len))
    {
        return false;
    }

    if (!find_child_klb_mp4(p_mdia, mdia_len, "minf", &p_minf, &minf_len))
    {
        return false;
    }

    if (!find_child_klb_mp4(p_minf, minf_len, "stbl", &p_stbl, &stbl_len))
    {
        return false;
    }

    if (!find_child_klb_mp4(p_stbl, stbl_len, "stsd", &p_stsd, &stsd_len))
    {
        return false;
    }

    if (!parse_stsd_klb_mp4(p_mp4, p_stsd, stsd_len))
    {
        clear_track_klb_mp4(p_mp4);
        return false;
    }

    if (!build_samples_klb_mp4(p_mp4, p_stbl, stbl_len))
    {
        return false;
    }

    return true;
}

static bool parse_moov_klb_mp4(klbmp4_t* p_mp4, const uint8_t* p, int size)
{
    int pos = 0;
    bool found = false;

    while (pos + 8 <= size)
    {
        int body = 0;
        int body_len = 0;
        char type[4] = { 0 };
        int next = next_box_klb_mp4(p, size, pos, &body, &body_len, type);

        if (next < 0)
        {
            return false;
        }

        if (tag_is_klb_mp4(type, "trak"))
        {
            if (parse_trak_klb_mp4(p_mp4, p + body, body_len))
            {
                found = true;
                break;
            }
        }

        pos = next;
    }

    return found;
}

static bool load_moov_klb_mp4(klbmp4_t* p_mp4)
{
    int64_t file_size = size_klb_mp4(p_mp4->pf);
    int64_t pos = 0;
    int64_t moov_off = -1;
    int64_t moov_size = 0;
    int moov_hdr = 0;
    int body_len = 0;
    uint8_t* p_body = NULL;
    bool ok = false;

    if (file_size < 8)
    {
        p_mp4->err = KLBMP4_ERR_FORMAT;
        return false;
    }

    while (pos + 8 <= file_size)
    {
        uint8_t hdr[16] = { 0 };
        int hdr_len = 8;
        int64_t box = 0;
        char type[4] = { 0 };

        if (!read_at_klb_mp4(p_mp4->pf, pos, hdr, 8))
        {
            p_mp4->err = KLBMP4_ERR_IO;
            return false;
        }

        box = KLB_RB32(hdr);
        memcpy(type, hdr + 4, 4);

        if (1 == box)
        {
            if (!read_at_klb_mp4(p_mp4->pf, pos + 8, hdr + 8, 8))
            {
                p_mp4->err = KLBMP4_ERR_IO;
                return false;
            }

            box = (int64_t)KLB_RB64(hdr + 8);
            hdr_len = 16;
        }
        else if (0 == box)
        {
            box = file_size - pos;
        }

        if (box < hdr_len || pos > file_size - box)
        {
            p_mp4->err = KLBMP4_ERR_FORMAT;
            return false;
        }

        if (tag_is_klb_mp4(type, "moov"))
        {
            moov_off = pos;
            moov_size = box;
            moov_hdr = hdr_len;
            break;
        }

        pos += box;
    }

    if (moov_off < 0 || moov_size - moov_hdr <= 0 || moov_size - moov_hdr > MP4_MAX_MOOV)
    {
        p_mp4->err = (moov_size - moov_hdr > MP4_MAX_MOOV) ? KLBMP4_ERR_LIMIT : KLBMP4_ERR_MOOV;
        return false;
    }

    body_len = (int)(moov_size - moov_hdr);
    p_body = KLB_MALLOC(uint8_t, body_len, 0);

    if (!read_at_klb_mp4(p_mp4->pf, moov_off + moov_hdr, p_body, body_len))
    {
        KLB_FREE(p_body);
        p_mp4->err = KLBMP4_ERR_IO;
        return false;
    }

    ok = parse_moov_klb_mp4(p_mp4, p_body, body_len);
    KLB_FREE(p_body);

    if (!ok && KLBMP4_OK == p_mp4->err)
    {
        p_mp4->err = KLBMP4_ERR_TRACK;
    }

    return ok;
}


//////////////////////////////////////////////////////////////////////////
// 读样本

static uint32_t nal_len_klb_mp4(const uint8_t* p, int length_size)
{
    if (3 == length_size)
    {
        return ((uint32_t)p[0] << 16) | ((uint32_t)p[1] << 8) | p[2];
    }

    return KLB_RB32(p);
}

static int read_sample_klb_mp4(klbmp4_t* p_mp4, klb_buf_t** p_frame)
{
    mp4_sample_t* p_sample = NULL;
    uint8_t* p_raw = NULL;
    klb_buf_t* p_out = NULL;
    int raw_len = 0;
    int length_size = 0;
    bool key = false;
    int off = 0;
    int cap = 0;

    if (p_mp4->sample_index >= p_mp4->sample_count)
    {
        return 1;
    }

    p_sample = &p_mp4->p_samples[p_mp4->sample_index];
    raw_len = p_sample->size;
    length_size = p_mp4->length_size;
    p_raw = KLB_MALLOC(uint8_t, raw_len, 0);

    if (!read_at_klb_mp4(p_mp4->pf, p_sample->offset, p_raw, raw_len))
    {
        KLB_FREE(p_raw);
        return -1;
    }

    key = p_sample->sync;
    off = 0;

    while (off + length_size <= raw_len)
    {
        uint32_t nal = nal_len_klb_mp4(p_raw + off, length_size);

        off += length_size;

        if (0 == nal || (uint32_t)(raw_len - off) < nal)
        {
            KLB_FREE(p_raw);
            return -1;
        }

        if (KLB_H264_IDRSLICE == klb_h264_nalu_type(p_raw[off]))
        {
            key = true;
        }

        off += (int)nal;
    }

    cap = (int)sizeof(klb_mnp_media_t) + raw_len + raw_len + p_mp4->sps_len + p_mp4->pps_len + 16;
    p_out = klb_buf_malloc(cap, false);
    p_out->end = (int)sizeof(klb_mnp_media_t);

    if (key)
    {
        memcpy(p_out->p_buf + p_out->end, s_mp4_sc, 4);
        p_out->end += 4;
        memcpy(p_out->p_buf + p_out->end, p_mp4->p_sps, (size_t)p_mp4->sps_len);
        p_out->end += p_mp4->sps_len;

        memcpy(p_out->p_buf + p_out->end, s_mp4_sc, 4);
        p_out->end += 4;
        memcpy(p_out->p_buf + p_out->end, p_mp4->p_pps, (size_t)p_mp4->pps_len);
        p_out->end += p_mp4->pps_len;
    }

    off = 0;

    while (off + length_size <= raw_len)
    {
        uint32_t nal = nal_len_klb_mp4(p_raw + off, length_size);

        off += length_size;
        memcpy(p_out->p_buf + p_out->end, s_mp4_sc, 4);
        p_out->end += 4;
        memcpy(p_out->p_buf + p_out->end, p_raw + off, nal);
        p_out->end += (int)nal;
        off += (int)nal;
    }

    {
        klb_mnp_media_t* p_media = (klb_mnp_media_t*)p_out->p_buf;

        p_media->size = (uint32_t)p_out->end;
        p_media->dtype = KLB_MNP_DTYPE_H264;
        p_media->chnn = 0;
        p_media->sidx = 0;
        p_media->time = p_sample->time_ms * 1000;
        p_media->vtype = key ? KLB_MNP_VTYPE_I : KLB_MNP_VTYPE_P;
    }

    KLB_FREE(p_raw);
    p_mp4->sample_index += 1;
    *p_frame = p_out;

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// 写文件

static int bs_bit_klb_mp4(mp4_bs_t* p_bs)
{
    int v = 0;

    if (p_bs->err || p_bs->bit >= p_bs->bit_len)
    {
        p_bs->err = true;
        return 0;
    }

    v = (p_bs->p[p_bs->bit >> 3] >> (7 - (p_bs->bit & 7))) & 1;
    p_bs->bit += 1;

    return v;
}

static uint32_t bs_u_klb_mp4(mp4_bs_t* p_bs, int n)
{
    uint32_t v = 0;

    for (int i = 0; i < n; ++i)
    {
        v = (v << 1) | (uint32_t)bs_bit_klb_mp4(p_bs);
    }

    return v;
}

static uint32_t bs_ue_klb_mp4(mp4_bs_t* p_bs)
{
    int zeros = 0;
    uint32_t v = 0;

    while (zeros < 31 && 0 == bs_bit_klb_mp4(p_bs))
    {
        zeros += 1;
    }

    if (p_bs->err || 0 == zeros)
    {
        return 0;
    }

    v = bs_u_klb_mp4(p_bs, zeros);

    return ((1u << zeros) | v) - 1u;
}

static int bs_se_klb_mp4(mp4_bs_t* p_bs)
{
    uint32_t u = bs_ue_klb_mp4(p_bs);

    if (0 != (u & 1u))
    {
        return (int)((u + 1u) / 2u);
    }

    return -(int)(u / 2u);
}

static void bs_scaling_klb_mp4(mp4_bs_t* p_bs, int size)
{
    int last = 8;
    int next = 8;

    for (int j = 0; j < size; ++j)
    {
        if (0 != next)
        {
            int delta = bs_se_klb_mp4(p_bs);
            next = (last + delta + 256) % 256;
        }

        last = (0 == next) ? last : next;
    }
}

static bool high_profile_klb_mp4(uint8_t profile)
{
    if (100 == profile || 110 == profile || 122 == profile || 244 == profile)
    {
        return true;
    }

    if (44 == profile || 83 == profile || 86 == profile || 118 == profile)
    {
        return true;
    }

    if (128 == profile || 138 == profile || 139 == profile || 134 == profile || 135 == profile)
    {
        return true;
    }

    return false;
}

static bool sps_wh_klb_mp4(const uint8_t* p_sps, int sps_len, int* p_width, int* p_height)
{
    uint8_t* p_rbsp = NULL;
    int rbsp_len = 0;
    mp4_bs_t bs = { 0 };
    uint8_t profile = 0;
    int chroma = 1;
    uint32_t pic_w = 0;
    uint32_t pic_h = 0;
    int frame_mbs_only = 1;
    int width = 0;
    int height = 0;
    bool ok = false;

    if (sps_len < 4)
    {
        return false;
    }

    p_rbsp = KLB_MALLOC(uint8_t, sps_len, 0);
    rbsp_len = 0;

    for (int i = 1; i < sps_len; ++i)
    {
        if (i + 2 < sps_len && 0 == p_sps[i] && 0 == p_sps[i + 1] && 3 == p_sps[i + 2])
        {
            p_rbsp[rbsp_len] = 0;
            rbsp_len += 1;
            p_rbsp[rbsp_len] = 0;
            rbsp_len += 1;
            i += 2;
        }
        else
        {
            p_rbsp[rbsp_len] = p_sps[i];
            rbsp_len += 1;
        }
    }

    bs.p = p_rbsp;
    bs.bit_len = rbsp_len * 8;
    profile = (uint8_t)bs_u_klb_mp4(&bs, 8);
    bs_u_klb_mp4(&bs, 8);
    bs_u_klb_mp4(&bs, 8);
    bs_ue_klb_mp4(&bs);

    if (high_profile_klb_mp4(profile))
    {
        chroma = (int)bs_ue_klb_mp4(&bs);

        if (3 == chroma)
        {
            bs_bit_klb_mp4(&bs);
        }

        bs_ue_klb_mp4(&bs);
        bs_ue_klb_mp4(&bs);
        bs_bit_klb_mp4(&bs);

        if (0 != bs_bit_klb_mp4(&bs))
        {
            int nlist = (3 != chroma) ? 8 : 12;

            for (int i = 0; i < nlist; ++i)
            {
                if (0 != bs_bit_klb_mp4(&bs))
                {
                    if (i < 6)
                    {
                        bs_scaling_klb_mp4(&bs, 16);
                    }
                    else
                    {
                        bs_scaling_klb_mp4(&bs, 64);
                    }
                }
            }
        }
    }

    bs_ue_klb_mp4(&bs);

    {
        uint32_t poc = bs_ue_klb_mp4(&bs);

        if (0 == poc)
        {
            bs_ue_klb_mp4(&bs);
        }
        else if (1 == poc)
        {
            uint32_t nref = 0;

            bs_bit_klb_mp4(&bs);
            bs_se_klb_mp4(&bs);
            bs_se_klb_mp4(&bs);
            nref = bs_ue_klb_mp4(&bs);

            for (uint32_t i = 0; i < nref && i < 256; ++i)
            {
                bs_se_klb_mp4(&bs);
            }
        }
    }

    bs_ue_klb_mp4(&bs);
    bs_bit_klb_mp4(&bs);
    pic_w = bs_ue_klb_mp4(&bs);
    pic_h = bs_ue_klb_mp4(&bs);
    frame_mbs_only = bs_bit_klb_mp4(&bs);

    if (0 == frame_mbs_only)
    {
        bs_bit_klb_mp4(&bs);
    }

    bs_bit_klb_mp4(&bs);
    width = (int)(pic_w + 1) * 16;
    height = (2 - frame_mbs_only) * (int)(pic_h + 1) * 16;

    if (0 != bs_bit_klb_mp4(&bs))
    {
        uint32_t crop_l = bs_ue_klb_mp4(&bs);
        uint32_t crop_r = bs_ue_klb_mp4(&bs);
        uint32_t crop_t = bs_ue_klb_mp4(&bs);
        uint32_t crop_b = bs_ue_klb_mp4(&bs);
        int sub_w = 1;
        int sub_h = 1;

        if (1 == chroma)
        {
            sub_w = 2;
            sub_h = 2;
        }
        else if (2 == chroma)
        {
            sub_w = 2;
            sub_h = 1;
        }

        if (0 == chroma)
        {
            width -= (int)(crop_l + crop_r);
            height -= (int)(crop_t + crop_b) * (2 - frame_mbs_only);
        }
        else
        {
            width -= (int)(crop_l + crop_r) * sub_w;
            height -= (int)(crop_t + crop_b) * sub_h * (2 - frame_mbs_only);
        }
    }

    if (!bs.err && width > 0 && height > 0 && width <= 65535 && height <= 65535)
    {
        *p_width = width;
        *p_height = height;
        ok = true;
    }

    KLB_FREE(p_rbsp);

    return ok;
}

static void wbuf_add_klb_mp4(mp4_wbuf_t* p_buf, const void* p, int n)
{
    int cap = 0;
    uint8_t* p_new = NULL;

    if (n <= 0)
    {
        return;
    }

    if (p_buf->len + n > p_buf->cap)
    {
        cap = p_buf->cap * 2;

        if (cap < p_buf->len + n)
        {
            cap = p_buf->len + n;
        }

        if (cap < 256)
        {
            cap = 256;
        }

        p_new = KLB_REALLOC(p_buf->p, uint8_t, cap, 0);
        assert(NULL != p_new);
        p_buf->p = p_new;
        p_buf->cap = cap;
    }

    memcpy(p_buf->p + p_buf->len, p, (size_t)n);
    p_buf->len += n;
}

static void wbuf_u8_klb_mp4(mp4_wbuf_t* p_buf, uint8_t v)
{
    wbuf_add_klb_mp4(p_buf, &v, 1);
}

static void wbuf_u16_klb_mp4(mp4_wbuf_t* p_buf, uint16_t v)
{
    uint8_t raw[2] = { 0 };

    KLB_WB16(raw, v);
    wbuf_add_klb_mp4(p_buf, raw, 2);
}

static void wbuf_u32_klb_mp4(mp4_wbuf_t* p_buf, uint32_t v)
{
    uint8_t raw[4] = { 0 };

    KLB_WB32(raw, v);
    wbuf_add_klb_mp4(p_buf, raw, 4);
}

static void wbuf_zeros_klb_mp4(mp4_wbuf_t* p_buf, int n)
{
    uint8_t z[32] = { 0 };

    while (n > 0)
    {
        int k = n;

        if (k > 32)
        {
            k = 32;
        }

        wbuf_add_klb_mp4(p_buf, z, k);
        n -= k;
    }
}

static void wbuf_matrix_klb_mp4(mp4_wbuf_t* p_buf)
{
    wbuf_u32_klb_mp4(p_buf, 0x00010000);
    wbuf_u32_klb_mp4(p_buf, 0);
    wbuf_u32_klb_mp4(p_buf, 0);
    wbuf_u32_klb_mp4(p_buf, 0);
    wbuf_u32_klb_mp4(p_buf, 0x00010000);
    wbuf_u32_klb_mp4(p_buf, 0);
    wbuf_u32_klb_mp4(p_buf, 0);
    wbuf_u32_klb_mp4(p_buf, 0);
    wbuf_u32_klb_mp4(p_buf, 0x40000000);
}

static int wbuf_box_klb_mp4(mp4_wbuf_t* p_buf, const char* p_type)
{
    int pos = p_buf->len;
    uint8_t hdr[8] = { 0 };

    memcpy(hdr + 4, p_type, 4);
    wbuf_add_klb_mp4(p_buf, hdr, 8);

    return pos;
}

static void wbuf_end_klb_mp4(mp4_wbuf_t* p_buf, int pos)
{
    KLB_WB32(p_buf->p + pos, (uint32_t)(p_buf->len - pos));
}

static bool annex_to_avcc_klb_mp4(klbmp4_t* p_mp4, const uint8_t* p, int len, uint8_t** p_out, int* p_out_len, bool* p_sync)
{
    int pos = 0;
    int need = 0;
    int nals = 0;
    uint8_t* p_avcc = NULL;
    int used = 0;

    while (pos < len)
    {
        int start = 0;
        int nlen = 0;
        int hlen = 0;
        bool end = false;
        int body = 0;
        uint8_t nal_type = 0;
        const uint8_t* p_nal = NULL;

        if (0 != klb_h26x_scan_nalu((char*)p + pos, len - pos, &start, &nlen, &hlen, &end))
        {
            break;
        }

        if (nlen <= hlen)
        {
            break;
        }

        p_nal = p + pos + start + hlen;
        body = nlen - hlen;
        nal_type = klb_h264_nalu_type(p_nal[0]);

        if (KLB_H264_SPS == nal_type)
        {
            if (!take_param_klb_mp4(&p_mp4->p_sps, &p_mp4->sps_len, p_nal, body))
            {
                return false;
            }
        }
        else if (KLB_H264_PPS == nal_type)
        {
            if (!take_param_klb_mp4(&p_mp4->p_pps, &p_mp4->pps_len, p_nal, body))
            {
                return false;
            }
        }
        else
        {
            need += 4 + body;
            nals += 1;

            if (KLB_H264_IDRSLICE == nal_type)
            {
                *p_sync = true;
            }
        }

        if (end)
        {
            break;
        }

        pos += start + nlen;
    }

    if (nals <= 0 || NULL == p_mp4->p_sps || NULL == p_mp4->p_pps)
    {
        return false;
    }

    p_avcc = KLB_MALLOC(uint8_t, need, 0);
    pos = 0;
    used = 0;

    while (pos < len)
    {
        int start = 0;
        int nlen = 0;
        int hlen = 0;
        bool end = false;
        int body = 0;
        uint8_t nal_type = 0;
        const uint8_t* p_nal = NULL;

        if (0 != klb_h26x_scan_nalu((char*)p + pos, len - pos, &start, &nlen, &hlen, &end))
        {
            break;
        }

        if (nlen <= hlen)
        {
            break;
        }

        p_nal = p + pos + start + hlen;
        body = nlen - hlen;
        nal_type = klb_h264_nalu_type(p_nal[0]);

        if (KLB_H264_SPS != nal_type && KLB_H264_PPS != nal_type)
        {
            KLB_WB32(p_avcc + used, (uint32_t)body);
            used += 4;
            memcpy(p_avcc + used, p_nal, (size_t)body);
            used += body;
        }

        if (end)
        {
            break;
        }

        pos += start + nlen;
    }

    *p_out = p_avcc;
    *p_out_len = used;

    return true;
}

static void write_moov_klb_mp4(klbmp4_t* p_mp4, mp4_wbuf_t* p_buf, int64_t duration, int width, int height, bool co64)
{
    int n = p_mp4->wsample_count;
    int sync_n = 0;
    int moov = 0;
    int trak = 0;
    int mdia = 0;
    int minf = 0;
    int stbl = 0;
    int stsd = 0;
    int avc1 = 0;
    int avcc = 0;
    uint8_t profile = 0x42;
    uint8_t compat = 0;
    uint8_t level = 0x1E;

    for (int i = 0; i < n; ++i)
    {
        if (p_mp4->p_wsamples[i].sync)
        {
            sync_n += 1;
        }
    }

    if (p_mp4->sps_len >= 4)
    {
        profile = p_mp4->p_sps[1];
        compat = p_mp4->p_sps[2];
        level = p_mp4->p_sps[3];
    }

    moov = wbuf_box_klb_mp4(p_buf, "moov");

    {
        int mvhd = wbuf_box_klb_mp4(p_buf, "mvhd");

        wbuf_u32_klb_mp4(p_buf, 0);
        wbuf_u32_klb_mp4(p_buf, 0);
        wbuf_u32_klb_mp4(p_buf, 0);
        wbuf_u32_klb_mp4(p_buf, 1000);
        wbuf_u32_klb_mp4(p_buf, (uint32_t)duration);
        wbuf_u32_klb_mp4(p_buf, 0x00010000);
        wbuf_u16_klb_mp4(p_buf, 0x0100);
        wbuf_zeros_klb_mp4(p_buf, 10);
        wbuf_matrix_klb_mp4(p_buf);
        wbuf_zeros_klb_mp4(p_buf, 24);
        wbuf_u32_klb_mp4(p_buf, 2);
        wbuf_end_klb_mp4(p_buf, mvhd);
    }

    trak = wbuf_box_klb_mp4(p_buf, "trak");

    {
        int tkhd = wbuf_box_klb_mp4(p_buf, "tkhd");

        wbuf_u32_klb_mp4(p_buf, 0x00000003);
        wbuf_u32_klb_mp4(p_buf, 0);
        wbuf_u32_klb_mp4(p_buf, 0);
        wbuf_u32_klb_mp4(p_buf, 1);
        wbuf_u32_klb_mp4(p_buf, 0);
        wbuf_u32_klb_mp4(p_buf, (uint32_t)duration);
        wbuf_zeros_klb_mp4(p_buf, 8);
        wbuf_u16_klb_mp4(p_buf, 0);
        wbuf_u16_klb_mp4(p_buf, 0);
        wbuf_u16_klb_mp4(p_buf, 0);
        wbuf_u16_klb_mp4(p_buf, 0);
        wbuf_matrix_klb_mp4(p_buf);
        wbuf_u32_klb_mp4(p_buf, (uint32_t)width << 16);
        wbuf_u32_klb_mp4(p_buf, (uint32_t)height << 16);
        wbuf_end_klb_mp4(p_buf, tkhd);
    }

    mdia = wbuf_box_klb_mp4(p_buf, "mdia");

    {
        int mdhd = wbuf_box_klb_mp4(p_buf, "mdhd");

        wbuf_u32_klb_mp4(p_buf, 0);
        wbuf_u32_klb_mp4(p_buf, 0);
        wbuf_u32_klb_mp4(p_buf, 0);
        wbuf_u32_klb_mp4(p_buf, 1000);
        wbuf_u32_klb_mp4(p_buf, (uint32_t)duration);
        wbuf_u16_klb_mp4(p_buf, 0x55C4);
        wbuf_u16_klb_mp4(p_buf, 0);
        wbuf_end_klb_mp4(p_buf, mdhd);
    }

    {
        int hdlr = wbuf_box_klb_mp4(p_buf, "hdlr");

        wbuf_u32_klb_mp4(p_buf, 0);
        wbuf_u32_klb_mp4(p_buf, 0);
        wbuf_add_klb_mp4(p_buf, "vide", 4);
        wbuf_zeros_klb_mp4(p_buf, 12);
        wbuf_add_klb_mp4(p_buf, "VideoHandler", 13);
        wbuf_end_klb_mp4(p_buf, hdlr);
    }

    minf = wbuf_box_klb_mp4(p_buf, "minf");

    {
        int vmhd = wbuf_box_klb_mp4(p_buf, "vmhd");

        wbuf_u32_klb_mp4(p_buf, 0x00000001);
        wbuf_zeros_klb_mp4(p_buf, 8);
        wbuf_end_klb_mp4(p_buf, vmhd);
    }

    {
        int dinf = wbuf_box_klb_mp4(p_buf, "dinf");
        int dref = wbuf_box_klb_mp4(p_buf, "dref");
        int url = 0;

        wbuf_u32_klb_mp4(p_buf, 0);
        wbuf_u32_klb_mp4(p_buf, 1);
        url = wbuf_box_klb_mp4(p_buf, "url ");
        wbuf_u32_klb_mp4(p_buf, 0x00000001);
        wbuf_end_klb_mp4(p_buf, url);
        wbuf_end_klb_mp4(p_buf, dref);
        wbuf_end_klb_mp4(p_buf, dinf);
    }

    stbl = wbuf_box_klb_mp4(p_buf, "stbl");
    stsd = wbuf_box_klb_mp4(p_buf, "stsd");
    wbuf_u32_klb_mp4(p_buf, 0);
    wbuf_u32_klb_mp4(p_buf, 1);
    avc1 = wbuf_box_klb_mp4(p_buf, "avc1");
    wbuf_zeros_klb_mp4(p_buf, 6);
    wbuf_u16_klb_mp4(p_buf, 1);
    wbuf_zeros_klb_mp4(p_buf, 16);
    wbuf_u16_klb_mp4(p_buf, (uint16_t)width);
    wbuf_u16_klb_mp4(p_buf, (uint16_t)height);
    wbuf_u32_klb_mp4(p_buf, 0x00480000);
    wbuf_u32_klb_mp4(p_buf, 0x00480000);
    wbuf_u32_klb_mp4(p_buf, 0);
    wbuf_u16_klb_mp4(p_buf, 1);
    wbuf_zeros_klb_mp4(p_buf, 32);
    wbuf_u16_klb_mp4(p_buf, 0x0018);
    wbuf_u16_klb_mp4(p_buf, 0xFFFF);

    avcc = wbuf_box_klb_mp4(p_buf, "avcC");
    wbuf_u8_klb_mp4(p_buf, 1);
    wbuf_u8_klb_mp4(p_buf, profile);
    wbuf_u8_klb_mp4(p_buf, compat);
    wbuf_u8_klb_mp4(p_buf, level);
    wbuf_u8_klb_mp4(p_buf, 0xFF);
    wbuf_u8_klb_mp4(p_buf, 0xE1);
    wbuf_u16_klb_mp4(p_buf, (uint16_t)p_mp4->sps_len);
    wbuf_add_klb_mp4(p_buf, p_mp4->p_sps, p_mp4->sps_len);
    wbuf_u8_klb_mp4(p_buf, 1);
    wbuf_u16_klb_mp4(p_buf, (uint16_t)p_mp4->pps_len);
    wbuf_add_klb_mp4(p_buf, p_mp4->p_pps, p_mp4->pps_len);
    wbuf_end_klb_mp4(p_buf, avcc);
    wbuf_end_klb_mp4(p_buf, avc1);
    wbuf_end_klb_mp4(p_buf, stsd);

    {
        int stts = wbuf_box_klb_mp4(p_buf, "stts");
        int64_t prev = 40;

        wbuf_u32_klb_mp4(p_buf, 0);
        wbuf_u32_klb_mp4(p_buf, (uint32_t)n);

        for (int i = 0; i < n; ++i)
        {
            int64_t delta = prev;

            if (i + 1 < n)
            {
                delta = p_mp4->p_wsamples[i + 1].time_ms - p_mp4->p_wsamples[i].time_ms;

                if (delta <= 0)
                {
                    delta = 40;
                }

                prev = delta;
            }

            wbuf_u32_klb_mp4(p_buf, 1);
            wbuf_u32_klb_mp4(p_buf, (uint32_t)delta);
        }

        wbuf_end_klb_mp4(p_buf, stts);
    }

    {
        int stss = wbuf_box_klb_mp4(p_buf, "stss");

        wbuf_u32_klb_mp4(p_buf, 0);
        wbuf_u32_klb_mp4(p_buf, (uint32_t)sync_n);

        for (int i = 0; i < n; ++i)
        {
            if (p_mp4->p_wsamples[i].sync)
            {
                wbuf_u32_klb_mp4(p_buf, (uint32_t)(i + 1));
            }
        }

        wbuf_end_klb_mp4(p_buf, stss);
    }

    {
        int stsc = wbuf_box_klb_mp4(p_buf, "stsc");

        wbuf_u32_klb_mp4(p_buf, 0);
        wbuf_u32_klb_mp4(p_buf, 1);
        wbuf_u32_klb_mp4(p_buf, 1);
        wbuf_u32_klb_mp4(p_buf, 1);
        wbuf_u32_klb_mp4(p_buf, 1);
        wbuf_end_klb_mp4(p_buf, stsc);
    }

    {
        int stsz = wbuf_box_klb_mp4(p_buf, "stsz");

        wbuf_u32_klb_mp4(p_buf, 0);
        wbuf_u32_klb_mp4(p_buf, 0);
        wbuf_u32_klb_mp4(p_buf, (uint32_t)n);

        for (int i = 0; i < n; ++i)
        {
            wbuf_u32_klb_mp4(p_buf, (uint32_t)p_mp4->p_wsamples[i].size);
        }

        wbuf_end_klb_mp4(p_buf, stsz);
    }

    if (co64)
    {
        int box = wbuf_box_klb_mp4(p_buf, "co64");

        wbuf_u32_klb_mp4(p_buf, 0);
        wbuf_u32_klb_mp4(p_buf, (uint32_t)n);

        for (int i = 0; i < n; ++i)
        {
            uint8_t raw[8] = { 0 };

            KLB_WB64(raw, (uint64_t)p_mp4->p_wsamples[i].file_off);
            wbuf_add_klb_mp4(p_buf, raw, 8);
        }

        wbuf_end_klb_mp4(p_buf, box);
    }
    else
    {
        int stco = wbuf_box_klb_mp4(p_buf, "stco");

        wbuf_u32_klb_mp4(p_buf, 0);
        wbuf_u32_klb_mp4(p_buf, (uint32_t)n);

        for (int i = 0; i < n; ++i)
        {
            wbuf_u32_klb_mp4(p_buf, (uint32_t)p_mp4->p_wsamples[i].file_off);
        }

        wbuf_end_klb_mp4(p_buf, stco);
    }

    wbuf_end_klb_mp4(p_buf, stbl);
    wbuf_end_klb_mp4(p_buf, minf);
    wbuf_end_klb_mp4(p_buf, mdia);
    wbuf_end_klb_mp4(p_buf, trak);
    wbuf_end_klb_mp4(p_buf, moov);
}

static bool flush_write_klb_mp4(klbmp4_t* p_mp4)
{
    int n = p_mp4->wsample_count;
    int64_t payload = 0;
    int mdat_hdr = 8;
    int64_t pos = 24;
    bool co64 = false;
    bool any_sync = false;
    int64_t duration = 0;
    int64_t prev = 40;
    int width = 0;
    int height = 0;
    mp4_wbuf_t moov = { 0 };
    uint8_t ftyp[24] = { 0 };
    uint8_t mdat[16] = { 0 };

    if (n <= 0 || NULL == p_mp4->p_sps || NULL == p_mp4->p_pps)
    {
        return false;
    }

    for (int i = 0; i < n; ++i)
    {
        payload += p_mp4->p_wsamples[i].size;

        if (p_mp4->p_wsamples[i].sync)
        {
            any_sync = true;
        }
    }

    if (!any_sync)
    {
        p_mp4->p_wsamples[0].sync = true;
    }

    if (payload + 8 > 0xFFFFFFFFLL)
    {
        mdat_hdr = 16;
    }

    pos += mdat_hdr;

    for (int i = 0; i < n; ++i)
    {
        p_mp4->p_wsamples[i].file_off = pos;

        if (pos > 0xFFFFFFFFLL)
        {
            co64 = true;
        }

        pos += p_mp4->p_wsamples[i].size;
    }

    for (int i = 0; i < n; ++i)
    {
        int64_t delta = prev;

        if (i + 1 < n)
        {
            delta = p_mp4->p_wsamples[i + 1].time_ms - p_mp4->p_wsamples[i].time_ms;

            if (delta <= 0)
            {
                delta = 40;
            }

            prev = delta;
        }

        duration += delta;
    }

    if (!sps_wh_klb_mp4(p_mp4->p_sps, p_mp4->sps_len, &width, &height))
    {
        width = 0;
        height = 0;
    }

    KLB_WB32(ftyp, 24);
    memcpy(ftyp + 4, "ftyp", 4);
    memcpy(ftyp + 8, "isom", 4);
    KLB_WB32(ftyp + 12, 0x200);
    memcpy(ftyp + 16, "isom", 4);
    memcpy(ftyp + 20, "avc1", 4);

    if (8 == mdat_hdr)
    {
        KLB_WB32(mdat, (uint32_t)(payload + 8));
        memcpy(mdat + 4, "mdat", 4);
    }
    else
    {
        KLB_WB32(mdat, 1);
        memcpy(mdat + 4, "mdat", 4);
        KLB_WB64(mdat + 8, (uint64_t)payload + 16);
    }

    if (0 != seek_klb_mp4(p_mp4->pf, 0))
    {
        return false;
    }

    if (1 != fwrite(ftyp, sizeof(ftyp), 1, p_mp4->pf))
    {
        return false;
    }

    if (1 != fwrite(mdat, (size_t)mdat_hdr, 1, p_mp4->pf))
    {
        return false;
    }

    for (int i = 0; i < n; ++i)
    {
        if (1 != fwrite(p_mp4->p_wsamples[i].p_data, (size_t)p_mp4->p_wsamples[i].size, 1, p_mp4->pf))
        {
            return false;
        }
    }

    write_moov_klb_mp4(p_mp4, &moov, duration, width, height, co64);

    if (NULL == moov.p || 1 != fwrite(moov.p, (size_t)moov.len, 1, p_mp4->pf))
    {
        KLB_FREE(moov.p);
        return false;
    }

    KLB_FREE(moov.p);

    return true;
}


//////////////////////////////////////////////////////////////////////////
// 句柄

static void free_klb_mp4(klbmp4_t* p_mp4)
{
    if (NULL == p_mp4)
    {
        return;
    }

    if (NULL != p_mp4->pf)
    {
        fclose(p_mp4->pf);
        p_mp4->pf = NULL;
    }

    KLB_FREE(p_mp4->p_sps);
    KLB_FREE(p_mp4->p_pps);
    KLB_FREE(p_mp4->p_samples);

    if (NULL != p_mp4->p_wsamples)
    {
        for (int i = 0; i < p_mp4->wsample_count; ++i)
        {
            KLB_FREE(p_mp4->p_wsamples[i].p_data);
        }

        KLB_FREE(p_mp4->p_wsamples);
    }

    KLB_FREE(p_mp4);
}

static klbmp4_t* open_file_klb_mp4(const char* p_path, bool write, int* p_err)
{
    klbmp4_t* p_mp4 = NULL;
    FILE* pf = NULL;

    if (NULL == p_path || '\0' == p_path[0])
    {
        if (NULL != p_err)
        {
            *p_err = KLBMP4_ERR_ARG;
        }

        return NULL;
    }

    pf = fopen(p_path, write ? "wb" : "rb");

    if (NULL == pf)
    {
        assert(ENOMEM != errno);

        if (NULL != p_err)
        {
            *p_err = KLBMP4_ERR_OPEN;
        }

        return NULL;
    }

    p_mp4 = KLB_MALLOCZ(klbmp4_t, 1, 0);
    p_mp4->pf = pf;
    p_mp4->write = write;
    p_mp4->length_size = 4;
    p_mp4->timescale = 1000;

    return p_mp4;
}


//////////////////////////////////////////////////////////////////////////

klbmp4_t* klbmp4_open_read_ex(const char* p_path, int* p_err)
{
    klbmp4_t* p_mp4 = NULL;

    if (NULL != p_err)
    {
        *p_err = KLBMP4_OK;
    }

    p_mp4 = open_file_klb_mp4(p_path, false, p_err);

    if (NULL == p_mp4)
    {
        return NULL;
    }

    if (!load_moov_klb_mp4(p_mp4))
    {
        int err = p_mp4->err;

        free_klb_mp4(p_mp4);

        if (NULL != p_err)
        {
            *p_err = (KLBMP4_OK != err) ? err : KLBMP4_ERR_FORMAT;
        }

        return NULL;
    }

    return p_mp4;
}

klbmp4_t* klbmp4_open_read(const char* p_path)
{
    return klbmp4_open_read_ex(p_path, NULL);
}

klbmp4_t* klbmp4_open_write_ex(const char* p_path, int* p_err)
{
    if (NULL != p_err)
    {
        *p_err = KLBMP4_OK;
    }

    return open_file_klb_mp4(p_path, true, p_err);
}

klbmp4_t* klbmp4_open_write(const char* p_path)
{
    return klbmp4_open_write_ex(p_path, NULL);
}

int klbmp4_close(klbmp4_t* p_mp4)
{
    if (NULL == p_mp4)
    {
        return 0;
    }

    int rc = 0;

    if (p_mp4->write && !flush_write_klb_mp4(p_mp4))
    {
        rc = -1;
    }

    free_klb_mp4(p_mp4);

    return rc;
}

int klbmp4_info(klbmp4_t* p_mp4, klbmp4_info_t* p_info)
{
    int width = 0;
    int height = 0;
    int count = 0;
    int64_t duration_ms = 0;

    if (NULL == p_mp4 || NULL == p_info)
    {
        return -1;
    }

    memset(p_info, 0, sizeof(klbmp4_info_t));

    p_info->timescale = p_mp4->timescale;
    p_info->profile = p_mp4->profile;
    p_info->level = p_mp4->level;

    if (p_mp4->write)
    {
        count = p_mp4->wsample_count;

        if (count > 0)
        {
            int64_t last = p_mp4->p_wsamples[count - 1].time_ms;
            int64_t delta = 40;

            if (count >= 2)
            {
                delta = last - p_mp4->p_wsamples[count - 2].time_ms;
            }

            if (delta <= 0)
            {
                delta = 40;
            }

            duration_ms = last + delta;
        }
    }
    else
    {
        count = p_mp4->sample_count;
        duration_ms = p_mp4->duration_ms;
    }

    width = p_mp4->width;
    height = p_mp4->height;

    if ((width <= 0 || height <= 0) && NULL != p_mp4->p_sps)
    {
        if (!sps_wh_klb_mp4(p_mp4->p_sps, p_mp4->sps_len, &width, &height))
        {
            width = 0;
            height = 0;
        }
    }

    if (0 == p_info->profile && NULL != p_mp4->p_sps && p_mp4->sps_len >= 4)
    {
        p_info->profile = p_mp4->p_sps[1];
        p_info->level = p_mp4->p_sps[3];
    }

    p_info->width = width;
    p_info->height = height;
    p_info->sample_count = count;
    p_info->duration_ms = duration_ms;

    if (duration_ms > 0)
    {
        p_info->fps = (double)count * 1000.0 / (double)duration_ms;
    }

    return 0;
}

int klbmp4_seek(klbmp4_t* p_mp4, int64_t time_ms)
{
    int lo = 0;
    int hi = 0;

    if (NULL == p_mp4 || p_mp4->write || NULL == p_mp4->p_samples)
    {
        return -1;
    }

    if (time_ms < 0)
    {
        time_ms = 0;
    }

    hi = p_mp4->sample_count;

    while (lo < hi)
    {
        int mid = lo + (hi - lo) / 2;

        if (p_mp4->p_samples[mid].time_ms < time_ms)
        {
            lo = mid + 1;
        }
        else
        {
            hi = mid;
        }
    }

    p_mp4->sample_index = lo;

    return 0;
}

int klbmp4_read(klbmp4_t* p_mp4, klb_buf_t** p_frame)
{
    int rc = 0;

    if (NULL == p_mp4 || p_mp4->write || NULL == p_frame)
    {
        return -1;
    }

    *p_frame = NULL;

    rc = read_sample_klb_mp4(p_mp4, p_frame);

    if (0 > rc)
    {
        p_mp4->err = KLBMP4_ERR_IO;
    }

    return rc;
}

int klbmp4_write(klbmp4_t* p_mp4, const klb_buf_t* p_frame)
{
    const klb_mnp_media_t* p_media = NULL;
    const uint8_t* p_payload = NULL;
    int payload_len = 0;
    uint8_t* p_avcc = NULL;
    int avcc_len = 0;
    bool sync = false;
    int cap = 0;
    mp4_wsample_t* p_slot = NULL;

    if (NULL == p_mp4 || !p_mp4->write || NULL == p_frame || NULL == p_frame->p_buf)
    {
        return -1;
    }

    if (p_frame->end - p_frame->start < (int)sizeof(klb_mnp_media_t))
    {
        return -1;
    }

    p_media = (const klb_mnp_media_t*)(p_frame->p_buf + p_frame->start);

    if (0 != p_media->dtype && KLB_MNP_DTYPE_H264 != p_media->dtype)
    {
        return -1;
    }

    p_payload = (const uint8_t*)p_frame->p_buf + p_frame->start + sizeof(klb_mnp_media_t);
    payload_len = (p_frame->end - p_frame->start) - (int)sizeof(klb_mnp_media_t);

    if (payload_len <= 0)
    {
        return -1;
    }

    sync = (KLB_MNP_VTYPE_I == p_media->vtype);

    if (!annex_to_avcc_klb_mp4(p_mp4, p_payload, payload_len, &p_avcc, &avcc_len, &sync))
    {
        return -1;
    }

    if (p_mp4->wsample_count >= MP4_MAX_SAMPLES)
    {
        KLB_FREE(p_avcc);
        return -1;
    }

    if (p_mp4->wsample_count >= p_mp4->wsample_cap)
    {
        mp4_wsample_t* p_new = NULL;

        cap = (0 == p_mp4->wsample_cap) ? 16 : p_mp4->wsample_cap * 2;
        p_new = KLB_REALLOC(p_mp4->p_wsamples, mp4_wsample_t, cap, 0);
        assert(NULL != p_new);

        if (cap > p_mp4->wsample_cap)
        {
            memset(p_new + p_mp4->wsample_cap, 0, sizeof(mp4_wsample_t) * (size_t)(cap - p_mp4->wsample_cap));
        }

        p_mp4->p_wsamples = p_new;
        p_mp4->wsample_cap = cap;
    }

    p_slot = &p_mp4->p_wsamples[p_mp4->wsample_count];
    p_slot->p_data = p_avcc;
    p_slot->size = avcc_len;
    p_slot->sync = sync;

    if (p_media->time <= 0)
    {
        p_slot->time_ms = 0;
    }
    else
    {
        p_slot->time_ms = p_media->time / 1000;
    }

    p_mp4->wsample_count += 1;

    return 0;
}

// end
