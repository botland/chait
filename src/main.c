#define _XOPEN_SOURCE 700

#include "client.h"
#include "multiagent/multiagent.h"

#include <fcntl.h>
#include <termios.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <signal.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <ctype.h>

int master_fd = -1;

static volatile sig_atomic_t running = 1;
static void sig_handler(int sig) { (void)sig; running = 0; }

int append_string(char **buf, size_t *len, const char *text) {
    if (!text || !*text) {
        return 1;   /* success - nothing to do */
    }

    size_t add = strlen(text);
    size_t needed = *len + add + 1;

    char *new_buf = realloc(*buf, needed);
    if (!new_buf) {
        return 0;   /* failure */
    }

    *buf = new_buf;

    memcpy(*buf + *len, text, add);
    *len += add;
    (*buf)[*len] = '\0';

    return 1;   /* success */
}

int run_chat_client() {
    // Initialize conversation history (no more flat string)
    // Load persistent input history from file (emulates bash ~/.bash_history)
    if (read_history(CHAT_HISTORY_FILE) != 0) {
        perror("read_history (optional)");
    }

    int socket_fd = create_unix_socket_server();
    if (socket_fd == -1) {
        printf("Warning: could not start unix socket server\n");
    } else {
        // Make non-blocking
        int flags = fcntl(socket_fd, F_GETFL, 0);
        fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
    }

    printf("LLaMA Chat Client (type 'quit' to exit)\n");
    printf("========================================\n");

    char *input;
    while (1) {
        printf(COLOR_LLAMA);
        printf("You: ");
        printf(COLOR_RESET);

        // Use readline for bash-style editing
        rl_already_prompted = 1;
        input = readline("You: ");
        if (input == NULL) {
            break;
        }

        // Add input to history
        add_history(input);

        // Check for commands
        int i = is_command_local(input);
        if (i > 0) {
            continue;
        } else if (i < 0) {
            break;
        }

        // Add user message to structured history
        add_to_history("user", input);

        // Save input history to file for next session
        if (write_history(CHAT_HISTORY_FILE) != 0) {
            perror("write_history");
        }

        if (is_command(input)) {
            // Execute command
            system(input);
        } else {
            if (enable_agents) {
                orchestrator_main(input);
            } else {
//                ask_inference_engine(input, NULL);  // No history param anymore
                AgentContext ctx = {0};
                run_multiloop_agent(&ctx, input, 12);
            }
        }

        prune_history();  // Prune after adding user (before request)
        free(input);
    }

    // Cleanup
    free_history();
    if (socket_fd != -1) {
        close(socket_fd);
        unlink(SOCKET_PATH);
    }
    return 0;
}

void handle_llm_prompt(const char *input) {
    // Add user message to structured history
    add_to_history("user", input);

    // Save input history to file for next session
    if (write_history(CHAT_HISTORY_FILE) != 0) {
        perror("write_history");
    }

//    llm_printf("%s\n", input);
    if (enable_agents) {
        orchestrator_main(input);
    } else {
//        ask_inference_engine(input, NULL);  // No history param anymore
        AgentContext ctx = {0};
        run_multiloop_agent(&ctx, input, 12);
    }

    prune_history();  // Prune after adding user (before request)
}

static void handle_llm_with_history(const char *typed_line) {
    /*
     * At this point:
     * - bash has already accepted the line
     * - history is already updated
     * - prompt has advanced to the next line
     *
     * We ONLY print the LLM output.
     */

    // Safety: ensure separation from prompt
//    write(master_fd, "\n", 1);
//    write(master_fd, "\n", 1);
//    write(master_fd, "\n", 1);


    // Run LLM, force all output through the PTY
    handle_llm_prompt(typed_line);

    // Ensure we end on a clean line before bash redraws prompt
//    write(master_fd, "\n", 1);
}
bool in_escape = false;
bool in_csi = false;
bool in_ansi_seq = false;
// ────────────────────────────────────────────────
// PTY raw input loop (extracted for clarity)
// ────────────────────────────────────────────────
static void handle_raw_input_loop() {
    fd_set rset;
    char buf[4096];
    char line[4096] = {0};      // accumulate printable for decision only
    size_t line_len = 0;

    while (running) {
        FD_ZERO(&rset);
        FD_SET(STDIN_FILENO, &rset);
        FD_SET(master_fd,   &rset);

        if (select(master_fd + 1, &rset, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) continue;
            perror("select");
            break;
        }

        // ───── Keyboard input ─────
        if (FD_ISSET(STDIN_FILENO, &rset)) {
            ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
            if (n <= 0) break;

            for (ssize_t i = 0; i < n; i++) {
                unsigned char c = (unsigned char)buf[i];

    // Handle escape sequence state transitions
    if (in_escape) {
        if (!in_csi && c == '[') {
            in_csi = true;
        } else if (in_csi && ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))) {
            // End of CSI sequence (e.g., A for up arrow)
            in_escape = false;
            in_csi = false;
        }
        // In escape: forward byte but skip further processing that accumulates
        write(master_fd, &c, 1);
        continue;  // Skip accumulation and other checks
    } else if (c == '\x1b') {  // ESC (27)
        in_escape = true;
        write(master_fd, &c, 1);
        continue;  // Skip accumulation
    }

// Handle escape sequence state transitions
/*if (in_escape) {
    if (!in_ansi_seq && (c == '[' || c == 'O')) {
        in_ansi_seq = true;
    } else if (in_ansi_seq && (c >= '@' && c <= '~')) {  // 0x40-0x7E covers letters, ~, etc.
        in_escape = false;
        in_ansi_seq = false;
    } else if (!in_ansi_seq && (c >= '@' && c <= '~')) {
        in_escape = false;
        in_ansi_seq = false;
    }
    // In escape: forward byte but skip further processing that accumulates
    write(master_fd, &c, 1);
    continue;  // Skip accumulation and other checks
} else if (c == '\x1b') {  // ESC (27)
    in_escape = true;
    write(master_fd, &c, 1);
    continue;  // Skip accumulation
}*/

                if (c == '\n' || c == '\r') {
                    line[line_len] = '\0';

                    if (line_len > 0) {
                        if (is_command(line)) {
                            write(master_fd, &c, 1);
                        } else {
                            
                            // LLM → erase the line from PTY (backspaces) to prevent bash processing
                            for (size_t j = 0; j < line_len; j++) {
                                write(master_fd, "\b \b", 3);
                            }

                            printf("\n\r");
//write(master_fd, "\n", 1); write(master_fd, "\r", 1);
                            if (!is_command_local(line)) {
                                handle_llm_prompt(line);
                            }

                            // Send \n to bash to reprint the prompt after LLM
                            write(master_fd, "\n", 1);
                        }
                    } else {
                        write(master_fd, &c, 1);
                    }

                    line_len = 0;
                } else if (c == '\b' || c == 0x7f) {
                    write(master_fd, &c, 1);
                    // Backspace → forwarded above; adjust buffer if printable was deleted
                    if (line_len > 0) {
                        line_len--;
                    }
                    // No manual visual — bash handles it
                } else if (c >= 32 && c <= 126) {
                    write(master_fd, &c, 1);
                    // Printable → forwarded above; accumulate for decision
                    if (line_len < sizeof(line)-1) {
                        line[line_len++] = c;
                    }
                    // No manual echo — bash handles it
                } else {
                    write(master_fd, &c, 1);
                }
            }
        }

        // ───── Output from bash ─────
        if (FD_ISSET(master_fd, &rset)) {
            ssize_t n = read(master_fd, buf, sizeof(buf));
            if (n > 0) {
                write(STDOUT_FILENO, buf, n);
            } else {
                perror("read pty");
                break;
            }
        }
    }
}
void winch_handler(int sig) {
    struct winsize ws;
    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == 0) {
        ioctl(master_fd, TIOCSWINSZ, &ws);
    }
}
// ────────────────────────────────────────────────
// Main entry point
// ────────────────────────────────────────────────
int main(void) {
#ifdef RUN_CHAT_CLIENT
    return run_chat_client();
#else
    char slave_name[64] = {0};
    struct winsize ws;
    struct termios orig_term, raw_term;

    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) != 0) {
        ws.ws_row = 24; ws.ws_col = 80; ws.ws_xpixel = ws.ws_ypixel = 0;
    }

    master_fd = posix_openpt(O_RDWR | O_NOCTTY);
    if (master_fd == -1) { perror("posix_openpt"); return 1; }

    if (grantpt(master_fd) == -1 && errno != EINVAL) {
        perror("grantpt"); close(master_fd); return 1;
    }

    if (unlockpt(master_fd) == -1) {
        perror("unlockpt"); close(master_fd); return 1;
    }

    const char *slave_ptr = ptsname(master_fd);
    if (!slave_ptr) { perror("ptsname"); close(master_fd); return 1; }
    strncpy(slave_name, slave_ptr, sizeof(slave_name)-1);

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); close(master_fd); return 1; }

    if (pid == 0) {   // child — bash
        close(master_fd);
        setsid();

        int slave_fd = open(slave_name, O_RDWR | O_NOCTTY);
        if (slave_fd < 0) { perror("open slave"); _exit(1); }

        ioctl(slave_fd, TIOCSWINSZ, &ws);
        ioctl(master_fd, TIOCSWINSZ, &ws);

        dup2(slave_fd, 0);
        dup2(slave_fd, 1);
        dup2(slave_fd, 2);
        if (slave_fd > 2) close(slave_fd);

        setenv("PS1", "\\[\\e[1;34m\\]PTY-SHELL \\w \\$ \\[\\e[0m\\] ", 1);
        execlp("/bin/bash", "bash", "--norc", (char *)NULL);

        perror("execlp bash"); _exit(127);
    }

    // parent
    tcgetattr(STDIN_FILENO, &orig_term);
    raw_term = orig_term;
    cfmakeraw(&raw_term);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_term);

    signal(SIGINT,  sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGHUP,  sig_handler);
    signal(SIGWINCH, winch_handler);

    handle_raw_input_loop();

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_term);

    if (pid > 0) {
        kill(pid, SIGHUP);
        waitpid(pid, NULL, 0);
    }

    if (master_fd >= 0) close(master_fd);
    return 0;
#endif
}
