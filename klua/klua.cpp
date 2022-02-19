#include <stdio.h>
#include "klua/klua.h"

#if defined(WIN32) && defined(DEBUG_VLD)
#include "vld.h"
#endif

int main(int argc, char** argv)
{
    int ret = klua_main(argc, argv, NULL);

    return ret;
}
