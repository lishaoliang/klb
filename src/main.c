#include <stdio.h>

#ifndef _WIN32

#include "klua/klua.h"
#include "libklua_go.h"

int main(int argc, char **argv)
{
    //int ret = klua_go_main(argc, argv);
    int ret = klua_main(argc, argv, klua_go_openlibs);

    return ret;
}

#endif
