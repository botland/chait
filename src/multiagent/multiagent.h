#pragma once

#include "../agent_context.h"
#include <pthread.h>
#include <stdbool.h>

// Multi-agent role definitions
typedef enum {
    ROLE_ARCHITECT = 0,
    ROLE_CODER,
    ROLE_TESTER,
    ROLE_DEBUGGER,
    ROLE_ORCHESTRATOR
} AgentRole;

// Per-agent instance
typedef struct {
    AgentRole role;
    AgentContext ctx;
    pthread_t thread;
    int role_id;
} AgentInstance;

// Message for inter-agent communication
typedef struct {
    AgentRole from_role;
    AgentRole to_role;
    char payload[1024];
    char result[4096];
    bool is_handled;
} AgentMessage;

int run_multiagent_orchestrator(const char *initial_input, int max_agents);
void init_multiagent_system(void);
