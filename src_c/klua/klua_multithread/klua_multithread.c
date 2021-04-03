#include "klua/klua_multithread/klua_multithread.h"
#include <assert.h>


// frome ./klua/klua_multithread/klua_kmcache.c
extern int klua_kmcache_init();
extern void klua_kmcache_quit();



int klua_multithread_init()
{
    // kmcache, 线程间共享缓存数据
    klua_kmcache_init();



    return 0;
}

void klua_multithread_quit()
{
    // kmcache, 线程间共享缓存数据
    klua_kmcache_quit();
}
