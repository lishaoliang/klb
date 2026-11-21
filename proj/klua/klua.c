// Doc Encode : UTF-8 BOM, Unix(LF)
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if defined(_WIN32) || defined(WIN32)
#include <windows.h>
#else
#include <limits.h>
#include <unistd.h>
#endif
#include "klua/klua.h"
#include "klbbase/klb_base.h"
#include "klbapp/klb_app.h"

#if !defined(_WIN32) && !defined(WIN32)
#define KLUA_PLUGINS_SYS    "/usr/local/lib/klua/plugins"
#endif


// vld - debug
#if defined(WIN32) && defined(_DEBUG)
#include "vld.h"
#endif // 


// 所有 lua环境 预加载 库
static int klua_openlibs(lua_State* L)
{
    klua_loadlib_all(L);

    return 0;
}

// Win32: cwd + exe 旁; Linux: ./plugins + 系统 lib/klua/plugins + exe 旁 + LD_LIBRARY_PATH
static void klua_setup_plugins_paths(void)
{
    klb_app_enable_plugins(true);

#if defined(_WIN32) || defined(WIN32)
    // cwd 相对 (源树/开发)
    klb_app_push_plugins_path(".");
    klb_app_push_plugins_path("./lib");
    klb_app_push_plugins_path("./libs");
    klb_app_push_plugins_path("./plugins");

    // exe 旁绝对路径: release 安装后任意 cwd 均可加载 plugins/*.dll
    {
        char exe_path[MAX_PATH];
        DWORD n = GetModuleFileNameA(NULL, exe_path, (DWORD)sizeof(exe_path));

        if (0 < n && n < (DWORD)sizeof(exe_path))
        {
            char* slash = strrchr(exe_path, '\\');
            if (NULL == slash)
            {
                slash = strrchr(exe_path, '/');
            }
            if (NULL != slash)
            {
                char sub[MAX_PATH];

                *slash = '\0';
                klb_app_push_plugins_path(exe_path);

                snprintf(sub, sizeof(sub), "%s\\lib", exe_path);
                klb_app_push_plugins_path(sub);
                snprintf(sub, sizeof(sub), "%s\\libs", exe_path);
                klb_app_push_plugins_path(sub);
                snprintf(sub, sizeof(sub), "%s\\plugins", exe_path);
                klb_app_push_plugins_path(sub);
            }
        }
    }
#else
    klb_app_push_plugins_path("./plugins");
    klb_app_push_plugins_path(KLUA_PLUGINS_SYS);

    // exe 旁: /usr/local/bin/klua → ../lib/klua/plugins
    {
        char exe_path[PATH_MAX];
        ssize_t n = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);

        if (0 < n)
        {
            char* slash = NULL;
            char sub[PATH_MAX];

            exe_path[n] = '\0';
            slash = strrchr(exe_path, '/');

            if (NULL != slash)
            {
                *slash = '\0';
                snprintf(sub, sizeof(sub), "%s/../lib/klua/plugins", exe_path);
                klb_app_push_plugins_path(sub);
                snprintf(sub, sizeof(sub), "%s/plugins", exe_path);
                klb_app_push_plugins_path(sub);
            }
        }
    }

    {
        const char* ld_path = getenv("LD_LIBRARY_PATH");

        if (NULL != ld_path && '\0' != ld_path[0])
        {
            char* dup = strdup(ld_path);

            if (NULL != dup)
            {
                char* token = dup;

                while (NULL != token && '\0' != *token)
                {
                    char* colon = strchr(token, ':');

                    if (NULL != colon)
                    {
                        *colon = '\0';
                    }

                    if ('\0' != token[0])
                    {
                        klb_app_push_plugins_path(token);
                    }

                    token = (NULL != colon) ? colon + 1 : NULL;
                }

                free(dup);
            }
        }
    }
#endif
}

int main(int argc, char** argv)
{
    klb_base_init(NULL);

    klua_setup_plugins_paths();

    int ret = klb_app_main(argc, argv, klua_openlibs, false);

    klb_base_quit();
    return ret;
}
