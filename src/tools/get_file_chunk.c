// get_file_chunk.c
#include "../client.h"

// Assume ToolCall struct, BUFFER_SIZE, add_to_history, ask_inference_engine are defined elsewhere

cJSON *create_get_file_chunk_tool(void) {
    cJSON *tool = cJSON_CreateObject();
    cJSON_AddStringToObject(tool, "type", "function");

    cJSON *func = cJSON_CreateObject();
    cJSON_AddStringToObject(func, "name", "get_file_chunk");
    cJSON_AddStringToObject(func, "description",
        "Read arbitrary code ranges. Outputs { code: string }.");

    cJSON *params = cJSON_CreateObject();
    cJSON_AddStringToObject(params, "type", "object");

    cJSON *props = cJSON_CreateObject();
    cJSON *p1 = cJSON_CreateObject();
    cJSON_AddStringToObject(p1, "type", "string");
    cJSON_AddStringToObject(p1, "description", "The full path to the file.");
    cJSON_AddItemToObject(props, "file_path", p1);

    cJSON *p2 = cJSON_CreateObject();
    cJSON_AddStringToObject(p2, "type", "integer");
    cJSON_AddStringToObject(p2, "description", "Starting line number (1-indexed).");
    cJSON_AddItemToObject(props, "start_line", p2);

    cJSON *p3 = cJSON_CreateObject();
    cJSON_AddStringToObject(p3, "type", "integer");
    cJSON_AddStringToObject(p3, "description", "Ending line number (1-indexed).");
    cJSON_AddItemToObject(props, "end_line", p3);

    cJSON_AddItemToObject(params, "properties", props);

    cJSON *req = cJSON_CreateArray();
    cJSON_AddItemToArray(req, cJSON_CreateString("file_path"));
    cJSON_AddItemToArray(req, cJSON_CreateString("start_line"));
    cJSON_AddItemToArray(req, cJSON_CreateString("end_line"));
    cJSON_AddItemToObject(params, "required", req);

    cJSON_AddItemToObject(func, "parameters", params);
    cJSON_AddItemToObject(tool, "function", func);

    return tool;
}

void execute_get_file_chunk(const ToolCall *tool) {
    cJSON *args_root = cJSON_Parse(tool->function_arguments);
    if (args_root == NULL) {
        fprintf(stderr, "Error parsing tool arguments\n");
        return;
    }

    cJSON *file_path = cJSON_GetObjectItemCaseSensitive(args_root, "file_path");
    cJSON *start_line = cJSON_GetObjectItemCaseSensitive(args_root, "start_line");
    cJSON *end_line = cJSON_GetObjectItemCaseSensitive(args_root, "end_line");

    printf("Tool call detected: %s with arguments %s\n", tool->function_name, tool->function_arguments ? tool->function_arguments : "");

    char *tool_result = NULL;

    // Real impl: fopen(file_path), skip to start_line-1, read end_line-start_line+1 lines with fgets, concatenate
    // For now, simulate
    tool_result = strdup("{\"code\": \"line1\\nline2\\nline3\"}");

    char tool_msg[BUFFER_SIZE];
    snprintf(tool_msg, sizeof(tool_msg), "{\"role\": \"tool\", \"tool_call_id\": \"%s\", \"name\": \"%s\", \"content\": \"%s\"}",
             tool->id ? tool->id : "", tool->function_name, tool_result);
    add_to_history("tool", tool_msg);

    free(tool_result);
    cJSON_Delete(args_root);

    ask_inference_engine(NULL);
}