// find_file_path.c
#include "../client.h"
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Assume ToolCall struct, BUFFER_SIZE, add_to_history, ask_inference_engine are defined elsewhere

cJSON *create_find_file_path_tool(void) {
    cJSON *tool = cJSON_CreateObject();
    cJSON_AddStringToObject(tool, "type", "function");

    cJSON *func = cJSON_CreateObject();
    cJSON_AddStringToObject(func, "name", "find_file_path");
    cJSON_AddStringToObject(func, "description",
        "Searches for a file if its path is not specified. ");
//        "Searches for files matching a given abbreviated or partial file name within a root path. ");

    cJSON *params = cJSON_CreateObject();
    cJSON_AddStringToObject(params, "type", "object");

    cJSON *props = cJSON_CreateObject();
    cJSON *p1 = cJSON_CreateObject();
    cJSON_AddStringToObject(p1, "type", "string");
    cJSON_AddStringToObject(p1, "description", "The abbreviated or partial file name to search for (case-insensitive contains match).");
    cJSON_AddItemToObject(props, "file_name", p1);

    cJSON *p2 = cJSON_CreateObject();
    cJSON_AddStringToObject(p2, "type", "string");
    cJSON_AddStringToObject(p2, "description", "The root path to start searching from (optional, defaults to '.').");
    cJSON_AddItemToObject(props, "root_path", p2);

    cJSON *p3 = cJSON_CreateObject();
    cJSON_AddStringToObject(p3, "type", "integer");
    cJSON_AddStringToObject(p3, "description", "Maximum number of results to return (optional, default 10).");
    cJSON_AddItemToObject(props, "max_results", p3);

    cJSON_AddItemToObject(params, "properties", props);

    cJSON *req = cJSON_CreateArray();
    cJSON_AddItemToArray(req, cJSON_CreateString("file_name"));
    cJSON_AddItemToObject(params, "required", req);

    cJSON_AddItemToObject(func, "parameters", params);
    cJSON_AddItemToObject(tool, "function", func);

    return tool;
}

// Helper function to recurse directories
static void search_files(const char *dir_path, const char *target_name, cJSON *results, int max_results, int *count) {
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && *count < max_results) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                search_files(full_path, target_name, results, max_results, count);
            } else if (strcasestr(entry->d_name, target_name) != NULL) {
                cJSON_AddItemToArray(results, cJSON_CreateString(full_path));
                (*count)++;
            }
        }
    }

    closedir(dir);
}

void execute_find_file_path(StreamState *state, const ToolCall *tool) {
    cJSON *args_root = cJSON_Parse(tool->function_arguments);
    if (args_root == NULL) {
        fprintf(stderr, "Error parsing tool arguments\n");
        return;
    }

    cJSON *file_name_item = cJSON_GetObjectItemCaseSensitive(args_root, "file_name");
    cJSON *root_path_item = cJSON_GetObjectItemCaseSensitive(args_root, "root_path");
    cJSON *max_results_item = cJSON_GetObjectItemCaseSensitive(args_root, "max_results");

    if (!cJSON_IsString(file_name_item)) {
        fprintf(stderr, "Invalid arguments for find_file_path\n");
        cJSON_Delete(args_root);
        return;
    }

    const char *file_name = file_name_item->valuestring;
    const char *root_path = root_path_item && cJSON_IsString(root_path_item) ? root_path_item->valuestring : ".";
    int max_results = max_results_item && cJSON_IsNumber(max_results_item) ? (int)max_results_item->valuedouble : 10;

    printf("Tool call detected: %s with arguments %s\n", tool->function_name, tool->function_arguments ? tool->function_arguments : "");

    cJSON *results_array = cJSON_CreateArray();
    int count = 0;
    search_files(root_path, file_name, results_array, max_results, &count);

    char *content = cJSON_PrintUnformatted(results_array);
    cJSON_Delete(results_array);

    cJSON_Delete(args_root);

    if (!count) {
        send_tool_response(state, tool, TOOL_ERROR, "No matching files found.");
    } else {
        send_tool_response(state, tool, TOOL_SUCCESS, content);
    }
    free(content); 
}

// ====================== HANDLER ======================
ToolHandler find_file_path_handler = {
    .name = "find_file_path",
    .create_definition = create_find_file_path_tool,
    .execute = execute_find_file_path
};