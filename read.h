#ifndef __READ_H
#define __READ_H

#include "common.h"
#include "object.h"
#include "str.h"
#include "token.h"
#include "cons.h"

fixnum_t parse_num(const char *s);
int is_eof(str_t *code, size_t i);
size_t skip_comment(str_t *code, size_t i);
size_t skip_ws(str_t *code, size_t i);
str_t *read_str(str_t *code, size_t *i);
size_t seek_char(str_t *code, size_t i);
token_t *read_token(str_t *code, size_t *i, int *skip);
cons_t *read_sexp(str_t *code, size_t *i);

cons_t *read_code(str_t *code);

#endif
