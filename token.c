#include <string.h>
#include "str.h"
#include "token.h"

stoken_t *new_stoken(size_t size, u32 grow)
{
    str_t *s = new_str(size, grow);
    stoken_t *st = (stoken_t *) s;
    st->str.obj.type = stoken_type;

    CHECK_PTR(s);
    return s;
}

/*
token_t *new_token_c(const char *s, size_t size)
{
    size_t len = (size>0)? size : strlen(s);
    str_t str = {
        .obj = {
            .id = 0,
            .type = str_type,
        },
        .used = len,
        .size = len,
        // this doesn't allow us to declare const s
        .data = s,
    };

    return new_token(&str);
}
*/

token_t *new_token_c(const char *s, size_t len)
{
    if (len == 0)
        len = strlen(s) + 1;
    // \0 is in u8 data field, no need to add +1
    size_t size = sizeof(token_t) + len;
    ALLOC(token_t, size, t);
    if (t == NULL) {
        FREE(t);
        raise("new token: could not allocate %zu", size);
        return NULL;
    }

    t->obj.type = token_type;
    t->obj.id = new_instance;

    memcpy((void *) TOKEN_DATA(t), (void *) s, len + 1);
    // just to be sure
    TOKEN_DATA(t)[len] = 0;
    t->size = len;

    CHECK_PTR(t);
    return t;
}

/* using macro
token_t *new_token(str_t *s)
{
    // \0 is in u8 data field, no need to add +1
    size_t size = sizeof(token_t) + s->used;
    ALLOC(token_t, size, t);
    if (t == NULL) {
        FREE(t);
        raise("new token: could not allocate %zu", size);
        return NULL;
    }

    t->obj.type = token_type;
    t->obj.id = new_instance;

    memcpy((void *) &t->data, (void *) s->data, s->used + 1);
    // just to be sure
    ((char *) &t->data)[s->used] = 0;
    t->size = s->used;
    return t;
}
*/

str_t *new_str_c(const char *s, size_t len)
{
    if (len == 0)
        len = strlen(s);

    str_t *str = new_str(len, 0);

    memcpy((void *) str->data, (void *) s, len + 1);
    // just to be sure
    str->data[len] = 0;
    str->used = len;

    CHECK_PTR(str);
    return str;
}

#include "cons.h"
void test_token()
{
    puts("token_t");
    str_t *s = new_str(10, 0);
    for (int i=0; i<9; i++) {
        str_append(s, (char) '1'+i);
    }
    // create from str_t
    token_t *t = new_token(s);
    printf("%zu\n", t->size);
    puts(TOKEN_DATA(t));
    for (int i=0; i<5; i++) {
        char *s = "hello";
        token_t *t = new_token_c(s, 0);
        print_obj(t);
    }
}

void test_stoken()
{
    puts("stoken_t");
    stoken_t *st = new_stoken(10, 1);
    for (int i=0; i< 400; i++)
        stoken_append(st, (char) '#');
    printf("%zd\n", st->str.used);
    printf("%zd\n", st->str.size);
    printf("%d\n", st->str.data[st->str.used]);
    puts(st->str.data);
    stoken_t *stbig = new_stoken(10240, 1024);
    puts(stbig->str.data);
}
