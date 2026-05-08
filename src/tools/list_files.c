// list_files.c
#include "../client.h"

// Assume ToolCall struct, BUFFER_SIZE, add_to_history, ask_inference_engine are defined elsewhere

cJSON *create_list_files_tool(void) {
    cJSON *tool = cJSON_CreateObject();
    cJSON_AddStringToObject(tool, "type", "function");

    cJSON *func = cJSON_CreateObject();
    cJSON_AddStringToObject(func, "name", "list_files");
    cJSON_AddStringToObject(func, "description",
        "Directory-level awareness. Outputs { files: string[] }.");

    cJSON *params = cJSON_CreateObject();
    cJSON_AddStringToObject(params, "type", "object");

    cJSON *props = cJSON_CreateObject();
    cJSON *p1 = cJSON_CreateObject();
    cJSON_AddStringToObject(p1, "type", "string");
    cJSON_AddStringToObject(p1, "description", "The path to list files from.");
    cJSON_AddItemToObject(props, "path", p1);

    cJSON *p2 = cJSON_CreateObject();
    cJSON_AddStringToObject(p2, "type", "boolean");
    cJSON_AddStringToObject(p2, "description", "Whether to list recursively (optional).");
    cJSON_AddItemToObject(props, "recursive", p2);

    cJSON *p3 = cJSON_CreateObject();
    cJSON_AddStringToObject(p3, "type", "string");
    cJSON_AddStringToObject(p3, "description", "Glob pattern to match (optional).");
    cJSON_AddItemToObject(props, "pattern", p3);

    cJSON_AddItemToObject(params, "properties", props);

    cJSON *req = cJSON_CreateArray();
    cJSON_AddItemToArray(req, cJSON_CreateString("path"));
    cJSON_AddItemToObject(params, "required", req);

    cJSON_AddItemToObject(func, "parameters", params);
    cJSON_AddItemToObject(tool, "function", func);

    return tool;
}

void execute_list_files(const ToolCall *tool) {
    cJSON *args_root = cJSON_Parse(tool->function_arguments);
    if (args_root == NULL) {
        fprintf(stderr, "Error parsing tool arguments\n");
        return;
    }

    cJSON *path = cJSON_GetObjectItemCaseSensitive(args_root, "path");
    cJSON *recursive = cJSON_GetObjectItemCaseSensitive(args_root, "recursive");
    cJSON *pattern = cJSON_GetObjectItemCaseSensitive(args_root, "pattern");

    printf("Tool call detected: %s with arguments %s\n", tool->function_name, tool->function_arguments ? tool->function_arguments : "");

    char *tool_result = NULL;

    // Real impl: Use opendir/readdir for listing, recursive function if recursive=true, fnmatch for pattern
    // Include <dirent.h>, <fnmatch.h>
    // For now, simulate
    tool_result = strdup("{\"files\": [\"file1.c\", \"file2.h\", \"subdir/file3.c\"]}");

    char tool_msg[BUFFER_SIZE];
    snprintf(tool_msg, sizeof(tool_msg), "{\"role\": \"tool\", \"tool_call_id\": \"%s\", \"name\": \"%s\", \"content\": \"%s\"}",
             tool->id ? tool->id : "", tool->function_name, tool_result);
    add_to_history("tool", tool_msg);

    free(tool_result);
    cJSON_Delete(args_root);

    ask_inference_engine(NULL);
}