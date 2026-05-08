#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// === Missing helpers (moved back from old tool_calls) ===
void send_tool_response(StreamState *state, const char *status, const char *content) {
    // your original implementation here (or the one from old tool_calls.c)
    // for now a minimal version:
    printf("[TOOL RESPONSE] %s: %s\n", status, content ? content : "");
    // TODO: call build_tool_response if you want full history
}

void reset_state(StreamState *state) {
    if (state) {
        state->content[0] = '\0';
        state->content_len = 0;
        state->pending_tool_calls = 0;
        state->tool_calls_size = 0;
    }
}

ToolResponseParams *create_tool_response_params(const ToolCall *tool, char *content) {
    ToolResponseParams *p = malloc(sizeof(ToolResponseParams));
    if (p) {
        p->tool_call_id = strdup(tool->id);
        p->tool_name = strdup(tool->function_name);
        p->content = content ? strdup(content) : NULL;
    }
    return p;
}

void free_tool_response_params(ToolResponseParams *p) {
    if (p) {
        free(p->tool_call_id);
        free(p->tool_name);
        free(p->content);
        free(p);
    }
}

// === Registry execute_tool ===
void execute_tool(StreamState *state, const ToolCall *tool) {
    if (!tool || !tool->function_name) {
        send_tool_response(state, "error", "Invalid tool call");
        return;
    }

#if DEBUG_LEVEL > 1
    printf("[TOOL] Executing: %s\n", tool->function_name);
#endif

    ToolHandler *handler = find_tool_handler(tool->function_name);
    if (handler && handler->execute) {
        handler->execute(state, tool);
    } else {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "Unknown tool: %s", tool->function_name);
        send_tool_response(state, "error", error_msg);
    }
}
