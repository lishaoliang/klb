#include <stdio.h>
#include "klbbase/klb_base.h"
#include "klua/klua.h"

int main(int argc, char** argv)
{
    klb_base_init(NULL);

    int ret = klua_main(argc, argv, NULL);

    klb_base_quit();
    return ret;
}
