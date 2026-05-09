#include "message_queue.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define QUEUE_SIZE 64

static MessageQueue queue;

void init_message_queue() {
    memset(&queue, 0, sizeof(MessageQueue));
    pthread_mutex_init(&queue.mutex, NULL);
    pthread_cond_init(&queue.not_empty, NULL);
    pthread_cond_init(&queue.not_full, NULL);
}

void enqueue_message(AgentMessage msg) {
    pthread_mutex_lock(&queue.mutex);
    while (queue.count == QUEUE_SIZE) pthread_cond_wait(&queue.not_full, &queue.mutex);
    queue.messages[queue.tail] = msg;
    queue.tail = (queue.tail + 1) % QUEUE_SIZE;
    queue.count++;
    pthread_cond_signal(&queue.not_empty);
    pthread_mutex_unlock(&queue.mutex);
}

AgentMessage dequeue_message() {
    AgentMessage msg;
    pthread_mutex_lock(&queue.mutex);
    while (queue.count == 0) pthread_cond_wait(&queue.not_empty, &queue.mutex);
    msg = queue.messages[queue.head];
    queue.head = (queue.head + 1) % QUEUE_SIZE;
    queue.count--;
    pthread_cond_signal(&queue.not_full);
    pthread_mutex_unlock(&queue.mutex);
    return msg;
}

void destroy_message_queue() {
    pthread_mutex_destroy(&queue.mutex);
    pthread_cond_destroy(&queue.not_empty);
    pthread_cond_destroy(&queue.not_full);
}
