// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_onvif_serve.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   ONVIF 服务端监听/会话 (桩)
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_ONVIF_SERVE_H__
#define __KLB_ONVIF_SERVE_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klb_onvif_serve_t_ klb_onvif_serve_t;


/// @brief 创建 ONVIF 服务端
/// @return klb_onvif_serve_t* 成功非 NULL; 失败 NULL
/// @note 桩: 当前恒返回 NULL
klb_onvif_serve_t* klb_onvif_serve_create(void);


/// @brief 销毁 ONVIF 服务端
/// @param [in] *p_serve    服务端; 可为 NULL
void klb_onvif_serve_destroy(klb_onvif_serve_t* p_serve);


/// @brief 开始监听
/// @param [in] *p_serve    服务端
/// @param [in] port        端口
/// @return int 0 成功; 非 0 失败
/// @note 桩: 当前恒返回 -1
int klb_onvif_serve_listen(klb_onvif_serve_t* p_serve, uint16_t port);


#ifdef __cplusplus
}
#endif

#endif // __KLB_ONVIF_SERVE_H__

// end
