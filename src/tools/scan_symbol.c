// scan_symbol.c
#include "../client.h"

// Assume ToolCall struct, BUFFER_SIZE, add_to_history, ask_inference_engine are defined elsewhere

cJSON *create_scan_symbol_tool(void) {
    cJSON *tool = cJSON_CreateObject();
    cJSON_AddStringToObject(tool, "type", "function");

    cJSON *func = cJSON_CreateObject();
    cJSON_AddStringToObject(func, "name", "scan_symbol");
    cJSON_AddStringToObject(func, "description",
        "Generalized symbol search (structs, classes, macros, globals). Outputs { file_path, line, snippet, symbol_type }.");

    cJSON *params = cJSON_CreateObject();
    cJSON_AddStringToObject(params, "type", "object");

    cJSON *props = cJSON_CreateObject();
    cJSON *p1 = cJSON_CreateObject();
    cJSON_AddStringToObject(p1, "type", "string");
    cJSON_AddStringToObject(p1, "description", "The symbol to search for.");
    cJSON_AddItemToObject(props, "symbol", p1);

    cJSON *p2 = cJSON_CreateObject();
    cJSON_AddStringToObject(p2, "type", "string");
    cJSON_AddStringToObject(p2, "description", "The root path to search in.");
    cJSON_AddItemToObject(props, "root_path", p2);

    cJSON *p3 = cJSON_CreateObject();
    cJSON_AddStringToObject(p3, "type", "string");
    cJSON_AddStringToObject(p3, "description", "The type of symbol (optional).");
    cJSON *enum_arr = cJSON_CreateArray();
    cJSON_AddItemToArray(enum_arr, cJSON_CreateString("function"));
    cJSON_AddItemToArray(enum_arr, cJSON_CreateString("struct"));
    cJSON_AddItemToArray(enum_arr, cJSON_CreateString("class"));
    cJSON_AddItemToArray(enum_arr, cJSON_CreateString("enum"));
    cJSON_AddItemToArray(enum_arr, cJSON_CreateString("macro"));
    cJSON_AddItemToArray(enum_arr, cJSON_CreateString("variable"));
    cJSON_AddItemToObject(p3, "enum", enum_arr);
    cJSON_AddItemToObject(props, "symbol_type", p3);

    cJSON_AddItemToObject(params, "properties", props);

    cJSON *req = cJSON_CreateArray();
    cJSON_AddItemToArray(req, cJSON_CreateString("symbol"));
    cJSON_AddItemToArray(req, cJSON_CreateString("root_path"));
    cJSON_AddItemToObject(params, "required", req);

    cJSON_AddItemToObject(func, "parameters", params);
    cJSON_AddItemToObject(tool, "function", func);

    return tool;
}

void execute_scan_symbol(const ToolCall *tool) {
    cJSON *args_root = cJSON_Parse(tool->function_arguments);
    if (args_root == NULL) {
        fprintf(stderr, "Error parsing tool arguments\n");
        return;
    }

    cJSON *symbol = cJSON_GetObjectItemCaseSensitive(args_root, "symbol");
    cJSON *root_path = cJSON_GetObjectItemCaseSensitive(args_root, "root_path");
    cJSON *symbol_type = cJSON_GetObjectItemCaseSensitive(args_root, "symbol_type");

    printf("Tool call detected: %s with arguments %s\n", tool->function_name, tool->function_arguments ? tool->function_arguments : "");

    char *tool_result = NULL;

    // Real impl: Similar to scan_function, but filter by symbol_type patterns (e.g., "struct symbol {" for struct)
    // For now, simulate
    tool_result = strdup("[{\"file_path\": \"/path/to/file.c\", \"line\": 10, \"snippet\": \"struct symbol { ... };\", \"symbol_type\": \"struct\"}]");

    char tool_msg[BUFFER_SIZE];
    snprintf(tool_msg, sizeof(tool_msg), "{\"role\": \"tool\", \"tool_call_id\": \"%s\", \"name\": \"%s\", \"content\": \"%s\"}",
             tool->id ? tool->id : "", tool->function_name, tool_result);
    add_to_history("tool", tool_msg);

    free(tool_result);
    cJSON_Delete(args_root);

    ask_inference_engine(NULL);
}