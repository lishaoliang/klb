// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbstore.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb 可裁剪包 klbstore 对外入口 (根仅对外; 实现见 core/ store_klua/)
/// @version 0.3
/// @history 修改历史
///  \n [2026-10] 创建文件 (桩; Lua 契约在 klbcore.klbstore)
///  \n [2026-11] 错码对齐 klb_basecode_e (与 klbcore.klbcode 一致)
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_STORE_H__
#define __KLB_STORE_H__

#include "klb_type.h"
#include "klbbase/klb_basecode.h"
#include "klua/klua.h"

#if defined(__cplusplus)
extern "C" {
#endif



/// @brief 存储对象
typedef struct klb_store_t_ klb_store_t;


/// @brief 创建存储对象
/// @param [in] *p_root     数据根目录
/// @return klb_store_t* 成功非 NULL; 失败 NULL
KLB_API klb_store_t* klb_store_create(const char* p_root);

/// @brief 释放存储对象
/// @param [in] *p_store    存储对象
/// @return 无
KLB_API void klb_store_destroy(klb_store_t* p_store);

/// @brief 写入文件 (整包或分片)
/// @param [in] *p_store    存储对象
/// @param [in] *p_id       文件标识 group/M00/xx/yy/<name>[.<ext>]
/// @param [in] *p_data     数据
/// @param [in] len         数据长度
/// @param [in] offset      落盘偏移
/// @return int 0.成功; 非0.错误码
/// @note 桩: 回 KLB_BASECODE_NOTIMPL
KLB_API int klb_store_write(klb_store_t* p_store, const char* p_id,
    const uint8_t* p_data, int len, int64_t offset);

/// @brief 读取文件片段
/// @param [in] *p_store    存储对象
/// @param [in] *p_id       文件标识
/// @param [in] offset      起始偏移
/// @param [out] *p_out     输出缓冲
/// @param [in] len         期望长度
/// @return int 实际字节数; 非正数.错误码
/// @note 桩: 回 KLB_BASECODE_NOTIMPL
KLB_API int klb_store_read(klb_store_t* p_store, const char* p_id,
    int64_t offset, uint8_t* p_out, int len);

/// @brief 取文件元信息
/// @param [in] *p_store    存储对象
/// @param [in] *p_id       文件标识
/// @param [out] *p_size    文件大小 (可为 NULL)
/// @param [out] *p_mtime   修改时间 (可为 NULL)
/// @return int 0.成功; 非0.错误码
/// @note 桩: 回 KLB_BASECODE_NOTIMPL
KLB_API int klb_store_stat(klb_store_t* p_store, const char* p_id,
    int64_t* p_size, int64_t* p_mtime);

/// @brief 删除文件
/// @param [in] *p_store    存储对象
/// @param [in] *p_id       文件标识
/// @return int 0.成功; 非0.错误码
/// @note 桩: 回 KLB_BASECODE_NOTIMPL
KLB_API int klb_store_remove(klb_store_t* p_store, const char* p_id);

/// @brief 校验文件标识
/// @param [in] *p_id       文件标识
/// @return bool true.合法; false.非法
KLB_API bool klb_store_valid_id(const char* p_id);


/// @brief 扩展库 "kstore" (C 侧路径与校验面; Lua 契约在 klbcore.klbstore)
/// @param [in] *L          Lua 状态
/// @return int 返回 1
KLB_API int klua_open_kstore(lua_State* L);


#if defined(__cplusplus)
}
#endif

#endif // __KLB_STORE_H__

// end
