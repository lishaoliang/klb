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
        return KLB_BASECODE_EPERM;
    }

#ifdef _WIN32
    if (ERROR_FILE_NOT_FOUND == syserr || ERROR_PATH_NOT_FOUND == syserr)
    {
        return KLB_BASECODE_ENOENT;
    }
    else if (ERROR_ACCESS_DENIED == syserr)
    {
        return KLB_BASECODE_EACCES;
    }
    else if (ERROR_NOT_ENOUGH_MEMORY == syserr || ERROR_OUTOFMEMORY == syserr)
    {
        return KLB_BASECODE_ENOMEM;
    }
    else if (ERROR_INVALID_PARAMETER == syserr || ERROR_INVALID_HANDLE == syserr)
    {
        return KLB_BASECODE_EINVAL;
    }
    else if (ERROR_BUSY == syserr)
    {
        return KLB_BASECODE_EBUSY;
    }
    else if (ERROR_ALREADY_EXISTS == syserr || ERROR_FILE_EXISTS == syserr)
    {
        return KLB_BASECODE_EEXIST;
    }
    else if (ERROR_DISK_FULL == syserr || ERROR_HANDLE_DISK_FULL == syserr)
    {
        return KLB_BASECODE_ENOSPC;
    }
    else if (ERROR_OPERATION_ABORTED == syserr)
    {
        return KLB_BASECODE_ECANCELED;
    }
    else if (WAIT_TIMEOUT == syserr || ERROR_TIMEOUT == syserr)
    {
        return KLB_BASECODE_ETIMEDOUT;
    }
    else if (ERROR_NOT_SUPPORTED == syserr)
    {
        return KLB_BASECODE_EOPNOTSUPP;
    }

    return KLB_BASECODE_EIO;
#else
    if (EPERM == syserr)
    {
        return KLB_BASECODE_EPERM;
    }
    else if (ENOENT == syserr)
    {
        return KLB_BASECODE_ENOENT;
    }
    else if (EINTR == syserr)
    {
        return KLB_BASECODE_EINTR;
    }
    else if (EIO == syserr)
    {
        return KLB_BASECODE_EIO;
    }
    else if (ENOEXEC == syserr)
    {
        return KLB_BASECODE_ENOEXEC;
    }
    else if (EBADF == syserr)
    {
        return KLB_BASECODE_EBADF;
    }
    else if (EAGAIN == syserr)
    {
        return KLB_BASECODE_EAGAIN;
    }
    else if (ENOMEM == syserr)
    {
        return KLB_BASECODE_ENOMEM;
    }
    else if (EACCES == syserr)
    {
        return KLB_BASECODE_EACCES;
    }
    else if (EBUSY == syserr)
    {
        return KLB_BASECODE_EBUSY;
    }
    else if (EEXIST == syserr)
    {
        return KLB_BASECODE_EEXIST;
    }
    else if (ENOTDIR == syserr)
    {
        return KLB_BASECODE_ENOTDIR;
    }
    else if (EISDIR == syserr)
    {
        return KLB_BASECODE_EISDIR;
    }
    else if (EINVAL == syserr)
    {
        return KLB_BASECODE_EINVAL;
    }
    else if (ENOSPC == syserr)
    {
        return KLB_BASECODE_ENOSPC;
    }
    else if (EROFS == syserr)
    {
        return KLB_BASECODE_EROFS;
    }
    else if (EPIPE == syserr)
    {
        return KLB_BASECODE_EPIPE;
    }
    else if (ERANGE == syserr)
    {
        return KLB_BASECODE_ERANGE;
    }
    else if (ENAMETOOLONG == syserr)
    {
        return KLB_BASECODE_ENAMETOOLONG;
    }
    else if (ENOTEMPTY == syserr)
    {
        return KLB_BASECODE_ENOTEMPTY;
    }
    else if (EBADMSG == syserr)
    {
        return KLB_BASECODE_EBADMSG;
    }
    else if (EILSEQ == syserr)
    {
        return KLB_BASECODE_EILSEQ;
    }
    else if (EMSGSIZE == syserr)
    {
        return KLB_BASECODE_EMSGSIZE;
    }
    else if (EOPNOTSUPP == syserr)
    {
        return KLB_BASECODE_EOPNOTSUPP;
    }
    else if (EAFNOSUPPORT == syserr)
    {
        return KLB_BASECODE_EAFNOSUPPORT;
    }
    else if (EADDRINUSE == syserr)
    {
        return KLB_BASECODE_EADDRINUSE;
    }
    else if (EADDRNOTAVAIL == syserr)
    {
        return KLB_BASECODE_EADDRNOTAVAIL;
    }
    else if (ENETUNREACH == syserr)
    {
        return KLB_BASECODE_ENETUNREACH;
    }
    else if (ECONNABORTED == syserr)
    {
        return KLB_BASECODE_ECONNABORTED;
    }
    else if (ECONNRESET == syserr)
    {
        return KLB_BASECODE_ECONNRESET;
    }
    else if (ENOBUFS == syserr)
    {
        return KLB_BASECODE_ENOBUFS;
    }
    else if (EISCONN == syserr)
    {
        return KLB_BASECODE_EISCONN;
    }
    else if (ENOTCONN == syserr)
    {
        return KLB_BASECODE_ENOTCONN;
    }
    else if (ETIMEDOUT == syserr)
    {
        return KLB_BASECODE_ETIMEDOUT;
    }
    else if (ECONNREFUSED == syserr)
    {
        return KLB_BASECODE_ECONNREFUSED;
    }
    else if (EHOSTUNREACH == syserr)
    {
        return KLB_BASECODE_EHOSTUNREACH;
    }
    else if (EALREADY == syserr)
    {
        return KLB_BASECODE_EALREADY;
    }
    else if (EINPROGRESS == syserr)
    {
        return KLB_BASECODE_EINPROGRESS;
    }
    else if (ECANCELED == syserr)
    {
        return KLB_BASECODE_ECANCELED;
    }

    return KLB_BASECODE_EIO;
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
    case KLB_BASECODE_EPERM:
        return "[klbbase] Error";
    case KLB_BASECODE_ENOENT:
        return "[klbbase] No such file or directory";
    case KLB_BASECODE_EINTR:
        return "[klbbase] Interrupted system call";
    case KLB_BASECODE_EIO:
        return "[klbbase] I/O error";
    case KLB_BASECODE_ENOEXEC:
        return "[klbbase] Exec format error";
    case KLB_BASECODE_EBADF:
        return "[klbbase] Bad file descriptor";
    case KLB_BASECODE_EAGAIN:
        return "[klbbase] Try again";
    case KLB_BASECODE_ENOMEM:
        return "[klbbase] Not enough resources";
    case KLB_BASECODE_EACCES:
        return "[klbbase] Permission denied";
    case KLB_BASECODE_EBUSY:
        return "[klbbase] Device or resource busy";
    case KLB_BASECODE_EEXIST:
        return "[klbbase] Already exists";
    case KLB_BASECODE_ENOTDIR:
        return "[klbbase] Not a directory";
    case KLB_BASECODE_EISDIR:
        return "[klbbase] Is a directory";
    case KLB_BASECODE_EINVAL:
        return "[klbbase] Invalid argument";
    case KLB_BASECODE_ENOSPC:
        return "[klbbase] No space left";
    case KLB_BASECODE_EROFS:
        return "[klbbase] Read-only file system";
    case KLB_BASECODE_EPIPE:
        return "[klbbase] Broken pipe";
    case KLB_BASECODE_ERANGE:
        return "[klbbase] Result too large";
    case KLB_BASECODE_ENAMETOOLONG:
        return "[klbbase] File name too long";
    case KLB_BASECODE_ENOTEMPTY:
        return "[klbbase] Directory not empty";
    case KLB_BASECODE_EBADMSG:
        return "[klbbase] Bad message";
    case KLB_BASECODE_EILSEQ:
        return "[klbbase] Illegal byte sequence";
    case KLB_BASECODE_EMSGSIZE:
        return "[klbbase] Message too long";
    case KLB_BASECODE_EOPNOTSUPP:
        return "[klbbase] Not supported";
    case KLB_BASECODE_EAFNOSUPPORT:
        return "[klbbase] Address family not supported";
    case KLB_BASECODE_EADDRINUSE:
        return "[klbbase] Address already in use";
    case KLB_BASECODE_EADDRNOTAVAIL:
        return "[klbbase] Cannot assign requested address";
    case KLB_BASECODE_ENETUNREACH:
        return "[klbbase] Network is unreachable";
    case KLB_BASECODE_ECONNABORTED:
        return "[klbbase] Software caused connection abort";
    case KLB_BASECODE_ECONNRESET:
        return "[klbbase] Connection reset by peer";
    case KLB_BASECODE_ENOBUFS:
        return "[klbbase] No buffer space available";
    case KLB_BASECODE_EISCONN:
        return "[klbbase] Transport endpoint is already connected";
    case KLB_BASECODE_ENOTCONN:
        return "[klbbase] Transport endpoint is not connected";
    case KLB_BASECODE_ETIMEDOUT:
        return "[klbbase] Connection timed out";
    case KLB_BASECODE_ECONNREFUSED:
        return "[klbbase] Connection refused";
    case KLB_BASECODE_EHOSTUNREACH:
        return "[klbbase] No route to host";
    case KLB_BASECODE_EALREADY:
        return "[klbbase] Operation already in progress";
    case KLB_BASECODE_EINPROGRESS:
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
