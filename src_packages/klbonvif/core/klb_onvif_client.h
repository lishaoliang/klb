// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_onvif_client.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   ONVIF 客户端连接 (桩)
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_ONVIF_CLIENT_H__
#define __KLB_ONVIF_CLIENT_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klb_onvif_client_t_ klb_onvif_client_t;


/// @brief 创建 ONVIF 客户端
/// @return klb_onvif_client_t* 成功非 NULL; 失败 NULL
/// @note 桩: 当前恒返回 NULL
klb_onvif_client_t* klb_onvif_client_create(void);


/// @brief 销毁 ONVIF 客户端
/// @param [in] *p_client   客户端; 可为 NULL
void klb_onvif_client_destroy(klb_onvif_client_t* p_client);


/// @brief 发起 SOAP 调用
/// @param [in] *p_client   客户端
/// @param [in] *p_action   SOAP Action / 操作名
/// @param [in] *p_req      请求体 (XML); 可为 NULL
/// @param [out] *pp_rsp    响应体 (XML); 成功时由调用方释放; 可为 NULL
/// @return int 0 成功; 非 0 失败
/// @note 桩: 当前恒返回 -1
int klb_onvif_client_call(klb_onvif_client_t* p_client, const char* p_action,
                          const char* p_req, char** pp_rsp);


#ifdef __cplusplus
}
#endif

#endif // __KLB_ONVIF_CLIENT_H__

// end
