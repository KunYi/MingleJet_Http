#pragma once

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

#ifndef __cplusplus

#ifndef bool
#define bool _Bool
#define true (1)
#define false (0)
#endif

#ifdef _WIN32
#include <windows.h>
#define MAX_PATH_LENGTH MAX_PATH
#else
#include <limits.h>
#define MAX_PATH_LENGTH PATH_MAX
#endif

#define STR_HELPER(x) #x
#define STR_VERSION(x) STR_HELPER(x)

#endif
