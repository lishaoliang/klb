#include "klbnet/klb_nsp.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"
#include <assert.h>


typedef struct klb_nsp_t_
{
    int a;
}klb_nsp_t;


/// @brief 创建nsp(net server preprocess); 网络连接预处理模块
klb_nsp_t* klb_nsp_create()
{
    return NULL;
}


/// @brief 销毁nsp
void klb_nsp_destroy(klb_nsp_t* p_nsp)
{

}
