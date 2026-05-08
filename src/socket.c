#include "client.h"

/*#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#define SOCKET_PATH "/tmp/llm.sock"
#define SOCKET_BACKLOG 5*/

#include <stdarg.h>

// Returns socket fd on success, -1 on failure
int create_unix_socket_server() {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        return -1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    unlink(SOCKET_PATH);  // remove old socket if exists

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(sock);
        return -1;
    }

    if (listen(sock, SOCKET_BACKLOG) == -1) {
        perror("listen");
        close(sock);
        return -1;
    }

    printf("Unix socket server listening on %s\n", SOCKET_PATH);
    return sock;
}

int client = -1;

// Call this inside your while(1) loop, e.g. after readline()
void handle_socket_input(int server_sock) {
    struct sockaddr_un client_addr;
    socklen_t len = sizeof(client_addr);

    // Non-blocking accept
    client = accept(server_sock, (struct sockaddr *)&client_addr, &len);
    if (client == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("accept");
        }
        return;
    }

    // Read prompt from client
    char buf[8192] = {0};
    ssize_t n = read(client, buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        printf("\nReceived external prompt: %s\n", buf);

        // Process it exactly like user input
        // Reuse your existing logic (trim, check commands, send to LLM, etc.)
        char *prompt = strdup(buf);  // or process directly
        // ... call your send_to_llm(prompt) or whatever handles user input ...

        add_to_history("user", prompt);

        ask_inference_engine(prompt, NULL);

        // Send reply back (example)
//        const char *reply = "Answer from LLM: ...";
//        write(client, reply, strlen(reply));
    }

    close(client);
}

// Call this early in main()
void daemonize_if_needed() {
    // If stdin is not a terminal → probably backgrounded
    if (!isatty(STDIN_FILENO)) {
        // Fork once
        pid_t pid = fork();
        if (pid < 0) exit(1);
        if (pid > 0) exit(0);  // parent exits

        // Create new session
        if (setsid() < 0) exit(1);

        // Second fork to avoid session leader issues
        pid = fork();
        if (pid < 0) exit(1);
        if (pid > 0) exit(0);

        // Change working dir (optional)
        chdir("/");

        // Close all fds
        for (int fd = 0; fd <= 2; fd++) close(fd);

        // Redirect to /dev/null
        open("/dev/null", O_RDWR);  // stdin
        dup(0);                     // stdout
        dup(0);                     // stderr

        // Optional: ignore SIGHUP
        signal(SIGHUP, SIG_IGN);

        printf("Daemonized (PID %d)\n", getpid());  // goes nowhere unless log
    }
}

/*int llm_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);

    char buf[8192];
    int len = vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    if (len < 0) return -1;

    if (client != -1) {
        // Write to socket client
        ssize_t written = write(client, buf, len);
        if (written != len) {
            // Optional: log error
            perror("write to socket");
        }
        // Optional: add newline if needed
        if (len > 0 && buf[len-1] != '\n') {
            write(client, "\n", 1);
        }
        return len;
    } else {
        // Normal console mode: stdout
        return vprintf(format, args);  // or just printf, but vprintf matches
    }
}

int llm_putchar(int c) {
    unsigned char ch = (unsigned char)c;

    if (client != -1) {
        // Write single byte to socket
        ssize_t w = write(client, &ch, 1);
        if (w != 1) {
            // Optional: fallback or error
            return EOF;
        }
        return c;
    } else {
        // Normal console mode
        return putchar(c);
    }
}*/

int llm_printf2(const char *format, ...) {
    va_list args;
    va_start(args, format);

    char buf[8192];
    int len = vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    if (len < 0) return -1;

    // Remove or replace \0 in buffer (socket-safe)
    for (int i = 0; i < len; i++) {
        if (buf[i] == '\0') buf[i] = ' ';  // or '.' or skip
    }

    if (client != -1) {
        ssize_t written = write(client, buf, len);
        if (written != len) perror("write");
        if (len > 0 && buf[len-1] != '\n') write(client, "\n", 1);
        return len;
    } else {
        return vprintf(format, args);
    }
}

void llm_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    // Format the string like printf
    char buffer[8192];  // large enough for most responses
    vsnprintf(buffer, sizeof(buffer), fmt, args);

    va_end(args);

    // Ensure it starts on a new line at column 1
    write(master_fd, "\r\n", 2);

    // Write the formatted buffer
    write(master_fd, buffer, strlen(buffer));

    // Optional: end with newline for clean separation (remove if not needed)
    write(master_fd, "\n", 1);
}

int llm_putchar(int c) {
    unsigned char ch = (unsigned char)c;

    // Skip null bytes – they break string handling on receiver side
    if (ch == 0) return c;  // or replace with ' ' or log it

    if (client != -1) {
        ssize_t w = write(client, &ch, 1);
        return (w == 1) ? c : EOF;
    }
    return putchar(c);
}


#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>

int pty_printf2(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char buf[1024];  // Original formatting buffer
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (len < 0 || len >= sizeof(buf)) {
        // Handle formatting error or truncation
        return -1;
    }

    char newbuf[2048];  // Expanded buffer for \r insertions (double size to be safe)
    int j = 0;
    for (int i = 0; i < len; i++) {
        if (buf[i] == '\n') {
            if (j >= sizeof(newbuf) - 1) {  // Check for overflow
                return -1;
            }
            newbuf[j++] = '\r';
        }
        if (j >= sizeof(newbuf)) {  // Check for overflow
            return -1;
        }
        newbuf[j++] = buf[i];
    }

    // Write the modified buffer to the PTY fd
    return write(master_fd, newbuf, j);
}

int pty_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char buf[1024];  // Original formatting buffer
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (len < 0 || len >= sizeof(buf)) {
        // Handle formatting error or truncation
        return -1;
    }

    char newbuf[2048];  // Expanded buffer for \r insertions (double size to be safe)
    int j = 0;
    for (int i = 0; i < len; i++) {
        if (buf[i] == '\n') {
            if (j >= sizeof(newbuf) - 1) {  // Check for overflow
                return -1;
            }
            newbuf[j++] = '\r';
        }
        if (j >= sizeof(newbuf)) {  // Check for overflow
            return -1;
        }
        newbuf[j++] = buf[i];
    }

    // Write the modified buffer to stdout
    return fwrite(newbuf, 1, j, stdout);
}


