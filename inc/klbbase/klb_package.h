///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022,GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_package.h
/// @brief   资源文件打包等: eg. 升级包,资源文件等打包
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
///  \n https://gitee.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_PACKAGE_H__
#define __KLB_PACKAGE_H__

#include "klb_type.h"
#include "klbmem/klb_buf.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @struct klb_package_w_t
/// @brief  写数据包
typedef struct klb_package_w_t_ klb_package_w_t;


/// @brief 打开待写文件
/// @param [in]  *p_path        文件路径; eg. "/tmp/aaa.kpa"
/// @return klb_package_w_t* 模块指针
KLB_API klb_package_w_t* klb_package_w_open(const char* p_path);


/// @brief 关闭文件
/// @param [in]  *p_kpa         模块
/// @return 无
KLB_API void klb_package_w_close(klb_package_w_t* p_kpa);


/// @brief 写入打包文件: 数据源为缓存
/// @param [in]  *p_kpa        模块
/// @return int 0.成功; 非0.错误
KLB_API int klb_package_w_write(klb_package_w_t* p_kpa, const char* p_key, const char* p_data, int data_len);



/// @brief 写入文件: 数据源为文件
/// @param [in]  *p_kpa        模块
/// @return int 0.成功; 非0.错误
KLB_API int klb_package_w_write_file(klb_package_w_t* p_kpa, const char* p_key, const char* p_path);


/// @struct klb_package_r_t
/// @brief  读数据包
typedef struct klb_package_r_t_ klb_package_r_t;


/// @brief 打开待读文件
/// @param [in]  *p_path        文件路径; eg. "/tmp/aaa.kpa"
/// @return klb_package_r_t* 模块指针
KLB_API klb_package_r_t* klb_package_r_open(const char* p_path);


/// @brief 关闭文件
/// @param [in]  *p_kpa         模块
/// @return 无
KLB_API void klb_package_r_close(klb_package_r_t* p_kpa);


/// @brief 文件数目
/// @param [in]  *p_kpa         模块
/// @return int64_t
KLB_API int64_t klb_package_r_size(klb_package_r_t* p_kpa);


/// @brief 文件数目
/// @param [in]  *p_kpa         模块
/// @return int64_t
KLB_API int klb_package_r_read(klb_package_r_t* p_kpa, int64_t idx, char** p_key, klb_buf_t** p_value);


#ifdef __cplusplus
}
#endif

#endif // __KLB_PACKAGE_H__
//end
