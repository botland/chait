// get_function_code.c
#include "../client.h"

// Assume ToolCall struct, BUFFER_SIZE, add_to_history, ask_inference_engine are defined elsewhere

cJSON *create_get_function_code_tool(void) {
    cJSON *tool = cJSON_CreateObject();
    cJSON_AddStringToObject(tool, "type", "function");

    cJSON *func = cJSON_CreateObject();
    cJSON_AddStringToObject(func, "name", "get_function_code");
    cJSON_AddStringToObject(func, "description",
        "Retrieves the full source code of a specific function from a given file path. "
        "Use this after locating the function (e.g., via scan_function) to analyze, refactor, or modify the code. "
        "Returns the function's code snippet, including definitions and body.");

    cJSON *params = cJSON_CreateObject();
    cJSON_AddStringToObject(params, "type", "object");

    cJSON *props = cJSON_CreateObject();
    // function_name (required)
    cJSON *p1 = cJSON_CreateObject();
    cJSON_AddStringToObject(p1, "type", "string");
    cJSON_AddStringToObject(p1, "description",
        "The exact name of the function to retrieve (case-sensitive).");
    cJSON_AddItemToObject(props, "function_name", p1);
    // file_path (required for precision)
    cJSON *p2 = cJSON_CreateObject();
    cJSON_AddStringToObject(p2, "type", "string");
    cJSON_AddStringToObject(p2, "description",
        "The full path to the file containing the function. Obtain this from scan_function if unknown.");
    cJSON_AddItemToObject(props, "file_path", p2);

    cJSON_AddItemToObject(params, "properties", props);

    cJSON *req = cJSON_CreateArray();
    cJSON_AddItemToArray(req, cJSON_CreateString("function_name"));
    cJSON_AddItemToArray(req, cJSON_CreateString("file_path"));  // Make file_path required to encourage chaining
    cJSON_AddItemToObject(params, "required", req);

    cJSON_AddItemToObject(func, "parameters", params);
    cJSON_AddItemToObject(tool, "function", func);

    return tool;
}

void execute_get_function_code(const ToolCall *tool) {
    cJSON *args_root = cJSON_Parse(tool->function_arguments);
    if (args_root == NULL) {
        fprintf(stderr, "Error parsing tool arguments\n");
        return;
    }

    cJSON *func_name = cJSON_GetObjectItemCaseSensitive(args_root, "function_name");
    cJSON *file_path = cJSON_GetObjectItemCaseSensitive(args_root, "file_path");

    printf("Tool call detected: %s with arguments %s\n", tool->function_name, tool->function_arguments ? tool->function_arguments : "");

    char *tool_result = NULL;  // To store simulated/executed result

    // Real impl: Open file, find function definition, extract block with brace matching
    // For now, simulate
    tool_result = strdup("{\"code\": \"void xxx() { printf(\\\"Hello\\\"); }\", \"start_line\": 42, \"end_line\": 50}");

    // Append tool response to history
    char tool_msg[BUFFER_SIZE];
    snprintf(tool_msg, sizeof(tool_msg), "{\"role\": \"tool\", \"tool_call_id\": \"%s\", \"name\": \"%s\", \"content\": \"%s\"}",
             tool->id ? tool->id : "", tool->function_name, tool_result);
    add_to_history("tool", tool_msg);  // Use "tool" role for OpenAI compat

    free(tool_result);
    cJSON_Delete(args_root);

    // Re-request to continue conversation (model will see tool result)
    ask_inference_engine(NULL);  // Pass NULL input to resume
}