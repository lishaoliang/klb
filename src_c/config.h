/* Automatically generated by configure - do not modify! */
#ifndef FFMPEG_CONFIG_H
#define FFMPEG_CONFIG_H

#include "klb_type.h"

#define FFMPEG_CONFIGURATION "--enable-cross-compile --target-os=none --arch=x86_64 --cpu=generic --disable-asm --disable-runtime-cpudetect"
#define FFMPEG_LICENSE "LGPL version 2.1 or later"
#define av_restrict restrict
#define HAVE_MMX2 HAVE_MMXEXT
#define HAVE_MMXEXT 0


#define HAVE_MALLOC_H 1
#define CONFIG_MEMORY_POISONING 0
#define HAVE_FAST_UNALIGNED 0
#define CONFIG_FTRAPV 0


#define AV_HAVE_BIGENDIAN 0
#define AV_HAVE_FAST_UNALIGNED 0
#define HAVE_FAST_UNALIGNED 0

// windows
#if defined(_WIN32)
#define HAVE_WINDOWS_H 1
#define HAVE_BCRYPT 1
#define HAVE_W32THREADS 1
#define HAVE_DOS_PATHS 1
#define HAVE_GETSYSTEMTIMEASFILETIME 1
#define HAVE_SLEEP 1
#else
#define HAVE_GETTIMEOFDAY 1
#define HAVE_UNISTD_H 1
#define HAVE_CLOCK_GETTIME 1
#define CLOCK_MONOTONIC 1
#define HAVE_USLEEP 1
#endif


#define HAVE_CBRT 1
#define HAVE_CBRTF 1
#define HAVE_COPYSIGN 1
#define HAVE_ERF 1
#define HAVE_HYPOT 1
#define HAVE_RINT 1
#define HAVE_LRINT 1
#define HAVE_LRINTF 1
#define HAVE_ROUND 1
#define HAVE_ROUNDF 1
#define HAVE_TRUNC 1
#define HAVE_TRUNCF 1
#define HAVE_ISINF 1
#define HAVE_ISNAN 1
#define HAVE_ISFINITE 1

#endif /* FFMPEG_CONFIG_H */