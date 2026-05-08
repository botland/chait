#include "client.h"

// Callback function to write received data
size_t WriteCallback(void *contents, size_t size, size_t nmemb, char *userp) {
    size_t realsize = size * nmemb;
    size_t current_len = strlen(userp);
    
    // Check if we have enough space to append the new data
    if (current_len + realsize >= MAX_RESPONSE_SIZE) {
        // Truncate if necessary, but still append what we can
        size_t available = MAX_RESPONSE_SIZE - current_len - 1;
        if (available > 0) {
            strncat(userp, contents, available);
        }
        return realsize; // Return full size to indicate we received all data
    }
    
    strncat(userp, contents, realsize);
    return realsize;
}

size_t WriteCallbackNonStream(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    StreamState *state = (StreamState *)userp;

    if (realsize == 0) return 0;

    // Dynamic realloc for buffer growth
    char *new_buffer = realloc(state->nonstream_buffer, state->nonstream_buf_len + realsize + 1);
    if (!new_buffer) {
        fprintf(stderr, "Memory allocation failed in WriteCallbackNonStream\n");
        return 0;  // Error — curl will abort
    }
    state->nonstream_buffer = new_buffer;

    memcpy(state->nonstream_buffer + state->nonstream_buf_len, contents, realsize);
    state->nonstream_buf_len += realsize;
    state->nonstream_buffer[state->nonstream_buf_len] = '\0';

#if DEBUG_LEVEL > 2
    printf("NonStream: appended %zu bytes (total %zu)\n", realsize, state->nonstream_buf_len);
#endif

    return realsize;
}

// Static or per-state buffer for partial chunks (SSE can span multiple callbacks)
static char stream_buffer[65536] = {0};
static size_t stream_buf_len = 0;

size_t WriteCallbackStream(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    StreamState *state = (StreamState *)userp;

    if (realsize == 0) return 0;

#if DEBUG_LEVEL > 1
    printf("WriteCallbackStream: received %zu bytes (total buffered: %zu)\n", realsize, stream_buf_len + realsize);
#endif

    // Append data
    if (stream_buf_len + realsize >= sizeof(stream_buffer) - 1) {
        stream_buf_len = 0;
    }

    memcpy(stream_buffer + stream_buf_len, contents, realsize);
    stream_buf_len += realsize;
    stream_buffer[stream_buf_len] = '\0';

    char *ptr = stream_buffer;

    while (true) {
        // Look for event end (most common is \n\n)
        char *event_end = strstr(ptr, "\n\n");
        if (!event_end) event_end = strstr(ptr, "\r\n\r\n");
        
        // Also handle single \n (sometimes Ollama sends it this way)
        if (!event_end) {
            event_end = strstr(ptr, "\n");
            if (event_end && event_end > ptr) {
                // Only treat as end if it's likely a full line
                if (strstr(ptr, "data: ")) {
                    // accept single \n for data lines
                } else {
                    event_end = NULL;
                }
            }
        }

        if (!event_end) break;

        *event_end = '\0';

        // Process data: lines
        if (strncmp(ptr, "data: ", 6) == 0) {
            const char *json_str = ptr + 6;
            while (*json_str == ' ' || *json_str == '\t') json_str++;

            if (strcmp(json_str, "[DONE]") == 0) {
                printf("\n");
                fflush(stdout);
            } else if (strlen(json_str) > 10) {
#if DEBUG_LEVEL > 1
                printf("→ Processing chunk: %.100s...\n", json_str);
#endif
                process_json_to_events(json_str, state);
                process_events(state);
            }
        }

        ptr = event_end + 1;           // move past the newline
        if (*ptr == '\n') ptr++;       // handle \r\n
    }

    // Keep remaining data
    size_t processed = ptr - stream_buffer;
    size_t remaining = stream_buf_len - processed;

    if (remaining > 0) {
        memmove(stream_buffer, ptr, remaining);
    }
    stream_buf_len = remaining;
    stream_buffer[stream_buf_len] = '\0';

    return realsize;
}
