#ifndef __KLB_SELECT_H__
#define __KLB_SELECT_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define FD_SETSIZE      256

#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <sys/select.h>
    #include <sys/time.h>
    #include <unistd.h>
#endif


#define KLB_SELECT  select

KLB_API void klb_fd_zero(fd_set* p_fdset);
KLB_API void klb_fd_set(uintptr_t fd, fd_set* p_fdset);
KLB_API void klb_fd_clr(uintptr_t fd, fd_set* p_fdset);
KLB_API int klb_fd_is_set(uintptr_t fd, fd_set* p_fdset);

#ifdef __cplusplus
}
#endif

#endif // __KLB_SELECT_H__
//end
