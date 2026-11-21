// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_socket.h
/// @brief   文件简要描述
/// @version 0.1
/// @history 修改历史
///  \n 2019 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_SOCKET_H__
#define __KLB_SOCKET_H__

#include "klb_type.h"

// socket fd 类型
#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif

    #include <winsock2.h>
    #include <ws2tcpip.h>
    typedef SOCKET          klb_socket_fd;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>

    typedef int             klb_socket_fd;
    #define INVALID_SOCKET  -1
#endif


#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klb_socket_t_ klb_socket_t;


/// @brief 销毁
typedef void(*klb_socket_destroy_cb)(klb_socket_t* p_socket);

/// @brief 流式发送数据 (TCP / TLS 明文侧)
typedef int(*klb_socket_send_cb)(klb_socket_t* p_socket, const uint8_t* p_data, int len);

/// @brief 流式接收数据 (TCP / TLS 明文侧)
typedef int(*klb_socket_recv_cb)(klb_socket_t* p_socket, uint8_t* p_buf, int buf_len);

/// @brief 数据报发送 (UDP / DTLS 等; 业务用 klb_socket_udp_*)
typedef int(*klb_socket_sendto_cb)(klb_socket_t* p_socket, const uint8_t* p_data, int len, const struct sockaddr* p_addr, int addr_len);

/// @brief 数据报接收 (UDP / DTLS 等; 业务用 klb_socket_udp_*)
typedef int(*klb_socket_recvfrom_cb)(klb_socket_t* p_socket, uint8_t* p_buf, int buf_len, struct sockaddr* p_addr, int* p_addr_len);


/// @struct klb_socket_vtable_t
/// @brief  socket虚表/类socket操作
typedef struct klb_socket_vtable_t_
{
    klb_socket_destroy_cb   cb_destroy;     ///< 销毁
    klb_socket_send_cb      cb_send;        ///< TCP发送数据
    klb_socket_recv_cb      cb_recv;        ///< TCP接收数据
    klb_socket_recv_cb      cb_recv_peek;   ///< TCP窥探接收; 不消费队列
    klb_socket_sendto_cb    cb_sendto;      ///< datagram 发送(UDP/DTLS 等; 业务用 klb_socket_udp_*)
    klb_socket_recvfrom_cb  cb_recvfrom;    ///< datagram 接收(UDP/DTLS 等; 业务用 klb_socket_udp_*)
}klb_socket_vtable_t;


/// @struct klb_socket_status_e
/// @brief  基本状态
typedef enum klb_socket_status_e_
{
    KLB_SOCKET_OK,                      ///< 正常
    KLB_SOCKET_DISCONNECT,              ///< 连接断开(读写socket错误引发)
    KLB_SOCKET_TIMEOUT,                 ///< 超时
    KLB_SOCKET_CLOSED,                  ///< 连接已关闭
    KLB_SOCKET_ERR_PROTOCOL,            ///< 协议错误
    KLB_SOCKET_ERR_RESET,               ///< 对端复位 (RST 等)
    KLB_SOCKET_ERR_REFUSED,             ///< 连接被拒绝
    KLB_SOCKET_ERR_UNREACH,             ///< 网络/主机不可达
    KLB_SOCKET_ERR_ABORT,               ///< 连接中止 (ECONNABORTED 等)
    KLB_SOCKET_ERR_PIPE,                ///< 写时对端已关 (EPIPE 等)
    KLB_SOCKET_ERR_INVAL,               ///< 参数或状态非法
    KLB_SOCKET_ERR_ACCESS,              ///< 权限/访问被拒
    KLB_SOCKET_ERR_MAX,                 ///< 错误最大值 (含以上 ERR_*)

    KLB_SOCKET_CLOSEING         = 62,   ///< 主动关闭
    KLB_SOCKET_CONNECT          = 63,   ///< 连接成功
    KLB_SOCKET_STATUS_MAX       = 64,   ///< 最大值: 2^6
}klb_socket_status_e;

/// @struct klb_socket_status_rw_e
/// @brief  读写状态
typedef enum klb_socket_status_rw_e_
{
    KLB_SOCKET_RW_OK,
    KLB_SOCKET_WAIT_READ,
    KLB_SOCKET_WAIT_WRITE,
}klb_socket_status_rw_e;

/// @struct klb_socket_addr_t
/// @brief  紧凑地址 (IPv4 / IPv6)
typedef struct klb_socket_addr_t_
{
    union
    {
        struct sockaddr     sa;         ///< 通用地址
        struct sockaddr_in  in4;        ///< IPv4
        struct sockaddr_in6 in6;        ///< IPv6
    }addr;                              ///< 地址
    socklen_t               len;        ///< 有效长度
}klb_socket_addr_t;


/// @struct klb_socket_family_e
/// @brief  地址族 (项目枚举; 不存 AF_* 原值)
/// @note Windows 的 AF_INET6 为 23, 不能直接塞进位域
typedef enum klb_socket_family_e_
{
    KLB_SOCKET_FAMILY_UNSPEC = 0,       ///< 未知
    KLB_SOCKET_FAMILY_V4     = 1,       ///< IPv4
    KLB_SOCKET_FAMILY_V6     = 2,       ///< IPv6
}klb_socket_family_e;


/// @struct klb_socket_type_e
/// @brief  socket 类型
typedef enum klb_socket_type_e_
{
    KLB_SOCKET_TYPE_STREAM = 0,         ///< SOCK_STREAM (TCP)
    KLB_SOCKET_TYPE_DGRAM  = 1,         ///< SOCK_DGRAM  (UDP)
}klb_socket_type_e;


/// @struct klb_socket_t
/// @brief  socket 基础定义 (TCP / UDP / TLS 公共基类型; UDP API→klb_socket_udp.h; TLS API→klb_socket_tls.h)
typedef struct klb_socket_t_
{
    klb_socket_vtable_t vtable;         ///< 函数表

    klb_socket_fd       fd;             ///< socket fd

    uint16_t            connected : 1;  ///< 0(false).未连接; 1(true).已连接; TCP 指握手完成; UDP 指已打开
    uint16_t            nonblock : 1;   ///< 0(false).阻塞模式; 1(true).非阻塞
    uint16_t            writing : 1;    ///< 0(false).无数据写; 1(true).有数据写; 从缓存写向网络; UDP 恒 0
    uint16_t            reading : 1;    ///< 0(false).无需读取; 1(true).需读取; 从网络读取
    uint16_t            tls : 1;        ///< 0(false).非加密; 1(true).TLS加密
    uint16_t            family : 2;     ///< 地址族: klb_socket_family_e
    uint16_t            type : 2;       ///< socket 类型: klb_socket_type_e
    uint16_t            bcast : 1;      ///< 0(false).未开广播; 1(true).SO_BROADCAST 已开
    uint16_t            mcast : 1;      ///< 0(false).未入组播组; 1(true).已加入组播组
    uint16_t            rsv1 : 5;       ///< 保留

    uint16_t            status : 6;     ///< 基本状态(依据状态断开等): klb_socket_status_e
    uint16_t            status_rw : 4;  ///< 读写状态: klb_socket_status_rw_e
    uint16_t            rsv2 : 6;

    int64_t             last_recv_tc;   ///< 上次接收数据时间
    int64_t             last_send_tc;   ///< 上次发送数据时间

    klb_socket_addr_t   local;          ///< 本端地址 (bind / connect 后)
    klb_socket_addr_t   peer;           ///< 对端地址 (connect 或最近一次 recvfrom)

    void*               p_udata;        ///< user data; 用户附加数据

    char                extra[];        ///< 附加数据
}klb_socket_t;


/// @brief 系统socket模块初始化
/// @return 无
/// @note WIN32网络初始化
KLB_API int klb_socket_init();


/// @brief 系统socket模块退出
/// /// @return 无
/// @note WIN32网络退出
KLB_API void klb_socket_quit();

/// @brief 销毁
KLB_API void klb_socket_destroy(klb_socket_t* p_socket);

/// @brief 进入关闭状态, 调用者不再接收/发送此socket的数据, 进入等待被销毁
KLB_API void klb_socket_closing(klb_socket_t* p_socket);

/// @brief TCP发送数据
/// @return int >0.字节数; 0.对端关闭; <0.klb_netcode_e (常用 AGAIN/IO)
KLB_API int klb_socket_send(klb_socket_t* p_socket, const uint8_t* p_data, int len);

/// @brief TCP接收数据
/// @return int >0.字节数; 0.对端关闭; <0.klb_netcode_e (常用 AGAIN/IO)
KLB_API int klb_socket_recv(klb_socket_t* p_socket, uint8_t* p_buf, int buf_len);

/// @brief TCP窥探接收数据
/// @param [in]  *p_socket          socket
/// @param [out] *p_buf             接收缓存
/// @param [in]  buf_len            缓存大小
/// @return int >0.字节数; 0.对端关闭; <0.klb_netcode_e (常用 AGAIN/IO)
/// @note 不从套接字队列摘走数据 (MSG_PEEK); TLS 时窥探的是内核密文
KLB_API int klb_socket_recv_peek(klb_socket_t* p_socket, uint8_t* p_buf, int buf_len);

/// @brief 获取 fd
KLB_API klb_socket_fd klb_socket_get_fd(klb_socket_t* p_socket);

/// @brief 附加到socket fd
KLB_API void klb_socket_attach_fd(klb_socket_t* p_socket, klb_socket_fd fd);

/// @brief 分离出socket fd
KLB_API klb_socket_fd klb_socket_detach_fd(klb_socket_t* p_socket);

/// @brief 设置是否首次次连接上
KLB_API void klb_socket_set_connected(klb_socket_t* p_socket, bool connected);

/// @brief 获取是否首次次连接上
KLB_API bool klb_socket_is_connected(klb_socket_t* p_socket);

/// @brief 设置发送
KLB_API void klb_socket_set_writing(klb_socket_t* p_socket, bool sending);

/// @brief 获取是否需要发送数据
KLB_API bool klb_socket_is_writing(klb_socket_t* p_socket);

/// @brief 设置读取
KLB_API void klb_socket_set_reading(klb_socket_t* p_socket, bool reading);

/// @brief 获取是否需要读取数据
KLB_API bool klb_socket_is_reading(klb_socket_t* p_socket);

/// @brief 设置LS
KLB_API void klb_socket_set_tls(klb_socket_t* p_socket, bool tls);

/// @brief 获取是否为tls
KLB_API bool klb_socket_is_tls(klb_socket_t* p_socket);

/// @brief 设置状态
/// @param [in]  status             状态: klb_socket_status_e
KLB_API void klb_socket_set_status(klb_socket_t* p_socket, int status);

/// @brief 获取状态
KLB_API uint16_t klb_socket_get_status(klb_socket_t* p_socket);


/// @enum klb_socket_recv_judge_e
/// @brief select/读就绪后, 对单次 klb_socket_recv 返回值的判定
/// @note 调用约束: 须在读就绪回调内调用; 勿盲轮询
/// @note 非 TLS: n==0 为有序关闭(对端 FIN); n<0 且 EAGAIN/EINTR 为缓存读完仍存活; 其它 n<0 为异常关闭
/// @note TLS: 须先看 status_rw; WANT_READ/WRITE 时返回值可为 0 但未关; RW_OK 且 n==0 才为有序关(FIN/close_notify)
/// @note 调用方义务: 收到 EOF/ERROR 后须清 reading (或调 klb_socket_recv_closed), 否则 fd 恒可读致 select 空转
typedef enum klb_socket_recv_judge_e_
{
    KLB_SOCKET_RECV_DATA = 0,           ///< n>0: 有数据; 调用方处理并继续 while 排空
    KLB_SOCKET_RECV_AGAIN,              ///< 缓存已空且连接仍在 (非TLS: EAGAIN; TLS: WANT_*)
    KLB_SOCKET_RECV_EOF,                ///< 有序关闭
    KLB_SOCKET_RECV_ERROR,              ///< 异常关闭或致命错
}klb_socket_recv_judge_e;


/// @brief 判定一次 recv 结果 (TLS / 非 TLS 内部分支)
/// @param [in]  *p_socket          本次实际 recv 的 socket (裸 TCP 或 TLS 包装; 看 tls 标志)
/// @param [in]  n                  klb_socket_recv 返回值
/// @param [out] *p_status          可选; EOF/ERROR 时建议写入的 klb_socket_status_e; 可为 NULL
/// @return int                     klb_socket_recv_judge_e
/// @warning 本函数不执行 recv, 不关闭 fd; 仅判定. n>0 时直接返回 DATA, 不写 p_status
KLB_API int klb_socket_recv_judge(klb_socket_t* p_socket, int n, int* p_status);


/// @brief 读侧判定为关闭后的统一收尾: set_status + 清 reading/writing
/// @param [in]  *p_socket          socket
/// @param [in]  status             klb_socket_status_e (CLOSED / DISCONNECT / ERR_* 等)
/// @return 无
/// @note 用于防止对端半关后 reading 仍为 1 导致 select 空转 (Bug.2026-20 同型)
/// @note 写侧致命错亦可调用本函数收尾 (与 send_judge ERROR 配对)
KLB_API void klb_socket_recv_closed(klb_socket_t* p_socket, int status);


/// @enum klb_socket_send_judge_e
/// @brief select/写就绪后, 对单次 klb_socket_send 返回值的判定
/// @note 调用约束: 须在写就绪回调内调用; 调用方 while 排空直到 AGAIN/ERROR
/// @note 非 TLS: n>0 继续写; n<0 且 EAGAIN/EINTR 为发送缓存满仍存活; 其它 n<0 为异常关闭 (EPIPE/RST 等)
/// @note 非 TLS: n==0 少见, 当作 AGAIN (停等), 不作为有序关 (有序关看读侧 FIN)
/// @note TLS: 须先看 status_rw; WANT_* 时返回值可为 0 但未关; n<0 为致命错
/// @note 调用方义务: ERROR 后须 klb_socket_recv_closed + 通知上层; AGAIN 时保持 writing (队列非空)
typedef enum klb_socket_send_judge_e_
{
    KLB_SOCKET_SEND_DATA = 0,           ///< n>0: 已写出; 调用方推进偏移并继续 while
    KLB_SOCKET_SEND_AGAIN,              ///< 暂不可写且连接仍在 (非TLS: EAGAIN; TLS: WANT_*; 或 n==0)
    KLB_SOCKET_SEND_ERROR,              ///< 异常关闭或致命错
}klb_socket_send_judge_e;


/// @brief 判定一次 send 结果 (TLS / 非 TLS 内部分支; 写结束公共判定)
/// @param [in]  *p_socket          本次实际 send 的 socket
/// @param [in]  n                  klb_socket_send 返回值
/// @param [out] *p_status          可选; ERROR 时建议写入的 klb_socket_status_e; 可为 NULL
/// @return int                     klb_socket_send_judge_e
/// @note 返回 DATA: 继续 while 排空; AGAIN: 停等 (EAGAIN/WANT/n==0); ERROR: 关连接
/// @note 与读侧差异: 写路径 n==0 不表示有序关; 有序关看 recv_judge 的 EOF
/// @warning 本函数不执行 send, 不关闭 fd; 仅判定. n>0 时直接返回 DATA, 不写 p_status
KLB_API int klb_socket_send_judge(klb_socket_t* p_socket, int n, int* p_status);


/// @brief 设置 user data
KLB_API void klb_socket_set_udata(klb_socket_t* p_socket, void* p_udata);

/// @brief 获取 user data
KLB_API void* klb_socket_get_udata(klb_socket_t* p_socket);


/// @brief 关闭socket
KLB_API void klb_socket_close(klb_socket_fd fd);
#define KLB_SOCKET_CLOSE(FD_) {if(INVALID_SOCKET!=(FD_)){klb_socket_close((FD_));(FD_)=INVALID_SOCKET;}}


/// @brief 端口释放后立即就可以被再次使用
/// @param [in]  fd             socket fd
/// @return int 0.成功; 非0.失败
KLB_API int klb_socket_set_reuseaddr(klb_socket_fd fd);


/// @brief SO_REUSEPORT (多进程同口); 平台无此选项时返回非0
/// @param [in]  fd             socket fd
/// @return int 0.成功; 非0.失败
KLB_API int klb_socket_set_reuseport(klb_socket_fd fd);


/// @brief TCP_NODELAY
/// @param [in]  fd             socket fd
/// @param [in]  enable         true.开启; false.关闭
/// @return int 0.成功; 非0.失败
KLB_API int klb_socket_set_tcp_nodelay(klb_socket_fd fd, bool enable);


/// @brief SO_KEEPALIVE
/// @param [in]  fd             socket fd
/// @param [in]  enable         true.开启; false.关闭
/// @return int 0.成功; 非0.失败
KLB_API int klb_socket_set_keepalive(klb_socket_fd fd, bool enable);


/// @brief IPV6_V6ONLY
/// @param [in]  fd             AF_INET6 socket fd
/// @param [in]  v6only         true.仅 IPv6; false.双栈(接受 IPv4-mapped)
/// @return int 0.成功; 非0.失败
KLB_API int klb_socket_set_ipv6only(klb_socket_fd fd, bool v6only);


/// @brief 设置阻塞/非阻塞
/// @param [in]  fd             socket fd
/// @param [in]  blocking       true.阻塞; false.非阻塞
/// @return 无
KLB_API void klb_socket_set_block(klb_socket_fd fd, bool blocking);


/// @brief 使用TCP连接服务器 域名:端口
/// @param [in]  *p_host        域名: eg. "www.baidu.com", "127.0.0.1"
/// @param [in]  port           端口: eg. 80
/// @param [in]  time_out       超时(毫秒): 0. 表示创建非阻塞socket; 大于0. 表示创建阻塞socket并等待超时
/// @return klb_socket_fd INVALID_SOCKET, 大于0
KLB_API klb_socket_fd klb_socket_connect(const char* p_host, int port, int time_out);


/// @brief 使用TCP监听端口
/// @param [in]  port           端口: eg. 80
/// @param [in]  max_connect    监听列表最大数目
/// @return klb_socket_fd INVALID_SOCKET, 大于0
KLB_API klb_socket_fd klb_socket_listen(int port, int max_connect);


/// @brief 使用TCP监听端口 (指定绑定地址)
/// @param [in]  *p_addr        绑定地址 (AF_INET; sin_addr / sin_port 均已填好)
/// @param [in]  max_connect    监听列表最大数目
/// @return klb_socket_fd INVALID_SOCKET, 大于0
KLB_API klb_socket_fd klb_socket_listen_addr(const struct sockaddr_in* p_addr, int max_connect);


/// @brief 使用TCP监听端口 (sockaddr_storage; 支持 AF_INET / AF_INET6)
/// @param [in]  *p_addr        绑定地址
/// @param [in]  addr_len       p_addr 有效长度
/// @param [in]  max_connect    监听列表最大数目
/// @param [in]  reuseport      true 时尝试 SO_REUSEPORT
/// @param [in]  ipv6only       AF_INET6 时: true=仅 v6; false=双栈; AF_INET 忽略
/// @return klb_socket_fd INVALID_SOCKET, 大于0
KLB_API klb_socket_fd klb_socket_listen_sa(const struct sockaddr_storage* p_addr, socklen_t addr_len,
    int max_connect, bool reuseport, bool ipv6only);


/// @brief 解析 IPv4 绑定地址 (域名 / 点分十进制)
/// @param [in]  *p_host        主机; NULL/""/"0.0.0.0"/"*" 表示 INADDR_ANY
/// @param [in]  port           端口
/// @param [out] *p_addr        输出 AF_INET 地址
/// @return int 0.成功; 非0.失败
KLB_API int klb_socket_resolve_v4(const char* p_host, int port, struct sockaddr_in* p_addr);


/// @brief 解析绑定地址 (IPv4 / IPv6 / dual)
/// @param [in]  *p_host        主机; NULL/""/"*"/"0.0.0.0" 按 family 取任意地址; "::" 为 IPv6 任意
/// @param [in]  port           端口
/// @param [in]  family         AF_INET / AF_INET6 / AF_UNSPEC(dual, 通配时走 AF_INET6)
/// @param [out] *p_addr        输出地址
/// @param [out] *p_addr_len    输出地址长度
/// @return int 0.成功; 非0.失败
KLB_API int klb_socket_resolve_sa(const char* p_host, int port, int family,
    struct sockaddr_storage* p_addr, socklen_t* p_addr_len);


/// @brief 回读 socket 本地地址 (getsockname)
/// @param [in]  fd             socket fd
/// @param [out] *p_addr        输出本地地址 (AF_INET)
/// @return int 0.成功; 非0.失败
KLB_API int klb_socket_get_local_addr(klb_socket_fd fd, struct sockaddr_in* p_addr);


/// @brief 回读 socket 本地地址 (getsockname; AF_INET / AF_INET6)
/// @param [in]  fd             socket fd
/// @param [out] *p_addr        输出本地地址
/// @param [out] *p_addr_len    输出地址长度; 可为 NULL
/// @return int 0.成功; 非0.失败
KLB_API int klb_socket_get_local_sa(klb_socket_fd fd, struct sockaddr_storage* p_addr, socklen_t* p_addr_len);


/// @brief 回读 socket 对端地址 (getpeername; AF_INET / AF_INET6)
/// @param [in]  fd             socket fd
/// @param [out] *p_addr        输出对端地址
/// @param [out] *p_addr_len    输出地址长度; 可为 NULL
/// @return int 0.成功; 非0.失败
KLB_API int klb_socket_get_peer_sa(klb_socket_fd fd, struct sockaddr_storage* p_addr, socklen_t* p_addr_len);


/// @brief 将地址格式化为 ip 串 + 端口(主机序)
/// @param [in]  *p_addr        地址
/// @param [out] *p_ip          输出 ip 文本缓冲
/// @param [in]  ip_cap         p_ip 容量
/// @param [out] *p_port        输出端口; 可为 NULL
/// @return int 0.成功; 非0.失败
KLB_API int klb_socket_format_addr(const struct sockaddr_storage* p_addr, char* p_ip, int ip_cap, int* p_port);


/// @brief 将紧凑地址格式化为 ip 串 + 端口(主机序)
/// @param [in]  *p_addr        紧凑地址
/// @param [out] *p_ip          输出 ip 文本缓冲
/// @param [in]  ip_cap         p_ip 容量
/// @param [out] *p_port        输出端口; 可为 NULL
/// @return int 0.成功; 非0.失败
KLB_API int klb_socket_format_addr_t(const klb_socket_addr_t* p_addr, char* p_ip, int ip_cap, int* p_port);


/// @brief AF_* 转项目地址族
/// @param [in]  af             AF_INET / AF_INET6 / 其它
/// @return int klb_socket_family_e
KLB_API int klb_socket_af_to_family(int af);


/// @brief 项目地址族转 AF_*
/// @param [in]  family         klb_socket_family_e
/// @return int AF_INET / AF_INET6 / AF_UNSPEC
KLB_API int klb_socket_family_to_af(int family);


/// @brief 使用TCP监听unix节点
/// @param [in]  *p_path        unix节点路径
/// @param [in]  max_connect    监听列表最大数目
/// @return klb_socket_fd INVALID_SOCKET, 大于0
KLB_API klb_socket_fd klb_socket_listen_unix(const char* p_path, int max_connect);


/// @brief 连接 unix 节点(SOCK_STREAM)
/// @param [in]  *p_path        unix节点路径
/// @param [in]  time_out       超时毫秒; <=0 表示非阻塞尝试(不等待可写完成)
/// @return klb_socket_fd INVALID_SOCKET, 大于0
/// @note Win 不支持, 恒返回 INVALID_SOCKET
KLB_API klb_socket_fd klb_socket_connect_unix(const char* p_path, int time_out);


/// @brief 接收监听到的连接
/// @param [in]  fd_listen      监听fd
/// @param [out] *p_addr        输出监听到的地址
/// @return klb_socket_fd INVALID_SOCKET, 大于0
KLB_API klb_socket_fd klb_socket_accept(klb_socket_fd fd_listen, struct sockaddr_in* p_addr);


/// @brief 接收监听到的连接 (sockaddr_storage)
/// @param [in]  fd_listen      监听fd
/// @param [out] *p_addr        输出对端地址
/// @param [out] *p_addr_len    输出地址长度; 可为 NULL
/// @return klb_socket_fd INVALID_SOCKET, 大于0
KLB_API klb_socket_fd klb_socket_accept_sa(klb_socket_fd fd_listen, struct sockaddr_storage* p_addr, socklen_t* p_addr_len);


/// @brief 创建一个异步socket
/// @param [in]  fd             socket fd
/// @return klb_socket_t* NULL.创建失败; 非NULL
/// @note UDP 业务创建/收发见 `klb_socket_udp.h`(`klb_socket_udp_open` 等); 勿自建旁路
KLB_API klb_socket_t* klb_socket_async_create(klb_socket_fd fd);


/// @brief 回读地址族
/// @param [in]  *p_socket       socket 对象
/// @return int klb_socket_family_e
KLB_API int klb_socket_family(klb_socket_t* p_socket);


/// @brief 回读 socket 类型
/// @param [in]  *p_socket       socket 对象
/// @return int klb_socket_type_e
KLB_API int klb_socket_type(klb_socket_t* p_socket);


/// @brief 回读本端地址 (对象缓存; bind / connect 后有效)
/// @param [in]  *p_socket       socket 对象
/// @param [out] *p_addr        输出地址
/// @return int 0.成功; 非0.失败
KLB_API int klb_socket_local_addr(klb_socket_t* p_socket, klb_socket_addr_t* p_addr);


/// @brief 回读对端地址 (对象缓存; connect 或最近一次 recvfrom)
/// @param [in]  *p_socket       socket 对象
/// @param [out] *p_addr        输出地址
/// @return int 0.成功; 非0.失败
KLB_API int klb_socket_peer_addr(klb_socket_t* p_socket, klb_socket_addr_t* p_addr);


/// @brief 回读本端端口 (主机序)
/// @param [in]  *p_socket       socket 对象
/// @return int 端口; 小于 0 表示失败
KLB_API int klb_socket_local_port(klb_socket_t* p_socket);


/// @brief 回读对端端口 (主机序; 对象缓存)
/// @param [in]  *p_socket       socket 对象
/// @return int 端口; 小于 0 表示失败
KLB_API int klb_socket_peer_port(klb_socket_t* p_socket);


#ifdef __cplusplus
}
#endif

#endif // __KLB_SOCKET_H__

// end
