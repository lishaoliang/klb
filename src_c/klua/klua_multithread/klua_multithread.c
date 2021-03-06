﻿#include "klua/klua_multithread/klua_multithread.h"
#include <assert.h>


// frome ./klua/klua_multithread/klua_kmcache.c
extern int klua_kmcache_init();
extern void klua_kmcache_quit();


// frome ./klua/klua_multithread/klua_kthread.c
extern int klua_kthread_init();
extern void klua_kthread_quit();


int klua_multithread_init()
{
    // kmcache, 线程间共享缓存数据
    klua_kmcache_init();

    // kthread, 多线程模块初始化
    klua_kthread_init();

    return 0;
}

void klua_multithread_quit()
{
    // kthread, 多线程模块
    klua_kthread_quit();

    // kmcache, 线程间共享缓存数据
    klua_kmcache_quit();
}
