#include "go/klb_select.h"

void klb_fd_zero(fd_set* p_fdset)
{
    FD_ZERO(p_fdset);
}

void klb_fd_set(uintptr_t fd, fd_set* p_fdset)
{
    FD_SET(fd, p_fdset);
}

void klb_fd_clr(uintptr_t fd, fd_set* p_fdset)
{
    FD_CLR(fd, p_fdset);
}

int klb_fd_is_set(uintptr_t fd, fd_set* p_fdset)
{
    return FD_ISSET(fd, p_fdset);
}
