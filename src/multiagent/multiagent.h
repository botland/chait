#ifndef MULTIAGENT_H
#define MULTIAGENT_H

#include <pthread.h>
#include "../client.h"
#include "message_queue.h"

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
int run_multiagent_orchestrator(void *opts);

// === SUPERVISOR ROUTING LAYER (added from user partial - exact merge)
typedef struct {
    char decision[32]; // "route" | "spawn" | "direct_response"
    char target[64]; // agent name (for route)
    char task[8192]; // instruction / content to send
    char new_name[64]; // only for spawn
    char new_prompt[8192]; // full system prompt for new agent
    char reason[1024]; // for debugging / logging
} SupervisorDecision;

extern SupervisorDecision supervisor_decide(const char* user_input);
extern void register_active_agent(DynamicAgent* agent); // call from spawn
extern char* get_active_agents_summary(void); // for LLM prompt

#endif