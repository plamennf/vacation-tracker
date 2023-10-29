#pragma once

#include <stdint.h>
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef int64_t  s64;
typedef int32_t  s32;
typedef int16_t  s16;
typedef int8_t   s8;

// Copy-paste from https://gist.github.com/andrewrk/ffb272748448174e6cdb4958dae9f3d8
// Defer macro/thing.

#define CONCAT_INTERNAL(x,y) x##y
#define CONCAT(x,y) CONCAT_INTERNAL(x,y)

template<typename T>
struct ExitScope {
    T lambda;
    ExitScope(T lambda):lambda(lambda){}
    ~ExitScope(){lambda();}
    ExitScope(const ExitScope&);
  private:
    ExitScope& operator =(const ExitScope&);
};
 
class ExitScopeHelp {
  public:
    template<typename T>
        ExitScope<T> operator+(T t){ return t;}
};
 
#define defer const auto& CONCAT(defer__, __LINE__) = ExitScopeHelp() + [&]()

#define ArrayCount(array) (sizeof(array)/sizeof((array)[0]))
#define Square(x) ((x)*(x))
#define Min(x, y) ((x) < (y) ? (x) : (y))
#define Max(x, y) ((x) > (y) ? (x) : (y))

#define Kilobytes(x) ((x)*1024ULL)
#define Megabytes(x) (Kilobytes(x)*1024ULL)
#define Gigabytes(x) (Megabytes(x)*1024ULL)
#define Terabytes(x) (Gigabytes(x)*1024ULL)

char *mprintf(char *fmt, ...);
char *mprintf_valist(char *fmt, va_list args);

char *copy_string(char *s);
bool strings_match(char *a, char *b);
bool starts_with(char *a, char *b);
char *eat_spaces(char *s);
char *eat_trailing_spaces(char *s);
char *consume_next_line(char **text_ptr);
int get_codepoint(char *text, int *bytes_processed);
int get_utf8(char *text, int utf32);

void log(char *fmt, ...);
void log_error(char *fmt, ...);

struct Memory_Arena {
    s64 max_size;
    u8 *data;
    u8 *data_at;

    void init(s64 size);
    void *get(s64 size);
};
