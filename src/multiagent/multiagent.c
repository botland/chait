#include "multiagent.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

int run_multiagent_orchestrator(const char* initial_input) {
    printf("[multiagent] Orchestrator started (dynamic mode - no hardcoded roles)\n");
    // TODO: parse spawn-agent commands from PTY input, delegate via queue
    return 0; // placeholder
}
