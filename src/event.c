#include "client.h"
#include "event.h"
#include <stdlib.h>
#include <string.h>

const char* get_event(EventType event) {
    switch (event) {
        case EVENT_DONE: return "done";
        case EVENT_TEXT: return "text";
        case EVENT_TOOL_CALL: return "tool_call";
        default:
            return "undefined";
    }
}

void init_event_queue(EventQueue *q) {
    q->head = q->tail = 0;
}

void push_event(EventQueue *q, Event ev) {
    if (q->tail >= MAX_EVENTS) return;
    q->events[q->tail++] = ev;
    if (debug_level > 0 && ev.type != EVENT_TEXT) {
        fprintf(stderr, "[EVENT] push event type: %s\n", get_event(ev.type));
    }
}

int pop_event(EventQueue *q, Event *out) {
    if (q->head == q->tail) return 0;
    *out = q->events[q->head++];
    if (debug_level > 0 && out->type != EVENT_TEXT) {
        fprintf(stderr, "[EVENT] pop event type: %s\n", get_event(out->type));
    }
    return 1;
}

void free_event(Event *ev) {
    if (ev->type == EVENT_TEXT && ev->text) {
        free(ev->text);
    } else if (ev->type == EVENT_TOOL_CALL) {
        if (ev->tool.id) {
            free(ev->tool.id);
        }
        free(ev->tool.name);
        free(ev->tool.arguments);
    }
}

ToolResponseParams *process_events(StreamState *state) {
    Event ev;
    ToolResponseParams *trp = NULL;
    while (pop_event(&state->queue, &ev)) {
        switch (ev.type) {
        case EVENT_TEXT:
            printf("%s", ev.text);
            fflush(stdout);
            free(ev.text);
            break;

        case EVENT_TOOL_CALL:
            {
                ToolCall tc = {
                   .id = ev.tool.id,
                   .type = NULL,
                   .function_name = ev.tool.name,
                   .function_arguments = ev.tool.arguments
                };
                trp = execute_tool(state, &tc);
//printf("after call: %x\n", trp);
                free_event(&ev);
            }
            break;

        case EVENT_DONE:
            free_event(&ev);
        }
    }
//printf("before exit: %x\n", trp);
    return trp;
}
