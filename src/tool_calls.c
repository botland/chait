#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* get_status(ToolStatus status) {
    switch (status) {
        case TOOL_SUCCESS: return "success";
        case TOOL_ERROR: return "error";
        case TOOL_PARTIAL: return "partial";
        case TOOL_NOT_FOUND:return "not found";
        case TOOL_STATUS_UNDEFINED:
        default:
            return "undefined";
    }
}

void send_tool_response(StreamState *state, const ToolCall *tool, ToolStatus status, const char *content) {
    if (debug_level > 0) {
        printf("[TOOL RESPONSE] %s: %s\n", get_status(status), content ? content : "");
    }
    if (tool) {
        set_last_tool_response_params(tool, content, status);
    }
    // TODO: call build_tool_response if you want full history
}

void reset_state(StreamState *state) {
    if (state) {
//        state->content[0] = '\0';
//        state->content_len = 0;
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
        send_tool_response(state, tool, TOOL_NOT_FOUND, "Invalid tool call");
        return;
    }

    if (debug_level > 0) {
        printf("[TOOL CALL] Executing: %s\n", tool->function_name);
    }

    ToolHandler *handler = find_tool_handler(tool->function_name);
    if (handler && handler->execute) {
        handler->execute(state, tool);
    } else {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "Unknown tool: %s", tool->function_name);
        send_tool_response(state, tool, TOOL_ERROR, error_msg);
    }
}
