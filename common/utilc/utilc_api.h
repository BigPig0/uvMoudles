#ifndef _UTIL_CAPI_
#define _UTIL_CAPI_
#include "utilc_export.h"
#include <string.h>

#if defined(WINDOWS_IMPL)
#if _MSC_VER < 1700
#include <crtdefs.h>
#include <crtdbg.h>
typedef signed char        int8_t;
typedef short              int16_t;
typedef int                int32_t;
typedef long long          int64_t;
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;
#else
#include <stdint.h>
#endif

#define strcasecmp  _stricmp
#define strncasecmp _strnicmp
#define strtok_r    strtok_s
int  _UTILC_API getpid();
int  _UTILC_API gettid();
void _UTILC_API sleep(uint32_t dwMilliseconds);

#elif defined(LINUX_IMPL)
#include <unistd.h>
#endif

#endif