#include "klbbase/klb_package.h"
#include "klbutil/klb_hash.h"
#include "klbmem/klb_buf.h"
#include "klbutil/klb_list.h"
#include "klbutil/klb_hlist.h"
#include "klbmem/klb_mem.h"

#include "libavutil/base64.h"
#include "libavutil/md5.h"
#include "libavutil/des.h"

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

    int64_t     encrypt_pos;                        ///< 加密数据位置
    int64_t     encrypt_len;                        ///< 加密长度
    uint8_t     encrypt_md5[KLB_PACKAGE_MD5_MAX];   ///< 加密区域md5值

    int64_t     idx_pos;                            ///< 索引区位置
    int64_t     idx_len;                            ///< 索引区长度
    uint8_t     idx_md5[KLB_PACKAGE_MD5_MAX];       ///< 索引区域md5值

    int64_t     data_pos;                           ///< 数据区位置
    int64_t     data_len;                           ///< 数据长度
    uint8_t     data_md5[KLB_PACKAGE_MD5_MAX];      ///< 数据区域md5值

    uint8_t     resv[32];                           ///< 预留区域
    uint32_t    head_hash;                          ///< 头部哈希校验值
}klb_package_head_t;


/// @struct klb_package_idx_t
/// @brief  文件索引
typedef struct klb_package_idx_t_
{
    int64_t     key_pos;
    int64_t     key_len;

    int64_t     value_pos;
    int64_t     value_len;

    uint8_t     value_md5[KLB_PACKAGE_MD5_MAX];     ///< 
}klb_package_idx_t;

#pragma pack()



//////////////////////////////////////////////////////////////////////////
// 写


typedef struct klb_package_w_t_
{
    struct AVMD5*   p_avmd5;
}klb_package_w_t;


klb_package_w_t* klb_package_w_open(const char* p_path)
{
    return NULL;
}

void klb_package_w_close(klb_package_w_t* p_kpa)
{

}

int klb_package_w_write(klb_package_w_t* p_kpa, const char* p_key, const char* p_data, int data_len)
{
    return 0;
}

int klb_package_w_write_file(klb_package_w_t* p_kpa, const char* p_key, const char* p_path)
{
    return 0;
}


//////////////////////////////////////////////////////////////////////////
// 读

typedef struct klb_package_r_t_
{
    int a;
}klb_package_r_t;

klb_package_r_t* klb_package_r_open(const char* p_path)
{
    return NULL;
}

void klb_package_r_close(klb_package_r_t* p_kpa)
{

}

int64_t klb_package_r_size(klb_package_r_t* p_kpa)
{
    return 0;
}

int klb_package_r_read(klb_package_r_t* p_kpa, int64_t idx, char** p_key, klb_buf_t** p_value)
{
    return 0; 
}
