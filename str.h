#ifndef __STR_H
#define __STR_H

#include "common.h"
#include "object.h"

typedef struct
{
    instance_t obj;
    u32 grow;
    size_t used;
    // allocated size+1
    size_t size;
    char *data;
} str_t;

void *crealloc(void *old, size_t old_size, size_t new_size);

#define FREE(ptr) ((void *) ptr == NULL || (free(ptr), 0))
#define ALLOC(TYPE, SIZE, NAME) TYPE *NAME = (TYPE *) calloc(1, SIZE)
str_t *new_str(size_t size, u32 grow);
void str_append(str_t *s, char c);
void str_grow(str_t *s, size_t grow);

#define str_printf(...) str_sprintf(NULL, __VA_ARGS__)
str_t *str_sprintf(str_t *s, const char *fmt, ...);

str_t *read_file(const char *path);

#endif
