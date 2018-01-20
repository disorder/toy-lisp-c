#ifndef __TOKEN_H
#define __TOKEN_H

#include "common.h"
#include "object.h"
#include "str.h"

typedef struct
{
    // this has to be an exact copy
    str_t str;
} stoken_t;

// immutable from str_t
typedef struct
{
    instance_t obj;
    // allocated size+1 for data
    size_t size;
    // data follows, this is for \0 and easy access to pointer
    char data;
} token_t;


// this doesn't work for str with \0 but we also don't support \0 in symbols
#define new_token(str) new_token_c(str->data, 0)
//token_t *new_token(str_t *s);
token_t *new_token_c(const char *s, size_t size);
stoken_t *new_stoken(size_t size, u32 grow);
str_t *new_str_c(const char *s, size_t len);
#define stoken_append(st, c) str_append((str_t *) &st->str, c)
#define TOKEN_DATA(t) ((char *) &((t)->data))
#define STOKEN_DATA(st) ((st)->str.data)

#endif
