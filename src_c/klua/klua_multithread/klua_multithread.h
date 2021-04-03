///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2021/04/03
//
/// @file    klua_multithread.h
/// @brief   klua与多线程相关的部分: 注意进程内唯一
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
///  \n https://gitee.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
///  \n 2021/04/03 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_MULTITHREAD_H__
#define __KLUA_MULTITHREAD_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 初始化klua模块
/// @return int 0.成功; 非0.失败
int klua_multithread_init();


/// @brief 退出klua模块
/// @return 无
void klua_multithread_quit();


#ifdef __cplusplus
}
#endif

#endif // __KLUA_MULTITHREAD_H__
//end
