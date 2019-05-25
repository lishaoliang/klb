///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, MIT License
//  Created: 2019/05/25
//
/// @file    klb_thread.h
/// @brief   线程
///  \n 参考: https://github.com/gozfree/gear-lib/blob/master/libthread/libthread.h
/// @author  李绍良
///  \n 依赖库: -lpthread
/// @version 0.1
/// @history 修改历史
///  \n 2019/05/25 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_THREAD_H__
#define __KLB_THREAD_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @struct klb_thread_t
/// @brief  线程
typedef struct klb_thread_t_ klb_thread_t;


/// @brief 创建线程后的启动入口回调函数
/// @param [in] *p_obj      传入的对象
/// @param [in] *p_run      是否在运行
/// @return int 返回值
typedef int(*klb_thread_cb)(void* p_obj, int* p_run);


/// @brief 创建并启动线程
/// @param [in] cb_thread   新线程启动入口
/// @param [in] *p_obj      传递给新线程的对象参数
/// @param [in] cpu_idx     让线程运行的CPU序号; -1.自动, 0.第一个CPU, 1.第二个CPU, 以此类推
/// @param [in] *p_name     线程名称(部分平台支持)
/// @return klb_thread_t*   线程对象
klb_thread_t* klb_thread_create(klb_thread_cb cb_thread, void* p_obj, int cpu_idx, const char* p_name);


/// @brief 销毁线程
/// @param [in] *p_thread   线程对象
/// @return 无
void klb_thread_destroy(klb_thread_t* p_thread);


#ifdef __cplusplus
}
#endif

#endif // __KLB_THREAD_H__
//end
