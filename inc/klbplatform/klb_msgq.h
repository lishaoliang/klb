// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_msgq.h
/// @brief   (IPC)进程间消息队列(SHM 环缓 + semlock)
/// @version 0.1
/// @history 修改历史
///  \n 2026 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_MSGQ_H__
#define __KLB_MSGQ_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @struct klb_msgq_t
/// @brief  进程间消息队列
typedef struct klb_msgq_t_ klb_msgq_t;


/// @brief 创建消息队列
/// @param [in] *p_name         名称(非空)
/// @param [in] capacity        数据区字节容量(不含头); 建议 >= 4K
/// @param [in] max_msg         单条消息最大字节; 须 < capacity
/// @return klb_msgq_t* 对象; 失败返回 NULL
/// @note 底层: klb_shmem + klb_semlock; 跨平台语义一致
KLB_API klb_msgq_t* klb_msgq_create(const char* p_name, uint32_t capacity, uint32_t max_msg);


/// @brief 打开已有消息队列
/// @param [in] *p_name         名称(非空)
/// @param [in] capacity        须与创建方一致
/// @param [in] max_msg         须与创建方一致
/// @return klb_msgq_t* 对象; 失败返回 NULL
KLB_API klb_msgq_t* klb_msgq_open(const char* p_name, uint32_t capacity, uint32_t max_msg);


/// @brief 销毁本地句柄
/// @param [in] *p_msgq         对象
/// @return 无
/// @note 仅创建方拆除 SHM/semlock 内核对象
KLB_API void klb_msgq_destroy(klb_msgq_t* p_msgq);


/// @brief 发送一条消息
/// @param [in] *p_msgq         对象
/// @param [in] *p_data         消息体
/// @param [in] len             字节数; 0 < len <= max_msg
/// @param [in] timeout_ms      等待空位超时; 0=不等待; (uint32_t)-1=一直等
/// @return int 0 成功; 非0 失败/超时/队列满
KLB_API int klb_msgq_send(klb_msgq_t* p_msgq, const void* p_data, uint32_t len, uint32_t timeout_ms);


/// @brief 接收一条消息
/// @param [in]  *p_msgq        对象
/// @param [out] *p_buf         缓冲
/// @param [in]  buf_size       缓冲大小; 须 >= 对端消息长度
/// @param [out] *p_out_len     可选; 实际消息长度
/// @param [in]  timeout_ms     等待消息超时; 0=不等待; (uint32_t)-1=一直等
/// @return int 0 成功; 非0 失败/超时/空
KLB_API int klb_msgq_recv(klb_msgq_t* p_msgq, void* p_buf, uint32_t buf_size, uint32_t* p_out_len, uint32_t timeout_ms);


/// @brief 是否为创建方
/// @param [in] *p_msgq         对象
/// @return bool true=创建方
KLB_API bool klb_msgq_is_owner(klb_msgq_t* p_msgq);


#ifdef __cplusplus
}
#endif

#endif // __KLB_MSGQ_H__

// end
