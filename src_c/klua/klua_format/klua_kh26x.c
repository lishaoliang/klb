// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klbbase/klb_mnp.h"
#include "klbmem/klb_buf.h"
#include "klbmem/klb_fpool.h"
#include "klbutil/klb_h26x.h"
#include "klbmem/klb_mem.h"
#include "klbplatform/klb_time.h"
#include <assert.h>


#define KLUA_KH26X_HANDLE           "KLUA_KH26X_HANDLE*"    ///< Lua meta标示


typedef struct klua_kh26x_t_
{
    int             size;
    int             cur;
    klb_buf_t**     p_frame;

    klb_fpool_t*    p_fpool;
}klua_kh26x_t;


//////////////////////////////////////////////////////////////////////////
static klua_kh26x_t* new_klua_kh26x(lua_State* L)
{
    klua_kh26x_t* p_h26x = (klua_kh26x_t*)lua_newuserdata(L, sizeof(klua_kh26x_t));
    KLB_MEMSET(p_h26x, 0, sizeof(klua_kh26x_t));
    luaL_setmetatable(L, KLUA_KH26X_HANDLE);
    return p_h26x;
}

static klua_kh26x_t* to_klua_kh26x(lua_State* L, int index)
{
    klua_kh26x_t* p_h26x = (klua_kh26x_t*)luaL_checkudata(L, index, KLUA_KH26X_HANDLE);
    luaL_argcheck(L, NULL != p_h26x, index, "'kh26x' expected");
    return p_h26x;
}

////////////////////////////////////////

static void close_klua_kh26x(klua_kh26x_t* p_h26x)
{
    KLB_FREE(p_h26x->p_frame);
    KLB_FREE_BY(p_h26x->p_fpool, klb_fpool_destroy);
}

static int klua_kh26x_size(lua_State* L)
{
    klua_kh26x_t* p_h26x = to_klua_kh26x(L, 1);

    lua_pushinteger(L, p_h26x->size);
    return 1;
}

static int klua_kh26x_read(lua_State* L)
{
    klua_kh26x_t* p_h26x = to_klua_kh26x(L, 1);

    if (0 < p_h26x->size)
    {
        klb_buf_t* p_buf = p_h26x->p_frame[p_h26x->cur];

        p_h26x->cur += 1;
        if (p_h26x->size <= p_h26x->cur)
        {
            p_h26x->cur = 0;
        }

        klb_mnp_media_t* p_media = p_buf->p_buf + sizeof(klb_mnp_t);
        p_media->time = klb_tick_counti64();

        lua_pushlightuserdata(L, p_buf);
    }
    else
    {
        lua_pushnil(NULL);
    }

    return 1;
}

static int klua_kh26x_close(lua_State* L)
{
    klua_kh26x_t* p_h26x = to_klua_kh26x(L, 1);
    close_klua_kh26x(p_h26x);
    return 0;
}

////////////////////////////////////////

static int klua_kh26x_gc(lua_State* L)
{
    klua_kh26x_t* p_h26x = to_klua_kh26x(L, 1);
    close_klua_kh26x(p_h26x);
    return 0;
}

static int klua_kh26x_tostring(lua_State* L)
{
    klua_kh26x_t* p_h26x = to_klua_kh26x(L, 1);

    lua_pushfstring(L, "kh26x:%p", p_h26x);
    return 1;
}

static void klua_kh26x_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = { 
        { "size",           klua_kh26x_size },
        { "read",           klua_kh26x_read },

        { "close",          klua_kh26x_close },

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",        NULL },  /* place holder */
        { "__gc",           klua_kh26x_gc },
        { "__close",        klua_kh26x_gc },
        { "__tostring",     klua_kh26x_tostring },
        { NULL,             NULL }
    };

    luaL_newmetatable(L, KLUA_KH26X_HANDLE);/* metatable for KLUA_KH26X_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);          /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);              /* create method table */
    luaL_setfuncs(L, meth, 0);              /* add file methods to method table */
    lua_setfield(L, -2, "__index");         /* metatable.__index = method table */
    lua_pop(L, 1);                          /* pop metatable */
}

//////////////////////////////////////////////////////////////////////////

static klb_buf_t* read_h26x_file(const char* p_filename)
{
    FILE* pf = fopen(p_filename, "rb");
    if (NULL == pf)
    {
        return NULL;
    }

    klb_buf_t* p_buf = NULL;

    fseek(pf, 0, SEEK_END);
    int file_len = ftell(pf);

    if (0 < file_len)
    {
        fseek(pf, 0, SEEK_SET);

        p_buf = klb_buf_malloc(file_len, false);
        fread(p_buf->p_buf, 1, file_len, pf);
        p_buf->end = file_len;
    }

    fclose(pf);
    return p_buf;
}

static int scan_frame_h26x_file(klb_buf_t* p_file)
{
    if (NULL == p_file)
    {
        return 0;
    }

    int frame_num = 0;

    // 扫描帧数目等
    char* p_h26x = p_file->p_buf + p_file->start;
    int h26x_len = p_file->end - p_file->start;

    while (0 < h26x_len)
    {
        int nal_start = 0, nal_len = 0, nal_h_len = 0;
        int8_t is_end = false;

        if (0 == klb_h26x_scan_nalu(p_h26x, h26x_len, &nal_start, &nal_len, &nal_h_len, &is_end))
        {
            frame_num += 1;

            if (is_end)
            {
                break;
            }

            p_h26x = p_h26x + nal_start + nal_len;
            h26x_len = h26x_len - nal_start - nal_len;
        }
        else
        {
            break;
        }
    }

    return frame_num;
}

static void copy_sps_pps_klua_kh26x(klb_buf_t* p_dst, char* p_sps, int sps_len, char* p_pps, int pps_len)
{
    int data_len = sps_len + pps_len + sizeof(klb_mnp_media_t);
    klb_buf_t* ptr = p_dst;

    klb_mnp_t mnp = { 0 };
    mnp.magic = KLB_MNP_MAGIC;
    mnp.packtype = KLB_MNP_MEDIA;
    mnp.size = data_len + sizeof(klb_mnp_t);
    mnp.opt = KLB_MNP_FULL;

    klb_mnp_media_t media = { 0 };
    media.size = data_len;
    media.dtype = KLB_MNP_DTYPE_H264;
    media.vtype = KLB_MNP_VTYPE_CFG;

    int offset = 0;
    memcpy(ptr->p_buf + offset, &mnp, sizeof(klb_mnp_t));           offset += sizeof(klb_mnp_t);
    memcpy(ptr->p_buf + offset, &media, sizeof(klb_mnp_media_t));   offset += sizeof(klb_mnp_media_t);
    memcpy(ptr->p_buf + offset, p_sps, sps_len);                    offset += sps_len;
    memcpy(ptr->p_buf + offset, p_pps, pps_len);                    offset += pps_len;

    ptr->end = mnp.size;
}

static void copy_frame_klua_kh26x(klb_buf_t* p_dst, char* p_nal, int nal_len, uint8_t nalu_type)
{
    int data_len = nal_len + sizeof(klb_mnp_media_t);
    klb_buf_t* ptr = p_dst;

    klb_mnp_t mnp = { 0 };
    mnp.magic = KLB_MNP_MAGIC;
    mnp.packtype = KLB_MNP_MEDIA;

    klb_mnp_media_t media = { 0 };
    media.size = data_len;
    media.dtype = KLB_MNP_DTYPE_H264;

    switch (nalu_type)
    {
    case KLB_H264_ISLICE:
    case KLB_H264_IDRSLICE:
        media.vtype = KLB_MNP_VTYPE_I;
        break;
    case KLB_H264_PSLICE:
        media.vtype = KLB_MNP_VTYPE_P;
        break;
    case KLB_H264_BSLICE:
        media.vtype = KLB_MNP_VTYPE_B;
        break;
    default:
        break;
    }

    bool first = true;
    while (0 < data_len)
    {
        int cp_len = MIN(p_dst->buf_len - sizeof(klb_mnp_t), data_len);
        mnp.size = cp_len + sizeof(klb_mnp_t);

        if (first)
        {
            if (data_len == cp_len)
            {
                assert(nal_len == cp_len - sizeof(klb_mnp_media_t));

                mnp.opt = KLB_MNP_FULL;
            }
            else
            {
                mnp.opt = KLB_MNP_BEGIN;
            }

            memcpy(ptr->p_buf, &mnp, sizeof(klb_mnp_t));
            memcpy(ptr->p_buf + sizeof(klb_mnp_t), &media, sizeof(klb_mnp_media_t));
            memcpy(ptr->p_buf + sizeof(klb_mnp_t) + sizeof(klb_mnp_media_t), p_nal, cp_len - sizeof(klb_mnp_media_t));

            p_nal = p_nal + cp_len - sizeof(klb_mnp_media_t);

            first = false;
        }
        else
        {
            if (data_len == cp_len)
            {
                mnp.opt = KLB_MNP_END;
            }
            else
            {
                mnp.opt = KLB_MNP_CONTINUE;
            }

            memcpy(ptr->p_buf, &mnp, sizeof(klb_mnp_t));
            memcpy(ptr->p_buf + sizeof(klb_mnp_t), p_nal, cp_len);
            p_nal = p_nal + cp_len;
        }

        ptr->end = mnp.size;

        ptr = ptr->p_next;
        data_len -= cp_len;
    }
}

static void init_klua_kh26x(klua_kh26x_t* p_kh26x, klb_buf_t* p_file, int frame_num)
{
    char* p_h26x = p_file->p_buf + p_file->start;
    int h26x_len = p_file->end - p_file->start;

    p_kh26x->size = frame_num;
    p_kh26x->p_frame = KLB_MALLOCZ(klb_buf_t*, frame_num, 0);

    int block_num = ((h26x_len + 4095) / 4096) * 3 / 2;
    p_kh26x->p_fpool = klb_fpool_create(4096, block_num);

    int idx = 0;
    char* p_sps = NULL, * p_pps = NULL;
    int sps_len = 0, pps_len = 0;

    while (0 < h26x_len)
    {
        int nal_start = 0, nal_len = 0, nal_h_len = 0;
        int8_t is_end = false;
        bool has_frame = false;
        bool has_key_frame = false;

        if (0 == klb_h26x_scan_nalu(p_h26x, h26x_len, &nal_start, &nal_len, &nal_h_len, &is_end))
        {
            char* p_nal = p_h26x + nal_start;
            uint8_t nalu = (uint8_t)(p_h26x[nal_start + nal_h_len]);

            uint8_t type = klb_h264_nalu_type(nalu);

            switch (type)
            {
            case KLB_H264_SPS:
                p_sps = p_nal;
                sps_len = nal_len;
                break;
            case KLB_H264_PPS:
                p_pps = p_nal;
                pps_len = nal_len;
                break;
            case KLB_H264_IDRSLICE:
            case KLB_H264_ISLICE:
                has_frame = true;
                has_key_frame = true;
                break;
            case KLB_H264_PSLICE:
            case KLB_H264_BSLICE:
                has_frame = true;
                break;
            default:
                break;
            }

            if (has_key_frame && 0 < sps_len && 0 < pps_len)
            {
                int data_len = sps_len + pps_len + sizeof(klb_mnp_media_t);
                int total_len = data_len + ((data_len + 4095) / 4096) * sizeof(klb_mnp_t);

                klb_buf_t* p_buf = klb_fpool_malloc(p_kh26x->p_fpool, total_len);
                if (NULL != p_buf)
                {
                    copy_sps_pps_klua_kh26x(p_buf, p_sps, sps_len, p_pps, pps_len);
                    p_kh26x->p_frame[idx] = p_buf;

                    sps_len = 0;
                    pps_len = 0;

                    idx += 1;
                }
            }

            if (has_frame)
            {
                int data_len = nal_len + sizeof(klb_mnp_media_t);                
                int total_len = data_len + ((data_len + 4095) / 4096) * sizeof(klb_mnp_t);

                klb_buf_t* p_buf = klb_fpool_malloc(p_kh26x->p_fpool, total_len);
                if (NULL != p_buf)
                {
                    copy_frame_klua_kh26x(p_buf, p_nal, nal_len, type);
                    p_kh26x->p_frame[idx] = p_buf;

                    idx += 1;
                }
            }

            if (is_end)
            {
                break;
            }

            p_h26x = p_h26x + nal_start + nal_len;
            h26x_len = h26x_len - nal_start - nal_len;
        }
        else
        {
            break;
        }
    }

    p_kh26x->size = idx;
}

static int load_klua_kh26x(lua_State* L)
{
    const char* p_path = luaL_checkstring(L, 1);
    klb_buf_t* p_file =  read_h26x_file(p_path);
    int frame_num = scan_frame_h26x_file(p_file);

    if (frame_num <= 0)
    {
        luaL_error(L, "read h26x file error!path=[%s]!", p_path);
    }

    klua_kh26x_t* p_h26x = new_klua_kh26x(L);
    init_klua_kh26x(p_h26x, p_file, frame_num);

    KLB_FREE(p_file);
    return 1;
}

///< 读取h26x文件, 提供模拟流
int klua_open_kh26x(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "load",           load_klua_kh26x },

        { NULL,             NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    // meta
    klua_kh26x_createmeta(L);

    return 1;
}
