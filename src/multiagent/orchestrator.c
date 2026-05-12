#include "multiagent.h"
#include <stdio.h>
#include <string.h>

// === FULL SUPERVISOR-BASED ORCHESTRATOR (replaces the old stub - exact match to user partial, no breakage)
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
