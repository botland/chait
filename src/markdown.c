#include "client.h"

/*#include <stdio.h>
#include <string.h>
#include <stdbool.h>*/

typedef enum {
    MODE_NORMAL,
    MODE_CODE_BLOCK,
    MODE_INLINE_CODE,
    MODE_BOLD,
    MODE_ITALIC,
    MODE_HEADER,
    MODE_QUOTE
} ParseMode;

static ParseMode current_mode = MODE_NORMAL;
static int code_fence_level = 0;
static int header_level = 0;
static bool just_entered_mode = false;

// Colors (256-color / truecolor friendly)
static const char* C_RESET       = "\x1b[0m";
static const char* C_TEXT        = "\x1b[38;5;252m";    // light gray
static const char* C_ACCENT      = "\x1b[38;5;153m";    // soft blue/lavender
static const char* C_CODE        = "\x1b[38;5;114m";    // soft green
static const char* C_CODE_BG     = "\x1b[48;5;236m";    // very dark gray bg
static const char* C_CODE_FENCE  = "\x1b[38;5;242m";    // dim gray
static const char* C_BOLD        = "\x1b[1;38;5;223m";  // soft orange-bold
static const char* C_ITALIC      = "\x1b[3;38;5;189m";  // light purple italic
static const char* C_HEADER1     = "\x1b[1;38;5;204m";  // bright red-pink
static const char* C_HEADER2     = "\x1b[1;38;5;213m";  // magenta
static const char* C_QUOTE       = "\x1b[38;5;109m";    // teal-ish
static const char* C_LINK_TEXT   = "\x1b[38;5;81m";     // bright cyan
static const char* C_LINK_URL    = "\x1b[38;5;240m";    // dim gray

#include <wchar.h>
#include <stdio.h>

void safe_put_utf8_char(const char **pp) {
    const unsigned char *p = (const unsigned char *)*pp;
    
    if (*p < 0x80) {
        putchar(*p);
        (*pp)++;
    }
    else if ((*p & 0xe0) == 0xc0) {        // 2 bytes
        printf("%c%c", p[0], p[1]);
        *pp += 2;
    }
    else if ((*p & 0xf0) == 0xe0) {        // 3 bytes (most accented chars)
        printf("%c%c%c", p[0], p[1], p[2]);
        *pp += 3;
    }
    else if ((*p & 0xf8) == 0xf0) {        // 4 bytes
        printf("%c%c%c%c", p[0], p[1], p[2], p[3]);
        *pp += 4;
    }
    else {
        // fallback - invalid sequence
        putchar('?');
        (*pp)++;
    }
}

void reset_all_modes() {
    current_mode = MODE_NORMAL;
    code_fence_level = 0;
    header_level = 0;
    just_entered_mode = false;
}

void print_stream_advanced_markdown(const char* chunk) {
    const char* p = chunk;

    while (*p) {
        just_entered_mode = false;

        // ─── Code fence ─────────────────────────────────────────────────────
        if (strncmp(p, "```", 3) == 0) {
            int fence_len = 3;
            while (p[fence_len] == '`') fence_len++;

            if (current_mode == MODE_CODE_BLOCK && fence_len >= code_fence_level) {
                // close code block
                printf("%s%s```", C_CODE_FENCE, C_RESET);
                p += fence_len;
                reset_all_modes();
                continue;
            }
            else if (current_mode == MODE_NORMAL) {
                // open code block
                printf("%s", C_CODE_FENCE);
                printf("%.*s", fence_len, p);
                printf("%s%s", C_CODE, C_CODE_BG);
                p += fence_len;
                current_mode = MODE_CODE_BLOCK;
                code_fence_level = fence_len;
                just_entered_mode = true;
                continue;
            }
        }

        // ─── Inline code `code` ─────────────────────────────────────────────
        if (*p == '`' && current_mode != MODE_CODE_BLOCK) {
            if (current_mode == MODE_INLINE_CODE) {
                printf("%s`", C_RESET);
                current_mode = MODE_NORMAL;
            } else {
                printf("%s`", C_CODE);
                current_mode = MODE_INLINE_CODE;
            }
            p++;
            continue;
        }

        // ─── Bold ** **  or __ __ ───────────────────────────────────────────
        if ((strncmp(p, "**", 2) == 0 || strncmp(p, "__", 2) == 0) &&
            current_mode != MODE_CODE_BLOCK && current_mode != MODE_INLINE_CODE) {
            if (current_mode == MODE_BOLD) {
                printf("%s**", C_RESET);
                current_mode = MODE_NORMAL;
            } else {
                printf("%s**", C_BOLD);
                current_mode = MODE_BOLD;
            }
            p += 2;
            continue;
        }

        // ─── Italic * *  or _ _  (simple version, no *** handling) ──────────
        if ((*p == '*' || *p == '_') && *(p+1) != '*' && *(p+1) != '_' &&
            current_mode != MODE_CODE_BLOCK && current_mode != MODE_INLINE_CODE) {
            if (current_mode == MODE_ITALIC) {
                printf("%s%c", C_RESET, *p);
                current_mode = MODE_NORMAL;
            } else {
                printf("%s%c", C_ITALIC, *p);
                current_mode = MODE_ITALIC;
            }
            p++;
            continue;
        }

        // ─── Headers # ## ### ───────────────────────────────────────────────
        if (*p == '#' && (p == chunk || *(p-1) == '\n')) {
            header_level = 0;
            const char* q = p;
            while (*q == '#') { header_level++; q++; }
            if (*q == ' ') {
                const char* color;
                if (header_level == 1) color = C_HEADER1;
                else if (header_level == 2) color = C_HEADER2;
                else color = C_BOLD;

                printf("\n%s", color);
                printf("%.*s ", header_level, p);
                p = q + 1;
                current_mode = MODE_HEADER;
                just_entered_mode = true;
                continue;
            }
        }

        // ─── Blockquote > ──────────────────────────────────────────────────
        if (*p == '>' && (p == chunk || *(p-1) == '\n')) {
            if (current_mode != MODE_QUOTE) {
                printf("\n%s> ", C_QUOTE);
                current_mode = MODE_QUOTE;
                p++;
                continue;
            }
        }

        // ─── Links [text](url) ──────────────────────────────────────────────
        // Very naive version — only colors, doesn't hide url
        if (*p == '[' && current_mode == MODE_NORMAL) {
            printf("%s[", C_LINK_TEXT);
            p++;
            while (*p && *p != ']') {
//                putchar(*p);
//                p++;
                safe_put_utf8_char(&p);
            }
            if (*p == ']') {
                printf("%s]", C_RESET);
                p++;
                if (*p == '(') {
                    printf("%s(", C_LINK_URL);
                    p++;
                    while (*p && *p != ')') {
//                        putchar(*p);
//                        p++;
                        safe_put_utf8_char(&p);
                    }
                    if (*p == ')') {
                        printf("%s)", C_RESET);
                        p++;
                    }
                }
            }
            continue;
        }

        // Normal text + current style
        const char* color = C_TEXT;

        if (current_mode == MODE_CODE_BLOCK)        color = C_CODE;
        else if (current_mode == MODE_INLINE_CODE)  color = C_CODE;
        else if (current_mode == MODE_BOLD)         color = C_BOLD;
        else if (current_mode == MODE_ITALIC)       color = C_ITALIC;
        else if (current_mode == MODE_HEADER)       color = (header_level==1) ? C_HEADER1 : (header_level==2) ? C_HEADER2 : C_BOLD;
        else if (current_mode == MODE_QUOTE)        color = C_QUOTE;
        else                                        color = C_ACCENT;

        printf("%s", color);
//        putchar(*p);
//        p++;
        safe_put_utf8_char(&p);

        // Reset header mode after first line
        if (current_mode == MODE_HEADER && *p == '\n') {
            current_mode = MODE_NORMAL;
        }
    }

    fflush(stdout);
}
