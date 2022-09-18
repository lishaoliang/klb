// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/klua_multithread/klua_multithread.h"
#include <assert.h>


// from ./klua/klua_multithread/klua_kmcache.c
extern int klua_kmcache_init();
extern void klua_kmcache_quit();


// from ./klua/klua_multithread/klua_klist.c
extern int klua_klist_init();
extern void klua_klist_quit();


// from ./klua/klua_multithread/klua_kthread.c
extern int klua_kthread_init();
extern void klua_kthread_quit();


int klua_multithread_init()
{
    // kmcache, 线程间共享缓存数据
    klua_kmcache_init();

    // kstore, 线程间共享缓存数据, 单次存取
    klua_klist_init();

    // kthread, 多线程模块初始化
    klua_kthread_init();

    return 0;
}

void klua_multithread_quit()
{
    // kthread, 多线程模块
    klua_kthread_quit();

    // kstore, 线程间共享缓存数据, 单次存取
    klua_klist_quit();

    // kmcache, 线程间共享缓存数据
    klua_kmcache_quit();
}
