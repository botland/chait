#pragma once

#include <pthread.h>

#define MAX_QUEUE_SIZE 32

typedef struct {
    char target[64];
    char sender[64];
    char content[8192];
} AgentMessage;

typedef struct {
    AgentMessage messages[MAX_QUEUE_SIZE];
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    pthread_cond_t not_empty, not_full;
} MessageQueue;

void mq_init(MessageQueue *mq);
void mq_send(MessageQueue *mq, AgentMessage *msg);
int mq_receive(MessageQueue *mq, AgentMessage *out_msg);

// Added minimal declarations to match real calls in multiagent.c (no placeholders)
void init_message_queue(void);
void enqueue_message(AgentMessage msg);
AgentMessage dequeue_message(void);
