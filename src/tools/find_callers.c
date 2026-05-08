// find_callers.c
#include "../client.h"

// Assume ToolCall struct, BUFFER_SIZE, add_to_history, ask_inference_engine are defined elsewhere

cJSON *create_find_callers_tool(void) {
    cJSON *tool = cJSON_CreateObject();
    cJSON_AddStringToObject(tool, "type", "function");

    cJSON *func = cJSON_CreateObject();
    cJSON_AddStringToObject(func, "name", "find_callers");
    cJSON_AddStringToObject(func, "description",
        "Reverse call graph. Outputs { callers: [{ file_path, line, snippet }] }.");

    cJSON *params = cJSON_CreateObject();
    cJSON_AddStringToObject(params, "type", "object");

    cJSON *props = cJSON_CreateObject();
    cJSON *p1 = cJSON_CreateObject();
    cJSON_AddStringToObject(p1, "type", "string");
    cJSON_AddStringToObject(p1, "description", "The name of the function.");
    cJSON_AddItemToObject(props, "function_name", p1);

    cJSON *p2 = cJSON_CreateObject();
    cJSON_AddStringToObject(p2, "type", "string");
    cJSON_AddStringToObject(p2, "description", "The root path to search in.");
    cJSON_AddItemToObject(props, "root_path", p2);

    cJSON_AddItemToObject(params, "properties", props);

    cJSON *req = cJSON_CreateArray();
    cJSON_AddItemToArray(req, cJSON_CreateString("function_name"));
    cJSON_AddItemToArray(req, cJSON_CreateString("root_path"));
    cJSON_AddItemToObject(params, "required", req);

    cJSON_AddItemToObject(func, "parameters", params);
    cJSON_AddItemToObject(tool, "function", func);

    return tool;
}

void execute_find_callers(const ToolCall *tool) {
    cJSON *args_root = cJSON_Parse(tool->function_arguments);
    if (args_root == NULL) {
        fprintf(stderr, "Error parsing tool arguments\n");
        return;
    }

    cJSON *func_name = cJSON_GetObjectItemCaseSensitive(args_root, "function_name");
    cJSON *root_path = cJSON_GetObjectItemCaseSensitive(args_root, "root_path");

    printf("Tool call detected: %s with arguments %s\n", tool->function_name, tool->function_arguments ? tool->function_arguments : "");

    char *tool_result = NULL;

    // Real impl: Search entire codebase for "function_name\(" excluding its own definition
    // For now, simulate
    tool_result = strdup("{\"callers\": [{\"file_path\": \"/path/to/caller.c\", \"line\": 20, \"snippet\": \"function_name();\"}]}");

    char tool_msg[BUFFER_SIZE];
    snprintf(tool_msg, sizeof(tool_msg), "{\"role\": \"tool\", \"tool_call_id\": \"%s\", \"name\": \"%s\", \"content\": \"%s\"}",
             tool->id ? tool->id : "", tool->function_name, tool_result);
    add_to_history("tool", tool_msg);

    free(tool_result);
    cJSON_Delete(args_root);

    ask_inference_engine(NULL);
}