// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbbase/klb_package.h"
#include "klbutil/klb_hash.h"
#include "klbmem/klb_buf.h"
#include "klbutil/klb_hlist.h"
#include "klbutil/klb_rand.h"
#include "klbmem/klb_mem.h"

#include "libavutil/md5.h"
#include "libavutil/mem.h"

#include <stdio.h>


/// @def   KLB_PACKAGE_MD5_MAX
/// @brief MD5值大小
#define KLB_PACKAGE_MD5_MAX         16


#pragma pack(4)

/// @struct klb_package_head_t
/// @brief  文件头
typedef struct klb_package_head_t_
{
#define KLB_PACKAGE_MAGIC_MAX       16
#define KLB_PACKAGE_MAGIC           "klbpackage.kpa"
    uint8_t     magic[KLB_PACKAGE_MAGIC_MAX];       ///< 魔数: KLB_PACKAGE_MAGIC

#define KLB_PACKAGE_ENC_NULL        0
#define KLB_PACKAGE_RC4             KLB_FOURCC('K', 'R', 'C', '4')
    uint32_t    encrypt_type;                       ///< 加密类型

    int64_t     encrypt_pos;                        ///< 加密数据位置(相对于文件头)
    int64_t     encrypt_len;                        ///< 加密长度
    uint8_t     encrypt_md5[KLB_PACKAGE_MD5_MAX];   ///< 加密区域md5值

    int64_t     data_pos;                           ///< 数据区位置(相对于文件头)
    int64_t     data_len;                           ///< 数据长度
    uint8_t     data_md5[KLB_PACKAGE_MD5_MAX];      ///< 数据区域md5值

    int64_t     idx_pos;                            ///< 索引区位置(相对于文件头)
    int64_t     idx_len;                            ///< 索引区长度
    uint8_t     idx_md5[KLB_PACKAGE_MD5_MAX];       ///< 索引区域md5值

#define KLB_PACKAGE_RAND_MAX        8
    uint8_t     rand[KLB_PACKAGE_RAND_MAX];         ///< 随机"盐值"

    uint32_t    head_hash;                          ///< 头部哈希校验值
}klb_package_head_t;


/// @struct klb_package_idx_t
/// @brief  文件索引
typedef struct klb_package_idx_t_
{
    int64_t     key_pos;                            ///< 关键值(或文件名)位置(相对于文件头)
    int64_t     value_pos;                          ///< 数据位置(相对于文件头)

    int64_t     key_len : 16;                       ///< 关键值(或文件名)长度: 读取后需要在末尾补0
    int64_t     value_len : 48;                     ///< 数据长度

    uint8_t     value_md5[KLB_PACKAGE_MD5_MAX];     ///< 数据的md5值: 用于校验是否出现异常
}klb_package_idx_t;

#pragma pack()

//////////////////////////////////////////////////////////////////////////
// 文件结构:
// ----------------------------
//
//  klb_package_head_t(文件头)
//
// ---------------------------
//
//  [encrypt](秘钥信息,可选)
//
// ---------------------------
//
//  data(数据区)
//  [key,value]
//
// ----------------------------
//
//  klb_package_idx_t(索引区)
//
// ---------------------------
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// 写

/// @struct klb_package_w_t
/// @brief  写包
typedef struct klb_package_w_t_
{
    FILE*               pf;             ///< 文件指针
    int64_t             offset;         ///< 文件头偏移

    klb_package_head_t  head;           ///< 文件头参数

    struct AVMD5*       p_avmd5;        ///< md5

    klb_hlist_t*        p_idx_hlist;    ///< 索引: klb_package_idx_t*

    int64_t             data_size;      ///< 当前写入数据大小
}klb_package_w_t;


klb_package_w_t* klb_package_w_open(const char* p_path)
{
    FILE* pf = fopen(p_path, "wb");
    if (NULL == pf)
    {
        return NULL;
    }

    klb_package_w_t* p_kpa = KLB_MALLOCZ(klb_package_w_t, 1, 0);
    p_kpa->pf = pf;

    p_kpa->offset = 0;
    p_kpa->p_avmd5 = av_md5_alloc();
    av_md5_init(p_kpa->p_avmd5);

    p_kpa->p_idx_hlist = klb_hlist_create(0);

    // init head
    strcpy((char*)p_kpa->head.magic, KLB_PACKAGE_MAGIC);

    // encrypt
    p_kpa->head.encrypt_type = 0;
    p_kpa->head.encrypt_pos = sizeof(klb_package_head_t);
    p_kpa->head.encrypt_len = 0;

    // data
    p_kpa->head.data_pos = p_kpa->head.encrypt_pos + p_kpa->head.encrypt_len;

    return p_kpa;
}

void klb_package_w_close(klb_package_w_t* p_kpa)
{
    assert(NULL != p_kpa);
    assert(NULL != p_kpa->pf);

    // 
    p_kpa->head.data_len = p_kpa->data_size;
    av_md5_final(p_kpa->p_avmd5, p_kpa->head.data_md5);

    // 索引
    p_kpa->head.idx_pos = KLB_ALIGNED_4(p_kpa->head.data_pos + p_kpa->head.data_len);
    p_kpa->head.idx_len = (int64_t)sizeof(klb_package_idx_t) * klb_hlist_size(p_kpa->p_idx_hlist);

    av_md5_init(p_kpa->p_avmd5);
    if (0 < p_kpa->head.idx_len)
    {
        fseek(p_kpa->pf, (long)(p_kpa->offset + p_kpa->head.idx_pos), SEEK_SET);

        while (0 < klb_hlist_size(p_kpa->p_idx_hlist))
        {
            klb_package_idx_t* p_idx = (klb_package_idx_t*)klb_hlist_pop_head(p_kpa->p_idx_hlist);
            assert(NULL != p_idx);

            fwrite(p_idx, sizeof(klb_package_idx_t), 1, p_kpa->pf);
            av_md5_update(p_kpa->p_avmd5, (const uint8_t*)p_idx, sizeof(klb_package_idx_t));

            KLB_FREE(p_idx);
        }
    }

    av_md5_final(p_kpa->p_avmd5, p_kpa->head.idx_md5);

    // 文件头
    klb_rand_string((char*)p_kpa->head.rand, KLB_PACKAGE_RAND_MAX, false);
    p_kpa->head.head_hash = klb_hash32((const char*)&(p_kpa->head), sizeof(klb_package_head_t) - sizeof(uint32_t));

    fseek(p_kpa->pf, (long)p_kpa->offset, SEEK_SET);
    fwrite(&p_kpa->head, sizeof(klb_package_head_t), 1, p_kpa->pf);

    //
    KLB_FREE_BY(p_kpa->p_idx_hlist, klb_hlist_destroy);
    KLB_FREE_BY(p_kpa->p_avmd5, av_free);
    KLB_FREE_BY(p_kpa->pf, fclose);
    KLB_FREE(p_kpa);
}

static int klb_package_w_write_buf(klb_package_w_t* p_kpa, const char* p_key, int key_len, const char* p_data, int data_len)
{
    int64_t offset = p_kpa->head.data_pos + p_kpa->data_size;
    int64_t size = key_len + data_len;

    klb_package_idx_t* p_idx = KLB_MALLOCZ(klb_package_idx_t, 1, 0);

    p_idx->key_pos = offset;
    p_idx->key_len = key_len;
    p_idx->value_pos = offset + key_len;
    p_idx->value_len = data_len;
    av_md5_sum(p_idx->value_md5, (const uint8_t*)p_data, data_len);

    fseek(p_kpa->pf, (long)(p_kpa->offset + offset), SEEK_SET);
    if (0 < key_len)
    {
        fwrite(p_key, key_len, 1, p_kpa->pf);

        av_md5_update(p_kpa->p_avmd5, (const uint8_t*)p_key, key_len);
        p_kpa->data_size += key_len;
    }

    if (0 < data_len)
    {
        fwrite(p_data, data_len, 1, p_kpa->pf);

        av_md5_update(p_kpa->p_avmd5, (const uint8_t*)p_data, data_len);
        p_kpa->data_size += data_len;
    }

    klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_kpa->p_idx_hlist, p_key, key_len, p_idx);
    assert(NULL != p_iter);

    return 0;
}

int klb_package_w_write(klb_package_w_t* p_kpa, const char* p_key, int key_len, const char* p_data, int data_len)
{
    assert(NULL != p_kpa);
    assert(NULL != p_kpa->pf);

    if (NULL != klb_hlist_find_iter(p_kpa->p_idx_hlist, p_key, key_len))
    {
        return 1; // 已有key
    }

    return klb_package_w_write_buf(p_kpa, p_key, key_len, p_data, data_len);
}

#define KLB_PACKAGE_BUF_MAX 1024

static int klb_package_w_write_file_dump(klb_package_w_t* p_kpa, const char* p_key, int key_len, int64_t file_len, FILE* pf)
{
    assert(KLB_PACKAGE_BUF_MAX < file_len);

    int64_t offset = p_kpa->head.data_pos + p_kpa->data_size;
    int64_t size = key_len + file_len;

    klb_package_idx_t* p_idx = KLB_MALLOCZ(klb_package_idx_t, 1, 0);

    p_idx->key_pos = offset;
    p_idx->key_len = key_len;
    p_idx->value_pos = offset + key_len;
    p_idx->value_len = file_len;

    fseek(p_kpa->pf, (long)(p_kpa->offset + offset), SEEK_SET);
    if (0 < key_len)
    {
        fwrite(p_key, key_len, 1, p_kpa->pf);

        av_md5_update(p_kpa->p_avmd5, (const uint8_t*)p_key, key_len);
        p_kpa->data_size += key_len;
    }

    if (0 < file_len)
    {
        struct AVMD5* p_avmd5 = av_md5_alloc();
        av_md5_init(p_avmd5);

        uint8_t buf[KLB_PACKAGE_BUF_MAX];
        
        int64_t data_len = file_len;
        fseek(pf, 0, SEEK_SET);

        while (0 < data_len)
        {
            int64_t read_len = MIN(data_len, KLB_PACKAGE_BUF_MAX);
            fread(buf, (size_t)read_len, 1, pf);

            fwrite(buf, (size_t)read_len, 1, p_kpa->pf);

            av_md5_update(p_kpa->p_avmd5, buf, (int)read_len);
            av_md5_update(p_avmd5, buf, (int)read_len);

            data_len -= read_len;
        }

        av_md5_final(p_avmd5, p_idx->value_md5);
        KLB_FREE_BY(p_avmd5, av_free);

        p_kpa->data_size += file_len;
    }

    klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_kpa->p_idx_hlist, p_key, key_len, p_idx);
    assert(NULL != p_iter);

    return 0;
}

int klb_package_w_write_file(klb_package_w_t* p_kpa, const char* p_key, int key_len, const char* p_path)
{
    if (NULL != klb_hlist_find_iter(p_kpa->p_idx_hlist, p_key, key_len))
    {
        return 1; // 已有key
    }

    FILE* pf = fopen(p_path, "rb");
    if (NULL == pf)
    {
        return 2; // 打开文件失败 
    }

    int ret = 0;

    fseek(pf, 0, SEEK_END);
    long filelen = ftell(pf);

    if (filelen <= KLB_PACKAGE_BUF_MAX)
    {
        fseek(pf, 0, SEEK_SET);

        uint8_t buf[KLB_PACKAGE_BUF_MAX];
        fread(buf, (size_t)filelen, 1, pf);

        ret = klb_package_w_write_buf(p_kpa, p_key, key_len, (const char*)buf, (int)filelen);
    }
    else
    {
        ret = klb_package_w_write_file_dump(p_kpa, p_key, key_len, filelen, pf);
    }

    fclose(pf);
    return ret;
}


//////////////////////////////////////////////////////////////////////////
// 读


/// @struct klb_package_r_t
/// @brief  读包
typedef struct klb_package_r_t_
{
    FILE*               pf;             ///< 文件指针
    int64_t             offset;         ///< 文件头偏移

    klb_package_head_t  head;           ///< 文件头参数

    int64_t             idx_count;      ///< 索引个数
    klb_package_idx_t*  p_index;        ///< 索引
}klb_package_r_t;


static bool read_head_klb_package_r(FILE* pf, int64_t offset, klb_package_head_t* p_head)
{
    fseek(pf, 0, SEEK_END);
    long filelen = ftell(pf);

    fseek(pf, (long)offset, SEEK_SET);
    fread(p_head, sizeof(klb_package_head_t), 1, pf);

    return true;
}

static void read_idx_klb_package_r(klb_package_r_t* p_kpa)
{
    if (p_kpa->idx_count <= 0)
    {
        return;
    }

    int64_t idx_len = p_kpa->idx_count * sizeof(klb_package_idx_t);

    fseek(p_kpa->pf, (long)(p_kpa->offset + p_kpa->head.idx_pos), SEEK_SET);
    fread(p_kpa->p_index, (size_t)idx_len, 1, p_kpa->pf);
}

klb_package_r_t* klb_package_r_open(const char* p_path)
{
    FILE* pf = fopen(p_path, "rb");
    if (NULL == pf)
    {
        return NULL;
    }

    klb_package_head_t head = { 0 };
    int64_t offset = 0;

    if (!read_head_klb_package_r(pf, offset, &head))
    {
        fclose(pf);
        return NULL;
    }

    klb_package_r_t* p_kpa = KLB_MALLOCZ(klb_package_r_t, 1, 0);
    p_kpa->pf = pf;

    p_kpa->offset = offset;
    memcpy(&p_kpa->head, &head, sizeof(klb_package_head_t));

    p_kpa->idx_count = head.idx_len / sizeof(klb_package_idx_t);
    p_kpa->p_index = KLB_MALLOC(klb_package_idx_t, (size_t)p_kpa->idx_count, 4);

    read_idx_klb_package_r(p_kpa);

    return p_kpa;
}

void klb_package_r_close(klb_package_r_t* p_kpa)
{
    assert(NULL != p_kpa);
    assert(NULL != p_kpa->pf);

    KLB_FREE(p_kpa->p_index);
    KLB_FREE_BY(p_kpa->pf, fclose);
    KLB_FREE(p_kpa);
}

int64_t klb_package_r_size(klb_package_r_t* p_kpa)
{
    return p_kpa->idx_count;
}

int klb_package_r_read(klb_package_r_t* p_kpa, int64_t idx, char** p_key, klb_buf_t** p_value)
{
    if (idx < 0 || p_kpa->idx_count <= idx)
    {
        return 1;
    }

    klb_package_idx_t* p_info = &p_kpa->p_index[idx];

    if (0 < p_info->key_len)
    {
        char* ptr = KLB_MALLOC(char, (size_t)p_info->key_len, 4);

        fseek(p_kpa->pf, (long)(p_kpa->offset + p_info->key_pos), SEEK_SET);
        fread(ptr, (size_t)p_info->key_len, 1, p_kpa->pf);

        ptr[p_info->key_len] = 0;
        *p_key = ptr;
    }
    else
    {
        *p_key = KLB_MALLOCZ(char, 4, 0);
    }

    if (0 < p_info->value_len)
    {
        klb_buf_t* p_tmp = klb_buf_malloc((int)p_info->value_len, false);

        fseek(p_kpa->pf, (size_t)(p_kpa->offset + p_info->value_pos), SEEK_SET);
        fread(p_tmp->p_buf, (size_t)p_info->value_len, 1, p_kpa->pf);

        p_tmp->end = (int)p_info->value_len;

        *p_value = p_tmp;
    }
    else
    {
        *p_value = NULL;
    }

    return 0; 
}
