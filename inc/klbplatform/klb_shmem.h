///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2021/04/03
//
/// @file    klb_shmem.h
/// @brief   (IPC)进程之间共享内存
///         shmget,shmat,shmdt,shmctl
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
///  \n https://gitee.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
///  \n 2021/04/03 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_SHMEM_H__
#define __KLB_SHMEM_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif

/// @struct klb_shmem_t
/// @brief  (IPC)进程之间共享内存
typedef struct klb_shmem_t_ klb_shmem_t;


/// @enum  klb_shmem_e
/// @brief 创建标记
typedef enum klb_shmem_e_
{
    KLB_SHMEM_AUTO,             ///< 自动模式
    KLB_SHMEM_CREATE,           ///< 创建者
    KLB_SHMEM_SHARE,            ///< 分享者(非创建者)
}klb_shmem_e;


/// @brief 创建进程之间共享内存
/// @param [in]  flag           创建标记: klb_shmem_e
/// @param [in]  *p_name        名称: eg."aaa"
/// @param [in]  size           大小: 推荐4K整数倍
/// @return klb_shmem_t* 对象指针
klb_shmem_t* klb_shmem_create(int flag, const char* p_name, size_t size);


/// @brief 销毁
/// @return 无
void klb_shmem_destroy(klb_shmem_t* p_shmem);


/// @brief 获取映射的虚拟地址
/// @return void* 虚拟地址
void* klb_shmem_address(klb_shmem_t* p_shmem);


/// @brief 获取映射的内存大小
/// @return size_t 内存大小
size_t klb_shmem_size(klb_shmem_t* p_shmem);


#ifdef __cplusplus
}
#endif

#endif // __KLB_SHMEM_H__
//end
