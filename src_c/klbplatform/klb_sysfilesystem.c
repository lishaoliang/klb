#include "klbplatform/klb_sysfilesystem.h"
#include "klbmem/klb_mem.h"
#include "klbplatform/klb_getenv.h"


#define SDL_min(x, y)           (((x) < (y)) ? (x) : (y))
#define SDL_max(x, y)           (((x) > (y)) ? (x) : (y))
#define SDL_clamp(x, a, b)      (((x) < (a)) ? (a) : (((x) > (b)) ? (b) : (x)))

#define SDL_malloc              malloc
#define SDL_calloc              calloc
#define SDL_realloc             realloc
#define SDL_strdup              strdup
#define SDL_strchr              strchr
#define SDL_asprintf            asprintf
#define SDL_snprintf            snprintf
#define SDL_strlen              strlen
#define SDL_strrchr             strrchr
#define SDL_getenv              getenv

#define SDL_arraysize(array)    (sizeof(array)/sizeof(array[0]))
#define SDL_OutOfMemory()       ((void)0)

#define SDL_free(PTR_)          {if(NULL!=(PTR_)){free(PTR_);(PTR_)=NULL;}}


#if defined(_WIN32)

#include <windows.h>
#include <shlobj.h>

#define SDL_wcslen              wcslen
#define SDL_memcpy              memcpy

#define SUCCEEDED(hr)           (((HRESULT)(hr)) >= 0)
#define FAILED(hr)              (((HRESULT)(hr)) < 0)


// utf8 转 unicode
static WCHAR* WIN_UTF8ToStringW(const char* p_src)
{
    // unicode 宽度
    int size = MultiByteToWideChar(CP_UTF8, 0, p_src, -1, NULL, 0);

    WCHAR* p_str = (WCHAR*)SDL_malloc(sizeof(WCHAR) * (size + 4));
    p_str[0] = '\0';

    // 转换成 unicode
    MultiByteToWideChar(CP_UTF8, 0, p_src, -1, p_str, size + 4);

    return p_str;
}

// unicode 转 utf8
static char* WIN_StringToUTF8W(const WCHAR* p_src)
{
    // utf8 需要的长度
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, p_src, -1, NULL, 0, NULL, NULL);

    // 转换成 utf8
    char* p_str = (char*)SDL_malloc(utf8_len + 4);
    p_str[0] = '\0';

    WideCharToMultiByte(CP_UTF8, 0, p_src, -1, p_str, utf8_len + 4, NULL, NULL);

    return p_str;
}

sds klb_get_base_path()
{
    wchar_t path[MAX_PATH] = { 0 };
    wchar_t* p_newpath = NULL;
    wchar_t* p_path = path;

    // 需要的字符数
    DWORD size = GetModuleFileNameW(NULL, p_path, MAX_PATH);
    if (MAX_PATH - 1 <= size)
    {
        p_newpath = KLB_MALLOCZ(wchar_t, size + 4, 0);
        p_path = p_newpath;

        size = GetModuleFileNameW(NULL, p_path, size + 4);
    }

    // 只保留路径
    for (int i = size - 1; i > 0; i--)
    {
        if (p_path[i] == '\\')
        {
            p_path[i + 1] = '\0';
            break;
        }
    }

    // 转成 utf8
    char* p_utf8 = WIN_StringToUTF8W(p_path);
    sds utf8 = sdsnew(p_utf8);

    SDL_free(p_utf8);
    KLB_FREE(p_newpath);
    return utf8;
}

static size_t
SDL_wcslcpy(wchar_t *dst, const wchar_t *src, size_t maxlen)
{
    size_t srclen = SDL_wcslen(src);
    if (maxlen > 0) {
        size_t len = SDL_min(srclen, maxlen - 1);
        SDL_memcpy(dst, src, len * sizeof(wchar_t));
        dst[len] = '\0';
    }
    return srclen;
}

static size_t
SDL_wcslcat(wchar_t *dst, const wchar_t *src, size_t maxlen)
{
    size_t dstlen = SDL_wcslen(dst);
    size_t srclen = SDL_wcslen(src);
    if (dstlen < maxlen) {
        SDL_wcslcpy(dst + dstlen, src, maxlen - dstlen);
    }
    return dstlen + srclen;
}

static char *SDL_GetPrefPath(const char *org, const char *app)
{
    /*
    * Vista and later has a new API for this, but SHGetFolderPath works there,
    *  and apparently just wraps the new API. This is the new way to do it:
    *
    *     SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE,
    *                          NULL, &wszPath);
    */

    WCHAR path[MAX_PATH];
    char *retval = NULL;
    WCHAR *worg = NULL;
    WCHAR *wapp = NULL;
    size_t new_wpath_len = 0;
    BOOL api_result = FALSE;

    if (!app) {
        //SDL_InvalidParamError("app");
        return NULL;
    }
    if (!org) {
        org = "";
    }

    if (!SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, path))) {
        //WIN_SetError("Couldn't locate our prefpath");
        return NULL;
    }

    worg = WIN_UTF8ToStringW(org);
    if (!worg) {
        //SDL_OutOfMemory();
        return NULL;
    }

    wapp = WIN_UTF8ToStringW(app);
    if (!wapp) {
        SDL_free(worg);
        SDL_OutOfMemory();
        return NULL;
    }

    new_wpath_len = SDL_wcslen(worg) + SDL_wcslen(wapp) + SDL_wcslen(path) + 3;

    if ((new_wpath_len + 1) > MAX_PATH) {
        SDL_free(worg);
        SDL_free(wapp);
        //WIN_SetError("Path too long.");
        return NULL;
    }

    if (*worg) {
        SDL_wcslcat(path, L"\\", SDL_arraysize(path));
        SDL_wcslcat(path, worg, SDL_arraysize(path));
    }
    SDL_free(worg);

    api_result = CreateDirectoryW(path, NULL);
    if (api_result == FALSE) {
        if (GetLastError() != ERROR_ALREADY_EXISTS) {
            SDL_free(wapp);
            //WIN_SetError("Couldn't create a prefpath.");
            return NULL;
        }
    }

    SDL_wcslcat(path, L"\\", SDL_arraysize(path));
    SDL_wcslcat(path, wapp, SDL_arraysize(path));
    SDL_free(wapp);

    api_result = CreateDirectoryW(path, NULL);
    if (api_result == FALSE) {
        if (GetLastError() != ERROR_ALREADY_EXISTS) {
            //WIN_SetError("Couldn't create a prefpath.");
            return NULL;
        }
    }

    SDL_wcslcat(path, L"\\", SDL_arraysize(path));

    retval = WIN_StringToUTF8W(path);

    return retval;
}

sds klb_get_pref_path(const char* p_org, const char* p_app)
{
    char* p_pref_path = SDL_GetPrefPath(p_org, p_app);
    sds pref_path = sdsnew(p_pref_path);

    SDL_free(p_pref_path);
    return pref_path;
}

#else

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <fcntl.h>

// copy from : https://gitee.com/partic/SDL-mirror/blob/main/src/filesystem/unix/SDL_sysfilesystem.c

static char *readSymLink(const char *path)
{
    char *retval = NULL;
    ssize_t len = 64;
    ssize_t rc = -1;

    while (1) {
        char *ptr = (char *)SDL_realloc(retval, (size_t)len);
        if (!ptr) {
            SDL_OutOfMemory();
            break;
        }

        retval = ptr;

        rc = readlink(path, retval, len);
        if (rc == -1) {
            break; /* not a symlink, i/o error, etc. */
        }
        else if (rc < len) {
            retval[rc] = '\0'; /* readlink doesn't null-terminate. */
            return retval;     /* we're good to go. */
        }

        len *= 2; /* grow buffer, try again. */
    }

    SDL_free(retval);
    return NULL;
}

static char *SDL_GetBasePath(void)
{
    char *retval = NULL;

    /* is a Linux-style /proc filesystem available? */
    if (!retval && (access("/proc", F_OK) == 0)) {
        /* !!! FIXME: after 2.0.6 ships, let's delete this code and just
        use the /proc/%llu version. There's no reason to have
        two copies of this plus all the #ifdefs. --ryan. */
#if defined(__FREEBSD__)
        retval = readSymLink("/proc/curproc/file");
#elif defined(__NETBSD__)
        retval = readSymLink("/proc/curproc/exe");
#elif defined(__SOLARIS__)
        retval = readSymLink("/proc/self/path/a.out");
#elif defined(__QNXNTO__)
        retval = SDL_LoadFile("/proc/self/exefile", NULL);
#else
        retval = readSymLink("/proc/self/exe"); /* linux. */
        if (!retval) {
            /* older kernels don't have /proc/self ... try PID version... */
            char path[64];
            const int rc = SDL_snprintf(path, sizeof(path),
                "/proc/%llu/exe",
                (unsigned long long)getpid());
            if ((rc > 0) && (rc < sizeof(path))) {
                retval = readSymLink(path);
            }
        }
#endif
    }

#if defined(__SOLARIS__)  /* try this as a fallback if /proc didn't pan out */
    if (!retval) {
        const char *path = getexecname();
        if ((path != NULL) && (path[0] == '/')) { /* must be absolute path... */
            retval = SDL_strdup(path);
            if (!retval) {
                SDL_OutOfMemory();
                return NULL;
            }
        }
    }
#endif

    /* If we had access to argv[0] here, we could check it for a path,
    or troll through $PATH looking for it, too. */

    if (retval) { /* chop off filename. */
        char *ptr = SDL_strrchr(retval, '/');
        if (ptr) {
            *(ptr + 1) = '\0';
        }
        else { /* shouldn't happen, but just in case... */
            SDL_free(retval);
            retval = NULL;
        }
    }

    if (retval) {
        /* try to shrink buffer... */
        char *ptr = (char *)SDL_realloc(retval, SDL_strlen(retval) + 1);
        if (ptr) {
            retval = ptr; /* oh well if it failed. */
        }
    }

    return retval;
}

static char *SDL_GetPrefPath(const char *org, const char *app)
{
    /*
    * We use XDG's base directory spec, even if you're not on Linux.
    *  This isn't strictly correct, but the results are relatively sane
    *  in any case.
    *
    * http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html
    */
    const char *envr = SDL_getenv("XDG_DATA_HOME");
    const char *append;
    char *retval = NULL;
    char *ptr = NULL;
    size_t len = 0;

    if (!app) {
        //SDL_InvalidParamError("app");
        return NULL;
    }
    if (!org) {
        org = "";
    }

    if (!envr) {
        /* You end up with "$HOME/.local/share/Game Name 2" */
        envr = SDL_getenv("HOME");
        if (!envr) {
            /* we could take heroic measures with /etc/passwd, but oh well. */
            //SDL_SetError("neither XDG_DATA_HOME nor HOME environment is set");
            return NULL;
        }
        append = "/.local/share/";
    }
    else {
        append = "/";
    }

    len = SDL_strlen(envr);
    if (envr[len - 1] == '/') {
        append += 1;
    }

    len += SDL_strlen(append) + SDL_strlen(org) + SDL_strlen(app) + 3;
    retval = (char *)SDL_malloc(len);
    if (!retval) {
        SDL_OutOfMemory();
        return NULL;
    }

    if (*org) {
        (void)SDL_snprintf(retval, len, "%s%s%s/%s/", envr, append, org, app);
    }
    else {
        (void)SDL_snprintf(retval, len, "%s%s%s/", envr, append, app);
    }

    for (ptr = retval + 1; *ptr; ptr++) {
        if (*ptr == '/') {
            *ptr = '\0';
            if (mkdir(retval, 0700) != 0 && errno != EEXIST) {
                goto error;
            }
            *ptr = '/';
        }
    }
    if (mkdir(retval, 0700) != 0 && errno != EEXIST) {
    error:
        //SDL_SetError("Couldn't create directory '%s': '%s'", retval, strerror(errno));
        SDL_free(retval);
        return NULL;
    }

    return retval;
}

sds klb_get_base_path()
{
    char* p_path = SDL_GetBasePath();
    sds path = sdsnew(p_path);

    SDL_free(p_path);
    return path;
}

sds klb_get_pref_path(const char* p_org, const char* p_app)
{
    char* p_pref_path = SDL_GetPrefPath(p_org, p_app);
    sds path = sdsnew(p_pref_path);

    SDL_free(p_pref_path);
    return path;
}

#endif

//end
