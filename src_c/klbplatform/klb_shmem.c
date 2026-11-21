// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbplatform/klb_shmem.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_hash.h"
#include <assert.h>
#include <string.h>


#ifdef _WIN32
#include <windows.h>
#else
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif


/// @struct klb_shmem_t
/// @brief  共享内存
typedef struct klb_shmem_t_
{
    int             flag;           ///< 创建标记: klb_shmem_e
    size_t          size;           ///< 大小
    void*           p_address;      ///< 虚拟地址
    bool            is_owner;       ///< 是否创建方

#ifdef _WIN32
    HANDLE          handle;         ///< 句柄
#else
    key_t           key;            ///< key
    int             shmid;          ///< shmid
#endif
}klb_shmem_t;


//////////////////////////////////////////////////////////////////////////
#ifdef _WIN32

static klb_shmem_t* attach_win_klb_shmem(HANDLE handle, int flag, size_t size, bool is_owner)
{
    klb_shmem_t* p_shmem = KLB_MALLOCZ(klb_shmem_t, 1, 0);

    p_shmem->flag = flag;
    p_shmem->size = size;
    p_shmem->is_owner = is_owner;
    p_shmem->handle = handle;
    p_shmem->p_address = MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    assert(NULL != p_shmem->p_address);

    return p_shmem;
}

klb_shmem_t* klb_shmem_create(int flag, const char* p_name, size_t size)
{
    HANDLE handle = NULL;
    bool is_owner = false;

    assert(NULL != p_name);
    assert(0 < size);

    if (KLB_SHMEM_SHARE == flag)
    {
        handle = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, p_name);
        if (NULL == handle)
        {
            return NULL;
        }
    }
    else if (KLB_SHMEM_CREATE == flag)
    {
        handle = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, (DWORD)size, p_name);
        if (NULL == handle)
        {
            assert(ERROR_NOT_ENOUGH_MEMORY != GetLastError());
            return NULL;
        }

        if (ERROR_ALREADY_EXISTS == GetLastError())
        {
            CloseHandle(handle);
            return NULL;
        }

        is_owner = true;
    }
    else
    {
        // KLB_SHMEM_AUTO
        handle = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, p_name);
        if (NULL == handle)
        {
            handle = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, (DWORD)size, p_name);
            if (NULL == handle)
            {
                assert(ERROR_NOT_ENOUGH_MEMORY != GetLastError());
                return NULL;
            }

            if (ERROR_ALREADY_EXISTS != GetLastError())
            {
                is_owner = true;
            }
        }
    }

    return attach_win_klb_shmem(handle, flag, size, is_owner);
}

klb_shmem_t* klb_shmem_open(const char* p_name, size_t size)
{
    return klb_shmem_create(KLB_SHMEM_SHARE, p_name, size);
}

void klb_shmem_destroy(klb_shmem_t* p_shmem)
{
    assert(NULL != p_shmem);

    KLB_FREE_BY(p_shmem->p_address, UnmapViewOfFile);
    KLB_FREE_BY(p_shmem->handle, CloseHandle);
    KLB_FREE(p_shmem);
}

//////////////////////////////////////////////////////////////////////////
#else

static key_t key_from_name_klb_shmem(const char* p_name)
{
    uint32_t h = klb_hash32(p_name, (int)strlen(p_name));

    // SysV key 避免 0(IPC_PRIVATE)
    if (0 == h)
    {
        h = 1;
    }

    return (key_t)h;
}

static klb_shmem_t* attach_posix_klb_shmem(int shmid, key_t key, int flag, size_t size, bool is_owner)
{
    klb_shmem_t* p_shmem = KLB_MALLOCZ(klb_shmem_t, 1, 0);

    p_shmem->flag = flag;
    p_shmem->size = size;
    p_shmem->is_owner = is_owner;
    p_shmem->key = key;
    p_shmem->shmid = shmid;
    p_shmem->p_address = shmat(shmid, NULL, 0);
    assert((void*)-1 != p_shmem->p_address);
    assert(NULL != p_shmem->p_address);

    return p_shmem;
}

klb_shmem_t* klb_shmem_create(int flag, const char* p_name, size_t size)
{
    key_t key = 0;
    int shmid = -1;
    bool is_owner = false;

    assert(NULL != p_name);
    assert(0 < size);

    key = key_from_name_klb_shmem(p_name);

    if (KLB_SHMEM_CREATE == flag)
    {
        shmid = shmget(key, size, IPC_CREAT | IPC_EXCL | 0666);
        if (shmid < 0)
        {
            assert(ENOMEM != errno);
            return NULL;
        }

        is_owner = true;
    }
    else if (KLB_SHMEM_SHARE == flag)
    {
        shmid = shmget(key, size, 0666);
        if (shmid < 0)
        {
            assert(ENOMEM != errno);
            return NULL;
        }
    }
    else
    {
        // KLB_SHMEM_AUTO
        shmid = shmget(key, size, 0666);
        if (shmid < 0)
        {
            shmid = shmget(key, size, IPC_CREAT | IPC_EXCL | 0666);
            if (shmid < 0)
            {
                // 竞态: 他端刚创建
                assert(ENOMEM != errno);
                shmid = shmget(key, size, 0666);
                if (shmid < 0)
                {
                    assert(ENOMEM != errno);
                    return NULL;
                }
            }
            else
            {
                is_owner = true;
            }
        }
    }

    return attach_posix_klb_shmem(shmid, key, flag, size, is_owner);
}

klb_shmem_t* klb_shmem_open(const char* p_name, size_t size)
{
    return klb_shmem_create(KLB_SHMEM_SHARE, p_name, size);
}

void klb_shmem_destroy(klb_shmem_t* p_shmem)
{
    assert(NULL != p_shmem);

    if (NULL != p_shmem->p_address)
    {
        shmdt(p_shmem->p_address);
        p_shmem->p_address = NULL;
    }

    if (p_shmem->is_owner && 0 <= p_shmem->shmid)
    {
        shmctl(p_shmem->shmid, IPC_RMID, NULL);
        p_shmem->shmid = -1;
    }

    KLB_FREE(p_shmem);
}

#endif


bool klb_shmem_is_owner(klb_shmem_t* p_shmem)
{
    assert(NULL != p_shmem);
    return p_shmem->is_owner;
}

void* klb_shmem_address(klb_shmem_t* p_shmem)
{
    assert(NULL != p_shmem);
    return p_shmem->p_address;
}

size_t klb_shmem_size(klb_shmem_t* p_shmem)
{
    assert(NULL != p_shmem);
    return p_shmem->size;
}

// end
