// replace_function.c
#include "../client.h"

// Assume ToolCall struct, BUFFER_SIZE, add_to_history, ask_inference_engine are defined elsewhere

cJSON *create_replace_function_tool(void) {
    cJSON *tool = cJSON_CreateObject();
    cJSON_AddStringToObject(tool, "type", "function");

    cJSON *func = cJSON_CreateObject();
    cJSON_AddStringToObject(func, "name", "replace_function");
    cJSON_AddStringToObject(func, "description",
        "Function-scoped rewrite. Outputs { success, start_line, end_line }.");

    cJSON *params = cJSON_CreateObject();
    cJSON_AddStringToObject(params, "type", "object");

    cJSON *props = cJSON_CreateObject();
    cJSON *p1 = cJSON_CreateObject();
    cJSON_AddStringToObject(p1, "type", "string");
    cJSON_AddStringToObject(p1, "description", "The name of the function to replace.");
    cJSON_AddItemToObject(props, "function_name", p1);

    cJSON *p2 = cJSON_CreateObject();
    cJSON_AddStringToObject(p2, "type", "string");
    cJSON_AddStringToObject(p2, "description", "The full path to the file.");
    cJSON_AddItemToObject(props, "file_path", p2);

    cJSON *p3 = cJSON_CreateObject();
    cJSON_AddStringToObject(p3, "type", "string");
    cJSON_AddStringToObject(p3, "description", "The new code for the function.");
    cJSON_AddItemToObject(props, "new_code", p3);

    cJSON_AddItemToObject(params, "properties", props);

    cJSON *req = cJSON_CreateArray();
    cJSON_AddItemToArray(req, cJSON_CreateString("function_name"));
    cJSON_AddItemToArray(req, cJSON_CreateString("file_path"));
    cJSON_AddItemToArray(req, cJSON_CreateString("new_code"));
    cJSON_AddItemToObject(params, "required", req);

    cJSON_AddItemToObject(func, "parameters", params);
    cJSON_AddItemToObject(tool, "function", func);

    return tool;
}

void execute_replace_function(const ToolCall *tool) {
    cJSON *args_root = cJSON_Parse(tool->function_arguments);
    if (args_root == NULL) {
        fprintf(stderr, "Error parsing tool arguments\n");
        return;
    }

    cJSON *func_name = cJSON_GetObjectItemCaseSensitive(args_root, "function_name");
    cJSON *file_path = cJSON_GetObjectItemCaseSensitive(args_root, "file_path");
    cJSON *new_code = cJSON_GetObjectItemCaseSensitive(args_root, "new_code");

    printf("Tool call detected: %s with arguments %s\n", tool->function_name, tool->function_arguments ? tool->function_arguments : "");

    char *tool_result = NULL;

    // Real impl: Find function block in file, replace with new_code, write back to file
    // Use brace matching to find range
    // For now, simulate
    tool_result = strdup("{\"success\": true, \"start_line\": 42, \"end_line\": 60}");

    char tool_msg[BUFFER_SIZE];
    snprintf(tool_msg, sizeof(tool_msg), "{\"role\": \"tool\", \"tool_call_id\": \"%s\", \"name\": \"%s\", \"content\": \"%s\"}",
             tool->id ? tool->id : "", tool->function_name, tool_result);
    add_to_history("tool", tool_msg);

    free(tool_result);
    cJSON_Delete(args_root);

    ask_inference_engine(NULL);
}