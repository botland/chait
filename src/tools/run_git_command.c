#include "../client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ====================== JSON DEFINITION ======================
cJSON *create_run_git_command_tool(void) {
    cJSON *tool = cJSON_CreateObject();
    cJSON_AddStringToObject(tool, "type", "function");

    cJSON *function = cJSON_CreateObject();
    cJSON_AddStringToObject(function, "name", "run_git_command");
    cJSON_AddStringToObject(function, "description", "Run any git command locally in the current working directory (the repository root or any subfolder) and return the full stdout + stderr output. Use this to interact with the local git repository (e.g. status, log, diff, branch, show, etc.).");

    cJSON *parameters = cJSON_CreateObject();
    cJSON_AddStringToObject(parameters, "type", "object");

    cJSON *props = cJSON_CreateObject();

    cJSON *command_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(command_obj, "type", "string");
    cJSON_AddStringToObject(command_obj, "description", "The git command to execute (without the 'git' prefix). Examples: 'status', 'log --oneline -10', 'diff HEAD~1', 'branch -a', 'show HEAD~2:filename.txt'.");
    cJSON_AddItemToObject(props, "command", command_obj);

    cJSON_AddItemToObject(parameters, "properties", props);
    cJSON_AddItemToObject(parameters, "required", cJSON_CreateStringArray((const char*[]){"command"}, 1));

    cJSON_AddItemToObject(function, "parameters", parameters);
    cJSON_AddItemToObject(tool, "function", function);

    return tool;
}

// ====================== EXECUTION ======================
void execute_run_git_command(StreamState *state, const ToolCall *tool) {
    if (!tool || !tool->function_arguments) {
        send_tool_response(state, tool, "error", "No arguments provided");
        return;
    }

    // Simple argument parsing (arguments is JSON string)
    cJSON *args = cJSON_Parse(tool->function_arguments);
    if (!args) {
        send_tool_response(state, tool, "error", "Failed to parse arguments");
        return;
    }

    cJSON *command_item = cJSON_GetObjectItem(args, "command");
    if (!command_item || !command_item->valuestring || strlen(command_item->valuestring) == 0) {
        cJSON_Delete(args);
        send_tool_response(state, tool, "error", "No command provided");
        return;
    }

    const char *command = command_item->valuestring;

    // Build safe full command: git <command> 2>&1 (captures both stdout and stderr)
    char full_command[2048];
    snprintf(full_command, sizeof(full_command), "git %s 2>&1", command);

    FILE *pipe = popen(full_command, "r");
    if (!pipe) {
        cJSON_Delete(args);
        send_tool_response(state, tool, "error", "Failed to start git command");
        return;
    }

    // Read output dynamically
    char buffer[4096];
    char *output = malloc(1);
    if (!output) {
        pclose(pipe);
        cJSON_Delete(args);
        send_tool_response(state, tool, "error", "Memory allocation failed");
        return;
    }
    output[0] = '\0';
    size_t total_len = 0;

    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        size_t chunk_len = strlen(buffer);
        char *new_output = realloc(output, total_len + chunk_len + 1);
        if (!new_output) {
            free(output);
            pclose(pipe);
            cJSON_Delete(args);
            send_tool_response(state, tool, "error", "Memory reallocation failed while reading git output");
            return;
        }
        output = new_output;
        memcpy(output + total_len, buffer, chunk_len);
        total_len += chunk_len;
        output[total_len] = '\0';
    }

    int status = pclose(pipe);
    cJSON_Delete(args);

    // Always return success with the captured output (git errors appear in the output because of 2>&1)
    // The LLM can interpret non-zero exit codes via the content if needed.
    send_tool_response(state, tool, "success", output);
    free(output);
}

// ====================== HANDLER ======================
ToolHandler run_git_command_handler = {
    .name = "run_git_command",
    .create_definition = create_run_git_command_tool,
    .execute = execute_run_git_command
};
