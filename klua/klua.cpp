#include <stdio.h>
#include "klua/klua.h"
#include "klbbase/klb_base.h"

#if defined(WIN32) && defined(DEBUG_VLD)
#include "vld.h"
#endif

int main(int argc, char** argv)
{
    klb_base_init(NULL);


    int ret = klua_main(argc, argv, NULL);


    klb_base_quit();
    return ret;
}
