#include "pch.h"

#include <stdio.h>
#include <string.h> // For strlen
#include <ctype.h>

char *mprintf(char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    size_t n = 1 + vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    char *str = new char[n];
    va_start(args, fmt);
    vsnprintf(str, n, fmt, args);
    va_end(args);
    return str;
}

char *mprintf_valist(char *fmt, va_list args) {
    va_list ap = args;
    size_t n = 1 + vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    char *str = new char[n];
    vsnprintf(str, n, fmt, args);
    va_end(args);
    return str;
}

char *copy_string(char *s) {
    if (!s) return NULL;
    
    auto len = strlen(s);
    
    char *result = new char[len + 1];
    memcpy(result, s, len + 1);
    return result;
}

bool strings_match(char *a, char *b) {
    if (a == b) return true;
    if (!a || !b) return false;

    while (*a && *b) {
        if (*a != *b) {
            return false;
        }

        a++;
        b++;
    }

    return *a == 0 && *b == 0;
}

bool starts_with(char *a, char *b) {
    if (a == b) return true;
    if (!a || !b) return false;

    auto a_len = strlen(a);
    auto b_len = strlen(b);
    if (a_len < b_len) return false;
    for (int i = 0; i < b_len; i++) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

char *eat_spaces(char *s) {
    if (!s) return NULL;

    while (isspace(*s)) {
        s++;
    }
    return s;
}

char *eat_trailing_spaces(char *s) {
    if (!s) return NULL;

    char *end = s + strlen(s) - 1;
    while (end > s && isspace(*end)) end--;

    end[1] = 0;

    return s;
}

char *consume_next_line(char **text_ptr) {
    char *t = *text_ptr;
    if (!*t) return NULL;

    char *s = t;

    while (*t && (*t != '\n') && (*t != '\r')) t++;

    char *end = t;
    if (*t) {
        end++;

        if (*t == '\r') {
            if (*end == '\n') ++end;
        }

        *t = '\0';
    }
    
    *text_ptr = end;
    
    return s;
}

void log(char *fmt, ...) {
    char buf[4096];

    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    printf("%s", buf);
}

void log_error(char *fmt, ...) {
    char buf[4096];

    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    
    char *prefix = "\033[1;31m";
    char *postfix = "\033[0m";
    printf("%s%s%s", prefix, buf, postfix);
}

// Copy-paste from https://github.com/raysan5/raylib/blob/master/src/rtext.c
int get_codepoint(char *text, int *bytes_processed) {
    int code = 0x3f;
    int octet = (u8)(text[0]);
    *bytes_processed = 1;

    if (octet <= 0x7f) {
        // Only one octet (ASCII range x00-7F)
        code = text[0];
    } else if ((octet & 0xe0) == 0xc0) {
        // Two octets

        // [0]xC2-DF     [1]UTF8-tail(x80-BF)
        u8 octet1 = text[1];
        if ((octet1 == '\0') || ((octet1 >> 6) != 2)) { *bytes_processed = 2; return code; } // Unexpected sequence

        if ((octet >= 0xc2) && (octet <= 0xdf)) {
            code = ((octet & 0x1f) << 6) | (octet1 & 0x3f);
            *bytes_processed = 2;
        }
    } else if ((octet & 0xf0) == 0xe0) {
        u8 octet1 = text[1];
        u8 octet2 = '\0';

        if ((octet1 == '\0') || ((octet1 >> 6) != 2)) { *bytes_processed = 2; return code; } // Unexpected sequence

        octet2 = text[2];

        if ((octet2 == '\0') || ((octet2 >> 6) != 2)) { *bytes_processed = 3; return code; } // Unexpected sequence

        // [0]xE0    [1]xA0-BF       [2]UTF8-tail(x80-BF)
        // [0]xE1-EC [1]UTF8-tail    [2]UTF8-tail(x80-BF)
        // [0]xED    [1]x80-9F       [2]UTF8-tail(x80-BF)
        // [0]xEE-EF [1]UTF8-tail    [2]UTF8-tail(x80-BF)

        if (((octet == 0xe0) && !((octet1 >= 0xa0) && (octet1 <= 0xbf))) ||
            ((octet == 0xed) && !((octet1 >= 0x80) && (octet1 <= 0x9f)))) {
            *bytes_processed = 2;
            return code;
        }

        if ((octet >= 0xe0) && (0 <= 0xef)) {
            code = ((octet & 0xf) << 12) | ((octet1 & 0x3f) << 6) | (octet2 & 0x3f);
            *bytes_processed = 3;
        }
    } else if ((octet & 0xf8) == 0xf0) {
        // Four octets
        if (octet > 0xf4) return code;

        u8 octet1 = text[1];
        u8 octet2 = '\0';
        u8 octet3 = '\0';

        if ((octet1 == '\0') || ((octet1 >> 6) != 2)) { *bytes_processed = 2; return code; } // Unexpected sequence

        octet2 = text[2];

        if ((octet2 == '\0') || ((octet2 >> 6) != 2)) { *bytes_processed = 3; return code; } // Unexpected sequence

        octet3 = text[3];

        if ((octet3 == '\0') || ((octet3 >> 6) != 2)) { *bytes_processed = 4; return code; }  // Unexpected sequence

        // [0]xF0       [1]x90-BF       [2]UTF8-tail  [3]UTF8-tail
        // [0]xF1-F3    [1]UTF8-tail    [2]UTF8-tail  [3]UTF8-tail
        // [0]xF4       [1]x80-8F       [2]UTF8-tail  [3]UTF8-tail

        if (((octet == 0xf0) && !((octet1 >= 0x90) && (octet1 <= 0xbf))) ||
            ((octet == 0xf4) && !((octet1 >= 0x80) && (octet1 <= 0x8f)))) {
            *bytes_processed = 2; return code; // Unexpected sequence
        }

        if (octet >= 0xf0) {
            code = ((octet & 0x7) << 18) | ((octet1 & 0x3f) << 12) | ((octet2 & 0x3f) << 6) | (octet3 & 0x3f);
            *bytes_processed = 4;
        }
    }

    if (code > 0x10ffff) code = 0x3f; // Codepoints after U+10ffff are invalid

    return code;
}

int get_utf8(char *text, int utf32) {
    if (utf32 <= 0x7F) {
        text[0] = utf32;
        return 1;
    }
    if (utf32 <= 0x7FF) {
        text[0] = 0xC0 | (utf32 >> 6);            /* 110xxxxx */
        text[1] = 0x80 | (utf32 & 0x3F);          /* 10xxxxxx */
        return 2;
    }
    if (utf32 <= 0xFFFF) {
        text[0] = 0xE0 | (utf32 >> 12);           /* 1110xxxx */
        text[1] = 0x80 | ((utf32 >> 6) & 0x3F);   /* 10xxxxxx */
        text[2] = 0x80 | (utf32 & 0x3F);          /* 10xxxxxx */
        return 3;
    }
    if (utf32 <= 0x10FFFF) {
        text[0] = 0xF0 | (utf32 >> 18);           /* 11110xxx */
        text[1] = 0x80 | ((utf32 >> 12) & 0x3F);  /* 10xxxxxx */
        text[2] = 0x80 | ((utf32 >> 6) & 0x3F);   /* 10xxxxxx */
        text[3] = 0x80 | (utf32 & 0x3F);          /* 10xxxxxx */
        return 4;
    }
    return 0;
}

void Memory_Arena::init(s64 size) {
    max_size = size;
    data = new u8[size];
    data_at = data;
}

void *Memory_Arena::get(s64 size) {
    assert(data_at + size <= data + max_size);

    void *result = (void *)data_at;
    data_at += size;
    return result;
}
