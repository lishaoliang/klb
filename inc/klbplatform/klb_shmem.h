// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_shmem.h
/// @brief   (IPC)进程之间共享内存
/// @version 0.2
/// @history 修改历史
///  \n 2021 0.1 创建文件
///  \n 2026 0.2 补齐 open/owner 生命周期; Linux key 与 semlock 对齐为名哈希
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
    KLB_SHMEM_AUTO = 0,         ///< 自动: 已存在则附着, 否则创建(创建方为 owner)
    KLB_SHMEM_CREATE,           ///< 仅创建; 已存在则失败返回 NULL
    KLB_SHMEM_SHARE,            ///< 仅附着已有段; 不存在则失败返回 NULL
}klb_shmem_e;


/// @brief 创建或按 flag 打开进程间共享内存
/// @param [in]  flag           创建标记: klb_shmem_e
/// @param [in]  *p_name        名称(非空); eg."aaa"
/// @param [in]  size           大小: 推荐 4K 整数倍; 须与对端一致
/// @return klb_shmem_t* 对象指针; 失败返回 NULL
/// @note Linux: 名称哈希为 SysV key(无需已有路径文件); Win: 命名 FileMapping
KLB_API klb_shmem_t* klb_shmem_create(int flag, const char* p_name, size_t size);


/// @brief 打开已有进程间共享内存(非创建者)
/// @param [in]  *p_name        名称(非空)
/// @param [in]  size           大小: 须与创建方一致
/// @return klb_shmem_t* 对象指针; 不存在或失败返回 NULL
/// @note 等价于 klb_shmem_create(KLB_SHMEM_SHARE, p_name, size)
KLB_API klb_shmem_t* klb_shmem_open(const char* p_name, size_t size);


/// @brief 销毁本地映射
/// @param [in] *p_shmem        对象
/// @return 无
/// @note 仅 owner 在最后拆除内核对象(Linux IPC_RMID / Win 关句柄后无引用即释放)
KLB_API void klb_shmem_destroy(klb_shmem_t* p_shmem);


/// @brief 是否为创建方(owner)
/// @param [in] *p_shmem        对象
/// @return bool true=创建方
KLB_API bool klb_shmem_is_owner(klb_shmem_t* p_shmem);


/// @brief 获取映射的虚拟地址
/// @param [in] *p_shmem        对象
/// @return void* 虚拟地址
KLB_API void* klb_shmem_address(klb_shmem_t* p_shmem);


/// @brief 获取映射的内存大小
/// @param [in] *p_shmem        对象
/// @return size_t 内存大小
KLB_API size_t klb_shmem_size(klb_shmem_t* p_shmem);


#ifdef __cplusplus
}
#endif

#endif // __KLB_SHMEM_H__

// end
