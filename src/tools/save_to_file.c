// src/tools/save_to_file.c
#include "../client.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

cJSON *create_save_to_file_tool(void) {
    cJSON *tool = cJSON_CreateObject();
    cJSON_AddStringToObject(tool, "type", "function");

    cJSON *func = cJSON_CreateObject();
    cJSON_AddStringToObject(func, "name", "save_to_file");
    cJSON_AddStringToObject(func, "description",
        "Writes (or overwrites) the given content to the specified file path. "
        "Use this after refactoring or generating new code to persist the result to disk.");

    cJSON *params = cJSON_CreateObject();
    cJSON_AddStringToObject(params, "type", "object");

    cJSON *props = cJSON_CreateObject();

    // file_path
    cJSON *p1 = cJSON_CreateObject();
    cJSON_AddStringToObject(p1, "type", "string");
    cJSON_AddStringToObject(p1, "description", "Full absolute or relative path to the file to save/overwrite.");
    cJSON_AddItemToObject(props, "file_path", p1);

    // content
    cJSON *p2 = cJSON_CreateObject();
    cJSON_AddStringToObject(p2, "type", "string");
    cJSON_AddStringToObject(p2, "description", 
        "The full text content to write to the file (usually the refactored or newly generated code).");
    cJSON_AddItemToObject(props, "content", p2);

    cJSON_AddItemToObject(params, "properties", props);

    cJSON *req = cJSON_CreateArray();
    cJSON_AddItemToArray(req, cJSON_CreateString("file_path"));
    cJSON_AddItemToArray(req, cJSON_CreateString("content"));  // both required
    cJSON_AddItemToObject(params, "required", req);

    cJSON_AddItemToObject(func, "parameters", params);
    cJSON_AddItemToObject(tool, "function", func);

    return tool;
}

void execute_save_to_file(StreamState *state, const ToolCall *tool) {
    if (strcmp(tool->function_name, "save_to_file") != 0) return;

    cJSON *args_root = cJSON_Parse(tool->function_arguments);
    if (!args_root) {
        fprintf(stderr, "save_to_file: JSON parse failed on: %s\n", tool->function_arguments);
        return;
    }

    cJSON *path_item   = cJSON_GetObjectItemCaseSensitive(args_root, "file_path");
    cJSON *content_item = cJSON_GetObjectItemCaseSensitive(args_root, "content");

    if (!cJSON_IsString(path_item) || !cJSON_IsString(content_item)) {
        fprintf(stderr, "save_to_file: missing/invalid file_path or content\n");
        cJSON_Delete(args_root);
        return;
    }

    const char *file_path = path_item->valuestring;
    const char *content   = content_item->valuestring;

    printf("Executing save_to_file → path: %s  (content length: %zu)\n", 
           file_path, strlen(content));

    FILE *f = fopen(file_path, "w");
    if (!f) {
        perror("fopen failed");
    } else {
        size_t written = fwrite(content, 1, strlen(content), f);
        fclose(f);

        if (written == strlen(content)) {
            printf("File saved successfully: %s\n", file_path);
            char *success_msg = NULL;
            asprintf(&success_msg, "File successfully saved/overwritten at %s", file_path);

            send_tool_response(state, tool, TOOL_SUCCESS, success_msg);
            free(success_msg);
        } else {
            fprintf(stderr, "Partial write to %s (%zu/%zu bytes)\n", 
                    file_path, written, strlen(content));
        }
    }

    cJSON_Delete(args_root);
}

ToolHandler save_to_file_handler = {
    .name = "save_to_file",
    .create_definition = create_save_to_file_tool,
    .execute = execute_save_to_file
};
