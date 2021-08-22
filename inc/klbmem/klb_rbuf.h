///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_rbuf.h
/// @brief   文件简要描述
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
///  \n https://gitee.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_RBUF_H__
#define __KLB_RBUF_H__


// realloc buffer
typedef struct klb_rbuf_t_
{
    char*   ptr;
    int     start;
    int     end;
    int     buf_len;
}klb_rbuf_t;


klb_rbuf_t* klb_rbuf_new(const void* ptr, int len);
klb_rbuf_t* klb_rbuf_malloc(int buf_len);
klb_rbuf_t* klb_rbuf_dup(const klb_rbuf_t* p_buf);
void klb_rbuf_free(klb_rbuf_t* p_buf);

void klb_rbuf_reset(klb_rbuf_t* p_buf);

int klb_rbuf_cat(klb_rbuf_t* p_buf, const void* ptr, int len);

int klb_rbuf_buflen(klb_rbuf_t* p_buf);
int klb_rbuf_datalen(klb_rbuf_t* p_buf);

char* klb_rbuf_data(klb_rbuf_t* p_buf, int* p_len);
void klb_rbuf_use(klb_rbuf_t* p_buf, int used);


// 获取空闲缓存
char* klb_rbuf_idle(klb_rbuf_t* p_buf, int min_len, int* p_idle_len);
void klb_rbuf_cat_end(klb_rbuf_t* p_buf, int len);

void klb_rbuf_memmove(klb_rbuf_t* p_buf);

klb_rbuf_t* klb_rbuf_loadfile(const char* p_path);
int klb_rbuf_savefile(klb_rbuf_t* p_buf, const char* p_path);

void klb_rbuf_test();


#endif // __KLB_RBUF_H__
