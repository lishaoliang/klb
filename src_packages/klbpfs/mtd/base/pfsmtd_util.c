// Doc Encode : UTF-8 BOM, Unix(LF)
// pfsmtd 杂项公共辅助
#include "base/pfsmtd_util.h"

static const char g_pfsmtd_version[] = "0.1.0";

const char *pfsmtd_version(void)
{
    return g_pfsmtd_version;
}

// end
