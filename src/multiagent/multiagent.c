#include "multiagent.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../curl_utils.h"  // for LLM calls

DynamicAgent agents[MAX_AGENTS];
int agent_count = 0;

int spawn_dynamic_agent(const char* name, const char* system_prompt) {
    if (agent_count >= MAX_AGENTS) {
        fprintf(stderr, "Max agents reached\n");
        return -1;
    }
    DynamicAgent* ag = &agents[agent_count];
    strncpy(ag->name, name, 63);
    strncpy(ag->system_prompt, system_prompt, 8191);
    ag->id = agent_count;
    // TODO: init AgentContext & start pthread for this agent's ReAct loop
    printf("[multiagent] Spawned agent '%s' with custom prompt\n", name);
    agent_count++;
    return 0;
}

void send_to_agent(const char* target_name, const char* message) {
    // TODO: enqueue message via message_queue to target agent
    printf("[multiagent] Message to @%s: %s\n", target_name, message);
}

// NEW: LLM decides route OR spawn new agent
char* llm_route_or_spawn(const char* user_input) {
    // Build prompt for orchestrator LLM
    char prompt[4096];
    snprintf(prompt, sizeof(prompt),
        "You are the orchestrator.\n"
        "Active agents:\n");
    for (int i = 0; i < agent_count; i++) {
        snprintf(prompt + strlen(prompt), sizeof(prompt) - strlen(prompt),
                 "@%s: %s\n", agents[i].name, agents[i].system_prompt);
    }
    snprintf(prompt + strlen(prompt), sizeof(prompt) - strlen(prompt),
        "\nUser prompt: %s\n\n"
        "Reply in EXACTLY this format:\n"
        "@existingagentname   (if one fits)\n"
        "or\n"
        "SPAWN:newname:custom system prompt here   (if none fit)\n"
        "Choose the best or spawn a new specialized agent.", user_input);

    // Call inference engine (reuse existing ask_inference_engine)
    char* response = ask_inference_engine(prompt, NULL);  // TODO: wire full context
    // Parse response for @name or SPAWN:...
    if (strstr(response, "SPAWN:")) {
        // parse and call spawn_dynamic_agent
        char newname[64], newprompt[8192];
        sscanf(strstr(response, "SPAWN:") + 6, "%63[^:]:%8191s", newname, newprompt);
        spawn_dynamic_agent(newname, newprompt);
        return strdup(newname);
    }
    // else extract @name
    return strdup(response);  // simplified
}

int run_multiagent_orchestrator(const char* initial_input) {
    printf("[multiagent] Orchestrator started (dynamic mode - LLM can spawn)\n");
    // Example: LLM decides routing or spawning
    char* target = llm_route_or_spawn(initial_input);
    send_to_agent(target, initial_input);
    free(target);
    return 0; // placeholder
}
