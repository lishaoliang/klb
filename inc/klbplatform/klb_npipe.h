// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_npipe.h
/// @brief   (IPC)命名管道 Named Pipe / FIFO
/// @version 0.1
/// @history 修改历史
///  \n 2026 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_NPIPE_H__
#define __KLB_NPIPE_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @struct klb_npipe_t
/// @brief  命名管道对象
typedef struct klb_npipe_t_ klb_npipe_t;


/// @brief 创建命名管道并打开(服务端/创建方)
/// @param [in] *p_name         名称(非空)
/// @return klb_npipe_t* 对象; 失败返回 NULL
/// @note POSIX: 路径名(建议绝对路径) mkfifo+open(O_RDWR); Win: \\\\.\\pipe\\<name>
KLB_API klb_npipe_t* klb_npipe_create(const char* p_name);


/// @brief 打开已有命名管道(客户端)
/// @param [in] *p_name         名称(非空); 须与创建方一致
/// @return klb_npipe_t* 对象; 失败返回 NULL
KLB_API klb_npipe_t* klb_npipe_open(const char* p_name);


/// @brief 关闭并销毁
/// @param [in] *p_npipe        对象
/// @return 无
/// @note 创建方在 POSIX 上会 unlink FIFO 路径
KLB_API void klb_npipe_destroy(klb_npipe_t* p_npipe);


/// @brief 读数据
/// @param [in]  *p_npipe       对象
/// @param [out] *p_data        缓冲
/// @param [in]  max_len        缓冲最大字节
/// @return int >0 读到字节数; 0 对端关闭; <0 失败/无数据(非阻塞)
KLB_API int klb_npipe_read(klb_npipe_t* p_npipe, void* p_data, int max_len);


/// @brief 写数据
/// @param [in] *p_npipe        对象
/// @param [in] *p_data         数据
/// @param [in] len             字节数
/// @return int >=0 写出字节数; <0 失败
KLB_API int klb_npipe_write(klb_npipe_t* p_npipe, const void* p_data, int len);


/// @brief 设置阻塞模式
/// @param [in] *p_npipe        对象
/// @param [in] block           true=阻塞; false=非阻塞
/// @return int 0 成功; 非0 失败
KLB_API int klb_npipe_set_block(klb_npipe_t* p_npipe, bool block);


/// @brief 是否为创建方
/// @param [in] *p_npipe        对象
/// @return bool true=创建方
KLB_API bool klb_npipe_is_owner(klb_npipe_t* p_npipe);


#ifdef __cplusplus
}
#endif

#endif // __KLB_NPIPE_H__

// end
