#include "klbplatform/klb_shmem.h"
#include "klbmem/klb_mem.h"
#include <assert.h>


#ifdef _WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif


/// @struct klb_shmem_t
/// @brief  共享内存
typedef struct klb_shmem_t_
{
    int             flag;           ///< 创建标记: (klb_shmem_e) KLB_SHMEM_AUTO
    size_t          size;           ///< 大小
    void*           p_address;      ///< 虚拟地址

#ifdef _WIN32
    HANDLE          handle;         ///< 句柄
#else
    key_t           key;            ///< key
    int             fd;             ///< fd
#endif
}klb_shmem_t;


#ifdef _WIN32

/// @brief 创建进程之间共享内存
/// @param [in]  flag           创建标记: klb_shmem_e
/// @param [in]  *p_name        名称: eg."aaa"
/// @param [in]  size           大小: 推荐4K整数倍
/// @return klb_shmem_t* 对象指针
klb_shmem_t* klb_shmem_create(int flag, const char* p_name, size_t size)
{
    assert(NULL != p_name);
    klb_shmem_t* p_shmem = KLB_MALLOC(klb_shmem_t, 1, 0);
    KLB_MEMSET(p_shmem, 0, sizeof(klb_shmem_t));

    p_shmem->flag = flag;
    p_shmem->size = size;

    //获取
    HANDLE handle = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, p_name);
    if (NULL == handle)
    {
        // 未获取成功; 则创建
        handle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, p_shmem->size, p_name);
    }

    assert(NULL != handle);

    p_shmem->handle = handle;
    p_shmem->p_address = MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, 0); // 映射虚拟地址

    assert(NULL != p_shmem->p_address);
    return p_shmem;
}


/// @brief 销毁
/// @return 无
void klb_shmem_destroy(klb_shmem_t* p_shmem)
{
    assert(NULL != p_shmem);

    KLB_FREE_BY(p_shmem->p_address, UnmapViewOfFile);
    KLB_FREE_BY(p_shmem->handle, CloseHandle);
    KLB_FREE(p_shmem);
}

#else

/// @brief 创建进程之间共享内存
/// @param [in]  flag           创建标记: klb_shmem_e
/// @param [in]  *p_name        名称: eg."aaa"
/// @param [in]  size           大小: 推荐4K整数倍
/// @return klb_shmem_t* 对象指针
klb_shmem_t* klb_shmem_create(int flag, const char* p_name, size_t size)
{
    assert(NULL != p_name);
    klb_shmem_t* p_shmem = KLB_MALLOC(klb_shmem_t, 1, 0);
    KLB_MEMSET(p_shmem, 0, sizeof(klb_shmem_t));

    p_shmem->flag = flag;
    p_shmem->size = size;
    p_shmem->fd = -1;

    key_t key = ftok(p_name, 66);

    // 获取
    int fd = shmget(p_shmem->key, p_shmem->size, IPC_CREAT | 0666);
    if (fd <= 0)
    {
        // 未获取成功; 则创建
        fd = shmget(key, p_shmem->size, IPC_CREAT | IPC_EXCL | 0666);
    }

    assert(0 < fd);

    p_shmem->key = key;
    p_shmem->fd = fd;
    p_shmem->p_address = shmat(key, NULL, 0); // 映射虚拟地址

    assert(NULL != p_shmem->p_address);
    return p_shmem;
}


/// @brief 销毁
/// @return 无
void klb_shmem_destroy(klb_shmem_t* p_shmem)
{
    assert(NULL != p_shmem);

    KLB_FREE_BY(p_shmem->p_address, shmdt);

    if (0 < p_shmem->fd)
    {
        shmctl(p_shmem->fd, IPC_RMID, NULL);
        p_shmem->fd = -1;
    }

    KLB_FREE(p_shmem);
}

#endif


/// @brief 获取映射的虚拟地址
/// @return void* 虚拟地址
void* klb_shmem_address(klb_shmem_t* p_shmem)
{
    assert(NULL != p_shmem);
    return p_shmem->p_address;
}


/// @brief 获取映射的内存大小
/// @return size_t 内存大小
size_t klb_shmem_size(klb_shmem_t* p_shmem)
{
    assert(NULL != p_shmem);
    return p_shmem->size;
}
