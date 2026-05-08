// apply_patch.c
#include "../client.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

// Assume ToolCall struct, BUFFER_SIZE, add_to_history, ask_inference_engine are defined elsewhere

cJSON *create_apply_patch_tool(void) {
    cJSON *tool = cJSON_CreateObject();
    cJSON_AddStringToObject(tool, "type", "function");

    cJSON *func = cJSON_CreateObject();
    cJSON_AddStringToObject(func, "name", "apply_patch");
    cJSON_AddStringToObject(func, "description",
        "Single, atomic edit. Outputs { success: bool, affected_lines }.");

    cJSON *params = cJSON_CreateObject();
    cJSON_AddStringToObject(params, "type", "object");

    cJSON *props = cJSON_CreateObject();
    cJSON *p1 = cJSON_CreateObject();
    cJSON_AddStringToObject(p1, "type", "string");
    cJSON_AddStringToObject(p1, "description", "The full path to the file.");
    cJSON_AddItemToObject(props, "file_path", p1);

    cJSON *p2 = cJSON_CreateObject();
    cJSON_AddStringToObject(p2, "type", "string");
    cJSON_AddStringToObject(p2, "description", "The unified diff patch to apply.");
    cJSON_AddItemToObject(props, "patch", p2);

    cJSON_AddItemToObject(params, "properties", props);

    cJSON *req = cJSON_CreateArray();
    cJSON_AddItemToArray(req, cJSON_CreateString("file_path"));
    cJSON_AddItemToArray(req, cJSON_CreateString("patch"));
    cJSON_AddItemToObject(params, "required", req);

    cJSON_AddItemToObject(func, "parameters", params);
    cJSON_AddItemToObject(tool, "function", func);

    return tool;
}

/*void execute_apply_patch(const ToolCall *tool) {
    cJSON *args_root = cJSON_Parse(tool->function_arguments);
    if (args_root == NULL) {
        fprintf(stderr, "Error parsing tool arguments\n");
        return;
    }

    cJSON *file_path = cJSON_GetObjectItemCaseSensitive(args_root, "file_path");
    cJSON *patch = cJSON_GetObjectItemCaseSensitive(args_root, "patch");

    printf("Tool call detected: %s with arguments %s\n", tool->function_name, tool->function_arguments ? tool->function_arguments : "");

    char *tool_result = NULL;

    // Real impl: Write patch to temp file, use system("patch file_path < temp.patch"), check return code
    // For now, simulate
    tool_result = strdup("{\"success\": true, \"affected_lines\": 5}");

    char tool_msg[BUFFER_SIZE];
    snprintf(tool_msg, sizeof(tool_msg), "{\"role\": \"tool\", \"tool_call_id\": \"%s\", \"name\": \"%s\", \"content\": \"%s\"}",
             tool->id ? tool->id : "", tool->function_name, tool_result);
    add_to_history("tool", tool_msg);

    free(tool_result);
    cJSON_Delete(args_root);

    ask_inference_engine(NULL);
}*/

void execute_apply_patch(const ToolCall *tool) {
    cJSON *args_root = cJSON_Parse(tool->function_arguments);
    if (args_root == NULL) {
        fprintf(stderr, "Error parsing tool arguments\n");
        return;
    }

    cJSON *file_path_item = cJSON_GetObjectItemCaseSensitive(args_root, "file_path");
    cJSON *patch_item = cJSON_GetObjectItemCaseSensitive(args_root, "patch");

    if (!cJSON_IsString(file_path_item) || !cJSON_IsString(patch_item)) {
        fprintf(stderr, "Invalid arguments for apply_patch\n");
        cJSON_Delete(args_root);
        return;
    }

    const char *file_path = file_path_item->valuestring;
    const char *patch_content = patch_item->valuestring;

    printf("Tool call detected: %s with arguments %s\n", tool->function_name, tool->function_arguments ? tool->function_arguments : "");

    char *tool_result = NULL;

    // Create temporary file for patch
    char template[] = "/tmp/patchXXXXXX";
    int fd = mkstemp(template);
    if (fd == -1) {
        fprintf(stderr, "Error creating temp file\n");
        tool_result = strdup("{\"success\": false, \"affected_lines\": 0}");
    } else {
        FILE *temp = fdopen(fd, "w");
        if (temp == NULL) {
            fprintf(stderr, "Error opening temp file\n");
            close(fd);
            unlink(template);
            tool_result = strdup("{\"success\": false, \"affected_lines\": 0}");
        } else {
            fputs(patch_content, temp);
            fclose(temp);

            // Apply the patch
            char cmd[4096];
            snprintf(cmd, sizeof(cmd), "patch \"%s\" < \"%s\"", file_path, template);
            int ret = system(cmd);
            int success = (ret == 0);

            // Count affected lines (added or removed)
            int affected = 0;
            FILE *patchf = fopen(template, "r");
            if (patchf) {
                char line[4096];
                while (fgets(line, sizeof(line), patchf)) {
                    if ((line[0] == '+' && (line[1] != '+' || line[2] != '+')) ||
                        (line[0] == '-' && (line[1] != '-' || line[2] != '-'))) {
                        affected++;
                    }
                }
                fclose(patchf);
            }

            // Cleanup
            unlink(template);

            // Prepare result
            char result[256];
            snprintf(result, sizeof(result), "{\"success\": %s, \"affected_lines\": %d}",
                     success ? "true" : "false", success ? affected : 0);
            tool_result = strdup(result);
        }
    }

    if (tool_result == NULL) {
        tool_result = strdup("{\"success\": false, \"affected_lines\": 0}");
    }

    char tool_msg[BUFFER_SIZE];
    snprintf(tool_msg, sizeof(tool_msg), "{\"role\": \"tool\", \"tool_call_id\": \"%s\", \"name\": \"%s\", \"content\": \"%s\"}",
             tool->id ? tool->id : "", tool->function_name, tool_result);
    add_to_history("tool", tool_msg);

    free(tool_result);
    cJSON_Delete(args_root);

    ask_inference_engine(NULL);
}
