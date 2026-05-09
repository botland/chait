#pragma once

#include <pthread.h>
#include "multiagent.h"

#define MAX_QUEUE_SIZE 32

typedef struct {
    AgentMessage messages[MAX_QUEUE_SIZE];
    int head;
    int tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} MessageQueue;

void mq_init(MessageQueue *mq);
void mq_send(MessageQueue *mq, AgentMessage *msg);
int mq_receive(MessageQueue *mq, AgentMessage *out_msg);
