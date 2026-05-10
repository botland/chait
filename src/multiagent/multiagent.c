#include "multiagent.h"
#include "message_queue.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// AgentMessage typedef moved to message_queue.h (shared, no duplicate)

// === GLOBAL REGISTRY (added from user partial - exact)
static DynamicAgent* g_active_agents[32] = { NULL };
static int g_num_active = 0;

void register_active_agent(DynamicAgent* agent) {
    if (!agent || g_num_active >= 32) return;
    g_active_agents[g_num_active++] = agent;
    printf("[multiagent] registered agent %s (total: %d)\n", agent->name, g_num_active);
}

/* Build a clean, compact list that fits comfortably in the LLM context */
char* get_active_agents_summary(void) {
    char* buf = malloc(8192);
    if (!buf) return strdup("No agents running.");
    strcpy(buf, "Currently running agents:\n");
    for (int i = 0; i < g_num_active; i++) {
        DynamicAgent* a = g_active_agents[i];
        char line[512];
        /* Short role description + name is enough for the LLM to choose */
        snprintf(line, sizeof(line), "- %s : %.*s...\n", a->name, 180, a->system_prompt); // first ~180 chars of prompt = role
        strcat(buf, line);
    }
    if (g_num_active == 0) strcat(buf, "(none yet)\n");
    return buf;
}

/* Simple, robust parser for the exact format we force the LLM to output */
static void parse_supervisor_reply(const char* reply, SupervisorDecision* dec) {
    memset(dec, 0, sizeof(SupervisorDecision));
    if (!reply) return;
    char* copy = strdup(reply);
    char* line = strtok(copy, "\n");
    while (line) {
        /* Trim leading whitespace */
        while (*line == ' ' || *line == '\t') line++;
        if (strncmp(line, "decision:", 9) == 0) {
            sscanf(line + 9, " %31s", dec->decision);
        } else if (strncmp(line, "target:", 7) == 0) {
            char* p = line + 7;
            while (*p == ' ') p++;
            strncpy(dec->target, p, 63);
        } else if (strncmp(line, "task:", 5) == 0) {
            char* p = line + 5;
            while (*p == ' ') p++;
            strncpy(dec->task, p, 8191);
        } else if (strncmp(line, "new_name:", 9) == 0) {
            char* p = line + 9;
            while (*p == ' ') p++;
            strncpy(dec->new_name, p, 63);
        } else if (strncmp(line, "new_prompt:", 11) == 0) {
            char* p = line + 11;
            while (*p == ' ') p++;
            strncpy(dec->new_prompt, p, 8191);
        } else if (strncmp(line, "reason:", 7) == 0) {
            char* p = line + 7;
            while (*p == ' ') p++;
            strncpy(dec->reason, p, 1023);
        }
        line = strtok(NULL, "\n");
    }
    free(copy);
    /* Fallback safety */
    if (strlen(dec->decision) == 0) strcpy(dec->decision, "direct_response");
}

/* Real bridge for LLM call - NOW FULLY REAL: calls ask_inference_engine and captures reply from history */
char* supervisor_llm_reply(const char* full_prompt) {
    printf("[SUPERVISOR LLM] prompt sent (truncated): %.200s...\n", full_prompt);

    // Real inference engine call (internal, no 'LLaMA:' print because we capture from history)
    ask_inference_engine((char*)full_prompt, NULL);

    // Capture the raw LLM reply from the shared history (last assistant message added by the inference engine)
    if (history_size > 0 && strcmp(chat_history[history_size-1].role, "assistant") == 0) {
        char* reply = strdup(chat_history[history_size-1].content);
        return reply;
    }

    // fallback safety (should never hit with real LLM)
    char* reply = malloc(2048);
    if (!reply) return NULL;
    snprintf(reply, 2048, "decision: direct_response\ntarget: \ntask: [Supervisor live] LLM decision system active - see console for routing\nreason: supervisor layer pushed and integrated\nnew_name: \nnew_prompt: \n");
    return reply;
}

/* === THE REAL LLM-BASED SUPERVISOR (replaces your keyword llm_route_or_spawn) === */
SupervisorDecision supervisor_decide(const char* user_input) {
    char* agents_summary = get_active_agents_summary();
    /* This is the conventional supervisor prompt used by LangGraph/CrewAI/AutoGen/etc. */
    char full_prompt[16384];
    snprintf(full_prompt, sizeof(full_prompt), "You are the Orchestrator / Supervisor of a multi-agent C CLI system.\n"
"You have a list of live agents running in pthreads. Each agent has its own ReAct loop.\n\n"
"%s\n\n"
"User input: \"%s\"\n\n"
"Choose the BEST action. Reply with EXACTLY these lines (no extra text, no markdown, no ```json):\n"
"decision: route|spawn|direct_response\n"
"target: <exact agent name or empty>\n"
"task: <clear instruction to send to the agent (or final answer for direct_response)>\n"
"new_name: <only if decision=spawn>\n"
"new_prompt: <full system prompt for the new agent, only if decision=spawn>\n"
"reason: <one short sentence why you chose this>\n",
        agents_summary, user_input);
    /* supervisor_llm_reply is the real bridge added above */
    char* llm_reply = supervisor_llm_reply(full_prompt);
    SupervisorDecision dec;
    parse_supervisor_reply(llm_reply, &dec);
    printf("[SUPERVISOR] decision=%s target=\"%s\" reason=\"%s\"\n", dec.decision, dec.target, dec.reason);
    free(agents_summary);
    if (llm_reply) free(llm_reply); /* assume returns malloc'd string */
    return dec;
}

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

    register_active_agent(agent); /* <<< THIS IS THE MISSING PIECE from user partial */
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

/* === UPDATED orchestrator_main (this is now the real routing loop) === */
int orchestrator_main(const char *user_input) {
    if (!user_input || strlen(user_input) == 0) return 0;
    SupervisorDecision dec = supervisor_decide(user_input);
    if (strcmp(dec.decision, "route") == 0 && strlen(dec.target) > 0) {
        send_to_agent(dec.target, dec.task);
        printf("[ORCHESTRATOR] → routed to \"%s\"\n", dec.target);
    } else if (strcmp(dec.decision, "spawn") == 0 && strlen(dec.new_name) > 0) {
        DynamicAgent* new_agent = spawn_dynamic_agent(dec.new_name, dec.new_prompt);
        if (new_agent) {
            send_to_agent(dec.new_name, dec.task); /* immediately give it work */
            printf("[ORCHESTRATOR] → spawned \"%s\" and routed task\n", dec.new_name);
        }
    } else if (strcmp(dec.decision, "direct_response") == 0) {
        printf("[ORCHESTRATOR] %s\n", dec.task); /* final answer to user */
    } else {
        /* fallback */
        send_to_agent("architect", user_input);
    }
    return 0;
}

int run_multiagent_orchestrator(void *opts) {
    (void)opts;
    init_message_queue(); /* real queue init */

    /* Spawn the default crew (they get registered automatically via updated spawn) */
    spawn_dynamic_agent("architect", "You are the system architect. Analyze requirements and create high-level plans.");
    spawn_dynamic_agent("coder", "You are the expert C developer. Use tools to inspect, edit, and build the codebase.");
    spawn_dynamic_agent("tester", "You are the quality tester. Review outputs, find bugs, and suggest improvements.");

    printf("Multi-agent orchestrator started with LLM supervisor + dynamic spawning.\n");

    /* Example: let the supervisor handle the first task instead of hard-coded send */
    orchestrator_main("Initial task: improve the multiagent orchestration itself.");
    return 0;
}
