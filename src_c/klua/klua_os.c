#include "klua/klua.h"


#ifdef _WIN32
    //define something for Windows (32-bit and 64-bit, this part is common)
    #ifdef _WIN64
        //define something for Windows (64-bit only)
    #else
        //define something for Windows (32-bit only)
    #endif

    #define KLUA_OS_STR         "windows"
    #define KLUA_ARCH_STR       "amd64"
#elif __APPLE__
    #include "TargetConditionals.h"
    #if TARGET_IPHONE_SIMULATOR
        // iOS Simulator
    #elif TARGET_OS_IPHONE
        // iOS device
    #elif TARGET_OS_MAC
        // Other kinds of Mac OS
    #else
        #error "Unknown Apple platform"
    #endif

    #define KLUA_OS_STR         "apple"
    #define KLUA_ARCH_STR       "amd64"
#elif __linux__
    // linux
    #define KLUA_OS_STR         "linux"
    #define KLUA_ARCH_STR       "amd64"
#elif __unix__ // all unices not caught above
    // Unix
    #define KLUA_OS_STR         "unix"
    #define KLUA_ARCH_STR       "amd64"
#elif defined(_POSIX_VERSION)
    // POSIX
    #define KLUA_OS_STR         "unix"
    #define KLUA_ARCH_STR       "amd64"
#elif defined(__STM32__)
    // STM32
    #define KLUA_OS_STR         "stm32"
    #define KLUA_ARCH_STR       "arm"
#else
    //#error "Unknown compiler"
    #define KLUA_OS_STR         "unknown"
    #define KLUA_ARCH_STR       "unknown"
#endif


int klua_pre_open_kos(lua_State* L)
{
    static luaL_Reg kos_lib[] =
    {
        { NULL,         NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, kos_lib);

    lua_pushstring(L, KLUA_OS_STR);
    lua_setfield(L, -2, "os");

    lua_pushstring(L, KLUA_ARCH_STR);
    lua_setfield(L, -2, "arch");

    return 1;
}
