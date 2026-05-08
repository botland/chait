#ifndef EVENT_H
#define EVENT_H

typedef enum {
    EVENT_TEXT,
    EVENT_TOOL_CALL,
    EVENT_DONE
} EventType;

typedef struct {
    char *name;
    char *arguments;
} ToolCallEvent;

typedef struct {
    EventType type;
    char *text;           // EVENT_TEXT
    ToolCallEvent tool;   // EVENT_TOOL_CALL
} Event;

#define MAX_EVENTS 1024

typedef struct {
    Event events[MAX_EVENTS];
    int head;
    int tail;
} EventQueue;

void init_event_queue(EventQueue *q);
void push_event(EventQueue *q, Event ev);
int pop_event(EventQueue *q, Event *out);
void free_event(Event *ev);

#endif