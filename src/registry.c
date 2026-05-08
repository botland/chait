#include "client.h"

static ToolHandler *tool_registry = NULL;
static int registry_size = 0;
static int registry_capacity = 0;

void register_tool(ToolHandler handler) {
    if (registry_size >= registry_capacity) {
        registry_capacity = registry_capacity ? registry_capacity * 2 : 16;
        tool_registry = realloc(tool_registry, registry_capacity * sizeof(ToolHandler));
    }
    tool_registry[registry_size++] = handler;
}

ToolHandler *find_tool_handler(const char *name) {
    for (int i = 0; i < registry_size; i++) {
        if (strcmp(tool_registry[i].name, name) == 0) {
            return &tool_registry[i];
        }
    }
    return NULL;
}

void append_all_tool_definitions(cJSON *tools_array) {
    for (int i = 0; i < registry_size; i++) {
        if (tool_registry[i].create_definition) {
            cJSON *def = tool_registry[i].create_definition();
            if (def) cJSON_AddItemToArray(tools_array, def);
        }
    }
}

void register_all_tools(void) {
    registry_size = 0;

    extern ToolHandler get_file_content_handler;
    register_tool(get_file_content_handler);

    extern ToolHandler find_file_path_handler;
    register_tool(find_file_path_handler);
}
