// Doc Encode : UTF-8 BOM, Unix(LF)
// klb_netcode 消息码描述串
#include "klbnet/klb_netcode.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#else
#include <errno.h>
#endif


int klb_netcode_from_syserr(int syserr)
{
    if (0 == syserr)
    {
        return KLB_NETCODE_EPERM;
    }

#ifdef _WIN32
    if (WSAEINTR == syserr)
    {
        return KLB_NETCODE_EINTR;
    }
    else if (WSAEWOULDBLOCK == syserr)
    {
        return KLB_NETCODE_EAGAIN;
    }
    else if (WSAEINVAL == syserr)
    {
        return KLB_NETCODE_EINVAL;
    }
    else if (WSAEACCES == syserr)
    {
        return KLB_NETCODE_EACCES;
    }
    else if (WSAEADDRINUSE == syserr)
    {
        return KLB_NETCODE_EADDRINUSE;
    }
    else if (WSAEADDRNOTAVAIL == syserr)
    {
        return KLB_NETCODE_EADDRNOTAVAIL;
    }
    else if (WSAENETUNREACH == syserr)
    {
        return KLB_NETCODE_ENETUNREACH;
    }
    else if (WSAECONNABORTED == syserr)
    {
        return KLB_NETCODE_ECONNABORTED;
    }
    else if (WSAECONNRESET == syserr)
    {
        return KLB_NETCODE_ECONNRESET;
    }
    else if (WSAENOBUFS == syserr)
    {
        return KLB_NETCODE_ENOBUFS;
    }
    else if (WSAEISCONN == syserr)
    {
        return KLB_NETCODE_EISCONN;
    }
    else if (WSAENOTCONN == syserr)
    {
        return KLB_NETCODE_ENOTCONN;
    }
    else if (WSAETIMEDOUT == syserr)
    {
        return KLB_NETCODE_ETIMEDOUT;
    }
    else if (WSAECONNREFUSED == syserr)
    {
        return KLB_NETCODE_ECONNREFUSED;
    }
    else if (WSAEHOSTUNREACH == syserr)
    {
        return KLB_NETCODE_EHOSTUNREACH;
    }
    else if (WSAEALREADY == syserr)
    {
        return KLB_NETCODE_EALREADY;
    }
    else if (WSAEINPROGRESS == syserr)
    {
        return KLB_NETCODE_EINPROGRESS;
    }
    else if (WSAEAFNOSUPPORT == syserr)
    {
        return KLB_NETCODE_EAFNOSUPPORT;
    }
    else if (WSAEMSGSIZE == syserr)
    {
        return KLB_NETCODE_EMSGSIZE;
    }

    return KLB_NETCODE_EIO;
#else
    if (EPERM == syserr)
    {
        return KLB_NETCODE_EPERM;
    }
    else if (ENOENT == syserr)
    {
        return KLB_NETCODE_ENOENT;
    }
    else if (EINTR == syserr)
    {
        return KLB_NETCODE_EINTR;
    }
    else if (EIO == syserr)
    {
        return KLB_NETCODE_EIO;
    }
    else if (EBADF == syserr)
    {
        return KLB_NETCODE_EBADF;
    }
    else if (EAGAIN == syserr)
    {
        return KLB_NETCODE_EAGAIN;
    }
    else if (ENOMEM == syserr)
    {
        return KLB_NETCODE_ENOMEM;
    }
    else if (EACCES == syserr)
    {
        return KLB_NETCODE_EACCES;
    }
    else if (EBUSY == syserr)
    {
        return KLB_NETCODE_EBUSY;
    }
    else if (EEXIST == syserr)
    {
        return KLB_NETCODE_EEXIST;
    }
    else if (EINVAL == syserr)
    {
        return KLB_NETCODE_EINVAL;
    }
    else if (EPIPE == syserr)
    {
        return KLB_NETCODE_EPIPE;
    }
    else if (EMSGSIZE == syserr)
    {
        return KLB_NETCODE_EMSGSIZE;
    }
    else if (EOPNOTSUPP == syserr)
    {
        return KLB_NETCODE_EOPNOTSUPP;
    }
    else if (EAFNOSUPPORT == syserr)
    {
        return KLB_NETCODE_EAFNOSUPPORT;
    }
    else if (EADDRINUSE == syserr)
    {
        return KLB_NETCODE_EADDRINUSE;
    }
    else if (EADDRNOTAVAIL == syserr)
    {
        return KLB_NETCODE_EADDRNOTAVAIL;
    }
    else if (ENETUNREACH == syserr)
    {
        return KLB_NETCODE_ENETUNREACH;
    }
    else if (ECONNABORTED == syserr)
    {
        return KLB_NETCODE_ECONNABORTED;
    }
    else if (ECONNRESET == syserr)
    {
        return KLB_NETCODE_ECONNRESET;
    }
    else if (ENOBUFS == syserr)
    {
        return KLB_NETCODE_ENOBUFS;
    }
    else if (EISCONN == syserr)
    {
        return KLB_NETCODE_EISCONN;
    }
    else if (ENOTCONN == syserr)
    {
        return KLB_NETCODE_ENOTCONN;
    }
    else if (ETIMEDOUT == syserr)
    {
        return KLB_NETCODE_ETIMEDOUT;
    }
    else if (ECONNREFUSED == syserr)
    {
        return KLB_NETCODE_ECONNREFUSED;
    }
    else if (EHOSTUNREACH == syserr)
    {
        return KLB_NETCODE_EHOSTUNREACH;
    }
    else if (EALREADY == syserr)
    {
        return KLB_NETCODE_EALREADY;
    }
    else if (EINPROGRESS == syserr)
    {
        return KLB_NETCODE_EINPROGRESS;
    }
    else if (ECANCELED == syserr)
    {
        return KLB_NETCODE_ECANCELED;
    }

    return KLB_NETCODE_EIO;
#endif
}


int klb_netcode_last_syserr(void)
{
#ifdef _WIN32
    return klb_netcode_from_syserr((int)WSAGetLastError());
#else
    return klb_netcode_from_syserr(errno);
#endif
}


const char* klb_netcode_strerror(int code)
{
    switch (code)
    {
    // 负段 (-errno)
    case KLB_NETCODE_EPERM:
        return "[klbnet] Error";
    case KLB_NETCODE_ENOENT:
        return "[klbnet] No such file or directory";
    case KLB_NETCODE_EINTR:
        return "[klbnet] Interrupted system call";
    case KLB_NETCODE_EIO:
        return "[klbnet] I/O error";
    case KLB_NETCODE_EBADF:
        return "[klbnet] Bad file descriptor";
    case KLB_NETCODE_EAGAIN:
        return "[klbnet] Try again";
    case KLB_NETCODE_ENOMEM:
        return "[klbnet] Not enough resources";
    case KLB_NETCODE_EACCES:
        return "[klbnet] Permission denied";
    case KLB_NETCODE_EBUSY:
        return "[klbnet] Device or resource busy";
    case KLB_NETCODE_EEXIST:
        return "[klbnet] Already exists";
    case KLB_NETCODE_EINVAL:
        return "[klbnet] Invalid argument";
    case KLB_NETCODE_EPIPE:
        return "[klbnet] Broken pipe";
    case KLB_NETCODE_EMSGSIZE:
        return "[klbnet] Message too long";
    case KLB_NETCODE_EOPNOTSUPP:
        return "[klbnet] Not supported";
    case KLB_NETCODE_EAFNOSUPPORT:
        return "[klbnet] Address family not supported";
    case KLB_NETCODE_EADDRINUSE:
        return "[klbnet] Address already in use";
    case KLB_NETCODE_EADDRNOTAVAIL:
        return "[klbnet] Cannot assign requested address";
    case KLB_NETCODE_ENETUNREACH:
        return "[klbnet] Network is unreachable";
    case KLB_NETCODE_ECONNABORTED:
        return "[klbnet] Software caused connection abort";
    case KLB_NETCODE_ECONNRESET:
        return "[klbnet] Connection reset by peer";
    case KLB_NETCODE_ENOBUFS:
        return "[klbnet] No buffer space available";
    case KLB_NETCODE_EISCONN:
        return "[klbnet] Transport endpoint is already connected";
    case KLB_NETCODE_ENOTCONN:
        return "[klbnet] Transport endpoint is not connected";
    case KLB_NETCODE_ETIMEDOUT:
        return "[klbnet] Connection timed out";
    case KLB_NETCODE_ECONNREFUSED:
        return "[klbnet] Connection refused";
    case KLB_NETCODE_EHOSTUNREACH:
        return "[klbnet] No route to host";
    case KLB_NETCODE_EALREADY:
        return "[klbnet] Operation already in progress";
    case KLB_NETCODE_EINPROGRESS:
        return "[klbnet] Operation now in progress";
    case KLB_NETCODE_ECANCELED:
        return "[klbnet] Operation canceled";

    // socket 段
    case KLB_SOCKET_OK:
        return "[klbnet] Success";
    case KLB_SOCKET_DISCONNECT:
        return "[klbnet] Connection disconnected";
    case KLB_SOCKET_TIMEOUT:
        return "[klbnet] Timeout";
    case KLB_SOCKET_CLOSED:
        return "[klbnet] Connection closed";
    case KLB_SOCKET_ERR_PROTOCOL:
        return "[klbnet] Protocol error";
    case KLB_SOCKET_ERR_RESET:
        return "[klbnet] Connection reset";
    case KLB_SOCKET_ERR_REFUSED:
        return "[klbnet] Connection refused";
    case KLB_SOCKET_ERR_UNREACH:
        return "[klbnet] Network unreachable";
    case KLB_SOCKET_ERR_ABORT:
        return "[klbnet] Connection aborted";
    case KLB_SOCKET_ERR_PIPE:
        return "[klbnet] Broken pipe";
    case KLB_SOCKET_ERR_INVAL:
        return "[klbnet] Invalid argument";
    case KLB_SOCKET_ERR_ACCESS:
        return "[klbnet] Permission denied";
    case KLB_SOCKET_CLOSEING:
        return "[klbnet] Closing";
    case KLB_SOCKET_CONNECT:
        return "[klbnet] Connected";

    // H HTTP status
    case KLB_NETCODE_HTTP_CONTINUE:
        return "[klbnet] HTTP 100 Continue";
    case KLB_NETCODE_HTTP_SWITCHING_PROTOCOLS:
        return "[klbnet] HTTP 101 Switching Protocols";
    case KLB_NETCODE_HTTP_PROCESSING:
        return "[klbnet] HTTP 102 Processing";
    case KLB_NETCODE_HTTP_OK:
        return "[klbnet] HTTP 200 OK";
    case KLB_NETCODE_HTTP_CREATED:
        return "[klbnet] HTTP 201 Created";
    case KLB_NETCODE_HTTP_ACCEPTED:
        return "[klbnet] HTTP 202 Accepted";
    case KLB_NETCODE_HTTP_NON_AUTHORITATIVE_INFORMATION:
        return "[klbnet] HTTP 203 Non-Authoritative Information";
    case KLB_NETCODE_HTTP_NO_CONTENT:
        return "[klbnet] HTTP 204 No Content";
    case KLB_NETCODE_HTTP_RESET_CONTENT:
        return "[klbnet] HTTP 205 Reset Content";
    case KLB_NETCODE_HTTP_PARTIAL_CONTENT:
        return "[klbnet] HTTP 206 Partial Content";
    case KLB_NETCODE_HTTP_MULTI_STATUS:
        return "[klbnet] HTTP 207 Multi-Status";
    case KLB_NETCODE_HTTP_ALREADY_REPORTED:
        return "[klbnet] HTTP 208 Already Reported";
    case KLB_NETCODE_HTTP_IM_USED:
        return "[klbnet] HTTP 226 IM Used";
    case KLB_NETCODE_HTTP_MULTIPLE_CHOICES:
        return "[klbnet] HTTP 300 Multiple Choices";
    case KLB_NETCODE_HTTP_MOVED_PERMANENTLY:
        return "[klbnet] HTTP 301 Moved Permanently";
    case KLB_NETCODE_HTTP_FOUND:
        return "[klbnet] HTTP 302 Found";
    case KLB_NETCODE_HTTP_SEE_OTHER:
        return "[klbnet] HTTP 303 See Other";
    case KLB_NETCODE_HTTP_NOT_MODIFIED:
        return "[klbnet] HTTP 304 Not Modified";
    case KLB_NETCODE_HTTP_USE_PROXY:
        return "[klbnet] HTTP 305 Use Proxy";
    case KLB_NETCODE_HTTP_TEMPORARY_REDIRECT:
        return "[klbnet] HTTP 307 Temporary Redirect";
    case KLB_NETCODE_HTTP_PERMANENT_REDIRECT:
        return "[klbnet] HTTP 308 Permanent Redirect";
    case KLB_NETCODE_HTTP_BAD_REQUEST:
        return "[klbnet] HTTP 400 Bad Request";
    case KLB_NETCODE_HTTP_UNAUTHORIZED:
        return "[klbnet] HTTP 401 Unauthorized";
    case KLB_NETCODE_HTTP_PAYMENT_REQUIRED:
        return "[klbnet] HTTP 402 Payment Required";
    case KLB_NETCODE_HTTP_FORBIDDEN:
        return "[klbnet] HTTP 403 Forbidden";
    case KLB_NETCODE_HTTP_NOT_FOUND:
        return "[klbnet] HTTP 404 Not Found";
    case KLB_NETCODE_HTTP_METHOD_NOT_ALLOWED:
        return "[klbnet] HTTP 405 Method Not Allowed";
    case KLB_NETCODE_HTTP_NOT_ACCEPTABLE:
        return "[klbnet] HTTP 406 Not Acceptable";
    case KLB_NETCODE_HTTP_PROXY_AUTHENTICATION_REQUIRED:
        return "[klbnet] HTTP 407 Proxy Authentication Required";
    case KLB_NETCODE_HTTP_REQUEST_TIMEOUT:
        return "[klbnet] HTTP 408 Request Timeout";
    case KLB_NETCODE_HTTP_CONFLICT:
        return "[klbnet] HTTP 409 Conflict";
    case KLB_NETCODE_HTTP_GONE:
        return "[klbnet] HTTP 410 Gone";
    case KLB_NETCODE_HTTP_LENGTH_REQUIRED:
        return "[klbnet] HTTP 411 Length Required";
    case KLB_NETCODE_HTTP_PRECONDITION_FAILED:
        return "[klbnet] HTTP 412 Precondition Failed";
    case KLB_NETCODE_HTTP_PAYLOAD_TOO_LARGE:
        return "[klbnet] HTTP 413 Payload Too Large";
    case KLB_NETCODE_HTTP_URI_TOO_LONG:
        return "[klbnet] HTTP 414 URI Too Long";
    case KLB_NETCODE_HTTP_UNSUPPORTED_MEDIA_TYPE:
        return "[klbnet] HTTP 415 Unsupported Media Type";
    case KLB_NETCODE_HTTP_RANGE_NOT_SATISFIABLE:
        return "[klbnet] HTTP 416 Range Not Satisfiable";
    case KLB_NETCODE_HTTP_EXPECTATION_FAILED:
        return "[klbnet] HTTP 417 Expectation Failed";
    case KLB_NETCODE_HTTP_MISDIRECTED_REQUEST:
        return "[klbnet] HTTP 421 Misdirected Request";
    case KLB_NETCODE_HTTP_UNPROCESSABLE_ENTITY:
        return "[klbnet] HTTP 422 Unprocessable Entity";
    case KLB_NETCODE_HTTP_LOCKED:
        return "[klbnet] HTTP 423 Locked";
    case KLB_NETCODE_HTTP_FAILED_DEPENDENCY:
        return "[klbnet] HTTP 424 Failed Dependency";
    case KLB_NETCODE_HTTP_UPGRADE_REQUIRED:
        return "[klbnet] HTTP 426 Upgrade Required";
    case KLB_NETCODE_HTTP_PRECONDITION_REQUIRED:
        return "[klbnet] HTTP 428 Precondition Required";
    case KLB_NETCODE_HTTP_TOO_MANY_REQUESTS:
        return "[klbnet] HTTP 429 Too Many Requests";
    case KLB_NETCODE_HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE:
        return "[klbnet] HTTP 431 Request Header Fields Too Large";
    case KLB_NETCODE_HTTP_UNAVAILABLE_FOR_LEGAL_REASONS:
        return "[klbnet] HTTP 451 Unavailable For Legal Reasons";
    case KLB_NETCODE_HTTP_INTERNAL_SERVER_ERROR:
        return "[klbnet] HTTP 500 Internal Server Error";
    case KLB_NETCODE_HTTP_NOT_IMPLEMENTED:
        return "[klbnet] HTTP 501 Not Implemented";
    case KLB_NETCODE_HTTP_BAD_GATEWAY:
        return "[klbnet] HTTP 502 Bad Gateway";
    case KLB_NETCODE_HTTP_SERVICE_UNAVAILABLE:
        return "[klbnet] HTTP 503 Service Unavailable";
    case KLB_NETCODE_HTTP_GATEWAY_TIMEOUT:
        return "[klbnet] HTTP 504 Gateway Timeout";
    case KLB_NETCODE_HTTP_VERSION_NOT_SUPPORTED:
        return "[klbnet] HTTP 505 HTTP Version Not Supported";
    case KLB_NETCODE_HTTP_VARIANT_ALSO_NEGOTIATES:
        return "[klbnet] HTTP 506 Variant Also Negotiates";
    case KLB_NETCODE_HTTP_INSUFFICIENT_STORAGE:
        return "[klbnet] HTTP 507 Insufficient Storage";
    case KLB_NETCODE_HTTP_LOOP_DETECTED:
        return "[klbnet] HTTP 508 Loop Detected";
    case KLB_NETCODE_HTTP_NOT_EXTENDED:
        return "[klbnet] HTTP 510 Not Extended";
    case KLB_NETCODE_HTTP_NETWORK_AUTHENTICATION_REQUIRED:
        return "[klbnet] HTTP 511 Network Authentication Required";

    // W WebSocket close
    case KLB_NETCODE_WS_NORMAL:
        return "[klbnet] WS 1000 Normal Closure";
    case KLB_NETCODE_WS_GOING_AWAY:
        return "[klbnet] WS 1001 Going Away";
    case KLB_NETCODE_WS_PROTO:
        return "[klbnet] WS 1002 Protocol Error";
    case KLB_NETCODE_WS_UNSUPPORTED:
        return "[klbnet] WS 1003 Unsupported Data";
    case KLB_NETCODE_WS_NOSTATUS:
        return "[klbnet] WS 1005 No Status Rcvd (reserved)";
    case KLB_NETCODE_WS_ABNORMAL:
        return "[klbnet] WS 1006 Abnormal Closure (reserved)";
    case KLB_NETCODE_WS_UTF8:
        return "[klbnet] WS 1007 Invalid frame payload data";
    case KLB_NETCODE_WS_POLICY:
        return "[klbnet] WS 1008 Policy Violation";
    case KLB_NETCODE_WS_TOOBIG:
        return "[klbnet] WS 1009 Message Too Big";
    case KLB_NETCODE_WS_MANDATORY_EXT:
        return "[klbnet] WS 1010 Mandatory Ext.";
    case KLB_NETCODE_WS_INTERNAL:
        return "[klbnet] WS 1011 Internal Server Error";
    case KLB_NETCODE_WS_SERVICE_RESTART:
        return "[klbnet] WS 1012 Service Restart";
    case KLB_NETCODE_WS_TRY_AGAIN:
        return "[klbnet] WS 1013 Try Again Later";
    case KLB_NETCODE_WS_BAD_GATEWAY:
        return "[klbnet] WS 1014 Bad Gateway";
    case KLB_NETCODE_WS_TLS_HANDSHAKE:
        return "[klbnet] WS 1015 TLS handshake (reserved)";

    // A 流控
    case KLB_NETCODE_WBUF_EMPTY:
        return "[klbnet] Write buffer empty";
    case KLB_NETCODE_RBUF_FULL:
        return "[klbnet] Read buffer full";
    case KLB_NETCODE_WBUF_FULL:
        return "[klbnet] Write buffer full";

    // B 连接
    case KLB_NETCODE_CONNECT_FAIL:
        return "[klbnet] Connect failed";
    case KLB_NETCODE_CONNECT_REFUSED:
        return "[klbnet] Connect refused";
    case KLB_NETCODE_DNS_FAIL:
        return "[klbnet] DNS resolve failed";
    case KLB_NETCODE_CONNECT_TIMEOUT:
        return "[klbnet] Connect timeout";
    case KLB_NETCODE_CONNECT_HOST:
        return "[klbnet] Host unreachable";

    // C TLS
    case KLB_NETCODE_TLS_HANDSHAKE:
        return "[klbnet] TLS handshake failed";
    case KLB_NETCODE_TLS_CERT:
        return "[klbnet] TLS certificate error";
    case KLB_NETCODE_TLS_WANT_IO:
        return "[klbnet] TLS want read/write";
    case KLB_NETCODE_TLS_ALERT:
        return "[klbnet] TLS alert";
    case KLB_NETCODE_TLS_VERIFY:
        return "[klbnet] TLS verify failed";

    // D listen / mux
    case KLB_NETCODE_BIND_FAIL:
        return "[klbnet] Bind failed";
    case KLB_NETCODE_LISTEN_FAIL:
        return "[klbnet] Listen failed";
    case KLB_NETCODE_ACCEPT_FAIL:
        return "[klbnet] Accept failed";
    case KLB_NETCODE_QUEUE_OVERFLOW:
        return "[klbnet] Queue overflow";
    case KLB_NETCODE_PROBE_FAIL:
        return "[klbnet] Protocol probe failed";
    case KLB_NETCODE_ADDR_IN_USE:
        return "[klbnet] Address already in use";
    case KLB_NETCODE_ADDR_NOT_AVAIL:
        return "[klbnet] Address not available";

    // E 协议传输
    case KLB_NETCODE_PROTO_PARSE:
        return "[klbnet] Protocol parse error";
    case KLB_NETCODE_PROTO_TOO_LARGE:
        return "[klbnet] Protocol message too large";
    case KLB_NETCODE_PROTO_UNEXPECTED:
        return "[klbnet] Unexpected protocol message";
    case KLB_NETCODE_PROTO_VERSION:
        return "[klbnet] Unsupported protocol version";

    // G 会话/限额
    case KLB_NETCODE_CANCELED:
        return "[klbnet] Operation canceled";
    case KLB_NETCODE_LIMIT:
        return "[klbnet] Limit exceeded";

    default:
        return "[klbnet] Unknown error";
    }
}

// end
