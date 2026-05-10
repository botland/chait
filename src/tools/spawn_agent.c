#include "../client.h"
#include "../multiagent/multiagent.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ====================== JSON DEFINITION ======================
cJSON *create_spawn_agent_tool(void) {
    cJSON *tool = cJSON_CreateObject();
    cJSON_AddStringToObject(tool, "type", "function");

    cJSON *function = cJSON_CreateObject();
    cJSON_AddStringToObject(function, "name", "spawn_agent");
    cJSON_AddStringToObject(function, "description", "Spawn a new dynamic agent in the multi-agent system. Provide a unique name and system prompt to create specialized agents (coder, architect, tester, etc.).");

    cJSON *parameters = cJSON_CreateObject();
    cJSON_AddStringToObject(parameters, "type", "object");

    cJSON *props = cJSON_CreateObject();

    cJSON *name_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(name_obj, "type", "string");
    cJSON_AddStringToObject(name_obj, "description", "Unique name of the agent (e.g. 'coder', 'reviewer')");
    cJSON_AddItemToObject(props, "name", name_obj);

    cJSON *prompt_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(prompt_obj, "type", "string");
    cJSON_AddStringToObject(prompt_obj, "description", "Full system prompt/instructions for the new agent");
    cJSON_AddItemToObject(props, "system_prompt", prompt_obj);

    cJSON_AddItemToObject(parameters, "properties", props);
    cJSON_AddItemToObject(parameters, "required", cJSON_CreateStringArray((const char*[]){"name", "system_prompt"}, 2));

    cJSON_AddItemToObject(function, "parameters", parameters);
    cJSON_AddItemToObject(tool, "function", function);

    return tool;
}

// ====================== EXECUTION ======================
void execute_spawn_agent(StreamState *state, const ToolCall *tool) {
    if (!tool || !tool->function_arguments) {
        send_tool_response(state, tool, "error", "No arguments provided");
        return;
    }

    cJSON *args = cJSON_Parse(tool->function_arguments);
    if (!args) {
        send_tool_response(state, tool, "error", "Failed to parse arguments");
        return;
    }

    cJSON *name_item = cJSON_GetObjectItem(args, "name");
    cJSON *prompt_item = cJSON_GetObjectItem(args, "system_prompt");
    if (!name_item || !name_item->valuestring || !prompt_item || !prompt_item->valuestring) {
        cJSON_Delete(args);
        send_tool_response(state, tool, "error", "Both 'name' and 'system_prompt' are required");
        return;
    }

    const char *name = name_item->valuestring;
    const char *system_prompt = prompt_item->valuestring;

    DynamicAgent* agent = spawn_dynamic_agent(name, system_prompt);  // real call to existing multiagent impl
    if (agent) {
        char success_msg[256];
        snprintf(success_msg, sizeof(success_msg), "Successfully spawned agent '%s' (pthread started, ready for messages via queue)", name);
        send_tool_response(state, tool, "success", success_msg);
    } else {
        send_tool_response(state, tool, "error", "Failed to spawn agent (malloc/pthread failed)");
    }

    cJSON_Delete(args);
}

// ====================== HANDLER ======================
ToolHandler spawn_agent_handler = {
    .name = "spawn_agent",
    .create_definition = create_spawn_agent_tool,
    .execute = execute_spawn_agent
};