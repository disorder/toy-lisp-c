#ifdef EXPLAIN
#include <libexplain/vsnprintf.h>
#include <errno.h>
#endif

#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include "str.h"

void *crealloc(void *old, size_t old_size, size_t new_size)
{
    void *new = realloc(old, new_size);
    if (new == NULL) {
        raise("could not reallocate %zu", new_size);
        return NULL;
    }

    // zero
    if (new_size > old_size) {
        size_t fill = new_size - old_size;
        void *start = (void *) ((size_t) new + old_size);
        memset(start, 0, fill);
    }

    return new;
}

str_t *new_str(size_t size, u32 grow)
{
    // beware overflow
    ++size;
    NEW(str, s);
    ALLOC(char, size, data);

    if (data == NULL) {
        FREE(data);
        raise("new string: could not allocate %zu", size);
        return NULL;
    }

    s->data = data;
    s->size = size - 1;
    // s->used = 0;
    s->grow = (grow != 0)? grow : 32;

    CHECK_PTR(s);
    return s;
}

void str_grow(str_t *s, size_t grow)
{
    assert(s != NULL);
    size_t old_size = s->size + 1;
    if (grow == 0)
        // enforce grow size this one time
        grow = s->grow;
    size_t new_size = old_size + grow;
    char *data = (char *) crealloc((void *) s->data, old_size, new_size);
    s->data = data;
    //s->size += s->grow;
    s->size = new_size - 1;
}

void str_append(str_t *s, char c)
{
    assert(s->used <= s->size);

    if (s->used == s->size)
        str_grow(s, 0);

    s->data[s->used++] = c;
}

#define str_printf(...) str_sprintf(NULL, __VA_ARGS__)
// maybe use asprintf?
str_t *str_sprintf(str_t *s, const char *fmt, ...)
{
    va_list ap;

    if (s == NULL)
        s = new_str(32, 0);

    while (1) {
        // vsnprintf accounts for \0
        size_t size = s->size - s->used + 1;

        va_start(ap, fmt);
        void *data = &s->data[s->used];
        int n = vsnprintf(data, size, fmt, ap);

        #ifdef EXPLAIN
        //int n = explain_vsnprintf_or_die(&s->data[s->used], size, fmt, ap);
        if (n < 0) {
            int err = errno;
            char msg[3000];
            explain_message_errno_vsnprintf(msg, sizeof(msg), err, data,
                                            size, fmt, ap);
            fprintf(stderr, "%s\n", msg);
        }
        #endif

        va_end(ap);

        if (n < 0)
            // free str_t
            //return n;
            raise("vsnprintf error %d", n);
        else if (n < size) {
            s->used += n;
            break;
        }

        str_grow(s, n);
        //str_grow(s, n + (s->grow - (n % s->grow)));
    }

    return s;
}

void str_append_c(str_t *s, const char *c, size_t len)
{
    if (len == 0)
        len = strlen(c);

    assert(s->used <= s->size);

    if (s->used == s->size) {
        size_t old_size = s->size + 1;
        size_t new_size = s->used + 1 + len + s->grow;
        char *data = (char *) crealloc((void *) s->data, old_size, new_size);
        s->data = data;
        //s->size += len + s->grow;
        s->size = new_size - 1;
    }
    size_t space = s->size - s->used;
    if (space < len)
        str_grow(s, len-space);
        //n = len-space;
        //str_grow(s, n + (s->grow - (n % s->grow)));

    memcpy((void *) &s->data[s->used], (void *) c, len + 1);
}

str_t *read_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    // rewind
    fseek(f, 0, SEEK_SET);

    // new_str adds \0
    str_t *s = new_str(size, 0);
    fread((void *) s->data, size, 1, f);
    fclose(f);

    s->used = size;

    CHECK_PTR(s);
    return s;
}

void test_str()
{
    puts("str_t");
    str_t *s = new_str(10, 1);
    for (int i=0; i< 400; i++)
        str_append(s, (char) '#');
    puts(s->data);
    str_t *sbig = new_str(10240, 1024);
    puts(sbig->data);

    str_t *code = read_file("test.lisp");
    printf("used %zu\n", code->used);
    //puts(code->data);
}
