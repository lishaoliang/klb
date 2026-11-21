// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbplatform/klb_msgq.h"
#include "klbplatform/klb_shmem.h"
#include "klbplatform/klb_semlock.h"
#include "klbmem/klb_mem.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>


#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#include <errno.h>
#endif


#define KLB_MSGQ_MAGIC          0x4B4D5351u     ///< 'KMSQ'
#define KLB_MSGQ_VERSION        1u
#define KLB_MSGQ_NAME_MAX       256
#define KLB_MSGQ_ALIGN          4u


/// @struct klb_msgq_hdr_t
/// @brief  SHM 内队列头
typedef struct klb_msgq_hdr_t_
{
    uint32_t        magic;          ///< KLB_MSGQ_MAGIC
    uint32_t        version;        ///< KLB_MSGQ_VERSION
    uint32_t        capacity;       ///< 数据区字节
    uint32_t        max_msg;        ///< 单消息上限
    uint32_t        head;           ///< 写偏移
    uint32_t        tail;           ///< 读偏移
    uint32_t        used;           ///< 已用字节(含每条 len 头)
    uint32_t        msg_count;      ///< 消息条数
}klb_msgq_hdr_t;


/// @struct klb_msgq_t
typedef struct klb_msgq_t_
{
    klb_shmem_t*    p_shmem;
    klb_semlock_t*  p_lock;
    klb_msgq_hdr_t* p_hdr;
    uint8_t*        p_data;
    bool            is_owner;
}klb_msgq_t;


static uint32_t align_up_klb_msgq(uint32_t v)
{
    return (v + (KLB_MSGQ_ALIGN - 1u)) & ~(KLB_MSGQ_ALIGN - 1u);
}

static void make_names_klb_msgq(const char* p_name, char* p_shm, size_t shm_len, char* p_lock, size_t lock_len)
{
    assert(NULL != p_name);

    snprintf(p_shm, shm_len, "klb.msgq.shm.%s", p_name);
    snprintf(p_lock, lock_len, "klb.msgq.lock.%s", p_name);
    p_shm[shm_len - 1] = '\0';
    p_lock[lock_len - 1] = '\0';
}

static void sleep_ms_klb_msgq(uint32_t ms)
{
#ifdef _WIN32
    Sleep(ms);
#else
    struct timespec ts;

    ts.tv_sec = (time_t)(ms / 1000u);
    ts.tv_nsec = (long)((ms % 1000u) * 1000000u);
    nanosleep(&ts, NULL);
#endif
}

static uint32_t now_ms_klb_msgq(void)
{
#ifdef _WIN32
    return (uint32_t)GetTickCount();
#else
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000u + ts.tv_nsec / 1000000u);
#endif
}

static bool timed_out_klb_msgq(uint32_t start_ms, uint32_t timeout_ms)
{
    uint32_t now = 0;
    uint32_t elapsed = 0;

    if ((uint32_t)-1 == timeout_ms)
    {
        return false;
    }

    if (0 == timeout_ms)
    {
        return true;
    }

    now = now_ms_klb_msgq();
    elapsed = now - start_ms;
    return (elapsed >= timeout_ms);
}

static size_t shm_bytes_klb_msgq(uint32_t capacity)
{
    return sizeof(klb_msgq_hdr_t) + (size_t)capacity;
}

static void bind_view_klb_msgq(klb_msgq_t* p_msgq)
{
    uint8_t* p_base = NULL;

    assert(NULL != p_msgq);
    assert(NULL != p_msgq->p_shmem);

    p_base = (uint8_t*)klb_shmem_address(p_msgq->p_shmem);
    assert(NULL != p_base);

    p_msgq->p_hdr = (klb_msgq_hdr_t*)p_base;
    p_msgq->p_data = p_base + sizeof(klb_msgq_hdr_t);
}

static int init_hdr_klb_msgq(klb_msgq_t* p_msgq, uint32_t capacity, uint32_t max_msg)
{
    klb_msgq_hdr_t* p_hdr = NULL;

    assert(NULL != p_msgq);

    p_hdr = p_msgq->p_hdr;
    memset(p_hdr, 0, sizeof(*p_hdr));
    p_hdr->magic = KLB_MSGQ_MAGIC;
    p_hdr->version = KLB_MSGQ_VERSION;
    p_hdr->capacity = capacity;
    p_hdr->max_msg = max_msg;
    p_hdr->head = 0;
    p_hdr->tail = 0;
    p_hdr->used = 0;
    p_hdr->msg_count = 0;
    return 0;
}

static int check_hdr_klb_msgq(klb_msgq_t* p_msgq, uint32_t capacity, uint32_t max_msg)
{
    klb_msgq_hdr_t* p_hdr = NULL;

    assert(NULL != p_msgq);
    p_hdr = p_msgq->p_hdr;

    if (KLB_MSGQ_MAGIC != p_hdr->magic)
    {
        return -1;
    }

    if (KLB_MSGQ_VERSION != p_hdr->version)
    {
        return -1;
    }

    if (capacity != p_hdr->capacity || max_msg != p_hdr->max_msg)
    {
        return -1;
    }

    return 0;
}

static klb_msgq_t* open_common_klb_msgq(const char* p_name, uint32_t capacity, uint32_t max_msg, bool create)
{
    char shm_name[KLB_MSGQ_NAME_MAX];
    char lock_name[KLB_MSGQ_NAME_MAX];
    klb_msgq_t* p_msgq = NULL;
    size_t bytes = 0;
    int rc = 0;

    assert(NULL != p_name);
    assert(0 < capacity);
    assert(0 < max_msg);
    assert(max_msg < capacity);

    make_names_klb_msgq(p_name, shm_name, sizeof(shm_name), lock_name, sizeof(lock_name));
    bytes = shm_bytes_klb_msgq(capacity);

    p_msgq = KLB_MALLOCZ(klb_msgq_t, 1, 0);

    if (create)
    {
        p_msgq->p_shmem = klb_shmem_create(KLB_SHMEM_CREATE, shm_name, bytes);
        if (NULL == p_msgq->p_shmem)
        {
            KLB_FREE(p_msgq);
            return NULL;
        }

        p_msgq->p_lock = klb_semlock_create(lock_name);
        p_msgq->is_owner = true;
        bind_view_klb_msgq(p_msgq);
        init_hdr_klb_msgq(p_msgq, capacity, max_msg);
    }
    else
    {
        p_msgq->p_shmem = klb_shmem_open(shm_name, bytes);
        if (NULL == p_msgq->p_shmem)
        {
            KLB_FREE(p_msgq);
            return NULL;
        }

        p_msgq->p_lock = klb_semlock_open(lock_name);
        p_msgq->is_owner = false;
        bind_view_klb_msgq(p_msgq);

        rc = check_hdr_klb_msgq(p_msgq, capacity, max_msg);
        if (0 != rc)
        {
            klb_semlock_destroy(p_msgq->p_lock);
            klb_shmem_destroy(p_msgq->p_shmem);
            KLB_FREE(p_msgq);
            return NULL;
        }
    }

    return p_msgq;
}

klb_msgq_t* klb_msgq_create(const char* p_name, uint32_t capacity, uint32_t max_msg)
{
    return open_common_klb_msgq(p_name, capacity, max_msg, true);
}

klb_msgq_t* klb_msgq_open(const char* p_name, uint32_t capacity, uint32_t max_msg)
{
    return open_common_klb_msgq(p_name, capacity, max_msg, false);
}

void klb_msgq_destroy(klb_msgq_t* p_msgq)
{
    assert(NULL != p_msgq);

    if (p_msgq->is_owner)
    {
        // owner: 拆除 sem + shm 内核对象
        KLB_FREE_BY(p_msgq->p_lock, klb_semlock_destroy);
        KLB_FREE_BY(p_msgq->p_shmem, klb_shmem_destroy);
    }
    else
    {
#ifdef _WIN32
        // Win mutex 按句柄计数; destroy=CloseHandle, 安全
        KLB_FREE_BY(p_msgq->p_lock, klb_semlock_destroy);
#else
        // Linux SysV: semlock_destroy 会 IPC_RMID; 非 owner 只释放包装
        if (NULL != p_msgq->p_lock)
        {
            KLB_FREE(p_msgq->p_lock);
            p_msgq->p_lock = NULL;
        }
#endif
        KLB_FREE_BY(p_msgq->p_shmem, klb_shmem_destroy);
    }

    KLB_FREE(p_msgq);
}

static uint32_t free_bytes_klb_msgq(const klb_msgq_hdr_t* p_hdr)
{
    assert(NULL != p_hdr);
    assert(p_hdr->used <= p_hdr->capacity);
    return p_hdr->capacity - p_hdr->used;
}

static void write_bytes_klb_msgq(klb_msgq_t* p_msgq, const uint8_t* p_src, uint32_t len)
{
    klb_msgq_hdr_t* p_hdr = NULL;
    uint32_t first = 0;

    assert(NULL != p_msgq);
    p_hdr = p_msgq->p_hdr;

    if (0 == len)
    {
        return;
    }

    first = p_hdr->capacity - p_hdr->head;
    if (first >= len)
    {
        memcpy(p_msgq->p_data + p_hdr->head, p_src, len);
        p_hdr->head += len;
        if (p_hdr->head == p_hdr->capacity)
        {
            p_hdr->head = 0;
        }
    }
    else
    {
        memcpy(p_msgq->p_data + p_hdr->head, p_src, first);
        memcpy(p_msgq->p_data, p_src + first, len - first);
        p_hdr->head = len - first;
    }

    p_hdr->used += len;
}

static void read_bytes_klb_msgq(klb_msgq_t* p_msgq, uint8_t* p_dst, uint32_t len)
{
    klb_msgq_hdr_t* p_hdr = NULL;
    uint32_t first = 0;

    assert(NULL != p_msgq);
    p_hdr = p_msgq->p_hdr;

    if (0 == len)
    {
        return;
    }

    first = p_hdr->capacity - p_hdr->tail;
    if (first >= len)
    {
        memcpy(p_dst, p_msgq->p_data + p_hdr->tail, len);
        p_hdr->tail += len;
        if (p_hdr->tail == p_hdr->capacity)
        {
            p_hdr->tail = 0;
        }
    }
    else
    {
        memcpy(p_dst, p_msgq->p_data + p_hdr->tail, first);
        memcpy(p_dst + first, p_msgq->p_data, len - first);
        p_hdr->tail = len - first;
    }

    assert(p_hdr->used >= len);
    p_hdr->used -= len;
}

int klb_msgq_send(klb_msgq_t* p_msgq, const void* p_data, uint32_t len, uint32_t timeout_ms)
{
    uint32_t start = 0;
    uint32_t need = 0;
    uint32_t pad = 0;
    uint32_t len_le = 0;

    assert(NULL != p_msgq);
    assert(NULL != p_data);
    assert(0 < len);

    if (len > p_msgq->p_hdr->max_msg)
    {
        return -1;
    }

    need = sizeof(uint32_t) + align_up_klb_msgq(len);
    start = now_ms_klb_msgq();

    for (;;)
    {
        klb_semlock_lock(p_msgq->p_lock);

        if (free_bytes_klb_msgq(p_msgq->p_hdr) >= need)
        {
            len_le = len;
            write_bytes_klb_msgq(p_msgq, (const uint8_t*)&len_le, sizeof(len_le));
            write_bytes_klb_msgq(p_msgq, (const uint8_t*)p_data, len);

            pad = align_up_klb_msgq(len) - len;
            if (0 != pad)
            {
                uint8_t zeros[KLB_MSGQ_ALIGN] = { 0 };
                write_bytes_klb_msgq(p_msgq, zeros, pad);
            }

            p_msgq->p_hdr->msg_count += 1u;
            klb_semlock_unlock(p_msgq->p_lock);
            return 0;
        }

        klb_semlock_unlock(p_msgq->p_lock);

        if (timed_out_klb_msgq(start, timeout_ms))
        {
            return -1;
        }

        if (0 == timeout_ms)
        {
            return -1;
        }

        sleep_ms_klb_msgq(1);
    }
}

int klb_msgq_recv(klb_msgq_t* p_msgq, void* p_buf, uint32_t buf_size, uint32_t* p_out_len, uint32_t timeout_ms)
{
    uint32_t start = 0;
    uint32_t len = 0;
    uint32_t pad = 0;

    assert(NULL != p_msgq);
    assert(NULL != p_buf);
    assert(0 < buf_size);

    start = now_ms_klb_msgq();

    for (;;)
    {
        klb_semlock_lock(p_msgq->p_lock);

        if (0 < p_msgq->p_hdr->msg_count)
        {
            read_bytes_klb_msgq(p_msgq, (uint8_t*)&len, sizeof(len));

            if (len > p_msgq->p_hdr->max_msg || len > buf_size)
            {
                // 缓冲不足或损坏: 尽量跳过本条并报错
                if (len <= p_msgq->p_hdr->max_msg && len <= p_msgq->p_hdr->used)
                {
                    // 无法放入用户缓冲: 回滚困难, 直接丢弃本条对齐数据
                    uint8_t tmp[256];
                    uint32_t left = len;

                    while (0 < left)
                    {
                        uint32_t chunk = left;

                        if (chunk > sizeof(tmp))
                        {
                            chunk = (uint32_t)sizeof(tmp);
                        }

                        read_bytes_klb_msgq(p_msgq, tmp, chunk);
                        left -= chunk;
                    }

                    pad = align_up_klb_msgq(len) - len;
                    if (0 != pad)
                    {
                        uint8_t zeros[KLB_MSGQ_ALIGN];
                        read_bytes_klb_msgq(p_msgq, zeros, pad);
                    }

                    if (0 < p_msgq->p_hdr->msg_count)
                    {
                        p_msgq->p_hdr->msg_count -= 1u;
                    }
                }

                klb_semlock_unlock(p_msgq->p_lock);
                return -1;
            }

            read_bytes_klb_msgq(p_msgq, (uint8_t*)p_buf, len);
            pad = align_up_klb_msgq(len) - len;
            if (0 != pad)
            {
                uint8_t zeros[KLB_MSGQ_ALIGN];
                read_bytes_klb_msgq(p_msgq, zeros, pad);
            }

            p_msgq->p_hdr->msg_count -= 1u;
            klb_semlock_unlock(p_msgq->p_lock);

            if (NULL != p_out_len)
            {
                *p_out_len = len;
            }

            return 0;
        }

        klb_semlock_unlock(p_msgq->p_lock);

        if (timed_out_klb_msgq(start, timeout_ms))
        {
            return -1;
        }

        if (0 == timeout_ms)
        {
            return -1;
        }

        sleep_ms_klb_msgq(1);
    }
}

bool klb_msgq_is_owner(klb_msgq_t* p_msgq)
{
    assert(NULL != p_msgq);
    return p_msgq->is_owner;
}

// end
