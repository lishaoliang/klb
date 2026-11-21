// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_sysmq.h
/// @brief   (IPC)系统级命名消息队列
///          Linux/POSIX: mqueue (mq_open);
///          不支持平台(Win / 无 mqueue / KLB_NO_SYSMQ): create/open 返回 NULL
///          与 klb_msgq(SHM 环缓)并列, 非同一后端; 不做替代后端
/// @version 0.3
/// @history 修改历史
///  \n 2026 0.1 创建文件
/// @warning 没有警告
/// @note    残留: 异常退出未 destroy 时内核对象仍在; 同名 create(O_EXCL) 失败;
///          须换名或人工 mq_unlink; 仅 create 方 destroy 时 unlink.
///          不支持 = create/open 失败即可; 勿为 Win 另做 Mailslot/管道映射.
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_SYSMQ_H__
#define __KLB_SYSMQ_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @struct klb_sysmq_t
/// @brief  系统级命名消息队列
typedef struct klb_sysmq_t_ klb_sysmq_t;


/// @brief 创建系统级命名消息队列(服务侧/创建方)
/// @param [in] *p_name         名称(非空); POSIX 建议以 '/' 开头;
///                             非 '/' 开头则规范为 "/klb.<name>";
///                             除可选首 '/' 外不得再含 '/'
/// @param [in] max_msg         队列最大消息条数; 须 > 0
/// @param [in] max_size        单条消息最大字节; 须 > 0
/// @return klb_sysmq_t* 对象; 失败返回 NULL(含 Win / 无 mqueue / 名非法)
/// @note 底层: POSIX mq_open(O_CREAT|O_EXCL); 仅创建方 unlink
KLB_API klb_sysmq_t* klb_sysmq_create(const char* p_name, uint32_t max_msg, uint32_t max_size);


/// @brief 打开已有系统级命名消息队列(客户端)
/// @param [in] *p_name         名称(非空); 须与创建方一致
/// @param [in] max_msg         保留参数; 打开侧可不校验(传 0 亦可)
/// @param [in] max_size        保留参数; 打开侧可不校验(传 0 亦可)
/// @return klb_sysmq_t* 对象; 失败返回 NULL
KLB_API klb_sysmq_t* klb_sysmq_open(const char* p_name, uint32_t max_msg, uint32_t max_size);


/// @brief 销毁本地句柄
/// @param [in] *p_sysmq        对象
/// @return 无
/// @note 仅创建方拆除内核对象(mq_unlink)
KLB_API void klb_sysmq_destroy(klb_sysmq_t* p_sysmq);


/// @brief 发送一条消息
/// @param [in] *p_sysmq        对象
/// @param [in] *p_data         消息体
/// @param [in] len             字节数; 0 < len <= max_size
/// @param [in] timeout_ms      等待空位超时; 0=不等待; (uint32_t)-1=一直等
/// @return int 0 成功; 非0 失败/超时/队列满
KLB_API int klb_sysmq_send(klb_sysmq_t* p_sysmq, const void* p_data, uint32_t len, uint32_t timeout_ms);


/// @brief 接收一条消息
/// @param [in]  *p_sysmq       对象
/// @param [out] *p_buf         缓冲
/// @param [in]  buf_size       缓冲大小; 须 >= 对端消息长度
/// @param [out] *p_out_len     可选; 实际消息长度
/// @param [in]  timeout_ms     等待消息超时; 0=不等待; (uint32_t)-1=一直等
/// @return int 0 成功; 非0 失败/超时/空
KLB_API int klb_sysmq_recv(klb_sysmq_t* p_sysmq, void* p_buf, uint32_t buf_size, uint32_t* p_out_len, uint32_t timeout_ms);


/// @brief 是否为创建方
/// @param [in] *p_sysmq        对象
/// @return bool true=创建方
KLB_API bool klb_sysmq_is_owner(klb_sysmq_t* p_sysmq);


#if defined(__cplusplus)
}
#endif

#endif // __KLB_SYSMQ_H__

// end
