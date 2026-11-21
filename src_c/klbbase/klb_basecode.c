// Doc Encode : UTF-8 BOM, Unix(LF)
// klb_basecode 消息码描述串 / 系统错误映射
#include "klbbase/klb_basecode.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <errno.h>
#endif


int klb_basecode_from_syserr(int syserr)
{
    if (0 == syserr)
    {
        return KLB_BASECODE_ERR;
    }

#ifdef _WIN32
    if (ERROR_FILE_NOT_FOUND == syserr || ERROR_PATH_NOT_FOUND == syserr)
    {
        return KLB_BASECODE_NOENT;
    }
    else if (ERROR_ACCESS_DENIED == syserr)
    {
        return KLB_BASECODE_ACCES;
    }
    else if (ERROR_NOT_ENOUGH_MEMORY == syserr || ERROR_OUTOFMEMORY == syserr)
    {
        return KLB_BASECODE_NOMEM;
    }
    else if (ERROR_INVALID_PARAMETER == syserr || ERROR_INVALID_HANDLE == syserr)
    {
        return KLB_BASECODE_INVAL;
    }
    else if (ERROR_BUSY == syserr)
    {
        return KLB_BASECODE_BUSY;
    }
    else if (ERROR_ALREADY_EXISTS == syserr || ERROR_FILE_EXISTS == syserr)
    {
        return KLB_BASECODE_EXIST;
    }
    else if (ERROR_DISK_FULL == syserr || ERROR_HANDLE_DISK_FULL == syserr)
    {
        return KLB_BASECODE_NOSPC;
    }
    else if (ERROR_OPERATION_ABORTED == syserr)
    {
        return KLB_BASECODE_ECANCELED;
    }
    else if (WAIT_TIMEOUT == syserr || ERROR_TIMEOUT == syserr)
    {
        return KLB_BASECODE_TIMEDOUT;
    }
    else if (ERROR_NOT_SUPPORTED == syserr)
    {
        return KLB_BASECODE_NOTSUP;
    }

    return KLB_BASECODE_IO;
#else
    if (EPERM == syserr)
    {
        return KLB_BASECODE_ERR;
    }
    else if (ENOENT == syserr)
    {
        return KLB_BASECODE_NOENT;
    }
    else if (EINTR == syserr)
    {
        return KLB_BASECODE_INTR;
    }
    else if (EIO == syserr)
    {
        return KLB_BASECODE_IO;
    }
    else if (EBADF == syserr)
    {
        return KLB_BASECODE_BADF;
    }
    else if (EAGAIN == syserr)
    {
        return KLB_BASECODE_AGAIN;
    }
    else if (ENOMEM == syserr)
    {
        return KLB_BASECODE_NOMEM;
    }
    else if (EACCES == syserr)
    {
        return KLB_BASECODE_ACCES;
    }
    else if (EBUSY == syserr)
    {
        return KLB_BASECODE_BUSY;
    }
    else if (EEXIST == syserr)
    {
        return KLB_BASECODE_EXIST;
    }
    else if (EINVAL == syserr)
    {
        return KLB_BASECODE_INVAL;
    }
    else if (ENOSPC == syserr)
    {
        return KLB_BASECODE_NOSPC;
    }
    else if (EPIPE == syserr)
    {
        return KLB_BASECODE_PIPE;
    }
    else if (EMSGSIZE == syserr)
    {
        return KLB_BASECODE_MSGSIZE;
    }
    else if (EOPNOTSUPP == syserr)
    {
        return KLB_BASECODE_NOTSUP;
    }
    else if (EAFNOSUPPORT == syserr)
    {
        return KLB_BASECODE_AFNOSUPPORT;
    }
    else if (EADDRINUSE == syserr)
    {
        return KLB_BASECODE_ADDRINUSE;
    }
    else if (EADDRNOTAVAIL == syserr)
    {
        return KLB_BASECODE_ADDRNOTAVAIL;
    }
    else if (ENETUNREACH == syserr)
    {
        return KLB_BASECODE_NETUNREACH;
    }
    else if (ECONNABORTED == syserr)
    {
        return KLB_BASECODE_CONNABORTED;
    }
    else if (ECONNRESET == syserr)
    {
        return KLB_BASECODE_CONNRESET;
    }
    else if (ENOBUFS == syserr)
    {
        return KLB_BASECODE_NOBUFS;
    }
    else if (EISCONN == syserr)
    {
        return KLB_BASECODE_ISCONN;
    }
    else if (ENOTCONN == syserr)
    {
        return KLB_BASECODE_NOTCONN;
    }
    else if (ETIMEDOUT == syserr)
    {
        return KLB_BASECODE_TIMEDOUT;
    }
    else if (ECONNREFUSED == syserr)
    {
        return KLB_BASECODE_CONNREFUSED;
    }
    else if (EHOSTUNREACH == syserr)
    {
        return KLB_BASECODE_HOSTUNREACH;
    }
    else if (EALREADY == syserr)
    {
        return KLB_BASECODE_ALREADY;
    }
    else if (EINPROGRESS == syserr)
    {
        return KLB_BASECODE_INPROGRESS;
    }
    else if (ECANCELED == syserr)
    {
        return KLB_BASECODE_ECANCELED;
    }

    return KLB_BASECODE_IO;
#endif
}


int klb_basecode_last_syserr(void)
{
#ifdef _WIN32
    return klb_basecode_from_syserr((int)GetLastError());
#else
    return klb_basecode_from_syserr(errno);
#endif
}


const char* klb_basecode_strerror(int code)
{
    switch (code)
    {
    // 负段 (-errno)
    case KLB_BASECODE_ERR:
        return "[klbbase] Error";
    case KLB_BASECODE_NOENT:
        return "[klbbase] No such file or directory";
    case KLB_BASECODE_INTR:
        return "[klbbase] Interrupted system call";
    case KLB_BASECODE_IO:
        return "[klbbase] I/O error";
    case KLB_BASECODE_BADF:
        return "[klbbase] Bad file descriptor";
    case KLB_BASECODE_AGAIN:
        return "[klbbase] Try again";
    case KLB_BASECODE_NOMEM:
        return "[klbbase] Not enough resources";
    case KLB_BASECODE_ACCES:
        return "[klbbase] Permission denied";
    case KLB_BASECODE_BUSY:
        return "[klbbase] Device or resource busy";
    case KLB_BASECODE_EXIST:
        return "[klbbase] Already exists";
    case KLB_BASECODE_INVAL:
        return "[klbbase] Invalid argument";
    case KLB_BASECODE_NOSPC:
        return "[klbbase] No space left";
    case KLB_BASECODE_PIPE:
        return "[klbbase] Broken pipe";
    case KLB_BASECODE_MSGSIZE:
        return "[klbbase] Message too long";
    case KLB_BASECODE_NOTSUP:
        return "[klbbase] Not supported";
    case KLB_BASECODE_AFNOSUPPORT:
        return "[klbbase] Address family not supported";
    case KLB_BASECODE_ADDRINUSE:
        return "[klbbase] Address already in use";
    case KLB_BASECODE_ADDRNOTAVAIL:
        return "[klbbase] Cannot assign requested address";
    case KLB_BASECODE_NETUNREACH:
        return "[klbbase] Network is unreachable";
    case KLB_BASECODE_CONNABORTED:
        return "[klbbase] Software caused connection abort";
    case KLB_BASECODE_CONNRESET:
        return "[klbbase] Connection reset by peer";
    case KLB_BASECODE_NOBUFS:
        return "[klbbase] No buffer space available";
    case KLB_BASECODE_ISCONN:
        return "[klbbase] Transport endpoint is already connected";
    case KLB_BASECODE_NOTCONN:
        return "[klbbase] Transport endpoint is not connected";
    case KLB_BASECODE_TIMEDOUT:
        return "[klbbase] Connection timed out";
    case KLB_BASECODE_CONNREFUSED:
        return "[klbbase] Connection refused";
    case KLB_BASECODE_HOSTUNREACH:
        return "[klbbase] No route to host";
    case KLB_BASECODE_ALREADY:
        return "[klbbase] Operation already in progress";
    case KLB_BASECODE_INPROGRESS:
        return "[klbbase] Operation now in progress";
    case KLB_BASECODE_ECANCELED:
        return "[klbbase] Operation canceled";

    // 0
    case KLB_BASECODE_OK:
        return "[klbbase] Success";

    // A 通用
    case KLB_BASECODE_STATE:
        return "[klbbase] Bad state";
    case KLB_BASECODE_RANGE:
        return "[klbbase] Out of range";
    case KLB_BASECODE_OVERFLOW:
        return "[klbbase] Overflow";
    case KLB_BASECODE_CANCELED:
        return "[klbbase] Operation canceled";
    case KLB_BASECODE_LIMIT:
        return "[klbbase] Limit exceeded";
    case KLB_BASECODE_NOTIMPL:
        return "[klbbase] Not implemented";

    // B string
    case KLB_BASECODE_STRING_TRUNC:
        return "[klbbase] String truncated";
    case KLB_BASECODE_STRING_EMPTY:
        return "[klbbase] Empty string";

    // C package
    case KLB_BASECODE_PACKAGE_BAD:
        return "[klbbase] Bad package";
    case KLB_BASECODE_PACKAGE_NOENT:
        return "[klbbase] Package entry not found";
    case KLB_BASECODE_PACKAGE_IO:
        return "[klbbase] Package I/O error";

    // D mnp/smp
    case KLB_BASECODE_MNP_PROTO:
        return "[klbbase] MNP/SMP protocol error";
    case KLB_BASECODE_MNP_STATE:
        return "[klbbase] MNP/SMP bad state";
    case KLB_BASECODE_MNP_PEER:
        return "[klbbase] MNP/SMP peer error";

    // E codec/crypto
    case KLB_BASECODE_CODEC_PROTO:
        return "[klbbase] Codec protocol error";
    case KLB_BASECODE_CODEC_NOSUP:
        return "[klbbase] Codec not supported";
    case KLB_BASECODE_CRYPTO_FAIL:
        return "[klbbase] Crypto failed";

    default:
        return "[klbbase] Unknown error";
    }
}

// end
