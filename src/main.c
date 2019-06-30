#include <stdio.h>

#ifndef _MSC_VER

#include "libklb_go.h"

int main(int argc, char **argv)
{
    int ret = klua_go_main(argc, argv);

    return ret;
}

#endif
