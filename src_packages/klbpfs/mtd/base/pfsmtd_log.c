// Doc Encode : UTF-8 BOM, Unix(LF)
// pfsmtd 日志打印 (stdout; 固定 [pfsmtd] 标签)
#include "base/pfsmtd_log.h"

#include <stdio.h>

static int g_pfsmtd_log_level = PFSMTD_LOG_INFO;

void pfsmtd_log_set_level(int level)
{
    if (level < PFSMTD_LOG_ERROR)
    {
        level = PFSMTD_LOG_ERROR;
    }
    else if (level > PFSMTD_LOG_DEBUG)
    {
        level = PFSMTD_LOG_DEBUG;
    }
    g_pfsmtd_log_level = level;
}

int pfsmtd_log_get_level(void)
{
    return g_pfsmtd_log_level;
}

static char pfsmtd_log_level_char(int level)
{
    if (PFSMTD_LOG_ERROR == level)
    {
        return 'E';
    }
    if (PFSMTD_LOG_WARN == level)
    {
        return 'W';
    }
    if (PFSMTD_LOG_INFO == level)
    {
        return 'I';
    }
    if (PFSMTD_LOG_DEBUG == level)
    {
        return 'D';
    }
    return '?';
}

void pfsmtd_logv(int level, const char *fmt, va_list ap)
{
    if (NULL == fmt)
    {
        return;
    }
    if (level > g_pfsmtd_log_level)
    {
        return;
    }

    // 固定标签: [pfsmtd][E|W|I|D]
    fprintf(stdout, "[pfsmtd][%c] ", pfsmtd_log_level_char(level));
    vfprintf(stdout, fmt, ap);
    fputc('\n', stdout);
}

void pfsmtd_log(int level, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    pfsmtd_logv(level, fmt, ap);
    va_end(ap);
}

// end
