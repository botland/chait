#include "client.h"

#include <pwd.h>     // For getpwnam, getpwuid

int get_the_string_after_first_space(const char *input, char **output) {
    if (!input || !output) return 0;

    const char *space = strchr(input, ' ');
    if (!space || !*(space + 1)) {
        free(*output);
        *output = NULL;
        return 0;
    }

    const char *text = space + 1;
    size_t len = strlen(text);

    char *new_mem = realloc(*output, len + 1);
    if (!new_mem) return 0;

    *output = new_mem;
    strcpy(*output, text);
    return 1;
}

int get_the_int_after_first_space(const char *input) {
    if (!input) return 0;

    const char *space = strchr(input, ' ');
    if (!space || !*(space + 1))
        return 0;
    return atoi(space + 1);
}

int is_command_local(const char *input) {
    if (!strcmp(input, "help") || !strcmp(input, "h")) {
       printf("h or help          : help\n");
       printf("dl <level>         : debug level\n");
       printf("rh                 : reset history\n");
       printf("sp <system_prompt> : set system prompt\n");
       printf("ta                 : toggle agents\n");
       printf("ts                 : toggle stream mode\n");
       printf("tt                 : toggle tools\n");
       printf("q or quit          : quit\n");
       return 1;
    } else if (!strncmp(input, "dl ", 3)) {
        debug_level = get_the_int_after_first_space(input);
        printf("Debug level set to %d\n", debug_level);
        return 1;
    } else if (!strcmp(input, "rh")) {
        free_history();
        printf("Conversation history reset.\n");
        return 1;
    } else if (!strcmp(input, "sp")) {
        printf("System prompt is: %s\n", system_prompt);
        return 1;
    } else if (!strncmp(input, "sp ", 3)) {
        get_the_string_after_first_space(input, &system_prompt);
        printf("System prompt is: %s\n", system_prompt);
        return 1;
    } else if (!strcmp(input, "ta")) {
        enable_agents = !enable_agents;
        printf("Agents are %s\n", enable_agents ? "enabled" : "disabled");
        return 1;
    } else if (!strcmp(input, "ts")) {
        enable_stream = !enable_stream;
        printf("Stream is %s\n", enable_stream ? "enabled" : "disabled");
        return 1;
    } else if (!strcmp(input, "tt")) {
        enable_tools = !enable_tools;
        printf("Tools are %s\n", enable_tools ? "enabled" : "disabled");
        return 1;
    } else if (!strcmp(input, "quit") || !strcmp(input, "q")) {
        return -1;
    }
    return 0;
}

int is_command2(const char *s) {
    if (!s || !*s) return 0;

    // Extract first word
    char cmd[256];
    int i = 0;
    while (i < sizeof(cmd)-1 && s[i] && !isspace((unsigned char)s[i])) {
        cmd[i] = s[i];
        i++;
    }
    cmd[i] = '\0';

    if (i == 0) return 0;

    // Skip if it looks obviously like natural language
    if (strchr(cmd, '?') || !isalnum(cmd[0])) return 0;

    // Prepare command: command -v "firstword" >/dev/null 2>&1
    char check[512];
    snprintf(check, sizeof(check),
             "command -v %s >/dev/null 2>&1 || which %s >/dev/null 2>&1",
             cmd, cmd);

    FILE *fp = popen(check, "r");
    if (!fp) return 0;  // failed → assume not command

    // If command -v / which succeeds → exit code 0 → is command
    int status = pclose(fp);
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

int is_command(const char *s) {
    if (!s || !*s) return 0;

    // Extract first word
    char cmd[256];
    int i = 0;
    while (i < sizeof(cmd)-1 && s[i] && !isspace((unsigned char)s[i])) {
        cmd[i] = s[i];
        i++;
    }
    cmd[i] = '\0';

    if (i == 0) return 0;

    // Skip if it looks obviously like natural language (e.g., contains '?')
    if (strchr(cmd, '?')) return 0;

    // Allow commands starting with alnum, '_', '.', '/', '~'
    char c = cmd[0];
    if (!(isalnum((unsigned char)c) || c == '_' || c == '.' || c == '/' || c == '~')) return 0;

    // Check if it's a path-like (contains '/' or starts with '~')
    if (strchr(cmd, '/') || cmd[0] == '~') {
        // Expand tilde if present
        char expanded[512];
        strcpy(expanded, cmd);
        if (cmd[0] == '~') {
            const char *slash = strchr(cmd + 1, '/');
            char user[256] = {0};
            if (slash) {
                strncpy(user, cmd + 1, slash - (cmd + 1));
            }
            struct passwd *pw;
            if (*user == '\0') {
                pw = getpwuid(getuid());
            } else {
                pw = getpwnam(user);
            }
            if (pw) {
                snprintf(expanded, sizeof(expanded), "%s%s", pw->pw_dir, slash ? slash : "");
            } else {
                return 0;  // Can't expand tilde → assume not command
            }
        }

        // Check if it's an executable file (not directory)
        struct stat st;
        if (stat(expanded, &st) != 0) return 0;
        if (S_ISDIR(st.st_mode)) return 0;
        if (access(expanded, X_OK) != 0) return 0;
        return 1;
    } else {
        // Non-path: Check for builtin/alias/function/executable in PATH using shell
        char *shell = getenv("SHELL");
        if (!shell || !*shell) shell = "/bin/sh";

        char *shell_base = basename(shell);

        // Prepare shell-specific check command
        char check_cmd[512];
        // Escape cmd for shell safety (replace ' with '\'') 
        char escaped[512];
        int k = 0;
        escaped[k++] = '\'';
        for (int m = 0; cmd[m]; m++) {
            if (cmd[m] == '\'') {
                escaped[k++] = '\'';
                escaped[k++] = '\\';
                escaped[k++] = '\'';
                escaped[k++] = '\'';
            } else {
                escaped[k++] = cmd[m];
            }
        }
        escaped[k++] = '\'';
        escaped[k] = '\0';

        if (strcmp(shell_base, "fish") == 0) {
            snprintf(check_cmd, sizeof(check_cmd), "type -q %s >/dev/null 2>&1", escaped);
        } else if (strcmp(shell_base, "csh") == 0 || strcmp(shell_base, "tcsh") == 0) {
            snprintf(check_cmd, sizeof(check_cmd), "which %s >/dev/null 2>&1", escaped);
        } else {
            // Default for bash, zsh, sh, ksh, etc.
            snprintf(check_cmd, sizeof(check_cmd), "command -v %s >/dev/null 2>&1", escaped);
        }

        // Prepare full command: $SHELL -ic "check_cmd"
        char full_cmd[1024];
        snprintf(full_cmd, sizeof(full_cmd), "%s -ic \"%s\"", shell, check_cmd);

        FILE *fp = popen(full_cmd, "r");
        if (!fp) return 0;  // failed → assume not command

        // If succeeds → exit code 0 → is command
        int status = pclose(fp);
        return WIFEXITED(status) && WEXITSTATUS(status) == 0;
    }
}
