// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbtar.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb 可裁剪包 klbtar 对外入口 (根目录仅暴露外部接口; 实现见 core/ tar_klua/)
/// @version 0.3
/// @history 修改历史
///  \n [2026] 创建文件 (桩; tar/gzip/tar.gz 解包底座)
///  \n [2026-11] 错码对齐 klb_basecode_e (与 klbcore.klbcode 一致)
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBTAR_H__
#define __KLBTAR_H__

#include "klb_type.h"
#include "klbbase/klb_basecode.h"
#include "klua/klua.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbtar_t_ klbtar_t;


/// @enum klbtar_fmt_e
/// @brief 探测到的归档格式
typedef enum klbtar_fmt_e_
{
    KLBTAR_FMT_UNKNOWN  = 0,    ///< 未知
    KLBTAR_FMT_TAR      = 1,    ///< 裸 .tar
    KLBTAR_FMT_GZIP     = 2,    ///< 单文件 .gz
    KLBTAR_FMT_TAR_GZ   = 3,    ///< .tar.gz / .tgz
}klbtar_fmt_e;


/// @brief 探测归档格式
/// @param [in]  *p_path    文件路径
/// @param [out] *p_fmt     成功时写入 klbtar_fmt_e; 可为 NULL
/// @return int klb_basecode_e; 0 成功
/// @note 桩恒返回 KLB_BASECODE_EOPNOTSUPP
KLB_API int klbtar_probe(const char* p_path, int* p_fmt);


/// @brief 打开归档只读
/// @param [in] *p_path     文件路径
/// @return klbtar_t* 成功非 NULL; 失败 NULL
/// @note 桩恒返回 NULL
KLB_API klbtar_t* klbtar_open(const char* p_path);


/// @brief 打开归档只读 (带错误码)
/// @param [in]  *p_path    文件路径
/// @param [out] *p_err     失败时 klb_basecode_e; 可为 NULL
/// @return klbtar_t* 成功非 NULL; 失败 NULL
KLB_API klbtar_t* klbtar_open_ex(const char* p_path, int* p_err);


/// @brief 关闭归档
/// @param [in] *p_tar      句柄; 可为 NULL
/// @return 无
KLB_API void klbtar_close(klbtar_t* p_tar);


/// @brief 解压归档到目标路径
/// @param [in] *p_path     归档路径
/// @param [in] *p_dest     目标目录 (tar/tgz) 或文件 (单 .gz)
/// @return int 0 成功; 非 0 为 klb_basecode_e
/// @note 桩恒返回 KLB_BASECODE_EOPNOTSUPP
KLB_API int klbtar_extract(const char* p_path, const char* p_dest);


/// @brief 扩展库 "ktar"
/// @param [in] *L          Lua 状态
/// @return int 返回 1
/// @note available / probe / open / extract; 桩阶段 API 可加载但操作失败
KLB_API int klua_open_ktar(lua_State* L);


#if defined(__cplusplus)
}
#endif

#endif // __KLBTAR_H__

// end
