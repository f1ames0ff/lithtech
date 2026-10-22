#include <stdio.h>
#include <stdarg.h>

extern "C" int __snprintf(char *buffer, size_t count, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int result = _vsnprintf(buffer, count, format, args);
    va_end(args);
    return result;
}

#pragma comment(linker, "/alternatename:__imp___snprintf=___snprintf")
