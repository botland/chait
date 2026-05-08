// grep.c
#include "../client.h"

// Assume ToolCall struct, BUFFER_SIZE, add_to_history, ask_inference_engine are defined elsewhere

cJSON *create_grep_tool(void) {
    cJSON *tool = cJSON_CreateObject();
    cJSON_AddStringToObject(tool, "type", "function");

    cJSON *func = cJSON_CreateObject();
    cJSON_AddStringToObject(func, "name", "grep");
    cJSON_AddStringToObject(func, "description",
        "Raw text search (fallback). Outputs { matches: [{ file_path, line, snippet }] }.");

    cJSON *params = cJSON_CreateObject();
    cJSON_AddStringToObject(params, "type", "object");

    cJSON *props = cJSON_CreateObject();
    cJSON *p1 = cJSON_CreateObject();
    cJSON_AddStringToObject(p1, "type", "string");
    cJSON_AddStringToObject(p1, "description", "The search pattern.");
    cJSON_AddItemToObject(props, "pattern", p1);

    cJSON *p2 = cJSON_CreateObject();
    cJSON_AddStringToObject(p2, "type", "string");
    cJSON_AddStringToObject(p2, "description", "The root path to search in.");
    cJSON_AddItemToObject(props, "root_path", p2);

    cJSON *p3 = cJSON_CreateObject();
    cJSON_AddStringToObject(p3, "type", "boolean");
    cJSON_AddStringToObject(p3, "description", "Whether to use regex (optional).");
    cJSON_AddItemToObject(props, "regex", p3);

    cJSON_AddItemToObject(params, "properties", props);

    cJSON *req = cJSON_CreateArray();
    cJSON_AddItemToArray(req, cJSON_CreateString("pattern"));
    cJSON_AddItemToArray(req, cJSON_CreateString("root_path"));
    cJSON_AddItemToObject(params, "required", req);

    cJSON_AddItemToObject(func, "parameters", params);
    cJSON_AddItemToObject(tool, "function", func);

    return tool;
}

void execute_grep(const ToolCall *tool) {
    cJSON *args_root = cJSON_Parse(tool->function_arguments);
    if (args_root == NULL) {
        fprintf(stderr, "Error parsing tool arguments\n");
        return;
    }

    cJSON *pattern = cJSON_GetObjectItemCaseSensitive(args_root, "pattern");
    cJSON *root_path = cJSON_GetObjectItemCaseSensitive(args_root, "root_path");
    cJSON *regex = cJSON_GetObjectItemCaseSensitive(args_root, "regex");

    printf("Tool call detected: %s with arguments %s\n", tool->function_name, tool->function_arguments ? tool->function_arguments : "");

    char *tool_result = NULL;

    // Real impl: Use system("grep -rn ...") and capture output, parse to JSON
    // For now, simulate
    tool_result = strdup("{\"matches\": [{\"file_path\": \"/path/to/file.c\", \"line\": 15, \"snippet\": \"matching line\"}]}");

    char tool_msg[BUFFER_SIZE];
    snprintf(tool_msg, sizeof(tool_msg), "{\"role\": \"tool\", \"tool_call_id\": \"%s\", \"name\": \"%s\", \"content\": \"%s\"}",
             tool->id ? tool->id : "", tool->function_name, tool_result);
    add_to_history("tool", tool_msg);

    free(tool_result);
    cJSON_Delete(args_root);

    ask_inference_engine(NULL);
}