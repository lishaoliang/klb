// Doc Encode : UTF-8 BOM, Unix(LF)
// pfs 日志打印 (stdout; 固定 [pfs] 标签)
#include "pfs_log.h"
#include "pfs_compiler.h"

#include <stdio.h>

static int g_pfs_log_level = PFS_LOG_INFO;

void pfs_log_set_level(int level)
{
    if (level < PFS_LOG_ERROR)
    {
        level = PFS_LOG_ERROR;
    }
    else if (level > PFS_LOG_DEBUG)
    {
        level = PFS_LOG_DEBUG;
    }
    g_pfs_log_level = level;
}

int pfs_log_get_level(void)
{
    return g_pfs_log_level;
}

static char pfs_log_level_char(int level)
{
    if (PFS_LOG_ERROR == level)
    {
        return 'E';
    }
    else if (PFS_LOG_WARN == level)
    {
        return 'W';
    }
    else if (PFS_LOG_INFO == level)
    {
        return 'I';
    }
    else if (PFS_LOG_DEBUG == level)
    {
        return 'D';
    }
    return '?';
}

void pfs_logv(int level, const char *fmt, va_list ap)
{
    if (pfs_unlikely(NULL == fmt))
    {
        return;
    }
    if (level > g_pfs_log_level)
    {
        return;
    }

    // 固定标签: [pfs][E|W|I|D]
    fprintf(stdout, "[pfs][%c] ", pfs_log_level_char(level));
    vfprintf(stdout, fmt, ap);
    fputc('\n', stdout);
}

void pfs_log(int level, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    pfs_logv(level, fmt, ap);
    va_end(ap);
}

// end
