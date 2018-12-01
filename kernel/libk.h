#ifndef _LIBK_H_
#define _LIBK_H_

#include <stdarg.h>
#include "io.h"

class K {
public:
    static void snprintf (OutputStream<char>& sink, long maxlen, const char *fmt, ...);
    static void vsnprintf (OutputStream<char>& sink, long maxlen, const char *fmt, va_list arg);
    static long strlen(const char* str);
    static int isdigit(int c);
};

#endif
