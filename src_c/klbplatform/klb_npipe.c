// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbplatform/klb_npipe.h"
#include "klbmem/klb_mem.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>


#ifdef _WIN32
#include <windows.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif


#define KLB_NPIPE_PATH_MAX  512


/// @struct klb_npipe_t
typedef struct klb_npipe_t_
{
    bool            is_owner;
    char            path[KLB_NPIPE_PATH_MAX];

#ifdef _WIN32
    HANDLE          handle;
#else
    int             fd;
#endif
}klb_npipe_t;


//////////////////////////////////////////////////////////////////////////
#ifdef _WIN32

static void make_path_win_klb_npipe(char* p_out, size_t out_len, const char* p_name)
{
    assert(NULL != p_out);
    assert(NULL != p_name);

    if (0 == strncmp(p_name, "\\\\.\\pipe\\", 9) || 0 == strncmp(p_name, "\\\\.\\PIPE\\", 9))
    {
        snprintf(p_out, out_len, "%s", p_name);
    }
    else
    {
        snprintf(p_out, out_len, "\\\\.\\pipe\\klb_%s", p_name);
    }

    p_out[out_len - 1] = '\0';
}

klb_npipe_t* klb_npipe_create(const char* p_name)
{
    klb_npipe_t* p_npipe = NULL;
    HANDLE handle = INVALID_HANDLE_VALUE;

    assert(NULL != p_name);

    p_npipe = KLB_MALLOCZ(klb_npipe_t, 1, 0);
    p_npipe->is_owner = true;
    make_path_win_klb_npipe(p_npipe->path, sizeof(p_npipe->path), p_name);

    // PIPE_NOWAIT: create 立即返回; 客户端 CreateFile 后再通信
    handle = CreateNamedPipeA(
        p_npipe->path,
        PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_NOWAIT,
        1,
        4096,
        4096,
        0,
        NULL);

    if (INVALID_HANDLE_VALUE == handle)
    {
        assert(ERROR_NOT_ENOUGH_MEMORY != GetLastError());
        KLB_FREE(p_npipe);
        return NULL;
    }

    if (!ConnectNamedPipe(handle, NULL))
    {
        DWORD err = GetLastError();

        // ERROR_PIPE_LISTENING / ERROR_NO_DATA: 尚无客户端, 可接受
        if (ERROR_PIPE_CONNECTED != err &&
            ERROR_PIPE_LISTENING != err &&
            ERROR_NO_DATA != err)
        {
            CloseHandle(handle);
            KLB_FREE(p_npipe);
            return NULL;
        }
    }

    p_npipe->handle = handle;
    return p_npipe;
}

klb_npipe_t* klb_npipe_open(const char* p_name)
{
    klb_npipe_t* p_npipe = NULL;
    HANDLE handle = INVALID_HANDLE_VALUE;

    assert(NULL != p_name);

    p_npipe = KLB_MALLOCZ(klb_npipe_t, 1, 0);
    p_npipe->is_owner = false;
    make_path_win_klb_npipe(p_npipe->path, sizeof(p_npipe->path), p_name);

    handle = CreateFileA(
        p_npipe->path,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (INVALID_HANDLE_VALUE == handle)
    {
        assert(ERROR_NOT_ENOUGH_MEMORY != GetLastError());
        KLB_FREE(p_npipe);
        return NULL;
    }

    p_npipe->handle = handle;
    return p_npipe;
}

void klb_npipe_destroy(klb_npipe_t* p_npipe)
{
    assert(NULL != p_npipe);

    if (INVALID_HANDLE_VALUE != p_npipe->handle && NULL != p_npipe->handle)
    {
        if (p_npipe->is_owner)
        {
            DisconnectNamedPipe(p_npipe->handle);
        }

        CloseHandle(p_npipe->handle);
        p_npipe->handle = INVALID_HANDLE_VALUE;
    }

    KLB_FREE(p_npipe);
}

int klb_npipe_read(klb_npipe_t* p_npipe, void* p_data, int max_len)
{
    DWORD nread = 0;

    assert(NULL != p_npipe);
    assert(NULL != p_data);
    assert(0 < max_len);

    if (!ReadFile(p_npipe->handle, p_data, (DWORD)max_len, &nread, NULL))
    {
        DWORD err = GetLastError();

        if (ERROR_NO_DATA == err || ERROR_PIPE_NOT_CONNECTED == err || ERROR_BROKEN_PIPE == err)
        {
            return 0;
        }

        if (ERROR_IO_PENDING == err)
        {
            return -1;
        }

        return -1;
    }

    return (int)nread;
}

int klb_npipe_write(klb_npipe_t* p_npipe, const void* p_data, int len)
{
    DWORD nwrite = 0;

    assert(NULL != p_npipe);
    assert(NULL != p_data);
    assert(0 < len);

    if (!WriteFile(p_npipe->handle, p_data, (DWORD)len, &nwrite, NULL))
    {
        return -1;
    }

    return (int)nwrite;
}

int klb_npipe_set_block(klb_npipe_t* p_npipe, bool block)
{
    DWORD mode = 0;

    assert(NULL != p_npipe);

    mode = PIPE_READMODE_BYTE | (block ? PIPE_WAIT : PIPE_NOWAIT);

    if (!SetNamedPipeHandleState(p_npipe->handle, &mode, NULL, NULL))
    {
        return -1;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
#else

static void make_path_posix_klb_npipe(char* p_out, size_t out_len, const char* p_name)
{
    assert(NULL != p_out);
    assert(NULL != p_name);

    if ('/' == p_name[0])
    {
        snprintf(p_out, out_len, "%s", p_name);
    }
    else
    {
        snprintf(p_out, out_len, "/tmp/klb_npipe_%s", p_name);
    }

    p_out[out_len - 1] = '\0';
}

klb_npipe_t* klb_npipe_create(const char* p_name)
{
    klb_npipe_t* p_npipe = NULL;
    int fd = -1;
    int rc = 0;

    assert(NULL != p_name);

    p_npipe = KLB_MALLOCZ(klb_npipe_t, 1, 0);
    p_npipe->is_owner = true;
    p_npipe->fd = -1;
    make_path_posix_klb_npipe(p_npipe->path, sizeof(p_npipe->path), p_name);

    unlink(p_npipe->path);

    rc = mkfifo(p_npipe->path, 0666);
    if (0 != rc)
    {
        assert(ENOMEM != errno);
        KLB_FREE(p_npipe);
        return NULL;
    }

    // O_RDWR 避免单端 open 永久阻塞
    fd = open(p_npipe->path, O_RDWR | O_CLOEXEC);
    if (fd < 0)
    {
        assert(ENOMEM != errno);
        unlink(p_npipe->path);
        KLB_FREE(p_npipe);
        return NULL;
    }

    p_npipe->fd = fd;
    return p_npipe;
}

klb_npipe_t* klb_npipe_open(const char* p_name)
{
    klb_npipe_t* p_npipe = NULL;
    int fd = -1;

    assert(NULL != p_name);

    p_npipe = KLB_MALLOCZ(klb_npipe_t, 1, 0);
    p_npipe->is_owner = false;
    p_npipe->fd = -1;
    make_path_posix_klb_npipe(p_npipe->path, sizeof(p_npipe->path), p_name);

    fd = open(p_npipe->path, O_RDWR | O_CLOEXEC);
    if (fd < 0)
    {
        assert(ENOMEM != errno);
        KLB_FREE(p_npipe);
        return NULL;
    }

    p_npipe->fd = fd;
    return p_npipe;
}

void klb_npipe_destroy(klb_npipe_t* p_npipe)
{
    assert(NULL != p_npipe);

    if (0 <= p_npipe->fd)
    {
        close(p_npipe->fd);
        p_npipe->fd = -1;
    }

    if (p_npipe->is_owner && '\0' != p_npipe->path[0])
    {
        unlink(p_npipe->path);
    }

    KLB_FREE(p_npipe);
}

int klb_npipe_read(klb_npipe_t* p_npipe, void* p_data, int max_len)
{
    ssize_t n = 0;

    assert(NULL != p_npipe);
    assert(NULL != p_data);
    assert(0 < max_len);

    n = read(p_npipe->fd, p_data, (size_t)max_len);
    if (n < 0)
    {
        if (EAGAIN == errno || EWOULDBLOCK == errno)
        {
            return -1;
        }

        return -1;
    }

    return (int)n;
}

int klb_npipe_write(klb_npipe_t* p_npipe, const void* p_data, int len)
{
    ssize_t n = 0;

    assert(NULL != p_npipe);
    assert(NULL != p_data);
    assert(0 < len);

    n = write(p_npipe->fd, p_data, (size_t)len);
    if (n < 0)
    {
        return -1;
    }

    return (int)n;
}

int klb_npipe_set_block(klb_npipe_t* p_npipe, bool block)
{
    int flags = 0;

    assert(NULL != p_npipe);
    assert(0 <= p_npipe->fd);

    flags = fcntl(p_npipe->fd, F_GETFL, 0);
    if (flags < 0)
    {
        return -1;
    }

    if (block)
    {
        flags &= ~O_NONBLOCK;
    }
    else
    {
        flags |= O_NONBLOCK;
    }

    if (fcntl(p_npipe->fd, F_SETFL, flags) < 0)
    {
        return -1;
    }

    return 0;
}

#endif


bool klb_npipe_is_owner(klb_npipe_t* p_npipe)
{
    assert(NULL != p_npipe);
    return p_npipe->is_owner;
}

// end
