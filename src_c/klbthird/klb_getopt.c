// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbthird/klb_getopt.h"

#ifndef STATIC_GETOPT
#define STATIC_GETOPT
#endif
#include "getopt.h"


KLB_API char* klb_optarg = NULL;
KLB_API int klb_optind = 1;
KLB_API int klb_opterr = 1;
KLB_API int klb_optopt = '?';


//////////////////////////////////////////////////////////////////////////

static void sync_to_getopt(void)
{
    optind = klb_optind;
    opterr = klb_opterr;
    optopt = klb_optopt;
}

static void sync_from_getopt(void)
{
    klb_optarg = optarg_a;
    klb_optind = optind;
    klb_opterr = opterr;
    klb_optopt = optopt;
}


int klb_getopt(int argc, char* const* argv, const char* optstring)
{
    int ret;

    sync_to_getopt();
    ret = getopt_a(argc, argv, optstring);
    sync_from_getopt();

    return ret;
}

int klb_getopt_long(int argc, char* const* argv, const char* options,
    const klb_option_t* long_options, int* opt_index)
{
    int ret;

    sync_to_getopt();
    ret = getopt_long_a(argc, argv, options, (const struct option_a*)long_options, opt_index);
    sync_from_getopt();

    return ret;
}

int klb_getopt_long_only(int argc, char* const* argv, const char* options,
    const klb_option_t* long_options, int* opt_index)
{
    int ret;

    sync_to_getopt();
    ret = getopt_long_only_a(argc, argv, options, (const struct option_a*)long_options, opt_index);
    sync_from_getopt();

    return ret;
}

// end
