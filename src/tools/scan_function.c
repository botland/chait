// scan_function.c
#include "../client.h"

// Assume ToolCall struct, BUFFER_SIZE, add_to_history, ask_inference_engine are defined elsewhere

cJSON *create_scan_function_tool(void) {
    cJSON *tool = cJSON_CreateObject();
    cJSON_AddStringToObject(tool, "type", "function");

    cJSON *func = cJSON_CreateObject();
    cJSON_AddStringToObject(func, "name", "scan_function");
    cJSON_AddStringToObject(func, "description",
        "Find where a function is defined or referenced. Locates function definitions and calls. "
        "Outputs List of { file_path, line_start, line_end, snippet, kind } where kind is 'definition' or 'call'.");

    cJSON *params = cJSON_CreateObject();
    cJSON_AddStringToObject(params, "type", "object");

    cJSON *props = cJSON_CreateObject();
    cJSON *p1 = cJSON_CreateObject();
    cJSON_AddStringToObject(p1, "type", "string");
    cJSON_AddStringToObject(p1, "description", "The name of the function to scan for.");
    cJSON_AddItemToObject(props, "function_name", p1);

    cJSON *p2 = cJSON_CreateObject();
    cJSON_AddStringToObject(p2, "type", "string");
    cJSON_AddStringToObject(p2, "description", "The root path to search in.");
    cJSON_AddItemToObject(props, "root_path", p2);

    cJSON *p3 = cJSON_CreateObject();
    cJSON_AddStringToObject(p3, "type", "integer");
    cJSON_AddStringToObject(p3, "description", "Maximum number of results to return (optional).");
    cJSON_AddItemToObject(props, "max_results", p3);

    cJSON_AddItemToObject(params, "properties", props);

    cJSON *req = cJSON_CreateArray();
    cJSON_AddItemToArray(req, cJSON_CreateString("function_name"));
    cJSON_AddItemToArray(req, cJSON_CreateString("root_path"));
    cJSON_AddItemToObject(params, "required", req);

    cJSON_AddItemToObject(func, "parameters", params);
    cJSON_AddItemToObject(tool, "function", func);

    return tool;
}

void execute_scan_function(const ToolCall *tool) {
    cJSON *args_root = cJSON_Parse(tool->function_arguments);
    if (args_root == NULL) {
        fprintf(stderr, "Error parsing tool arguments\n");
        return;
    }

    cJSON *func_name = cJSON_GetObjectItemCaseSensitive(args_root, "function_name");
    cJSON *root_path = cJSON_GetObjectItemCaseSensitive(args_root, "root_path");
    cJSON *max_res = cJSON_GetObjectItemCaseSensitive(args_root, "max_results");

    printf("Tool call detected: %s with arguments %s\n", tool->function_name, tool->function_arguments ? tool->function_arguments : "");

    char *tool_result = NULL;

    // Real impl: Traverse directory recursively using dirent.h, open .c/.h files, search for patterns like "^.* function_name *\(" for def, or "function_name\(" for calls
    // For now, simulate
    tool_result = strdup("[{\"file_path\": \"/path/to/file1.c\", \"line_start\": 42, \"line_end\": 50, \"snippet\": \"void function() {...}\", \"kind\": \"definition\"}, {\"file_path\": \"/path/to/file2.c\", \"line_start\": 100, \"line_end\": 100, \"snippet\": \"function();\", \"kind\": \"call\"}]");

    char tool_msg[BUFFER_SIZE];
    snprintf(tool_msg, sizeof(tool_msg), "{\"role\": \"tool\", \"tool_call_id\": \"%s\", \"name\": \"%s\", \"content\": \"%s\"}",
             tool->id ? tool->id : "", tool->function_name, tool_result);
    add_to_history("tool", tool_msg);

    free(tool_result);
    cJSON_Delete(args_root);

    ask_inference_engine(NULL);
}