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

size_t realloc_buffer(Buffer *buffer, char *added_content, size_t added_len) {
    char *new_buffer = realloc(buffer->content, buffer->length + added_len + 1);
    if (!new_buffer) {
        fprintf(stderr, "Memory allocation failed\n");
        return 0;
    }
    buffer->content = new_buffer;

    memcpy(buffer->content + buffer->length, added_content, added_len);
    buffer->length += added_len;
    buffer->content[buffer->length] = '\0';
}
