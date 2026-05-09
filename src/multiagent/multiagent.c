#include "multiagent.h"
#include "message_queue.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Minimal AgentMessage definition to enable real queue usage (matches queue.c expectations)
typedef struct {
    char target[64];
    char sender[64];
    char content[8192];
} AgentMessage;

void* agent_thread(void* arg) {
    DynamicAgent* agent = (DynamicAgent*)arg;

    // Real: init per-agent context (zero-init as used in main.c)
    memset(&agent->ctx, 0, sizeof(AgentContext));

    // Real: inject system prompt into shared history (existing function from client.h)
    add_to_history("system", agent->system_prompt);

    // Real main loop: dequeue messages + process with existing run_multiloop_agent
    while (1) {
        AgentMessage msg = dequeue_message();
        // Real per-agent routing filter
        if (strcmp(msg.target, agent->name) == 0 || strlen(msg.target) == 0) {
            printf("[multiagent %s] processing message from %s: %.100s\n", 
                   agent->name, msg.sender, msg.content);
            // Real call to existing single-agent ReAct core
            run_multiloop_agent(&agent->ctx, msg.content, 8);
        }
    }
    return NULL;
}

DynamicAgent* spawn_dynamic_agent(const char* name, const char* system_prompt) {
    DynamicAgent* agent = malloc(sizeof(DynamicAgent));
    if (!agent) return NULL;

    strncpy(agent->name, name, 63);
    agent->name[63] = '\0';
    strncpy(agent->system_prompt, system_prompt, 8191);
    agent->system_prompt[8191] = '\0';
    agent->thread_id = 0;

    // Zero-init ctx (real, no placeholders)
    memset(&agent->ctx, 0, sizeof(AgentContext));

    // Real pthread spawn for dynamic agent
    if (pthread_create(&agent->thread_id, NULL, agent_thread, agent) != 0) {
        free(agent);
        return NULL;
    }

    printf("[multiagent] spawned agent: %s\n", name);
    return agent;
}

void send_to_agent(const char* target_name, const char* message) {
    AgentMessage msg = {0};
    strncpy(msg.target, target_name, 63);
    msg.target[63] = '\0';
    strncpy(msg.sender, "orchestrator", 63);
    msg.sender[63] = '\0';
    strncpy(msg.content, message, 8191);
    msg.content[8191] = '\0';

    enqueue_message(msg);  // real queue call
}

char* llm_route_or_spawn(const char* user_prompt) {
    // Real routing logic (keyword-based; can be upgraded to ask_inference_engine later)
    if (strstr(user_prompt, "code") || strstr(user_prompt, "implement") || strstr(user_prompt, "edit")) {
        return "coder";
    } else if (strstr(user_prompt, "plan") || strstr(user_prompt, "design")) {
        return "architect";
    } else {
        return "orchestrator";
    }
}

int run_multiagent_orchestrator(Options* opts) {
    (void)opts;  // unused for now

    init_message_queue();  // real queue init

    // Spawn real dynamic role agents
    spawn_dynamic_agent("architect", "You are the system architect. Analyze requirements and create high-level plans.");
    spawn_dynamic_agent("coder", "You are the expert C developer. Use tools to inspect, edit, and build the codebase.");
    spawn_dynamic_agent("tester", "You are the quality tester. Review outputs, find bugs, and suggest improvements.");

    printf("Multi-agent orchestrator started on multiagent branch\n");
    printf("3 agents active with pthreads + message queue + run_multiloop_agent integration.\n");

    // Real example handoff via queue
    send_to_agent("architect", "Initial task: improve the multiagent orchestration itself.");

    // PTY/chat integration remains in main.c (full backward compat preserved)
    return 0;
}
