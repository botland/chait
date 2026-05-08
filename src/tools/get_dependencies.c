// get_dependencies.c
#include "../client.h"

// Assume ToolCall struct, BUFFER_SIZE, add_to_history, ask_inference_engine are defined elsewhere

cJSON *create_get_dependencies_tool(void) {
    cJSON *tool = cJSON_CreateObject();
    cJSON_AddStringToObject(tool, "type", "function");

    cJSON *func = cJSON_CreateObject();
    cJSON_AddStringToObject(func, "name", "get_dependencies");
    cJSON_AddStringToObject(func, "description",
        "Understand coupling. Outputs { includes: string[], imports: string[] }.");

    cJSON *params = cJSON_CreateObject();
    cJSON_AddStringToObject(params, "type", "object");

    cJSON *props = cJSON_CreateObject();
    cJSON *p1 = cJSON_CreateObject();
    cJSON_AddStringToObject(p1, "type", "string");
    cJSON_AddStringToObject(p1, "description", "The full path to the file.");
    cJSON_AddItemToObject(props, "file_path", p1);

    cJSON_AddItemToObject(params, "properties", props);

    cJSON *req = cJSON_CreateArray();
    cJSON_AddItemToArray(req, cJSON_CreateString("file_path"));
    cJSON_AddItemToObject(params, "required", req);

    cJSON_AddItemToObject(func, "parameters", params);
    cJSON_AddItemToObject(tool, "function", func);

    return tool;
}

void execute_get_dependencies(const ToolCall *tool) {
    cJSON *args_root = cJSON_Parse(tool->function_arguments);
    if (args_root == NULL) {
        fprintf(stderr, "Error parsing tool arguments\n");
        return;
    }

    cJSON *file_path = cJSON_GetObjectItemCaseSensitive(args_root, "file_path");

    printf("Tool call detected: %s with arguments %s\n", tool->function_name, tool->function_arguments ? tool->function_arguments : "");

    char *tool_result = NULL;

    // Real impl: Read file, grep for lines starting with #include or import (language-dependent)
    // For now, simulate
    tool_result = strdup("{\"includes\": [\"stdio.h\", \"string.h\"], \"imports\": []}");

    char tool_msg[BUFFER_SIZE];
    snprintf(tool_msg, sizeof(tool_msg), "{\"role\": \"tool\", \"tool_call_id\": \"%s\", \"name\": \"%s\", \"content\": \"%s\"}",
             tool->id ? tool->id : "", tool->function_name, tool_result);
    add_to_history("tool", tool_msg);

    free(tool_result);
    cJSON_Delete(args_root);

    ask_inference_engine(NULL);
}