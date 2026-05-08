// replace_file_content.c
#include "../client.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Assume ToolCall struct, BUFFER_SIZE, add_to_history, ask_inference_engine are defined elsewhere

cJSON *create_replace_file_content_tool(void) {
    cJSON *tool = cJSON_CreateObject();
    cJSON_AddStringToObject(tool, "type", "function");

    cJSON *func = cJSON_CreateObject();
    cJSON_AddStringToObject(func, "name", "replace_file_content");
    cJSON_AddStringToObject(func, "description",
        "Replaces the entire content of a file with new content. Use this to apply refactored or modified code to a file.");

    cJSON *params = cJSON_CreateObject();
    cJSON_AddStringToObject(params, "type", "object");

    cJSON *props = cJSON_CreateObject();
    cJSON *p1 = cJSON_CreateObject();
    cJSON_AddStringToObject(p1, "type", "string");
    cJSON_AddStringToObject(p1, "description", "The full path to the file to overwrite.");
    cJSON_AddItemToObject(props, "file_path", p1);

    cJSON *p2 = cJSON_CreateObject();
    cJSON_AddStringToObject(p2, "type", "string");
    cJSON_AddStringToObject(p2, "description", "The new content to write to the file.");
    cJSON_AddItemToObject(props, "new_content", p2);

    cJSON_AddItemToObject(params, "properties", props);

    cJSON *req = cJSON_CreateArray();
    cJSON_AddItemToArray(req, cJSON_CreateString("file_path"));
    cJSON_AddItemToArray(req, cJSON_CreateString("new_content"));
    cJSON_AddItemToObject(params, "required", req);

    cJSON_AddItemToObject(func, "parameters", params);
    cJSON_AddItemToObject(tool, "function", func);

    return tool;
}

void execute_replace_file_content(const ToolCall *tool) {
    cJSON *args_root = cJSON_Parse(tool->function_arguments);
    if (args_root == NULL) {
        fprintf(stderr, "Error parsing tool arguments\n");
        return;
    }

    cJSON *file_path_item = cJSON_GetObjectItemCaseSensitive(args_root, "file_path");
    cJSON *new_content_item = cJSON_GetObjectItemCaseSensitive(args_root, "new_content");

    if (!cJSON_IsString(file_path_item) || !cJSON_IsString(new_content_item)) {
        fprintf(stderr, "Invalid arguments for replace_file_content\n");
        cJSON_Delete(args_root);
        return;
    }

    const char *file_path = file_path_item->valuestring;
    const char *new_content = new_content_item->valuestring;

    printf("Tool call detected: %s with arguments %s\n", tool->function_name, tool->function_arguments ? tool->function_arguments : "");

    char *tool_result = NULL;

    FILE *file = fopen(file_path, "w");
    if (file == NULL) {
        fprintf(stderr, "Error opening file %s for writing\n", file_path);
        tool_result = strdup("{\"success\": false, \"error\": \"Failed to open file\"}");
    } else {
        size_t written = fwrite(new_content, 1, strlen(new_content), file);
        int closed = fclose(file);
        int success = (written == strlen(new_content) && closed == 0);

        if (success) {
            tool_result = strdup("{\"success\": true}");
        } else {
            tool_result = strdup("{\"success\": false, \"error\": \"Failed to write content\"}");
        }
    }

    if (tool_result == NULL) {
        tool_result = strdup("{\"success\": false, \"error\": \"Unknown error\"}");
    }

    char tool_msg[BUFFER_SIZE];
    snprintf(tool_msg, sizeof(tool_msg), "{\"role\": \"tool\", \"tool_call_id\": \"%s\", \"name\": \"%s\", \"content\": \"%s\"}",
             tool->id ? tool->id : "", tool->function_name, tool_result);
    add_to_history("tool", tool_msg);

    free(tool_result);
    cJSON_Delete(args_root);

    ask_inference_engine(NULL);
}
