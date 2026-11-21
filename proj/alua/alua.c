// Doc Encode : UTF-8 BOM, Unix(LF)
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <unistd.h>
#if defined(_WIN32) || defined(WIN32)
#include <windows.h>
#endif
#include "klua/klua.h"
#include "klbbase/klb_base.h"
#include "klbapp/klb_app.h"
#include "klbthird/klb_getopt.h"
#if !defined(ALUA_NO_KSVNRA)
#include "alua/svn_ra/svn_ra.h"
#endif


// vld - debug
#if defined(WIN32) && defined(_DEBUG)
#include "vld.h"
#endif //


//////////////////////////////////////////////////////////////////////////
// alua: AI agent 辅助可执行程序
// 仅启动并读取 klb/bin/klbagent 包 (不跑任意用户脚本)

#define ALUA_BOOT_REL       "klbagent/alua_boot.lua"
#define ALUA_VERSION_REL    "klbagent/VERSION"
#define ALUA_ENV_BIN        "ALUA_KLB_BIN"
#define ALUA_ENV_SHARE      "ALUA_SHARE_KLUA"
#define ALUA_SHARE_DEFAULT  "/usr/local/share/klua"
#define ALUA_PLUGINS_SYS    "/usr/local/lib/klua/plugins"
#define ALUA_VER_FALLBACK   "0.0.0.000000"


// 所有 lua环境 预加载 库
static int alua_openlibs(lua_State* L)
{
    klua_loadlib_all(L);
#if !defined(ALUA_NO_KSVNRA)
    klua_loadlib(L, klua_open_ksvnra, "ksvnra");
#endif

    return 0;
}


/// @brief 打印 alua 壳用法 (入口固定 boot; 命令帮助由 klbagent Lua 再打)
static void alua_print_usage(const char* p_prog)
{
    const char* name = (NULL != p_prog && '\0' != p_prog[0]) ? p_prog : "alua";

    fprintf(stdout,
        "Usage: %s [options] [command] [args...]\n"
        "  Force entry: klbagent/alua_boot.lua; remaining args go to klbagent.\n"
        "Shell options:\n"
        "  -h, --help             Show this help (shell + klbagent commands)\n"
        "  -v, --version          Show alua / klb version\n"
        "  -p, --plugins PATH     Enable plugins; append PATH to plugin search\n"
        "                         (repeatable; also scans ./lib, ./libs, ./plugins, LD_LIBRARY_PATH)\n"
        "  -s, --serve            Serve-side entry (klbagent.entry_serve); args: [port] [profile]\n"
        "\n",
        name);
}


/// @brief 消化壳选项 (-h/-v/-p/-s); 位置参数留给 boot 之后传给 klbagent
/// @param [in]  argc        参数个数
/// @param [in]  argv        参数数组
/// @param [out] p_want_serve 是否服务侧入口 (-s / --serve)
/// @return int 0.继续; 1.已打壳 help, 再进 Lua 打命令帮助; 2.需打版本; -1.失败应返回非0
static int alua_parse_shell_args(int argc, char** argv, bool* p_want_serve)
{
    static const klb_option_t long_opts[] =
    {
        { "help",    KLB_no_argument,       NULL, 'h' },
        { "version", KLB_no_argument,       NULL, 'v' },
        { "plugins", KLB_required_argument, NULL, 'p' },
        { "serve",   KLB_no_argument,       NULL, 's' },
        { NULL, 0, NULL, 0 }
    };

    const char* p_prog = (0 < argc) ? argv[0] : "alua";
    int opt = 0;
    bool want_help = false;
    bool want_version = false;
    bool want_serve = false;

    klb_optind = 1;
    klb_opterr = 1;

    while (true)
    {
        // + : stop at first non-option (alua status -h 留给子命令, 勿当壳 -h)
        opt = klb_getopt_long(argc, argv, "+hvp:s", long_opts, NULL);
        if (-1 == opt)
        {
            break;
        }

        if ('h' == opt)
        {
            want_help = true;
        }
        else if ('v' == opt)
        {
            want_version = true;
        }
        else if ('p' == opt)
        {
            klb_app_enable_plugins(true);
            klb_app_push_plugins_path(klb_optarg);
        }
        else if ('s' == opt)
        {
            want_serve = true;
        }
        else
        {
            alua_print_usage(p_prog);
            return -1;
        }
    }

    *p_want_serve = want_serve;

    if (want_help)
    {
        alua_print_usage(p_prog);
        return 1;
    }

    if (want_version)
    {
        return 2;
    }

    return 0;
}


// Win32: cwd + exe 旁; Linux: ./plugins + 系统 lib/klua/plugins + exe 旁 + LD_LIBRARY_PATH
static void alua_setup_plugins_paths(void)
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
        char exe_path[PATH_MAX];
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
                char sub[PATH_MAX];

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
    klb_app_push_plugins_path(ALUA_PLUGINS_SYS);

    // exe 旁: /usr/local/bin/alua → ../lib/klua/plugins
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


/// @brief 安装 share/klua 根 (klbcore / klbagent); 可用 ALUA_SHARE_KLUA 覆盖
static const char* alua_share_klua(void)
{
    const char* env_share = getenv(ALUA_ENV_SHARE);

    if (NULL != env_share && '\0' != env_share[0])
    {
        return env_share;
    }

    return ALUA_SHARE_DEFAULT;
}


/// @brief 候选目录下是否存在 klbagent/alua_boot.lua
static bool alua_boot_exists(const char* script_root)
{
    char boot[PATH_MAX];

    if (NULL == script_root || '\0' == script_root[0])
    {
        return false;
    }

    if ((size_t)PATH_MAX <= strlen(script_root) + strlen(ALUA_BOOT_REL) + 1)
    {
        return false;
    }

    snprintf(boot, sizeof(boot), "%s/%s", script_root, ALUA_BOOT_REL);
    return (0 == access(boot, F_OK));
}


/// @brief 解析脚本根 (含 klbagent/); cwd 无关
/// @note  次序: ALUA_KLB_BIN → exe/../bin(源树) → exe 旁(debug) → share/klua(安装)
/// @return 0 成功; 非0 失败
static int alua_resolve_script_root(char* out, size_t out_size, const char* argv0)
{
    const char* env_bin = getenv(ALUA_ENV_BIN);
    const char* share = alua_share_klua();

    if (NULL != env_bin && '\0' != env_bin[0] && alua_boot_exists(env_bin))
    {
        if (strlen(env_bin) + 1 > out_size)
        {
            return 1;
        }

        snprintf(out, out_size, "%s", env_bin);
        return 0;
    }

#if defined(_WIN32) || defined(WIN32)
    {
        char exe_path[PATH_MAX];
        char exe_dir[PATH_MAX];
        char candidate[PATH_MAX];
        char* slash = NULL;
        DWORD n = GetModuleFileNameA(NULL, exe_path, (DWORD)sizeof(exe_path));

        (void)argv0;
        exe_dir[0] = '\0';
        candidate[0] = '\0';

        if (0 < n && n < (DWORD)sizeof(exe_path))
        {
            snprintf(exe_dir, sizeof(exe_dir), "%s", exe_path);
            slash = strrchr(exe_dir, '\\');
            if (NULL == slash)
            {
                slash = strrchr(exe_dir, '/');
            }
            if (NULL != slash)
            {
                *slash = '\0';
            }
        }

        // step1. release 平铺: alua.exe 旁即 klbagent/
        if ('\0' != exe_dir[0] && alua_boot_exists(exe_dir))
        {
            snprintf(out, out_size, "%s", exe_dir);
            return 0;
        }

        // step2. 源树: lib/mingw/alua.exe -> ../../bin/klbagent
        if ('\0' != exe_dir[0])
        {
            snprintf(candidate, sizeof(candidate), "%s/../../bin", exe_dir);
            if (alua_boot_exists(candidate))
            {
                snprintf(out, out_size, "%s", candidate);
                return 0;
            }
        }

        // step3. cwd 源树根: ./bin/klbagent (历史行为)
        if (alua_boot_exists(".\\bin"))
        {
            snprintf(out, out_size, "%s", ".\\bin");
            return 0;
        }

        // step4. 安装 share (ALUA_SHARE_KLUA 覆盖)
        if (alua_boot_exists(share))
        {
            snprintf(out, out_size, "%s", share);
            return 0;
        }

        // 回落: exe 旁, 便于报错信息定位
        if ('\0' != exe_dir[0])
        {
            snprintf(out, out_size, "%s", exe_dir);
            return 0;
        }

        snprintf(out, out_size, "%s", ".\\bin");
        return 0;
    }
#else
    {
        char exe_dir[PATH_MAX];
        char candidate[PATH_MAX];
        char* slash = NULL;
        ssize_t n = readlink("/proc/self/exe", exe_dir, sizeof(exe_dir) - 1);

        if (0 > n)
        {
            if (NULL != argv0 && '\0' != argv0[0])
            {
                snprintf(exe_dir, sizeof(exe_dir), "%s", argv0);
            }
            else
            {
                exe_dir[0] = '\0';
            }
        }
        else
        {
            exe_dir[n] = '\0';
        }

        if ('\0' != exe_dir[0])
        {
            slash = strrchr(exe_dir, '/');
            if (NULL != slash)
            {
                *slash = '\0';
            }

            // step1. 源树: lib/alua -> ../bin
            snprintf(candidate, sizeof(candidate), "%s/../bin", exe_dir);
            if (alua_boot_exists(candidate))
            {
                if (strlen(candidate) + 1 > out_size)
                {
                    return 1;
                }

                snprintf(out, out_size, "%s", candidate);
                return 0;
            }

            // step2. debug 树: alua 旁 klbagent/
            if (alua_boot_exists(exe_dir))
            {
                if (strlen(exe_dir) + 1 > out_size)
                {
                    return 1;
                }

                snprintf(out, out_size, "%s", exe_dir);
                return 0;
            }
        }

        // step3. 系统安装: /usr/local/share/klua/klbagent
        if (alua_boot_exists(share))
        {
            if (strlen(share) + 1 > out_size)
            {
                return 1;
            }

            snprintf(out, out_size, "%s", share);
            return 0;
        }

        // 回落: 仍填源树相对路径, 便于报错信息
        if ('\0' != exe_dir[0])
        {
            snprintf(out, out_size, "%s/../bin", exe_dir);
            return 0;
        }

        snprintf(out, out_size, "%s", "./bin");
        return 0;
    }
#endif
}


/// @brief 设 LUA_PATH: 脚本根 (klbagent) + 安装 share (klbcore); 不依赖 cwd
static void alua_setup_package_path(const char* script_root)
{
    char lua_path[PATH_MAX * 4];
    const char* share = alua_share_klua();

    if (0 == strcmp(script_root, share))
    {
        // 脚本根即安装 share: klbagent 与 klbcore 同树
        snprintf(lua_path, sizeof(lua_path),
            "%s/?.lua;%s/?/init.lua",
            share, share);
    }
    else
    {
        // 源树/debug 优先, 再安装 share (require("klbcore.*"))
        snprintf(lua_path, sizeof(lua_path),
            "%s/?.lua;%s/?/init.lua;%s/?.lua;%s/?/init.lua",
            script_root, script_root, share, share);
    }

#if defined(_WIN32) || defined(WIN32)
    // mingw/MSVC 无 POSIX setenv; _putenv_s 同进程生效 (覆盖)
    _putenv_s("LUA_PATH", lua_path);
#else
    setenv("LUA_PATH", lua_path, 1);
#endif
}


/// @brief 从脚本根读 klbagent/VERSION (一行: base.YYYYMM)
static void alua_read_script_version(const char* script_root, char* out, size_t out_size)
{
    char path[PATH_MAX];
    FILE* fp = NULL;
    size_t n = 0;

    if (NULL == out || 0 == out_size)
    {
        return;
    }

    out[0] = '\0';

    if (NULL == script_root || '\0' == script_root[0])
    {
        snprintf(out, out_size, "%s", ALUA_VER_FALLBACK);
        return;
    }

    if ((size_t)PATH_MAX <= strlen(script_root) + strlen(ALUA_VERSION_REL) + 1)
    {
        snprintf(out, out_size, "%s", ALUA_VER_FALLBACK);
        return;
    }

    snprintf(path, sizeof(path), "%s/%s", script_root, ALUA_VERSION_REL);
    fp = fopen(path, "rb");
    if (NULL == fp)
    {
        snprintf(out, out_size, "%s", ALUA_VER_FALLBACK);
        return;
    }

    if (NULL == fgets(out, (int)out_size, fp))
    {
        snprintf(out, out_size, "%s", ALUA_VER_FALLBACK);
        fclose(fp);
        return;
    }

    fclose(fp);

    n = strlen(out);
    while (0 < n && ('\n' == out[n - 1] || '\r' == out[n - 1] || ' ' == out[n - 1] || '\t' == out[n - 1]))
    {
        out[n - 1] = '\0';
        n -= 1;
    }

    if (0 == n)
    {
        snprintf(out, out_size, "%s", ALUA_VER_FALLBACK);
    }
}


/// @brief 打印: alua <脚本版本> (klb <核心版本>)
static void alua_print_version(const char* argv0)
{
    char script_root[PATH_MAX];
    char script_ver[64];

    script_root[0] = '\0';
    if (0 != alua_resolve_script_root(script_root, sizeof(script_root), argv0)
        || !alua_boot_exists(script_root))
    {
        // 脚本树不可用时仍给出核心版本
        fprintf(stdout, "alua %s (klb %s)\n", ALUA_VER_FALLBACK, klb_version());
        return;
    }

    alua_read_script_version(script_root, script_ver, sizeof(script_ver));
    fprintf(stdout, "alua %s (klb %s)\n", script_ver, klb_version());
}


int main(int argc, char** argv)
{
    char script_root[PATH_MAX];
    char boot_path[PATH_MAX + 64];
    char** new_argv = NULL;
    char* serve_args[2] = { NULL, NULL };
    int serve_argc = 0;
    int new_argc = 0;
    int pos_count = 0;
    int parse_rc = 0;
    int ret = 0;
    bool want_serve = false;

    klb_base_init(NULL);

    alua_setup_plugins_paths();

    // step0. 先消化壳选项; Agent CLI 位置参数不在此解析
    // parse_rc==1 或无位置参数: 先打壳 usage, 再进 Lua 打 klbagent 命令帮助
    {
        bool show_agent_help = false;
        bool serve_help = false;

        parse_rc = alua_parse_shell_args(argc, argv, &want_serve);
        if (2 == parse_rc)
        {
            alua_print_version((0 < argc) ? argv[0] : NULL);
            klb_base_quit();
            return 0;
        }

        if (0 > parse_rc)
        {
            klb_base_quit();
            return 1;
        }

        if (1 == parse_rc)
        {
            // -h: 壳 usage 已打; 再进 Lua
            show_agent_help = true;
        }
        else if (klb_optind >= argc && !want_serve)
        {
            // 直接 alua: 与 -h 同序 (壳 + klbagent); 带 --serve 时视为裸启动, 不打帮助
            alua_print_usage((0 < argc) ? argv[0] : "alua");
            show_agent_help = true;
        }

        if (show_agent_help)
        {
            // 不向 klbagent 传位置参数, 由其默认打印命令帮助
            klb_optind = argc;

            // 服务侧 + -h: 交 CLI 打 serve 命令帮助, 不启动监听
            serve_help = want_serve;
        }

        // step0c. 服务侧标记作为首个位置参数交给 boot (boot 从第 3 项起取); help 紧随其后
        if (want_serve)
        {
            serve_args[serve_argc] = (char*)"--serve";
            serve_argc += 1;
        }

        if (serve_help)
        {
            serve_args[serve_argc] = (char*)"-h";
            serve_argc += 1;
        }
    }

    // step1. 定位脚本根 (klbagent) + LUA_PATH (含安装 klbcore); 不依赖 cwd
    if (0 != alua_resolve_script_root(script_root, sizeof(script_root), (0 < argc) ? argv[0] : NULL))
    {
        fprintf(stderr, "alua: resolve script root failed\n");
        klb_base_quit();
        return 1;
    }

    if (!alua_boot_exists(script_root))
    {
        fprintf(stderr, "alua: boot not found under '%s' (set %s or install klbagent)\n",
            script_root, ALUA_ENV_BIN);
        klb_base_quit();
        return 1;
    }

    alua_setup_package_path(script_root);

    if ((size_t)PATH_MAX <= strlen(script_root) + strlen(ALUA_BOOT_REL) + 1)
    {
        fprintf(stderr, "alua: boot path too long\n");
        klb_base_quit();
        return 1;
    }

    snprintf(boot_path, sizeof(boot_path), "%s/%s", script_root, ALUA_BOOT_REL);

    // step2. 强制入口 boot; 先给 serve 侧标记, 再追加壳解析后的位置参数
    pos_count = (klb_optind < argc) ? (argc - klb_optind) : 0;
    new_argc = 2 + serve_argc + pos_count;
    new_argv = (char**)calloc((size_t)new_argc + 1, sizeof(char*));
    if (NULL == new_argv)
    {
        fprintf(stderr, "alua: oom\n");
        klb_base_quit();
        return 1;
    }

    new_argv[0] = (0 < argc) ? argv[0] : (char*)"alua";
    new_argv[1] = boot_path;

    {
        int w = 2;

        for (int i = 0; i < serve_argc; ++i)
        {
            new_argv[w] = serve_args[i];
            w += 1;
        }

        for (int i = 0; i < pos_count; ++i)
        {
            new_argv[w] = argv[klb_optind + i];
            w += 1;
        }
    }

    new_argv[new_argc] = NULL;

    // klb_app_main 内会再次 getopt; 须复位
    klb_optind = 1;

    ret = klb_app_main(new_argc, new_argv, alua_openlibs, false);

    free(new_argv);
    klb_base_quit();
    return ret;
}

// end
