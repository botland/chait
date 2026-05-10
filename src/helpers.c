#include "client.h"
#include <stdarg.h>

const char* format(const char* format, ...) {
    static char buffer[1024];
    va_list args;
    va_start(args, format);

    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    return buffer;
}
