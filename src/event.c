#include "event.h"
#include <stdlib.h>
#include <string.h>

void init_event_queue(EventQueue *q) { q->head = q->tail = 0; }

void push_event(EventQueue *q, Event ev) {
    if (q->tail >= MAX_EVENTS) return;
    q->events[q->tail++] = ev;
}

int pop_event(EventQueue *q, Event *out) {
    if (q->head == q->tail) return 0;
    *out = q->events[q->head++];
    return 1;
}

void free_event(Event *ev) {
    if (ev->type == EVENT_TEXT && ev->text) free(ev->text);
    else if (ev->type == EVENT_TOOL_CALL) {
        if (ev->tool.id) free(ev->tool.id);
        free(ev->tool.name);
        free(ev->tool.arguments);
    }
}