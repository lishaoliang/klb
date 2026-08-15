// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbnet/klb_netmulti.h"
#include "klbnet/klb_iopoll_in.h"
#include "klbmem/klb_mem.h"
#include <assert.h>


//////////////////////////////////////////////////////////////////////////

/// @brief 创建
klb_netmulti_t* klb_netmulti_create(int64_t tc)
{
    klb_netmulti_t* p_multi = g_klb_iopoll_ops.create(tc);

    p_multi->ops.destroy = g_klb_iopoll_ops.destroy;
    p_multi->ops.push = g_klb_iopoll_ops.push;

    p_multi->ops.remove = g_klb_iopoll_ops.remove;
    p_multi->ops.remove_by_name = g_klb_iopoll_ops.remove_by_name;

    p_multi->ops.closing = g_klb_iopoll_ops.closing;
    p_multi->ops.loop_once = g_klb_iopoll_ops.loop_once;

    return p_multi;
}

/// @brief 销毁
void klb_netmulti_destroy(klb_netmulti_t* p_multi)
{
    p_multi->ops.destroy(p_multi);
}

/// @brief 放入连接对象
int klb_netmulti_push(klb_netmulti_t* p_multi, klb_netconn_t* p_conn)
{
    return p_multi->ops.push(p_multi, p_conn);
}

/// @brief 移除连接对象
/// @return int 0.成功; 非0. 失败
int klb_netmulti_remove(klb_netmulti_t* p_multi, klb_netconn_t* p_conn)
{
    return p_multi->ops.remove(p_multi, p_conn);
}

/// @brief 移除连接对象
/// @return int 0.成功; 非0. 失败
klb_netconn_t* klb_netmulti_remove_by_name(klb_netmulti_t* p_multi, const char* p_name)
{
    return p_multi->ops.remove_by_name(p_multi, p_name);
}


/// @brief 托管关闭连接
int klb_netmulti_closing(klb_netmulti_t* p_multi, klb_netconn_t* p_conn)
{
    return p_multi->ops.closing(p_multi, p_conn);
}

/// @brief 调用一次; 需要定期调用
int klb_netmulti_loop_once(klb_netmulti_t* p_multi, int64_t now)
{
    return p_multi->ops.loop_once(p_multi, now);
}

//end
