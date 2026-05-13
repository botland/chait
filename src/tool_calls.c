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

void reset_state(StreamState *state) {
    if (state) {
//        state->content[0] = '\0';
//        state->content_len = 0;
        state->pending_tool_calls = 0;
        state->tool_calls_size = 0;
    }
}

ToolResponseParams *create_tool_response_params(const ToolCall *tool, ToolStatus status, char *content) {
    ToolResponseParams *p = malloc(sizeof(ToolResponseParams));
    if (p) {
        p->tool_call_id = strdup(tool->id);
        p->tool_name = strdup(tool->function_name);
        p->tool_arguments = tool->function_arguments ? strdup(tool->function_arguments) : NULL;
        p->content = content ? strdup(content) : NULL;
        p->status = status;
    }
    return p;
}

void free_tool_response_params(ToolResponseParams *p) {
    if (p) {
        free(p->tool_call_id);
        free(p->tool_name);
        if (p->tool_arguments) free(p->tool_arguments);
        if (p->content) free(p->content);
        free(p);
    }
}

// === Registry execute_tool ===
ToolResponseParams *execute_tool(StreamState *state, const ToolCall *tool) {
    if (!tool || !tool->function_name) {
        return create_tool_response_params(tool, TOOL_ERROR, "Invalid tool call");
    }

    if (debug_level > 0) {
        printf("[TOOL CALL] Executing: %s\n", tool->function_name);
    }

    ToolResponseParams *trp = NULL;
    ToolHandler *handler = find_tool_handler(tool->function_name);
    if (handler && handler->execute) {
        trp = handler->execute(state, tool);
    } else {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "Unknown tool: %s", tool->function_name);
        trp = create_tool_response_params(tool, TOOL_ERROR, error_msg);
    }

    if (debug_level > 0) {
        printf("[TOOL CALL] Execution status: %s\n", get_status(trp->status));
    }

    return trp;
}
