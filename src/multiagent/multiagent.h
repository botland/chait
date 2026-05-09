#ifndef MULTIAGENT_H
#define MULTIAGENT_H

#include <pthread.h>
#include "../client.h"
#include "message_queue.h"
#include "../command.h"  // for Options* (fixes unknown type in .c)

#define MAX_AGENTS 16

typedef struct {
    char name[64];
    char system_prompt[8192];
    pthread_t thread_id;
    AgentContext ctx;
    int id;
} DynamicAgent;

// Global orchestrator state (protected by mutex)
extern DynamicAgent agents[MAX_AGENTS];
extern int agent_count;

DynamicAgent* spawn_dynamic_agent(const char* name, const char* system_prompt);
void send_to_agent(const char* target_name, const char* message);
int run_multiagent_orchestrator(Options* opts);

#endif
