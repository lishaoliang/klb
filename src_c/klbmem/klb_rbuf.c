#include "klbmem/klb_rbuf.h"
#include <stdint.h>             /// int*_t uint*_t
#include <stddef.h>             /// intptr_t uintptr_t size_t
#include <stdbool.h>            /// bool false true
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>


#define KLB_RBUF_MAX_PREALLOC   (1024*1024)
#define KLB_RBUF_min(A_, B_)    (((A_) < (B_)) ? (A_) : (B_))


#define KLB_RBUF_malloc         malloc
#define KLB_RBUF_realloc        realloc
#define KLB_RBUF_free           free


static void klb_rbuf_new_init(klb_rbuf_t* p_buf, int buf_len, const void* ptr, int len)
{
    assert(len <= buf_len);

    if (0 < buf_len)
    {
        p_buf->ptr = (char*)KLB_RBUF_malloc(buf_len);
        p_buf->buf_len = buf_len;
    }

    if (NULL != ptr && 0 < len)
    {
        memcpy(p_buf->ptr, ptr, len);
        p_buf->end = len;
    }
}

klb_rbuf_t* klb_rbuf_new(const void* ptr, int len)
{
    klb_rbuf_t* p_buf = (klb_rbuf_t*)KLB_RBUF_malloc(sizeof(klb_rbuf_t));
    memset(p_buf, 0, sizeof(klb_rbuf_t));

    klb_rbuf_new_init(p_buf, len, ptr, len);

    return p_buf;
}

klb_rbuf_t* klb_rbuf_malloc(int buf_len)
{
    klb_rbuf_t* p_buf = (klb_rbuf_t*)KLB_RBUF_malloc(sizeof(klb_rbuf_t));
    memset(p_buf, 0, sizeof(klb_rbuf_t));

    klb_rbuf_new_init(p_buf, buf_len, NULL, 0);

    return p_buf;
}

klb_rbuf_t* klb_rbuf_dup(const klb_rbuf_t* p_buf)
{
    klb_rbuf_t* p_new = (klb_rbuf_t*)KLB_RBUF_malloc(sizeof(klb_rbuf_t));
    memset(p_new, 0, sizeof(klb_rbuf_t));

    if (NULL != p_buf)
    {
        klb_rbuf_new_init(p_new, p_buf->buf_len, p_buf->ptr + p_buf->start, p_buf->end - p_buf->start);
    }

    return p_new;
}

void klb_rbuf_free(klb_rbuf_t* p_buf)
{
    assert(NULL != p_buf);

    if (NULL != p_buf)
    {
        if (NULL != p_buf->ptr)
        {
            KLB_RBUF_free(p_buf->ptr);
        }

        KLB_RBUF_free(p_buf);
    } 
}

void klb_rbuf_reset(klb_rbuf_t* p_buf)
{
    assert(NULL != p_buf);

    if (NULL != p_buf->ptr)
    {
        KLB_RBUF_free(p_buf->ptr);
        p_buf->ptr = NULL;
    }

    memset(p_buf, 0, sizeof(klb_rbuf_t));
}

static void klb_rbuf_check_buf_size(klb_rbuf_t* p_buf, int add_len)
{
    // 检查是否有足够空间支持追加缓存大小;
    // 不足时扩充缓存

    if (p_buf->buf_len < p_buf->end + add_len)
    {
        char* p_nd = NULL;  // 新内存指针
        int newlen = p_buf->end + add_len; // 新缓存长度
        assert(0 < newlen);

        if (newlen < KLB_RBUF_MAX_PREALLOC)
        {
            newlen *= 2;
        }
        else
        {
            newlen += KLB_RBUF_MAX_PREALLOC;
        }

        // 重新分配内存
        if (NULL == p_buf->ptr)
        {
            assert(0 == p_buf->buf_len);
            p_nd = (char*)KLB_RBUF_malloc(newlen);
        }
        else
        {
            char* p_old = p_buf->ptr;
            p_nd = (char*)KLB_RBUF_realloc(p_old, newlen);
            if (NULL == p_nd)
            {
                p_nd = (char*)KLB_RBUF_malloc(newlen);
                if (0 < p_buf->end)
                {
                    assert(NULL != p_nd);
                    memcpy(p_nd, p_old, p_buf->end);
                }

                KLB_RBUF_free(p_old);
            }
        }

        assert(NULL != p_nd);

        p_buf->ptr = p_nd;
        p_buf->buf_len = newlen;
    }
}

int klb_rbuf_cat(klb_rbuf_t* p_buf, const void* ptr, int len)
{
    assert(NULL != p_buf);

    if (NULL == ptr || len <= 0)
    {
        return 0; // 返回数据量
    }

    klb_rbuf_check_buf_size(p_buf, len); // 检查并扩充缓存

    memcpy(p_buf->ptr + p_buf->end, ptr, len);
    p_buf->end += len;

    return len; // 返回数据量
}

int klb_rbuf_buflen(klb_rbuf_t* p_buf)
{
    assert(NULL != p_buf);
    return p_buf->buf_len;
}

int klb_rbuf_datalen(klb_rbuf_t* p_buf)
{
    assert(NULL != p_buf);
    return p_buf->end - p_buf->start; // 当前数据长度
}

char* klb_rbuf_data(klb_rbuf_t* p_buf, int* p_len)
{
    assert(NULL != p_buf);
    int data_len = p_buf->end - p_buf->start;

    if (NULL != p_len)
    {
        *p_len = data_len;
    }

    if (0 < data_len)
    {
        assert(NULL != p_buf->ptr);
        return p_buf->ptr + p_buf->start;
    }

    return NULL;
}

void klb_rbuf_use(klb_rbuf_t* p_buf, int used)
{
    assert(NULL != p_buf);
    int data_len = p_buf->end - p_buf->start;
    int off = KLB_RBUF_min(data_len, used);

    p_buf->start += off;

    assert(p_buf->start <= p_buf->end);
    assert(p_buf->end <= p_buf->buf_len);
}

char* klb_rbuf_idle(klb_rbuf_t* p_buf, int min_len, int* p_idle_len)
{
    assert(NULL != p_buf);

    klb_rbuf_check_buf_size(p_buf, min_len); // 检查并扩充缓存

    int idle_len = p_buf->buf_len - p_buf->end;
    assert(min_len <= idle_len);

    if (NULL != p_idle_len)
    {
        *p_idle_len = idle_len;
    }

    if (0 < idle_len)
    {
        return p_buf->ptr + p_buf->end;
    }

    assert(0 == min_len);
    return NULL;
}

void klb_rbuf_cat_end(klb_rbuf_t* p_buf, int len)
{
    assert(NULL != p_buf);
    int idle_len = p_buf->buf_len - p_buf->end;
    int off = KLB_RBUF_min(idle_len, len);

    p_buf->end += off;
    assert(p_buf->end <= p_buf->buf_len);
}

void klb_rbuf_memmove(klb_rbuf_t* p_buf)
{
    assert(NULL != p_buf);

    if (NULL != p_buf->ptr && 0 != p_buf->start)
    {
        int data_len = p_buf->end - p_buf->start;

        if (0 < data_len)
        {
            memmove(p_buf->ptr, p_buf->ptr + p_buf->start, data_len);

            p_buf->start = 0;
            p_buf->end = data_len;
        }
        else
        {
            assert(0 == data_len);

            p_buf->start = 0;
            p_buf->end = 0;
        }
    }
}

klb_rbuf_t* klb_rbuf_loadfile(const char* p_path)
{
    if (NULL == p_path)
    {
        return NULL;
    }

    FILE* pf = fopen(p_path, "rb");
    if (NULL == pf)
    {
        return NULL;
    }

    fseek(pf, 0, SEEK_END);
    int filelen = ftell(pf);

    klb_rbuf_t* p_buf = NULL;
    if (0 < filelen)
    {
        p_buf = klb_rbuf_malloc(filelen);
        if (NULL != p_buf)
        {
            fseek(pf, 0, SEEK_SET);
            int rlen = fread(p_buf->ptr, 1, filelen, pf);
            assert(rlen = filelen);

            p_buf->end = filelen;
        }
    }

    fclose(pf);
    return p_buf;
}

int klb_rbuf_savefile(klb_rbuf_t* p_buf, const char* p_path)
{
    assert(NULL != p_buf);

    if (NULL == p_path)
    {
        return -1;
    }

    int data_len = p_buf->end - p_buf->start;
    if (data_len <= 0)
    {
        return 1;
    }

    FILE* pf = fopen(p_path, "wb+");
    if (NULL == pf)
    {
        return -1;
    }

    int wlen = fwrite(p_buf->ptr + p_buf->start, 1, data_len, pf);
    assert(wlen == data_len);

    fclose(pf);
    return 0;
}

//////////////////////////////////////////////////////////////////////////
// test

static int __failed_tests = 0;
static int __test_num = 0;
#define test_cond(descr,_c) do { \
    __test_num++; printf("%d - %s: ", __test_num, descr); \
    if(_c) printf("PASSED\n"); else {printf("FAILED\n"); __failed_tests++;} \
} while(0);
#define test_report() do { \
    printf("%d tests, %d passed, %d failed\n", __test_num, \
                    __test_num-__failed_tests, __failed_tests); \
    if (__failed_tests) { \
        printf("=== WARNING === We have failed tests here...\n"); \
        exit(1); \
    } \
} while(0);

void klb_rbuf_test()
{
    klb_rbuf_t* p_rbuf = klb_rbuf_new("foo", 3);

    test_cond("Create a rbuf_t and obtain the length",
        klb_rbuf_datalen(p_rbuf) == 3 && memcmp(klb_rbuf_data(p_rbuf, NULL), "foo", 3) == 0);

    char buf[16 * 1024] = { 0 };
    memset(buf, 'a', sizeof(buf));

    int size = 3;
    for (int i = 0; i < sizeof(buf) / 2;  i++)
    {
        size += klb_rbuf_cat(p_rbuf, buf, i + 1);
    }

    test_cond("rbuf_cat length", klb_rbuf_datalen(p_rbuf) == size);

    //rbuf_savefile(p_rbuf, "./rbuf_txt_1.txt");

    klb_rbuf_reset(p_rbuf);
    klb_rbuf_free(p_rbuf);


    p_rbuf = klb_rbuf_malloc(16 * 1024);

    while (true)
    {
        int idle_len = 0;
        char* p_idle = klb_rbuf_idle(p_rbuf, 0, &idle_len);
        if (NULL != p_idle)
        {
            int cp_len = KLB_RBUF_min(64, idle_len);
            memcpy(p_idle, buf, cp_len);

            klb_rbuf_cat_end(p_rbuf, cp_len);
        }
        else
        {
            break;
        }
    }

    for (int i = 0; i < 100; i++)
    {
        char* p_idle = klb_rbuf_idle(p_rbuf, 64, NULL);
        memcpy(p_idle, buf, 64);
        klb_rbuf_cat_end(p_rbuf, 64);
    }

    char path[] = "./rbuf_txt.txt";

    klb_rbuf_savefile(p_rbuf, path);
    klb_rbuf_free(p_rbuf);

    p_rbuf = klb_rbuf_loadfile(path);

    while (true)
    {
        int data_len = 0;
        char* p_data = klb_rbuf_data(p_rbuf, &data_len);

        if (NULL == p_data)
        {
            break;
        }

        int cmp_len = KLB_RBUF_min(data_len, 49);
        if (0 != memcmp(p_data, buf, 49))
        {
            assert(false);
        }

        klb_rbuf_use(p_rbuf, cmp_len);

        klb_rbuf_memmove(p_rbuf);
    }

    klb_rbuf_free(p_rbuf);

    test_report();
}
