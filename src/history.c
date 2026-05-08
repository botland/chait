#include "client.h"

// Global history array
Message *chat_history = NULL;
int history_size = 0;
int history_capacity = 0;

void add_to_history(const char *role, const char *content) {
    if (history_size >= history_capacity) {
        history_capacity += 10;
        chat_history = realloc(chat_history, history_capacity * sizeof(Message));
        if (!chat_history) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(1);
        }
    }
    chat_history[history_size].role = strdup(role);
    chat_history[history_size].content = strdup(content ? content : "");
    history_size++;
}

// Prune history to last N turns (user + assistant pairs)
void prune_history() {
    int max_entries = MAX_HISTORY_TURNS * 2;  // user + assistant
    if (history_size > max_entries) {
        int shift = history_size - max_entries;
        for (int i = 0; i < shift; i++) {
            free(chat_history[i].role);
            free(chat_history[i].content);
        }
        memmove(chat_history, chat_history + shift, max_entries * sizeof(Message));
        history_size = max_entries;
    }
}

// Free history at cleanup
void free_history() {
    for (int i = 0; i < history_size; i++) {
        free(chat_history[i].role);
        free(chat_history[i].content);
    }
    free(chat_history);
    chat_history = NULL;
    history_size = 0;
    history_capacity = 0;
}