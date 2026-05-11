#include "client.h"
#include "event.h"

#define MAX_CONSECUTIVE_FAILURES 5

ushort debug_level = 0;
bool enable_agents = false;
bool enable_stream = true;
bool enable_tools = false;
char *system_prompt = NULL;

void build_prompt(cJSON *messages, char *role, char *content) {
    if (content != NULL) {
        cJSON *msg = cJSON_CreateObject();
        cJSON_AddStringToObject(msg, "role", role);
        cJSON_AddStringToObject(msg, "content", content);
        cJSON_AddItemToArray(messages, msg);
        if (strcmp(role, "system")) {
//            add_to_history(role, content);
        }
    }
}

void build_assistant_tool_call(cJSON *messages, ToolResponseParams *params) {
    cJSON *msg = cJSON_CreateObject();
    cJSON_AddStringToObject(msg, "role", "assistant");
    cJSON_AddNullToObject(msg, "content");

    cJSON *tool_calls = cJSON_CreateArray();
    cJSON *tool_call = cJSON_CreateObject();
    cJSON_AddStringToObject(tool_call, "id", params->tool_call_id);
    cJSON_AddStringToObject(tool_call, "type", "function");

    cJSON *function = cJSON_CreateObject();
    cJSON_AddStringToObject(function, "name", params->tool_name);
    cJSON_AddStringToObject(function, "arguments", params->tool_arguments ? params->tool_arguments : "{}");
    cJSON_AddItemToObject(tool_call, "function", function);

    cJSON_AddItemToArray(tool_calls, tool_call);
    cJSON_AddItemToObject(msg, "tool_calls", tool_calls);

    cJSON_AddItemToArray(messages, msg);

//    char history_msg[BUFFER_SIZE];
//    snprintf(history_msg, sizeof(history_msg), "calling %s %s", params->tool_name, params->tool_arguments);
//    snprintf(history_msg, sizeof(history_msg), "calling %s %s", params->tool_name ? params->tool_name : "", params->tool_arguments ? params->tool_arguments : "{}");
//    add_to_history("assistant", history_msg);
}

void build_tool_response(cJSON *messages, ToolResponseParams *params) {
    if (params->content != NULL) {
        cJSON *msg = cJSON_CreateObject();
        cJSON_AddStringToObject(msg, "role", "tool");
        cJSON_AddStringToObject(msg, "content", params->content ? params->content : "");
        cJSON_AddStringToObject(msg, "tool_call_id", params->tool_call_id);
        cJSON_AddStringToObject(msg, "name", params->tool_name);
        cJSON_AddItemToArray(messages, msg);
//        add_to_history("tool", params->content);
    }
}

int ask_inference_engine(char *user_input, ToolResponseParams *trp) {
    CURL *curl;
    CURLcode res;
    char *response = malloc(MAX_RESPONSE_SIZE);
    if (response == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        return 1;
    }
    response[0] = '\0';

    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Failed to initialize curl\n");
        free(response);
        return 1;
    }

    if (user_input) {
        printf(COLOR_LLAMA);
        printf("LLaMA: ");
        printf(COLOR_RESET);
    }

    int do_stream = enable_stream;

    cJSON *root = cJSON_CreateObject();
    cJSON *messages = cJSON_CreateArray();

    if (system_prompt) {
        build_prompt(messages, "system", system_prompt);
    }
    for (int i = 0; i < history_size; i++) {
        build_prompt(messages, chat_history[i].role, chat_history[i].content);
    }

    if (trp) {
        build_assistant_tool_call(messages, trp);
        build_tool_response(messages, trp);
    }
    clear_last_tool_response_params();

    cJSON_AddItemToObject(root, "messages", messages);
    cJSON_AddNumberToObject(root, "temperature", 0.1);
    cJSON_AddNumberToObject(root, "max_tokens", 8192);
//    cJSON_AddStringToObject(root, "model", "lexi8b");
    if (do_stream) {
        cJSON_AddTrueToObject(root, "stream");
    }

    // Enable call tools - Registry is now single source of truth
    if (enable_tools) {
        register_all_tools();

        cJSON *tools = cJSON_CreateArray();
        append_all_tool_definitions(tools);

        cJSON_AddItemToObject(root, "tools", tools);
        cJSON_AddStringToObject(root, "tool_choice", "auto");
    }

    char *json_request = cJSON_PrintUnformatted(root);
    if (debug_level > 1) {
        printf("request payload: %s\n", json_request);
    }

    cJSON_Delete(root);
    if (json_request == NULL) {
        fprintf(stderr, "Failed to create JSON request\n");
        free(response);
        curl_easy_cleanup(curl);
        return 1;
    }

    StreamState state = {0};
//    state.content[0] = '\0';  // Init accumulated content
//    state.content_len = 0;
    state.input = user_input;
    init_event_queue(&state.queue);

    curl_easy_setopt(curl, CURLOPT_URL, SERVER_URL);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_request);
    if (do_stream) {
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallbackStream);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &state);
    } else {
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallbackNonStream);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &state);
    }
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, NULL);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        free(json_request);
        free(response);
        curl_easy_cleanup(curl);
        return 1;
    }

    if (state.content.length > 0) {
        if (debug_level > 1) {
            printf("response message: %s\n", state.content.content);
        }
//        char *content = extract_message_from_json(state.content_buffer);
        add_to_history("assistant", state.content.content);
        free(state.content.content);
        state.content.length = 0;
    }

    if (!do_stream) {
        if (debug_level > 2) {
            printf("[JSON] processing non-stream content\n");
        }
        process_json_to_events(state.nonstream_buffer, &state);
    }

    process_events(&state);

    // cleanup queue
    Event dummy;
    while (pop_event(&state.queue, &dummy)) {
        free_event(&dummy);
    }

    reset_state(&state);

    response[0] = '\0';

    free(json_request);
    curl_easy_cleanup(curl);
    free(response);
    return 0;
}

void process_events(StreamState *state) {
    Event ev;
    while (pop_event(&state->queue, &ev)) {
        switch (ev.type) {
        case EVENT_TEXT:
            printf("%s", ev.text);
            fflush(stdout);
            free(ev.text);
            break;

        case EVENT_TOOL_CALL:
            {
                ToolCall tc = {
                   .id = ev.tool.id,
                   .type = NULL,
                   .function_name = ev.tool.name,
                   .function_arguments = ev.tool.arguments
                };
                execute_tool(state, &tc);
                free_event(&ev);
            }
            break;

        case EVENT_DONE:
            free_event(&ev);
            return;
        }
    }
}

static uint32_t hash_tool_calls(const char *tool_payload) {
    uint32_t hash = 5381;
    while (*tool_payload) {
        hash = ((hash << 5) + hash) + *tool_payload++;
    }
    return hash;
}

static ToolResponseParams last_trp_static = {0};

ToolResponseParams* get_last_tool_response_params(void) {
    return &last_trp_static;
}

void set_last_tool_response_params(const ToolCall *tool, const char *content, ToolStatus status) {
    if (last_trp_static.tool_call_id) free(last_trp_static.tool_call_id);
    if (last_trp_static.tool_name) free(last_trp_static.tool_name);
    if (last_trp_static.tool_arguments) free(last_trp_static.tool_arguments);
    if (last_trp_static.content) free(last_trp_static.content);
    last_trp_static.tool_call_id = tool->id ? strdup(tool->id) : NULL;
    last_trp_static.tool_name = tool->function_name ? strdup(tool->function_name) : NULL;
    last_trp_static.tool_arguments = tool->function_arguments ? strdup(tool->function_arguments) : NULL;
    last_trp_static.content = content ? strdup(content) : NULL;
    last_trp_static.status = status;
}

void clear_last_tool_response_params(void) {
    if (last_trp_static.tool_call_id) free(last_trp_static.tool_call_id);
    if (last_trp_static.tool_name) free(last_trp_static.tool_name);
    if (last_trp_static.tool_arguments) free(last_trp_static.tool_arguments);
    if (last_trp_static.content) free(last_trp_static.content);
    memset(&last_trp_static, 0, sizeof(last_trp_static));
}

bool last_response_has_tool_calls(void) {
    return (last_trp_static.tool_name != NULL);
}

int run_multiloop_agent(AgentContext *ctx, const char *initial_user_input, int max_loops) {
    if (!ctx) return -1;

    clear_last_tool_response_params();

    memset(&ctx->tool_response, 0, sizeof(ctx->tool_response));
    ctx->loop_count = 0;
    ctx->consecutive_failures = 0;
    ctx->finished = false;
    ctx->last_tool_hash = 0;

    const char *current_input = initial_user_input;
    ToolResponseParams *trp = NULL;

    printf("\033[1m\n[Hardened Multiloop Agent started - max %d loops]\033[0m\n", max_loops);

    while ((max_loops == 0 || ctx->loop_count < max_loops) && !ctx->finished) {
        ctx->loop_count++;

        int ret = ask_inference_engine((char*)current_input, trp);
        if (ret != 0) {
            fprintf(stderr, "\033[31m[ERROR] Inference failed in loop %d\033[0m\n", ctx->loop_count);
            break;
        }

        if (!last_response_has_tool_calls()) {
            ctx->finished = true;
            break;
        }

        trp = get_last_tool_response_params();

        char hash_input[2048];
        snprintf(hash_input, sizeof(hash_input), "%s|%s", trp->tool_name ? trp->tool_name : "", trp->tool_arguments ? trp->tool_arguments : "");
        uint32_t tool_hash = hash_tool_calls(hash_input);
        if (tool_hash == ctx->last_tool_hash) {
            ctx->consecutive_failures++;
            fprintf(stderr, "\033[33m[WARNING] Repeated tool pattern (loop %d)\033[0m\n", ctx->loop_count);
        } else {
            ctx->consecutive_failures = 0;
        }
        ctx->last_tool_hash = tool_hash;

        if (ctx->consecutive_failures > MAX_CONSECUTIVE_FAILURES) {
            fprintf(stderr, "\033[31m[SAFETY] Agent stuck in repeated tool loop — aborting\033[0m\n");
            add_to_history("system", "[SYSTEM] Previous tool loop was aborted due to repetition. Do not retry the same tool or action.");
            break;
        }

        current_input = NULL;
        memset(&ctx->tool_response, 0, sizeof(ctx->tool_response));
        ctx->tool_response = *trp;

        printf("\033[2m[Loop %d complete — tool response injected]\033[0m\n", ctx->loop_count);
    }

    if (!ctx->finished || (trp != NULL && trp->status != TOOL_SUCCESS)) {
        printf("\033[33m[Agent stopped after %d loop%s]\033[0m\n", ctx->loop_count, ctx->loop_count == 1 ? "" : "s");
        clear_last_tool_response_params();
        prune_last_n(ctx->loop_count);
    } else {
        printf("\033[32m\n[Agent finished after %d loop%s]\033[0m\n", ctx->loop_count, ctx->loop_count == 1 ? "" : "s");
    }
    return ctx->loop_count;
}
