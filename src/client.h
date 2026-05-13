#ifndef CLIENT_H
#define CLIENT_H

#define _GNU_SOURCE

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <cjson/cJSON.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>

#define RUN_CHAT_CLIENT

// Define constants
#define SERVER_URL "http://192.168.1.16:8080/v1/chat/completions"
#define BUFFER_SIZE 8192
#define MAX_HISTORY_TURNS 10  // Limit to last 10 user-assistant pairs
#define MAX_HISTORY_SIZE 4096
#define MAX_REQUEST_SIZE 8192
#define MAX_RESPONSE_SIZE 8192
#define CHAT_HISTORY_FILE ".llama_chat_history"

//#define MAX_HISTORY_SIZE 8192  // For any string buffers

// Define colors
#define COLOR_USER    "\033[1;34m"   // Bold blue
#define COLOR_LLAMA   "\033[1;32m"   // Bold green
#define COLOR_RESET   "\033[0m"

#define SOCKET_PATH "/tmp/llm.sock"
#define SOCKET_BACKLOG 5

typedef struct {
    char *content;
    size_t length;
} Buffer;

// Simple message struct for history
typedef struct {
    char *role;
    char *content;
} Message;

// Struct for tool call data
typedef struct {
    char *id;
    char *type;
    char *function_name;
    char *function_arguments;
} ToolCall;

typedef enum {
    TOOL_STATUS_UNDEFINED = 0,
    TOOL_SUCCESS,
    TOOL_ERROR,
    TOOL_PARTIAL,
    TOOL_NOT_FOUND
} ToolStatus;

// Struct for tool response data
typedef struct {
    char *tool_call_id;
    char *tool_name;
    char *tool_arguments;
    char *content;
    ToolStatus status;
} ToolResponseParams;

typedef struct {
    ToolResponseParams tool_response;
    int                loop_count;
    int                consecutive_failures;
    bool               finished;
    uint32_t           last_tool_hash;     // for repeated-tool detection
} AgentContext;

// Event model
#include "event.h"

// Struct to hold streaming state (passed via userp in curl)
typedef struct {
    char stream_buffer[BUFFER_SIZE];
    size_t stream_buf_len;
    ToolCall *tool_calls;
    int tool_calls_capacity;
    int tool_calls_size;
    int tool_call_started;  // From prior fixes
//    char content[BUFFER_SIZE];  // Accumulated assistant content (adjust size if needed)
//    size_t content_len;
    Buffer content;
    int pending_tool_calls;
    char *input;
    char *nonstream_buffer;
    size_t nonstream_buf_len;
    EventQueue queue;          // NEW: Event queue for decoupled processing
} StreamState;

typedef struct ToolHandler {
    const char *name;
    cJSON *(*create_definition)(void);
    ToolResponseParams *(*execute)(StreamState *state, const ToolCall *tool);
} ToolHandler;

void register_all_tools(void);
void register_tool(ToolHandler handler);
ToolHandler *find_tool_handler(const char *name);
void append_all_tool_definitions(cJSON *tools_array);

extern Message *chat_history;
extern int history_size;
extern int master_fd;

extern ushort debug_level;
extern bool enable_agents;
extern bool enable_stream;
extern bool enable_tools;
extern char *system_prompt;

// Function declarations

// Curl
size_t WriteCallback(void *contents, size_t size, size_t nmemb, char *userp);
size_t WriteCallbackNonStream(void *contents, size_t size, size_t nmemb, void *userp);
size_t WriteCallbackStream(void *contents, size_t size, size_t nmemb, void *userp);

// Client
int ask_inference_engine(char *user_input, ToolResponseParams **trp);
int run_multiloop_agent(AgentContext *ctx, const char *initial_user_input, int max_loops);
int run_chat_client();
int stream_from_llama_server(char *json_response);
void print_stream_advanced_markdown(const char* chunk);
int append_string(char **buf, size_t *len, const char *text);
void add_to_history(const char *role, const char *content);

// Json
char* extract_message_from_json(const char* json_response);
char* extract_message_from_json_stream(const char* json_response);
void process_json_chunk(const char *json_str, StreamState *state);
void process_json_complete(const char *json_str, StreamState *state);
void process_json_to_events(const char *json_str, StreamState *state);  // NEW: pure event emitter
bool check_json_response(const char *json_string);

// Tool calls
ToolResponseParams *create_tool_response_params(const ToolCall *tool, ToolStatus status, char *content);
ToolResponseParams *execute_tool(StreamState *state, const ToolCall *tool);
void free_tool_call(ToolCall *tool);
void free_tool_response_params(ToolResponseParams *params);
void reset_state(StreamState *state);
void print_stream_state(const StreamState *state, const char *label);

// Event system
ToolResponseParams *process_events(StreamState *state);

// Socket
int create_unix_socket_server();
void handle_socket_input(int server_sock);
void daemonize_if_needed();
void llm_printf(const char *format, ...);
int llm_putchar(int c);
int pty_printf(const char *fmt, ...);

// History
void add_to_history(const char *role, const char *content);
void prune_history();
void prune_last_n(int n);
void free_history();

// Command
int is_command_local(const char *input);
int is_command(const char *input);

// Helpers
const char* format(const char* format, ...);
size_t realloc_buffer(Buffer *buffer, char *added_content, size_t added_len);

#endif // CLIENT_H
