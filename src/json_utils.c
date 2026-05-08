#include "client.h"
#include "event.h"

// Function to extract the assistant message from JSON response using cJSON
char* extract_message_from_json(const char* json_response) {
    cJSON *root = cJSON_Parse(json_response);
    if (root == NULL) {
        return NULL;
    }

    cJSON *choices = cJSON_GetObjectItem(root, "choices");
    if (choices == NULL || !cJSON_IsArray(choices)) {
        cJSON_Delete(root);
        return NULL;
    }

    cJSON *first_choice = cJSON_GetArrayItem(choices, 0);
    if (first_choice == NULL) {
        cJSON_Delete(root);
        return NULL;
    }

    cJSON *message = cJSON_GetObjectItem(first_choice, "message");
    if (message == NULL) {
        cJSON_Delete(root);
        return NULL;
    }

    cJSON *content = cJSON_GetObjectItem(message, "content");
    if (content == NULL || !cJSON_IsString(content)) {
        cJSON_Delete(root);
        return NULL;
    }

    // Create a copy of the content string
    char *message_copy = strdup(content->valuestring);
    cJSON_Delete(root);
    
    return message_copy;
}

char* extract_message_from_json_stream(const char* json_response) {
    cJSON *root = cJSON_Parse(json_response);
    if (root == NULL) {
        return NULL;
    }

    cJSON *choices = cJSON_GetObjectItem(root, "choices");
    if (choices == NULL || !cJSON_IsArray(choices)) {
        cJSON_Delete(root);
        return NULL;
    }

    cJSON *delta = cJSON_GetObjectItem(root, "delta");
    if (choices == NULL) {
        cJSON_Delete(root);
        return NULL;
    }

    cJSON *content = cJSON_GetObjectItem(delta, "content");
    if (content == NULL || !cJSON_IsString(content)) {
        cJSON_Delete(root);
        return NULL;
    }

    // Create a copy of the content string
    char *message_copy = strdup(content->valuestring);
    cJSON_Delete(root);
    
    return message_copy;
}

void process_json_to_events(const char *json_str, StreamState *state) {
#if DEBUG_LEVEL > 2
    printf("entering process_json_to_events: %s\n", json_str);
#endif
    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        return;
    }

    cJSON *choices = cJSON_GetObjectItemCaseSensitive(root, "choices");
    if (!cJSON_IsArray(choices) || cJSON_GetArraySize(choices) == 0) {
        cJSON_Delete(root);
        return;
    }

    cJSON *choice = cJSON_GetArrayItem(choices, 0);

    // Support both streaming delta and non-streaming message
    cJSON *delta = cJSON_GetObjectItemCaseSensitive(choice, "delta");
    cJSON *message_node = cJSON_GetObjectItemCaseSensitive(choice, "message");

    // ── TEXT event ──
    cJSON *content = NULL;
    if (delta) {
        content = cJSON_GetObjectItemCaseSensitive(delta, "content");
    } else if (message_node) {
        content = cJSON_GetObjectItemCaseSensitive(message_node, "content");
    }
    if (cJSON_IsString(content) && content->valuestring && content->valuestring[0] != '\0') {
        Event ev = {.type = EVENT_TEXT, .text = strdup(content->valuestring)};
        push_event(&state->queue, ev);
    }

    // ── TOOL CALL event (keep accumulation logic for compatibility) ──
    cJSON *tool_calls_array = NULL;
    if (delta) {
        tool_calls_array = cJSON_GetObjectItemCaseSensitive(delta, "tool_calls");
    } else if (message_node) {
        tool_calls_array = cJSON_GetObjectItemCaseSensitive(message_node, "tool_calls");
    }
    if (cJSON_IsArray(tool_calls_array) && cJSON_GetArraySize(tool_calls_array) > 0) {
#if DEBUG_LEVEL > 2
        printf("payload with tool call: %s\n", json_str);
#endif

        // Exact accumulation code from original process_json_chunk (works for both modes)
        int array_size = cJSON_GetArraySize(tool_calls_array);
        for (int i = 0; i < array_size; i++) {
            cJSON *tc = cJSON_GetArrayItem(tool_calls_array, i);
            cJSON *index_item = cJSON_GetObjectItemCaseSensitive(tc, "index");
            if (!cJSON_IsNumber(index_item)) continue;
            int index = (int)index_item->valuedouble;

            // resize / init new slots
            if (index >= state->tool_calls_capacity) {
                int new_capacity = index + 1 + 4;
                ToolCall *new_tools = realloc(state->tool_calls, new_capacity * sizeof(ToolCall));
                if (!new_tools) {
                    fprintf(stderr, "Memory allocation failed\n");
                    continue;
                }
                for (int j = state->tool_calls_capacity; j < new_capacity; j++) {
                    new_tools[j].id = NULL;
                    new_tools[j].type = NULL;
                    new_tools[j].function_name = NULL;
                    new_tools[j].function_arguments = NULL;
                }
                state->tool_calls = new_tools;
                state->tool_calls_capacity = new_capacity;
            }
            if (index + 1 > state->tool_calls_size) {
                state->tool_calls_size = index + 1;
            }

            ToolCall *tool = &state->tool_calls[index];

            // id, type, name (only set once)
            cJSON *id_item = cJSON_GetObjectItemCaseSensitive(tc, "id");
            if (cJSON_IsString(id_item) && id_item->valuestring && !tool->id) {
                tool->id = strdup(id_item->valuestring);
            }

            cJSON *type_item = cJSON_GetObjectItemCaseSensitive(tc, "type");
            if (cJSON_IsString(type_item) && type_item->valuestring && !tool->type) {
                tool->type = strdup(type_item->valuestring);
            }

            cJSON *function = cJSON_GetObjectItemCaseSensitive(tc, "function");
            if (function) {
                cJSON *name_item = cJSON_GetObjectItemCaseSensitive(function, "name");
                if (cJSON_IsString(name_item) && name_item->valuestring && !tool->function_name) {
                    tool->function_name = strdup(name_item->valuestring);
                }

                cJSON *args_item = cJSON_GetObjectItemCaseSensitive(function, "arguments");
                if (cJSON_IsString(args_item) && args_item->valuestring) {
                    size_t curr_len = tool->function_arguments ? strlen(tool->function_arguments) : 0;
                    size_t add_len  = strlen(args_item->valuestring);
                    char *new_args = realloc(tool->function_arguments, curr_len + add_len + 1);
                    if (!new_args) {
                        fprintf(stderr, "Memory allocation failed\n");
                        continue;
                    }
                    if (!tool->function_arguments) new_args[0] = '\0';
                    strcat(new_args, args_item->valuestring);
                    tool->function_arguments = new_args;
                }
            }
        }
    }

    // ── EMIT TOOL CALLS ONLY WHEN COMPLETE (finish_reason present) ──
    // This guarantees full arguments for multi-arg tools in streaming mode
    cJSON *finish = cJSON_GetObjectItemCaseSensitive(choice, "finish_reason");
    if (finish && cJSON_IsString(finish)) {
        for (int i = 0; i < state->tool_calls_size; i++) {
            ToolCall *tool = &state->tool_calls[i];
            if (tool->function_name) {
                Event ev = {
                    .type = EVENT_TOOL_CALL,
                    .tool = {
                        .name = strdup(tool->function_name),
                        .arguments = tool->function_arguments ? strdup(tool->function_arguments) : NULL
                    }
                };
                push_event(&state->queue, ev);
            }
        }

        Event ev_done = {.type = EVENT_DONE};
        push_event(&state->queue, ev_done);
    }

    cJSON_Delete(root);
}

bool check_json_response(const char *json_string) {
    if (!json_string || !*json_string) {
        printf("Error: Empty or NULL JSON string received\n");
        return true;
    }

    cJSON *root = cJSON_Parse(json_string);
    if (root == NULL) {
        const char *err_ptr = cJSON_GetErrorPtr();
        if (err_ptr) {
            printf("JSON parse error near: %.40s...\n", err_ptr);
        } else {
            printf("Failed to parse JSON response\n");
        }
        return false;
    }

    // Look for the "error" object
    cJSON *error_obj = cJSON_GetObjectItemCaseSensitive(root, "error");
    if (!cJSON_IsObject(error_obj)) {
        printf("No 'error' object found in response\n");
        cJSON_Delete(root);
    }

    // Extract code
    cJSON *code_item = cJSON_GetObjectItemCaseSensitive(error_obj, "code");
    int code = -1;
    if (cJSON_IsNumber(code_item)) {
        code = (int)code_item->valuedouble;
        if (code < 400) {
            return true;
        }
    }

    // Extract message
    cJSON *msg_item = cJSON_GetObjectItemCaseSensitive(error_obj, "message");
    const char *message = NULL;
    if (cJSON_IsString(msg_item) && msg_item->valuestring) {
        message = msg_item->valuestring;
    }

    // Print nicely
    printf("LLM Server Error:\n");
    if (code != -1) {
        printf("  Code:    %d\n", code);
    } else {
        printf("  Code:    (not found)\n");
    }

    if (message) {
        // Truncate very long messages for readability
        const char *display_msg = message;
        size_t len = strlen(message);
        if (len > 3000) {
            printf("  Message: %.280s... (truncated, full length = %zu)\n", message, len);
        } else {
            printf("  Message: %s\n", message);
        }
    } else {
        printf("  Message: (not found)\n");
    }

    // Optional: show any other fields that might be present
    cJSON *extra = NULL;
    cJSON_ArrayForEach(extra, error_obj) {
        if (extra->string && 
            strcmp(extra->string, "code") != 0 && 
            strcmp(extra->string, "message") != 0) {
            char *extra_val = cJSON_PrintUnformatted(extra);
            if (extra_val) {
                printf("  %s: %s\n", extra->string, extra_val);
                free(extra_val);
            }
        }
    }

    cJSON_Delete(root);
    return false;
}
