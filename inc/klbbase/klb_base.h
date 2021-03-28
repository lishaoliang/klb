///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2021/03/28
//
/// @file    klb_base.h
/// @brief   klb需要初始化的部分
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
///  \n https://gitee.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
///  \n 2021/03/28 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_BASE_H__
#define __KLB_BASE_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief KLB库初始化
/// @param [in]  *p_json_cfg            json配置
/// @return int 0.成功; 非0.失败
KLB_API int klb_base_init(const char* p_json_cfg);


/// @brief KLB库退出
/// @return 无
KLB_API void klb_base_quit();


#ifdef __cplusplus
}
#endif

#endif // __KLB_BASE_H__
//end
