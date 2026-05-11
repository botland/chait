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
        fprintf(stderr, "[EVENT] got event type: %s\n", get_event(ev.type));
    }
}

int pop_event(EventQueue *q, Event *out) {
    if (q->head == q->tail) return 0;
    *out = q->events[q->head++];
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
