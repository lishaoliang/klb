// Doc Encode : UTF-8 BOM, Unix(LF)
// kuart: 串口 open/read/write/close (Linux termios + Windows COM)
#include "klua/klua.h"
#include "klbmem/klb_mem.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/stat.h>
#ifndef CRTSCTS
#define CRTSCTS 020000000000
#endif
/* 避免 include asm/termbits.h 与 termios.h 冲突; 自建 termios2 + ioctl */
#ifndef BOTHER
#define BOTHER 0010000
#endif
struct klua_kuart_termios2
{
    tcflag_t c_iflag;
    tcflag_t c_oflag;
    tcflag_t c_cflag;
    tcflag_t c_lflag;
    cc_t c_line;
    cc_t c_cc[19];
    speed_t c_ispeed;
    speed_t c_ospeed;
};
/* 勿用系统 TCGETS2(struct termios2): 未含完整类型定义时 sizeof 失败 */
#define KLUA_KUART_TCGETS2 _IOR('T', 0x2A, struct klua_kuart_termios2)
#define KLUA_KUART_TCSETS2 _IOW('T', 0x2B, struct klua_kuart_termios2)
#endif


#define KLUA_KUART_PORT_HANDLE          "KUART_PORT_HANDLE*"
#define KLUA_KUART_READ_MAX             (256 * 1024)
#define KLUA_KUART_LIST_MAX             256
#define KLUA_KUART_BAUD_MAX             12000000


/// @struct klua_kuart_port_t
/// @brief  串口会话
typedef struct klua_kuart_port_t_
{
#ifdef _WIN32
    HANDLE                      h;          ///< COM handle
#else
    int                         fd;         ///< tty fd
#endif
    bool                        closed;     ///< 已 close
}klua_kuart_port_t;


//////////////////////////////////////////////////////////////////////////
// userdata


static klua_kuart_port_t* new_klua_kuart_port(lua_State* L)
{
    klua_kuart_port_t* p_port = (klua_kuart_port_t*)lua_newuserdata(L, sizeof(klua_kuart_port_t));
    KLB_MEMSET(p_port, 0, sizeof(klua_kuart_port_t));
#ifdef _WIN32
    p_port->h = INVALID_HANDLE_VALUE;
#else
    p_port->fd = -1;
#endif
    p_port->closed = false;
    luaL_setmetatable(L, KLUA_KUART_PORT_HANDLE);
    return p_port;
}


static klua_kuart_port_t* to_klua_kuart_port(lua_State* L, int index)
{
    klua_kuart_port_t* p_port = (klua_kuart_port_t*)luaL_checkudata(L, index, KLUA_KUART_PORT_HANDLE);
    luaL_argcheck(L, NULL != p_port, index, "'kuart.port' expected");
    return p_port;
}


static void close_fd_klua_kuart_port(klua_kuart_port_t* p_port)
{
    if (NULL == p_port)
    {
        return;
    }

    if (p_port->closed)
    {
        return;
    }

#ifdef _WIN32
    if (INVALID_HANDLE_VALUE != p_port->h)
    {
        CloseHandle(p_port->h);
        p_port->h = INVALID_HANDLE_VALUE;
    }
#else
    if (0 <= p_port->fd)
    {
        close(p_port->fd);
        p_port->fd = -1;
    }
#endif

    p_port->closed = true;
}


//////////////////////////////////////////////////////////////////////////
// baud / opts


/// @brief 波特率合法性; 含非标准高波特率 (如 1500000); 硬上限留给驱动拒绝
static bool baud_ok_klua_kuart(int baud)
{
    if (0 >= baud)
    {
        return false;
    }

    if (KLUA_KUART_BAUD_MAX < baud)
    {
        return false;
    }

    return true;
}


#ifndef _WIN32
/// @brief 标准 Bxxxx 映射; 无宏时返回 false → 走 BOTHER + termios2
static bool baud_to_speed_klua_kuart(int baud, speed_t* p_spd)
{
    if (NULL == p_spd)
    {
        return false;
    }

    if (50 == baud) { *p_spd = B50; return true; }
    if (75 == baud) { *p_spd = B75; return true; }
    if (110 == baud) { *p_spd = B110; return true; }
    if (134 == baud) { *p_spd = B134; return true; }
    if (150 == baud) { *p_spd = B150; return true; }
    if (200 == baud) { *p_spd = B200; return true; }
    if (300 == baud) { *p_spd = B300; return true; }
    if (600 == baud) { *p_spd = B600; return true; }
    if (1200 == baud) { *p_spd = B1200; return true; }
    if (1800 == baud) { *p_spd = B1800; return true; }
    if (2400 == baud) { *p_spd = B2400; return true; }
    if (4800 == baud) { *p_spd = B4800; return true; }
    if (9600 == baud) { *p_spd = B9600; return true; }
    if (19200 == baud) { *p_spd = B19200; return true; }
    if (38400 == baud) { *p_spd = B38400; return true; }
    if (57600 == baud) { *p_spd = B57600; return true; }
    if (115200 == baud) { *p_spd = B115200; return true; }
#ifdef B230400
    if (230400 == baud) { *p_spd = B230400; return true; }
#endif
#ifdef B460800
    if (460800 == baud) { *p_spd = B460800; return true; }
#endif
#ifdef B500000
    if (500000 == baud) { *p_spd = B500000; return true; }
#endif
#ifdef B576000
    if (576000 == baud) { *p_spd = B576000; return true; }
#endif
#ifdef B921600
    if (921600 == baud) { *p_spd = B921600; return true; }
#endif
#ifdef B1000000
    if (1000000 == baud) { *p_spd = B1000000; return true; }
#endif
#ifdef B1152000
    if (1152000 == baud) { *p_spd = B1152000; return true; }
#endif
#ifdef B1500000
    if (1500000 == baud) { *p_spd = B1500000; return true; }
#endif
#ifdef B2000000
    if (2000000 == baud) { *p_spd = B2000000; return true; }
#endif
#ifdef B2500000
    if (2500000 == baud) { *p_spd = B2500000; return true; }
#endif
#ifdef B3000000
    if (3000000 == baud) { *p_spd = B3000000; return true; }
#endif
#ifdef B3500000
    if (3500000 == baud) { *p_spd = B3500000; return true; }
#endif
#ifdef B4000000
    if (4000000 == baud) { *p_spd = B4000000; return true; }
#endif

    return false;
}
#else
static DWORD baud_to_cbr_klua_kuart(int baud)
{
    if (110 == baud) { return CBR_110; }
    if (300 == baud) { return CBR_300; }
    if (600 == baud) { return CBR_600; }
    if (1200 == baud) { return CBR_1200; }
    if (2400 == baud) { return CBR_2400; }
    if (4800 == baud) { return CBR_4800; }
    if (9600 == baud) { return CBR_9600; }
    if (14400 == baud) { return CBR_14400; }
    if (19200 == baud) { return CBR_19200; }
    if (38400 == baud) { return CBR_38400; }
    if (56000 == baud) { return CBR_56000; }
    if (57600 == baud) { return CBR_57600; }
    if (115200 == baud) { return CBR_115200; }
    if (128000 == baud) { return CBR_128000; }
    if (256000 == baud) { return CBR_256000; }
    return (DWORD)baud;
}
#endif


/// @brief 从 opts 表读串口参数; 缺省 115200 8N1 flow=none
static int read_opts_klua_kuart(lua_State* L, int idx, int* p_baud, int* p_databits,
    char* p_parity, size_t parity_sz, int* p_stop, char* p_flow, size_t flow_sz)
{
    *p_baud = 115200;
    *p_databits = 8;
    *p_stop = 1;
    snprintf(p_parity, parity_sz, "none");
    snprintf(p_flow, flow_sz, "none");

    if (LUA_TTABLE != lua_type(L, idx))
    {
        return 0;
    }

    lua_getfield(L, idx, "baud");
    if (LUA_TNUMBER == lua_type(L, -1))
    {
        *p_baud = (int)lua_tointeger(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, idx, "databits");
    if (LUA_TNUMBER == lua_type(L, -1))
    {
        *p_databits = (int)lua_tointeger(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, idx, "stop");
    if (LUA_TNUMBER == lua_type(L, -1))
    {
        *p_stop = (int)lua_tointeger(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, idx, "parity");
    if (LUA_TSTRING == lua_type(L, -1))
    {
        const char* p = lua_tostring(L, -1);
        if (NULL != p)
        {
            snprintf(p_parity, parity_sz, "%s", p);
        }
    }
    lua_pop(L, 1);

    lua_getfield(L, idx, "flow");
    if (LUA_TSTRING == lua_type(L, -1))
    {
        const char* p = lua_tostring(L, -1);
        if (NULL != p)
        {
            snprintf(p_flow, flow_sz, "%s", p);
        }
    }
    lua_pop(L, 1);

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// platform open / configure


#ifdef _WIN32


static void win_path_klua_kuart(const char* p_path, char* p_out, size_t out_sz)
{
    // COM10+ 须 \\.\COMn
    if (0 == _strnicmp(p_path, "COM", 3) && NULL != p_path[3] && isdigit((unsigned char)p_path[3]))
    {
        int n = atoi(p_path + 3);
        if (10 <= n)
        {
            snprintf(p_out, out_sz, "\\\\.\\%s", p_path);
            return;
        }
    }

    if (0 == strncmp(p_path, "\\\\.\\", 4))
    {
        snprintf(p_out, out_sz, "%s", p_path);
        return;
    }

    snprintf(p_out, out_sz, "%s", p_path);
}


static int configure_win_klua_kuart(HANDLE h, int baud, int databits, const char* p_parity,
    int stop, const char* p_flow)
{
    DCB dcb;
    COMMTIMEOUTS to;

    KLB_MEMSET(&dcb, 0, sizeof(dcb));
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(h, &dcb))
    {
        return -1;
    }

    dcb.BaudRate = baud_to_cbr_klua_kuart(baud);
    dcb.ByteSize = (BYTE)databits;
    dcb.fBinary = TRUE;
    dcb.fParity = FALSE;
    dcb.Parity = NOPARITY;
    dcb.StopBits = (1 == stop) ? ONESTOPBIT : TWOSTOPBITS;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;

    if (0 == _stricmp(p_parity, "even"))
    {
        dcb.fParity = TRUE;
        dcb.Parity = EVENPARITY;
    }
    else if (0 == _stricmp(p_parity, "odd"))
    {
        dcb.fParity = TRUE;
        dcb.Parity = ODDPARITY;
    }
    else if (0 == _stricmp(p_parity, "mark"))
    {
        dcb.fParity = TRUE;
        dcb.Parity = MARKPARITY;
    }
    else if (0 == _stricmp(p_parity, "space"))
    {
        dcb.fParity = TRUE;
        dcb.Parity = SPACEPARITY;
    }

    if (0 == _stricmp(p_flow, "rtscts") || 0 == _stricmp(p_flow, "hardware"))
    {
        dcb.fOutxCtsFlow = TRUE;
        dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
    }
    else if (0 == _stricmp(p_flow, "xonxoff") || 0 == _stricmp(p_flow, "software"))
    {
        dcb.fOutX = TRUE;
        dcb.fInX = TRUE;
    }

    if (!SetCommState(h, &dcb))
    {
        return -1;
    }

    KLB_MEMSET(&to, 0, sizeof(to));
    to.ReadIntervalTimeout = MAXDWORD;
    to.ReadTotalTimeoutMultiplier = 0;
    to.ReadTotalTimeoutConstant = 0;
    to.WriteTotalTimeoutMultiplier = 0;
    to.WriteTotalTimeoutConstant = 5000;
    if (!SetCommTimeouts(h, &to))
    {
        return -1;
    }

    PurgeComm(h, PURGE_RXCLEAR | PURGE_TXCLEAR);
    return 0;
}


static HANDLE open_win_klua_kuart(const char* p_path, int baud, int databits, const char* p_parity,
    int stop, const char* p_flow)
{
    char win_path[280] = { 0 };
    HANDLE h = INVALID_HANDLE_VALUE;

    win_path_klua_kuart(p_path, win_path, sizeof(win_path));
    h = CreateFileA(win_path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == h)
    {
        return INVALID_HANDLE_VALUE;
    }

    if (0 != configure_win_klua_kuart(h, baud, databits, p_parity, stop, p_flow))
    {
        CloseHandle(h);
        return INVALID_HANDLE_VALUE;
    }

    return h;
}


#else /* POSIX */


/// @brief 非标准波特率: TCGETS2 + BOTHER + c_ispeed/c_ospeed
static int set_baud_custom_posix_klua_kuart(int fd, int baud)
{
    struct klua_kuart_termios2 tio2;

    if (0 != ioctl(fd, KLUA_KUART_TCGETS2, &tio2))
    {
        assert(ENOMEM != errno);
        return -1;
    }

    tio2.c_cflag &= (tcflag_t)~CBAUD;
#ifdef CIBAUD
    tio2.c_cflag &= (tcflag_t)~CIBAUD;
#endif
    tio2.c_cflag |= (tcflag_t)BOTHER;
    tio2.c_ispeed = (speed_t)baud;
    tio2.c_ospeed = (speed_t)baud;

    if (0 != ioctl(fd, KLUA_KUART_TCSETS2, &tio2))
    {
        assert(ENOMEM != errno);
        return -1;
    }

    return 0;
}


static int configure_posix_klua_kuart(int fd, int baud, int databits, const char* p_parity,
    int stop, const char* p_flow)
{
    struct termios tio;
    speed_t spd = B0;
    bool std_baud = baud_to_speed_klua_kuart(baud, &spd);

    if (0 != tcgetattr(fd, &tio))
    {
        assert(ENOMEM != errno);
        return -1;
    }

    cfmakeraw(&tio);
    if (std_baud)
    {
        cfsetispeed(&tio, spd);
        cfsetospeed(&tio, spd);
    }
    else
    {
        /* 占位; 随后 TCSETS2 + BOTHER 覆盖 */
        cfsetispeed(&tio, B38400);
        cfsetospeed(&tio, B38400);
    }

    tio.c_cflag |= (CLOCAL | CREAD);
    tio.c_cflag &= (tcflag_t)~CSIZE;
    if (5 == databits)
    {
        tio.c_cflag |= CS5;
    }
    else if (6 == databits)
    {
        tio.c_cflag |= CS6;
    }
    else if (7 == databits)
    {
        tio.c_cflag |= CS7;
    }
    else
    {
        tio.c_cflag |= CS8;
    }

    if (2 == stop)
    {
        tio.c_cflag |= CSTOPB;
    }
    else
    {
        tio.c_cflag &= (tcflag_t)~CSTOPB;
    }

    tio.c_cflag &= (tcflag_t)~(PARENB | PARODD);
    if (0 == strcmp(p_parity, "even"))
    {
        tio.c_cflag |= PARENB;
    }
    else if (0 == strcmp(p_parity, "odd"))
    {
        tio.c_cflag |= (PARENB | PARODD);
    }

    tio.c_cflag &= (tcflag_t)~CRTSCTS;
    tio.c_iflag &= (tcflag_t)~(IXON | IXOFF | IXANY);
    if (0 == strcmp(p_flow, "rtscts") || 0 == strcmp(p_flow, "hardware"))
    {
        tio.c_cflag |= CRTSCTS;
    }
    else if (0 == strcmp(p_flow, "xonxoff") || 0 == strcmp(p_flow, "software"))
    {
        tio.c_iflag |= (IXON | IXOFF);
    }

    tio.c_cc[VMIN] = 0;
    tio.c_cc[VTIME] = 0;

    if (0 != tcsetattr(fd, TCSANOW, &tio))
    {
        assert(ENOMEM != errno);
        return -1;
    }

    if (!std_baud)
    {
        if (0 != set_baud_custom_posix_klua_kuart(fd, baud))
        {
            return -1;
        }
    }

    tcflush(fd, TCIOFLUSH);
    return 0;
}


static int open_posix_klua_kuart(const char* p_path, int baud, int databits, const char* p_parity,
    int stop, const char* p_flow)
{
    int fd = -1;
    int flags = 0;

    fd = open(p_path, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (0 > fd)
    {
        assert(ENOMEM != errno);
        return -1;
    }

    flags = fcntl(fd, F_GETFL, 0);
    if (0 <= flags)
    {
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }

    if (0 != configure_posix_klua_kuart(fd, baud, databits, p_parity, stop, p_flow))
    {
        close(fd);
        return -1;
    }

    return fd;
}


#endif


//////////////////////////////////////////////////////////////////////////
// list


/// @param include_legacy 非 0 时含 ttyS* (PC 上常很多空节点)
static bool name_is_serial_klua_kuart(const char* p_name, int include_legacy)
{
    if (NULL == p_name || '\0' == p_name[0])
    {
        return false;
    }

#ifdef _WIN32
    (void)include_legacy;
    if (0 == _strnicmp(p_name, "COM", 3) && isdigit((unsigned char)p_name[3]))
    {
        return true;
    }
#else
    if (0 == strncmp(p_name, "ttyUSB", 6) || 0 == strncmp(p_name, "ttyACM", 6)
        || 0 == strncmp(p_name, "ttyAMA", 6)
        || 0 == strncmp(p_name, "cu.", 3) || 0 == strncmp(p_name, "tty.usb", 7))
    {
        return true;
    }

    if (0 != include_legacy && 0 == strncmp(p_name, "ttyS", 4))
    {
        return true;
    }
#endif

    return false;
}


/// @brief 列候选串口
/// @note Lua: list = kuart.list([max_entries [, include_legacy]])
static int klua_kuart_list(lua_State* L)
{
    int max_entries = KLUA_KUART_LIST_MAX;
    int include_legacy = 0;
    int n = 0;

    if (1 <= lua_gettop(L) && LUA_TNIL != lua_type(L, 1))
    {
        max_entries = (int)luaL_checkinteger(L, 1);
        if (1 > max_entries)
        {
            max_entries = 1;
        }
        if (KLUA_KUART_LIST_MAX < max_entries)
        {
            max_entries = KLUA_KUART_LIST_MAX;
        }
    }

    if (2 <= lua_gettop(L) && LUA_TNIL != lua_type(L, 2))
    {
        include_legacy = lua_toboolean(L, 2) ? 1 : 0;
    }

    lua_newtable(L);

#ifdef _WIN32
    {
        char devices[65536];
        DWORD got = 0;
        const char* p = NULL;

        (void)include_legacy;
        KLB_MEMSET(devices, 0, sizeof(devices));
        got = QueryDosDeviceA(NULL, devices, (DWORD)sizeof(devices));
        if (0 == got)
        {
            // fallback COM1..COM256 probe via GetDefaultCommConfig
            int i = 0;
            for (i = 1; i <= 256 && n < max_entries; ++i)
            {
                char name[16];
                COMMCONFIG cfg;
                DWORD sz = sizeof(cfg);

                snprintf(name, sizeof(name), "COM%d", i);
                KLB_MEMSET(&cfg, 0, sizeof(cfg));
                cfg.dwSize = sizeof(cfg);
                if (GetDefaultCommConfigA(name, &cfg, &sz))
                {
                    lua_pushstring(L, name);
                    lua_rawseti(L, -2, n + 1);
                    n += 1;
                }
            }
            return 1;
        }

        p = devices;
        while ('\0' != p[0] && n < max_entries)
        {
            if (name_is_serial_klua_kuart(p, 0))
            {
                lua_pushstring(L, p);
                lua_rawseti(L, -2, n + 1);
                n += 1;
            }
            p += strlen(p) + 1;
        }
    }
#else
    {
        DIR* p_dir = NULL;
        struct dirent* p_ent = NULL;

        p_dir = opendir("/dev");
        if (NULL == p_dir)
        {
            assert(ENOMEM != errno);
            return 1;
        }

        while (NULL != (p_ent = readdir(p_dir)) && n < max_entries)
        {
            char full[288];
            struct stat st;

            if (!name_is_serial_klua_kuart(p_ent->d_name, include_legacy))
            {
                continue;
            }

            snprintf(full, sizeof(full), "/dev/%s", p_ent->d_name);
            if (0 != stat(full, &st))
            {
                continue;
            }

            if (!S_ISCHR(st.st_mode))
            {
                continue;
            }

            lua_pushstring(L, full);
            lua_rawseti(L, -2, n + 1);
            n += 1;
        }

        closedir(p_dir);
    }
#endif

    return 1;
}


//////////////////////////////////////////////////////////////////////////
// open / close / read / write / flush


/// @brief 打开串口
/// @note Lua: port, err = kuart.open(path [, opts])
static int klua_kuart_open(lua_State* L)
{
    const char* p_path = NULL;
    int baud = 115200;
    int databits = 8;
    int stop = 1;
    char parity[16] = { 0 };
    char flow[16] = { 0 };
    klua_kuart_port_t* p_port = NULL;

    p_path = luaL_checkstring(L, 1);
    if (NULL == p_path || '\0' == p_path[0])
    {
        lua_pushnil(L);
        lua_pushstring(L, "bad path");
        return 2;
    }

    read_opts_klua_kuart(L, 2, &baud, &databits, parity, sizeof(parity), &stop, flow, sizeof(flow));

    if (!baud_ok_klua_kuart(baud))
    {
        lua_pushnil(L);
        lua_pushstring(L, "bad baud");
        return 2;
    }

    if (5 > databits || 8 < databits)
    {
        lua_pushnil(L);
        lua_pushstring(L, "bad databits");
        return 2;
    }

    if (1 != stop && 2 != stop)
    {
        lua_pushnil(L);
        lua_pushstring(L, "bad stop");
        return 2;
    }

#ifdef _WIN32
    {
        HANDLE h = open_win_klua_kuart(p_path, baud, databits, parity, stop, flow);
        if (INVALID_HANDLE_VALUE == h)
        {
            lua_pushnil(L);
            lua_pushstring(L, "open fail");
            return 2;
        }

        p_port = new_klua_kuart_port(L);
        p_port->h = h;
        p_port->closed = false;
        return 1;
    }
#else
    {
        int fd = open_posix_klua_kuart(p_path, baud, databits, parity, stop, flow);
        if (0 > fd)
        {
            lua_pushnil(L);
            lua_pushstring(L, "open fail");
            return 2;
        }

        p_port = new_klua_kuart_port(L);
        p_port->fd = fd;
        p_port->closed = false;
        return 1;
    }
#endif
}


static int klua_kuart_port_close(lua_State* L)
{
    klua_kuart_port_t* p_port = to_klua_kuart_port(L, 1);
    close_fd_klua_kuart_port(p_port);
    lua_pushboolean(L, 1);
    return 1;
}


/// @note Lua: n, err = port:write(data)
static int klua_kuart_port_write(lua_State* L)
{
    klua_kuart_port_t* p_port = to_klua_kuart_port(L, 1);
    size_t len = 0;
    const char* p_data = NULL;
    size_t off = 0;

    if (p_port->closed)
    {
        lua_pushnil(L);
        lua_pushstring(L, "closed");
        return 2;
    }

    p_data = luaL_checklstring(L, 2, &len);
    if (NULL == p_data)
    {
        lua_pushnil(L);
        lua_pushstring(L, "bad data");
        return 2;
    }

    if (0 == len)
    {
        lua_pushinteger(L, 0);
        return 1;
    }

#ifdef _WIN32
    while (off < len)
    {
        DWORD nwrite = 0;
        if (!WriteFile(p_port->h, p_data + off, (DWORD)(len - off), &nwrite, NULL))
        {
            if (0 < off)
            {
                lua_pushinteger(L, (lua_Integer)off);
                return 1;
            }
            lua_pushnil(L);
            lua_pushstring(L, "write fail");
            return 2;
        }
        if (0 == nwrite)
        {
            break;
        }
        off += (size_t)nwrite;
    }
#else
    while (off < len)
    {
        ssize_t nwrite = write(p_port->fd, p_data + off, len - off);
        if (0 > nwrite)
        {
            if (EAGAIN == errno || EWOULDBLOCK == errno)
            {
                break;
            }
            assert(ENOMEM != errno);
            if (0 < off)
            {
                lua_pushinteger(L, (lua_Integer)off);
                return 1;
            }
            lua_pushnil(L);
            lua_pushstring(L, "write fail");
            return 2;
        }
        if (0 == nwrite)
        {
            break;
        }
        off += (size_t)nwrite;
    }
#endif

    lua_pushinteger(L, (lua_Integer)off);
    return 1;
}


/// @note Lua: data, err = port:read(max_bytes [, timeout_ms])
static int klua_kuart_port_read(lua_State* L)
{
    klua_kuart_port_t* p_port = to_klua_kuart_port(L, 1);
    int max_bytes = 0;
    int timeout_ms = 0;
    char* p_buf = NULL;
    int nread_total = 0;

    if (p_port->closed)
    {
        lua_pushnil(L);
        lua_pushstring(L, "closed");
        return 2;
    }

    max_bytes = (int)luaL_checkinteger(L, 2);
    if (1 > max_bytes)
    {
        lua_pushnil(L);
        lua_pushstring(L, "bad max_bytes");
        return 2;
    }
    if (KLUA_KUART_READ_MAX < max_bytes)
    {
        max_bytes = KLUA_KUART_READ_MAX;
    }

    if (3 <= lua_gettop(L) && LUA_TNIL != lua_type(L, 3))
    {
        timeout_ms = (int)luaL_checkinteger(L, 3);
        if (0 > timeout_ms)
        {
            timeout_ms = 0;
        }
    }

    p_buf = (char*)KLB_MALLOC(char, (size_t)max_bytes, 0);

#ifdef _WIN32
    {
        COMMTIMEOUTS to;
        DWORD nread = 0;

        KLB_MEMSET(&to, 0, sizeof(to));
        if (0 >= timeout_ms)
        {
            to.ReadIntervalTimeout = MAXDWORD;
            to.ReadTotalTimeoutMultiplier = 0;
            to.ReadTotalTimeoutConstant = 0;
        }
        else
        {
            to.ReadIntervalTimeout = MAXDWORD;
            to.ReadTotalTimeoutMultiplier = MAXDWORD;
            to.ReadTotalTimeoutConstant = (DWORD)timeout_ms;
        }
        SetCommTimeouts(p_port->h, &to);

        if (!ReadFile(p_port->h, p_buf, (DWORD)max_bytes, &nread, NULL))
        {
            KLB_FREE(p_buf);
            lua_pushnil(L);
            lua_pushstring(L, "read fail");
            return 2;
        }

        nread_total = (int)nread;
    }
#else
    {
        if (0 < timeout_ms)
        {
            fd_set rfds;
            struct timeval tv;
            int nready = 0;

            FD_ZERO(&rfds);
            FD_SET(p_port->fd, &rfds);
            tv.tv_sec = timeout_ms / 1000;
            tv.tv_usec = (timeout_ms % 1000) * 1000;
            nready = select(p_port->fd + 1, &rfds, NULL, NULL, &tv);
            if (0 > nready)
            {
                assert(ENOMEM != errno);
                KLB_FREE(p_buf);
                lua_pushnil(L);
                lua_pushstring(L, "select fail");
                return 2;
            }
            if (0 == nready)
            {
                KLB_FREE(p_buf);
                lua_pushlstring(L, "", 0);
                return 1;
            }
        }

        {
            ssize_t nread = read(p_port->fd, p_buf, (size_t)max_bytes);
            if (0 > nread)
            {
                if (EAGAIN == errno || EWOULDBLOCK == errno)
                {
                    KLB_FREE(p_buf);
                    lua_pushlstring(L, "", 0);
                    return 1;
                }
                assert(ENOMEM != errno);
                KLB_FREE(p_buf);
                lua_pushnil(L);
                lua_pushstring(L, "read fail");
                return 2;
            }
            nread_total = (int)nread;
        }
    }
#endif

    lua_pushlstring(L, p_buf, (size_t)nread_total);
    KLB_FREE(p_buf);
    return 1;
}


/// @note Lua: ok, err = port:flush()
static int klua_kuart_port_flush(lua_State* L)
{
    klua_kuart_port_t* p_port = to_klua_kuart_port(L, 1);

    if (p_port->closed)
    {
        lua_pushnil(L);
        lua_pushstring(L, "closed");
        return 2;
    }

#ifdef _WIN32
    if (!PurgeComm(p_port->h, PURGE_RXCLEAR | PURGE_TXCLEAR))
    {
        lua_pushnil(L);
        lua_pushstring(L, "flush fail");
        return 2;
    }
#else
    if (0 != tcflush(p_port->fd, TCIOFLUSH))
    {
        assert(ENOMEM != errno);
        lua_pushnil(L);
        lua_pushstring(L, "flush fail");
        return 2;
    }
#endif

    lua_pushboolean(L, 1);
    return 1;
}


static void klua_kuart_port_createmeta(lua_State* L)
{
    static luaL_Reg meth[] =
    {
        { "close",  klua_kuart_port_close },
        { "write",  klua_kuart_port_write },
        { "read",   klua_kuart_port_read },
        { "flush",  klua_kuart_port_flush },
        { NULL,     NULL }
    };

    static const luaL_Reg metameth[] =
    {
        { "__index",    NULL },
        { "__gc",       klua_kuart_port_close },
        { "__close",    klua_kuart_port_close },
        { NULL,         NULL }
    };

    luaL_newmetatable(L, KLUA_KUART_PORT_HANDLE);
    luaL_setfuncs(L, metameth, 0);
    luaL_newlibtable(L, meth);
    luaL_setfuncs(L, meth, 0);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}


static int klua_kuart_available(lua_State* L)
{
#ifdef _WIN32
    lua_pushboolean(L, 1);
    lua_pushstring(L, "windows");
#else
    lua_pushboolean(L, 1);
    lua_pushstring(L, "posix");
#endif
    return 2;
}


int klua_open_kuart(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "available",  klua_kuart_available },
        { "list",       klua_kuart_list },
        { "open",       klua_kuart_open },
        { NULL,         NULL }
    };

    klua_kuart_port_createmeta(L);
    luaL_newlib(L, lib);
    return 1;
}

// end
