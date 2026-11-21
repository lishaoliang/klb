// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbplatform/klb_sysmq.h"
#include "klbmem/klb_mem.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>


#if defined(_WIN32) || defined(KLB_NO_SYSMQ)
#define KLB_SYSMQ_POSIX     0
#elif defined(__has_include)
#  if __has_include(<mqueue.h>)
#    define KLB_SYSMQ_POSIX 1
#  else
#    define KLB_SYSMQ_POSIX 0
#  endif
#else
#define KLB_SYSMQ_POSIX     1
#endif


#if (0 != KLB_SYSMQ_POSIX)
#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <time.h>
#endif


#define KLB_SYSMQ_NAME_MAX      256
#define KLB_SYSMQ_PREFIX        "/klb."
#define KLB_SYSMQ_PREFIX_LEN    5


/// @struct klb_sysmq_t
typedef struct klb_sysmq_t_
{
    bool            is_owner;
    uint32_t        max_size;       ///< 单条上限(字节)
#if (0 != KLB_SYSMQ_POSIX)
    mqd_t           mqdes;
    char            name[KLB_SYSMQ_NAME_MAX];
#else
    int             unused;
#endif
}klb_sysmq_t;


//////////////////////////////////////////////////////////////////////////
#if (0 == KLB_SYSMQ_POSIX)

klb_sysmq_t* klb_sysmq_create(const char* p_name, uint32_t max_msg, uint32_t max_size)
{
    assert(NULL != p_name);
    (void)max_msg;
    (void)max_size;
    return NULL;
}


klb_sysmq_t* klb_sysmq_open(const char* p_name, uint32_t max_msg, uint32_t max_size)
{
    assert(NULL != p_name);
    (void)max_msg;
    (void)max_size;
    return NULL;
}


void klb_sysmq_destroy(klb_sysmq_t* p_sysmq)
{
    assert(NULL != p_sysmq);
    KLB_FREE(p_sysmq);
}


int klb_sysmq_send(klb_sysmq_t* p_sysmq, const void* p_data, uint32_t len, uint32_t timeout_ms)
{
    assert(NULL != p_sysmq);
    (void)p_data;
    (void)len;
    (void)timeout_ms;
    return 1;
}


int klb_sysmq_recv(klb_sysmq_t* p_sysmq, void* p_buf, uint32_t buf_size, uint32_t* p_out_len, uint32_t timeout_ms)
{
    assert(NULL != p_sysmq);
    (void)p_buf;
    (void)buf_size;
    (void)timeout_ms;

    if (NULL != p_out_len)
    {
        *p_out_len = 0;
    }

    return 1;
}


bool klb_sysmq_is_owner(klb_sysmq_t* p_sysmq)
{
    assert(NULL != p_sysmq);
    return p_sysmq->is_owner;
}

#else
//////////////////////////////////////////////////////////////////////////

static bool valid_user_name_klb_sysmq(const char* p_name)
{
    size_t n = 0;
    size_t i = 0;
    size_t start = 0;

    if (NULL == p_name || '\0' == p_name[0])
    {
        return false;
    }

    n = strlen(p_name);
    start = ('/' == p_name[0]) ? 1u : 0u;

    if (start >= n)
    {
        // only "/"
        return false;
    }

    for (i = start; i < n; ++i)
    {
        if ('/' == p_name[i])
        {
            return false;
        }
    }

    if ('/' == p_name[0])
    {
        // absolute mq name; leave 1 for '\0'
        if (KLB_SYSMQ_NAME_MAX <= n)
        {
            return false;
        }
    }
    else
    {
        // "/klb." + name + '\0'
        if (KLB_SYSMQ_NAME_MAX <= (KLB_SYSMQ_PREFIX_LEN + n))
        {
            return false;
        }
    }

    return true;
}


static void make_name_klb_sysmq(char* p_out, size_t out_len, const char* p_name)
{
    assert(NULL != p_out);
    assert(NULL != p_name);
    assert(0 < out_len);

    if ('/' == p_name[0])
    {
        snprintf(p_out, out_len, "%s", p_name);
    }
    else
    {
        snprintf(p_out, out_len, "%s%s", KLB_SYSMQ_PREFIX, p_name);
    }

    p_out[out_len - 1] = '\0';
}


static void fill_abs_timeout_klb_sysmq(struct timespec* p_ts, uint32_t timeout_ms)
{
    assert(NULL != p_ts);

    clock_gettime(CLOCK_REALTIME, p_ts);

    if (0 == timeout_ms)
    {
        return;
    }

    p_ts->tv_sec += (time_t)(timeout_ms / 1000u);
    p_ts->tv_nsec += (long)(timeout_ms % 1000u) * 1000000L;

    if (p_ts->tv_nsec >= 1000000000L)
    {
        p_ts->tv_sec += 1;
        p_ts->tv_nsec -= 1000000000L;
    }
}


static klb_sysmq_t* open_common_klb_sysmq(const char* p_name, uint32_t max_msg, uint32_t max_size, bool create)
{
    klb_sysmq_t* p_sysmq = NULL;
    mqd_t mqdes = (mqd_t)-1;
    struct mq_attr attr;
    struct mq_attr got;
    int oflag = 0;

    if (!valid_user_name_klb_sysmq(p_name))
    {
        return NULL;
    }

    if (create)
    {
        if (0 == max_msg || 0 == max_size)
        {
            return NULL;
        }
    }

    p_sysmq = KLB_MALLOCZ(klb_sysmq_t, 1, 0);
    make_name_klb_sysmq(p_sysmq->name, sizeof(p_sysmq->name), p_name);

    if (create)
    {
        memset(&attr, 0, sizeof(attr));
        attr.mq_flags = 0;
        attr.mq_maxmsg = (long)max_msg;
        attr.mq_msgsize = (long)max_size;
        attr.mq_curmsgs = 0;

        oflag = O_CREAT | O_EXCL | O_RDWR;
        mqdes = mq_open(p_sysmq->name, oflag, 0666, &attr);
        if ((mqd_t)-1 == mqdes)
        {
            assert(ENOMEM != errno);
            KLB_FREE(p_sysmq);
            return NULL;
        }

        p_sysmq->is_owner = true;
        p_sysmq->max_size = max_size;
    }
    else
    {
        oflag = O_RDWR;
        mqdes = mq_open(p_sysmq->name, oflag);
        if ((mqd_t)-1 == mqdes)
        {
            assert(ENOMEM != errno);
            KLB_FREE(p_sysmq);
            return NULL;
        }

        p_sysmq->is_owner = false;

        memset(&got, 0, sizeof(got));
        if (0 != mq_getattr(mqdes, &got))
        {
            assert(ENOMEM != errno);
            mq_close(mqdes);
            KLB_FREE(p_sysmq);
            return NULL;
        }

        if (0 < max_size && (uint32_t)got.mq_msgsize != max_size)
        {
            mq_close(mqdes);
            KLB_FREE(p_sysmq);
            return NULL;
        }

        if (0 < max_msg && (uint32_t)got.mq_maxmsg != max_msg)
        {
            mq_close(mqdes);
            KLB_FREE(p_sysmq);
            return NULL;
        }

        p_sysmq->max_size = (uint32_t)got.mq_msgsize;
    }

    p_sysmq->mqdes = mqdes;
    return p_sysmq;
}


klb_sysmq_t* klb_sysmq_create(const char* p_name, uint32_t max_msg, uint32_t max_size)
{
    return open_common_klb_sysmq(p_name, max_msg, max_size, true);
}


klb_sysmq_t* klb_sysmq_open(const char* p_name, uint32_t max_msg, uint32_t max_size)
{
    return open_common_klb_sysmq(p_name, max_msg, max_size, false);
}


void klb_sysmq_destroy(klb_sysmq_t* p_sysmq)
{
    assert(NULL != p_sysmq);

    if ((mqd_t)-1 != p_sysmq->mqdes)
    {
        mq_close(p_sysmq->mqdes);
        p_sysmq->mqdes = (mqd_t)-1;
    }

    if (p_sysmq->is_owner)
    {
        // step1. owner 拆除内核对象
        mq_unlink(p_sysmq->name);
    }

    KLB_FREE(p_sysmq);
}


int klb_sysmq_send(klb_sysmq_t* p_sysmq, const void* p_data, uint32_t len, uint32_t timeout_ms)
{
    int rc = 0;
    struct timespec ts;

    assert(NULL != p_sysmq);
    assert(NULL != p_data);
    assert(0 < len);

    if (len > p_sysmq->max_size)
    {
        return 1;
    }

    if ((uint32_t)-1 == timeout_ms)
    {
        // step1. 一直等
        rc = mq_send(p_sysmq->mqdes, (const char*)p_data, (size_t)len, 0);
        if (0 != rc)
        {
            assert(ENOMEM != errno);
            return 1;
        }

        return 0;
    }

    // step2. 定时 / 不等待(timeout_ms==0 用当前时刻)
    fill_abs_timeout_klb_sysmq(&ts, timeout_ms);
    rc = mq_timedsend(p_sysmq->mqdes, (const char*)p_data, (size_t)len, 0, &ts);
    if (0 != rc)
    {
        assert(ENOMEM != errno);
        return 1;
    }

    return 0;
}


int klb_sysmq_recv(klb_sysmq_t* p_sysmq, void* p_buf, uint32_t buf_size, uint32_t* p_out_len, uint32_t timeout_ms)
{
    ssize_t n = 0;
    struct timespec ts;

    assert(NULL != p_sysmq);
    assert(NULL != p_buf);
    assert(0 < buf_size);

    if (buf_size < p_sysmq->max_size)
    {
        // POSIX: 缓冲须 >= mq_msgsize, 否则 mq_receive 失败
        if (NULL != p_out_len)
        {
            *p_out_len = 0;
        }

        return 1;
    }

    if ((uint32_t)-1 == timeout_ms)
    {
        n = mq_receive(p_sysmq->mqdes, (char*)p_buf, (size_t)buf_size, NULL);
        if (0 > n)
        {
            assert(ENOMEM != errno);
            if (NULL != p_out_len)
            {
                *p_out_len = 0;
            }

            return 1;
        }

        if (NULL != p_out_len)
        {
            *p_out_len = (uint32_t)n;
        }

        return 0;
    }

    fill_abs_timeout_klb_sysmq(&ts, timeout_ms);
    n = mq_timedreceive(p_sysmq->mqdes, (char*)p_buf, (size_t)buf_size, NULL, &ts);
    if (0 > n)
    {
        assert(ENOMEM != errno);
        if (NULL != p_out_len)
        {
            *p_out_len = 0;
        }

        return 1;
    }

    if (NULL != p_out_len)
    {
        *p_out_len = (uint32_t)n;
    }

    return 0;
}


bool klb_sysmq_is_owner(klb_sysmq_t* p_sysmq)
{
    assert(NULL != p_sysmq);
    return p_sysmq->is_owner;
}

#endif // KLB_SYSMQ_POSIX

// end
