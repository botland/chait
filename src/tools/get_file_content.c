#include "../client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// ====================== JSON DEFINITION ======================
cJSON *create_get_file_content_tool(void) {
    cJSON *tool = cJSON_CreateObject();
    cJSON_AddStringToObject(tool, "type", "function");

    cJSON *function = cJSON_CreateObject();
    cJSON_AddStringToObject(function, "name", "get_file_content");
    cJSON_AddStringToObject(function, "description", "Read full content of a file (or line range)");

    cJSON *parameters = cJSON_CreateObject();
    cJSON_AddStringToObject(parameters, "type", "object");

    cJSON *props = cJSON_CreateObject();

    cJSON *path_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(path_obj, "type", "string");
    cJSON_AddStringToObject(path_obj, "description", "Path to the file");
    cJSON_AddItemToObject(props, "path", path_obj);

    cJSON *start_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(start_obj, "type", "integer");
    cJSON_AddStringToObject(start_obj, "description", "Start line (optional)");
    cJSON_AddItemToObject(props, "start_line", start_obj);

    cJSON *end_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(end_obj, "type", "integer");
    cJSON_AddStringToObject(end_obj, "description", "End line (optional)");
    cJSON_AddItemToObject(props, "end_line", end_obj);

    cJSON_AddItemToObject(parameters, "properties", props);
    cJSON_AddItemToObject(parameters, "required", cJSON_CreateStringArray((const char*[]){"path"}, 1));

    cJSON_AddItemToObject(function, "parameters", parameters);
    cJSON_AddItemToObject(tool, "function", function);

    return tool;
}

// ====================== EXECUTION ======================
void execute_get_file_content(StreamState *state, const ToolCall *tool) {
    if (!tool || !tool->function_arguments) {
        send_tool_response(state, tool, TOOL_ERROR, "No arguments provided");
        return;
    }

    // Simple argument parsing (arguments is JSON string)
    cJSON *args = cJSON_Parse(tool->function_arguments);
    if (!args) {
        send_tool_response(state, tool, TOOL_ERROR, "Failed to parse arguments");
        return;
    }

    cJSON *path_item = cJSON_GetObjectItem(args, "path");
    if (!path_item || !path_item->valuestring) {
        cJSON_Delete(args);
        send_tool_response(state, tool, TOOL_ERROR, "No path provided");
        return;
    }

    const char *path = path_item->valuestring;

    if (access(path, R_OK) != 0) {
        cJSON_Delete(args);
        send_tool_response(state, tool, TOOL_ERROR, "Error opening file or file not found");
        return;
    }

    FILE *f = fopen(path, "r");
    if (!f) {
        cJSON_Delete(args);
        send_tool_response(state, tool, TOOL_ERROR, "Failed to open file");
        return;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *content = malloc(size + 1);
    fread(content, 1, size, f);
    content[size] = '\0';
    fclose(f);
    cJSON_Delete(args);

    send_tool_response(state, tool, TOOL_SUCCESS, content);
    free(content);
}

// ====================== HANDLER ======================
ToolHandler get_file_content_handler = {
    .name = "get_file_content",
    .create_definition = create_get_file_content_tool,
    .execute = execute_get_file_content
};
