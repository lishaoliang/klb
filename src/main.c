#include <stdio.h>

#ifndef _MSC_VER

#include "klua/klua.h"
#include "libklb_go.h"

int main(int argc, char **argv)
{
    //int ret = klua_go_main(argc, argv);
    int ret = klua_main(argc, argv, klua_go_openlibs);

    return ret;
}

#endif
