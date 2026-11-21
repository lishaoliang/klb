// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_endian.h
/// @brief   端序转换与 buffer 读写辅助
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    on-disk 小端: FAT/exFAT/NTFS/VHDX 等; 大端: qcow2/VHD 等
///  \n 标量 le*/be*_to_cpu 与 cpu_to_le*/be* 用于 struct 字段; read/write_le*/be* 用于 buffer
///  \n 未知平台须 -DPFS_FORCE_LITTLE_ENDIAN 或 -DPFS_FORCE_BIG_ENDIAN
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_ENDIAN_H_
#define _PFS_ENDIAN_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#if defined(PFS_FORCE_LITTLE_ENDIAN) && defined(PFS_FORCE_BIG_ENDIAN)
#error "pfs_endian: PFS_FORCE_LITTLE_ENDIAN and PFS_FORCE_BIG_ENDIAN conflict"
#elif defined(PFS_FORCE_LITTLE_ENDIAN)
#define PFS_LITTLE_ENDIAN   1
#elif defined(PFS_FORCE_BIG_ENDIAN)
#define PFS_BIG_ENDIAN      1
#endif

#if !defined(PFS_LITTLE_ENDIAN) && !defined(PFS_BIG_ENDIAN)
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define PFS_LITTLE_ENDIAN   1
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define PFS_BIG_ENDIAN      1
#elif defined(__LITTLE_ENDIAN__)
#define PFS_LITTLE_ENDIAN   1
#elif defined(__BIG_ENDIAN__)
#define PFS_BIG_ENDIAN      1
#endif
#endif

#if !defined(PFS_LITTLE_ENDIAN) && !defined(PFS_BIG_ENDIAN)
#if defined(_WIN32)
#define PFS_LITTLE_ENDIAN   1
#elif defined(__ANDROID__)
#define PFS_LITTLE_ENDIAN   1
#elif defined(__APPLE__) && (defined(__arm64__) || defined(__aarch64__) || \
        defined(__x86_64__) || defined(__i386__))
#define PFS_LITTLE_ENDIAN   1
#endif
#endif

#if !defined(PFS_LITTLE_ENDIAN) && !defined(PFS_BIG_ENDIAN) && defined(__linux__)
#include <endian.h>
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define PFS_LITTLE_ENDIAN   1
#elif __BYTE_ORDER == __BIG_ENDIAN
#define PFS_BIG_ENDIAN      1
#endif
#endif

#if !defined(PFS_LITTLE_ENDIAN) && !defined(PFS_BIG_ENDIAN)
#if defined(__i386__) || defined(__x86_64__) || defined(__amd64__) || \
        defined(_M_IX86) || defined(_M_X64) || defined(_M_AMD64) || \
        defined(__ARMEL__) || defined(__AARCH64EL__) || defined(__THUMBEL__) || \
        (defined(__arm__) && !defined(__ARMEB__)) || \
        (defined(__aarch64__) && !defined(__AARCH64EB__)) || \
        (defined(__riscv) && defined(__riscv_byteorder__) && __riscv_byteorder__ == 0)
#define PFS_LITTLE_ENDIAN   1
#elif defined(__ARMEB__) || defined(__AARCH64EB__) || defined(__THUMBEB__) || \
        (defined(__riscv) && defined(__riscv_byteorder__) && __riscv_byteorder__ == 1) || \
        defined(__MIPSEB__) || defined(__MIPSEB) || defined(_MIPSEB) || \
        defined(__ppc__) || defined(__PPC__) || defined(__powerpc__)
#define PFS_BIG_ENDIAN      1
#endif
#endif

#if !defined(PFS_LITTLE_ENDIAN) && !defined(PFS_BIG_ENDIAN)
#error "pfs_endian: cannot detect host endianness; define PFS_FORCE_LITTLE_ENDIAN or PFS_FORCE_BIG_ENDIAN"
#endif

#if defined(_MSC_VER)
#include <stdlib.h>
#define PFS_BSWAP16(x)      ((uint16_t)_byteswap_ushort((unsigned short)(x)))
#define PFS_BSWAP32(x)      ((uint32_t)_byteswap_ulong((unsigned long)(x)))
#define PFS_BSWAP64(x)      ((uint64_t)_byteswap_uint64(x))
#elif defined(__GNUC__) || defined(__clang__)
#define PFS_BSWAP16(x)      ((uint16_t)__builtin_bswap16((uint16_t)(x)))
#define PFS_BSWAP32(x)      ((uint32_t)__builtin_bswap32((uint32_t)(x)))
#define PFS_BSWAP64(x)      ((uint64_t)__builtin_bswap64((uint64_t)(x)))
#else
static inline uint16_t pfs_bswap16(uint16_t x)
{
    return (uint16_t)((x >> 8) | (x << 8));
}

static inline uint32_t pfs_bswap32(uint32_t x)
{
    return ((x >> 24) & 0x000000FFu) | ((x >> 8) & 0x0000FF00u) |
           ((x << 8) & 0x00FF0000u) | ((x << 24) & 0xFF000000u);
}

static inline uint64_t pfs_bswap64(uint64_t x)
{
    return ((uint64_t)pfs_bswap32((uint32_t)x) << 32) |
           (uint64_t)pfs_bswap32((uint32_t)(x >> 32));
}

#define PFS_BSWAP16(x)      pfs_bswap16(x)
#define PFS_BSWAP32(x)      pfs_bswap32(x)
#define PFS_BSWAP64(x)      pfs_bswap64(x)
#endif

#if defined(PFS_LITTLE_ENDIAN)
#define pfs_le16_to_cpu(x)  ((uint16_t)(x))
#define pfs_le32_to_cpu(x)  ((uint32_t)(x))
#define pfs_le64_to_cpu(x)  ((uint64_t)(x))
#define pfs_cpu_to_le16(x)  ((uint16_t)(x))
#define pfs_cpu_to_le32(x)  ((uint32_t)(x))
#define pfs_cpu_to_le64(x)  ((uint64_t)(x))
#define pfs_be16_to_cpu(x)  PFS_BSWAP16(x)
#define pfs_be32_to_cpu(x)  PFS_BSWAP32(x)
#define pfs_be64_to_cpu(x)  PFS_BSWAP64(x)
#define pfs_cpu_to_be16(x)  PFS_BSWAP16(x)
#define pfs_cpu_to_be32(x)  PFS_BSWAP32(x)
#define pfs_cpu_to_be64(x)  PFS_BSWAP64(x)
#else
#define pfs_le16_to_cpu(x)  PFS_BSWAP16(x)
#define pfs_le32_to_cpu(x)  PFS_BSWAP32(x)
#define pfs_le64_to_cpu(x)  PFS_BSWAP64(x)
#define pfs_cpu_to_le16(x)  PFS_BSWAP16(x)
#define pfs_cpu_to_le32(x)  PFS_BSWAP32(x)
#define pfs_cpu_to_le64(x)  PFS_BSWAP64(x)
#define pfs_be16_to_cpu(x)  ((uint16_t)(x))
#define pfs_be32_to_cpu(x)  ((uint32_t)(x))
#define pfs_be64_to_cpu(x)  ((uint64_t)(x))
#define pfs_cpu_to_be16(x)  ((uint16_t)(x))
#define pfs_cpu_to_be32(x)  ((uint32_t)(x))
#define pfs_cpu_to_be64(x)  ((uint64_t)(x))
#endif

/// @brief 从 uint8_t buffer 读 LE 值; 与主机序无关
static inline uint16_t pfs_read_le16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

/// @brief 从 uint8_t buffer 读 LE 32 位值; 与主机序无关
static inline uint32_t pfs_read_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/// @brief 从 uint8_t buffer 读 LE 64 位值; 与主机序无关
static inline uint64_t pfs_read_le64(const uint8_t *p)
{
    return (uint64_t)pfs_read_le32(p) | ((uint64_t)pfs_read_le32(p + 4) << 32);
}

/// @brief 向 uint8_t buffer 写 LE 16 位值; 与主机序无关
static inline void pfs_write_le16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)(v >> 8);
}

/// @brief 向 uint8_t buffer 写 LE 32 位值; 与主机序无关
static inline void pfs_write_le32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
    p[2] = (uint8_t)((v >> 16) & 0xFFu);
    p[3] = (uint8_t)(v >> 24);
}

/// @brief 向 uint8_t buffer 写 LE 64 位值; 与主机序无关
static inline void pfs_write_le64(uint8_t *p, uint64_t v)
{
    pfs_write_le32(p, (uint32_t)v);
    pfs_write_le32(p + 4, (uint32_t)(v >> 32));
}

/// @brief 从 uint8_t buffer 读 BE 16 位值; 与主机序无关
static inline uint16_t pfs_read_be16(const uint8_t *p)
{
    return (uint16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
}

/// @brief 从 uint8_t buffer 读 BE 32 位值; 与主机序无关
static inline uint32_t pfs_read_be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

/// @brief 从 uint8_t buffer 读 BE 64 位值; 与主机序无关
static inline uint64_t pfs_read_be64(const uint8_t *p)
{
    return ((uint64_t)pfs_read_be32(p) << 32) | (uint64_t)pfs_read_be32(p + 4);
}

/// @brief 向 uint8_t buffer 写 BE 16 位值; 与主机序无关
static inline void pfs_write_be16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)((v >> 8) & 0xFFu);
    p[1] = (uint8_t)(v & 0xFFu);
}

/// @brief 向 uint8_t buffer 写 BE 32 位值; 与主机序无关
static inline void pfs_write_be32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)((v >> 24) & 0xFFu);
    p[1] = (uint8_t)((v >> 16) & 0xFFu);
    p[2] = (uint8_t)((v >> 8) & 0xFFu);
    p[3] = (uint8_t)(v & 0xFFu);
}

/// @brief 向 uint8_t buffer 写 BE 64 位值; 与主机序无关
static inline void pfs_write_be64(uint8_t *p, uint64_t v)
{
    pfs_write_be32(p, (uint32_t)(v >> 32));
    pfs_write_be32(p + 4, (uint32_t)v);
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_ENDIAN_H_

// end
