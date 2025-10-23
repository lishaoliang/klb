// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbnet/klb_netconn.h"


/// @brief 销毁连接
void klb_netconn_destroy(klb_netconn_t* p_conn)
{
    klb_netconn_destroy_cb cb_destroy = p_conn->vtable.destroy;

    cb_destroy(p_conn);
}

int klb_netconn_bind_recv_data(klb_netconn_t* p_conn, klb_netconn_recv_data_cb cb_recv)
{
    p_conn->vtable.recv_data = cb_recv;
    return 0;
}

/// @brief 设置连接名称
void klb_netconn_set_name(klb_netconn_t* p_conn, const char* p_name)
{
    if (NULL != p_name)
    {
        strncpy(p_conn->name, p_name, KLB_NETCONN_NAME_max - 1);
    }
    else
    {
        p_conn->name[0] = '\0';
    }
}

/// @brief 获取连接名称
const char* klb_netconn_get_name(klb_netconn_t* p_conn)
{
    return p_conn->name;
}

/// @brief 发送文本数据
/// @return int 0.成功; 非0.失败
int klb_netconn_send_text(klb_netconn_t* p_conn, uint32_t sequence, uint32_t uid, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    if (NULL != p_conn->vtable.send_normal)
    {
        return p_conn->vtable.send_normal(p_conn, KLB_MNP_TEXT, sequence, uid, p_head, head_len, p_body, body_len);
    }

    return 1;
}


/// @brief 发送二进制数据
/// @return int 0.成功; 非0.失败
int klb_netconn_send_binary(klb_netconn_t* p_conn, uint32_t sequence, uint32_t uid, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    if (NULL != p_conn->vtable.send_normal)
    {
        return p_conn->vtable.send_normal(p_conn, KLB_MNP_BINARY, sequence, uid, p_head, head_len, p_body, body_len);
    }

    return 1;
}

/// @brief 发送媒体数据
/// @return int 0.成功; 非0.失败
int klb_netconn_send_media(klb_netconn_t* p_conn, klb_buf_t* p_data)
{
    if (NULL != p_conn->vtable.send_media)
    {
        return p_conn->vtable.send_media(p_conn, p_data);
    }

    return 1;
}


/// @brief 发送RPC-Lua数据
/// @return int 0.成功; 非0.失败
int klb_netconn_send_rpc(klb_netconn_t* p_conn, uint32_t sequence, uint32_t uid, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    if (NULL != p_conn->vtable.send_normal)
    {
        return p_conn->vtable.send_normal(p_conn, KLB_MNP_RPC_LUA, sequence, uid, p_head, head_len, p_body, body_len);
    }

    return 1;
}


/// @brief 发送RPC-Json数据
/// @return int 0.成功; 非0.失败
int klb_netconn_send_rpc_json(klb_netconn_t* p_conn, uint32_t sequence, uint32_t uid, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    if (NULL != p_conn->vtable.send_normal)
    {
        return p_conn->vtable.send_normal(p_conn, KLB_MNP_RPC_JSON, sequence, uid, p_head, head_len, p_body, body_len);
    }

    return 1;
}


/// @brief 对某个连接进行控制操作: get/set,etc.
/// @return int 0.成功; 非0.失败
int klb_netconn_ioctrl(klb_netconn_t* p_conn, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (NULL != p_conn->vtable.ioctrl)
    {
        return p_conn->vtable.ioctrl(p_conn, p_in, p_out);
    }

    return 1;
}

void klb_netconn_set_udata(klb_netconn_t* p_conn, void* p_udata)
{
    p_conn->p_udata = p_udata;
}

void klb_netconn_set_lparam(klb_netconn_t* p_conn, void* p_lparam)
{
    p_conn->p_lparam = p_lparam;
}

void klb_netconn_set_wparam(klb_netconn_t* p_conn, void* p_wparam)
{
    p_conn->p_wparam = p_wparam;
}

//end
