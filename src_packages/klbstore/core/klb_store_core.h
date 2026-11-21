// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_store_core.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klbstore 包内核心 (对象定义 / 路径布局 / 校验)
/// @version 0.1
/// @history 修改历史
///  \n [2026-10] 创建文件 (桩)
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_STORE_CORE_H__
#define __KLB_STORE_CORE_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


#define KLB_STORE_PATH_MAX       512     ///< 路径缓冲上限
#define KLB_STORE_ID_MAX         256     ///< file_id 长度上限


/// @brief 存储对象 (包内可见)
struct klb_store_t_
{
    char        root[KLB_STORE_PATH_MAX];    ///< 数据根目录 (无尾斜杠)
};


/// @brief 校验 file_id 形态 group/M00/xx/yy/<name>[.<ext>]
/// @param [in] *p_id       文件标识
/// @return bool true.合法; false.非法
bool klb_store_core_valid_id(const char* p_id);

/// @brief 由 file_id 取相对落盘路径 (group/M00/xx/yy/<name>[.<ext>])
/// @param [in]  *p_id      文件标识
/// @param [out] *p_out     输出缓冲
/// @param [in]  out_max    缓冲大小
/// @return int 0.成功; 非0.错误码
int klb_store_core_rel_path(const char* p_id, char* p_out, int out_max);

/// @brief 由 file_id 取绝对落盘路径 (root/相对路径)
/// @param [in]  *p_store   存储对象
/// @param [in]  *p_id      文件标识
/// @param [out] *p_out     输出缓冲
/// @param [in]  out_max    缓冲大小
/// @return int 0.成功; 非0.错误码
int klb_store_core_abs_path(const klb_store_t* p_store, const char* p_id, char* p_out, int out_max);

/// @brief 递归创建目录
/// @param [in] *p_dir      目录路径
/// @return int 0.成功; 非0.错误码
/// @note 桩: 回 KLB_BASECODE_NOTIMPL
int klb_store_core_mkdir_p(const char* p_dir);


#if defined(__cplusplus)
}
#endif

#endif // __KLB_STORE_CORE_H__

// end
