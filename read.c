#include "read.h"

#include <stdio.h>
#include <ctype.h>
#include <strings.h>
#include <inttypes.h>
#include <errno.h>

#define is_ws(c) (isspace(c) || c == 0)
#define is_nl(c) (c == '\n' || c == '\r')
// let's allow # and ; both for now but # breaks lisp syntax in editors
#define is_comment(c) (c == ';' || c == '#')
#define is_sexp(c) (c == '(')
#define is_end(c) (c == ')')
#define is_str(c) (c == '"' || c == '\'')
// TODO beware of empty string?  with str_t it's \0 and therefore false
// TODO FIXME negative numbers only through (- N)
#define is_num(token) (isdigit(token[0]))


fixnum_t parse_num(const char *s)
{
    // TODO float
    // TODO support bases
    // TODO FIXME silently returns 0
    intmax_t i = strtoimax(s, (char **) NULL, 10);
    if (errno) {
        errno = 0;
        raise("strtoimax error: %d", errno);
    }
    fixnum_t num = to_fixnum((size_t) i);
    debug("parse_num=%ju=%zx%zx\n", i, TAG(num), immediate(num));
    return num;
}

int is_eof(str_t *code, size_t i)
{
    if (i > code->size)
        raise("BUG: already reading past end!");

    if (i < code->size)
        return 0;
    else
        return 1;
}

#define CODE code->data
size_t next_char(str_t *code, size_t i)
{
    if (is_eof(code, i))
        raise("unexpected end");
    //debug("read[%c]\n", CODE[i]);
    return i+1;
}

size_t skip_comment(str_t *code, size_t i)
{
    debug("skip_comment at %zu\n", i); //{#{code.slice(i,10)]}}")
    if (!is_comment(CODE[i])) {
        raise("not comment"); //: #{code.slice(i,10)}")
        abort();
    }

    // consume #
    i = next_char(code, i);

    while (!is_eof(code, i) && !is_nl(CODE[i]))
        i = next_char(code, i);

    return i;
}

size_t skip_ws(str_t *code, size_t i)
{
    while (!is_eof(code, i) && is_ws(CODE[i]))
        i = next_char(code, i);
    return i;
}

str_t *read_str(str_t *code, size_t *i)
{
    debug("read_str at %zu\n", *i); //{#{code.slice(i,10)}}"
    if (!is_str(CODE[*i])) {
        raise("not str"); //: #{code.slice(i,10)}"
        abort();
    }

    // ' or "
    int quote = CODE[*i];
    // consume quote
    *i = next_char(code, *i);
    str_t *str = new_str(16, 8);
    int c;
    do {
        if (is_eof(code, *i)) {
            raise("unexpected end of string");
            abort();
        }

        c = CODE[*i];
        if (c == '\\') {
            // consume quote char
            *i = next_char(code, *i);
            switch (CODE[*i]) {
            case 'n':
                str_append(str, '\n');
                break;
            case 'r':
                str_append(str, '\r');
                break;
            case 't':
                str_append(str, '\t');
                break;
            case '0':
                str_append(str, '\0');
                break;
            default:
                str_append(str, CODE[*i]);
            }
            // consume quoted char
            *i = next_char(code, *i);
            continue;
        } else if (c != quote) {
            // normal char
            str_append(str, CODE[*i]);
            // consume char
            *i = next_char(code, *i);
            continue;
        } else
            // end quote
            break;
    } while (1);

    // consume quote
    *i = next_char(code, *i);
    debug("read_str=%s\n", str->data);
    return str;
}

size_t seek_char(str_t *code, size_t i)
{
    while (!is_eof(code, i)) {
        // skip whitespace
        i = skip_ws(code, i);

        // skip comment
        if (is_eof(code, i))
            break;
        else if (is_comment(CODE[i])) {
            i = skip_comment(code, i);
            continue;
        } else
            // finally an important char
            return i;
    }

    // EOF
    return i;
}

token_t *read_token(str_t *code, size_t *i, int *skip)
{
    *skip = 0;
    debug("read_token at %zu\n", *i); //{#{code.slice(i,10)}}"
    *i = seek_char(code, *i);

    str_t *str = new_str(16, 8);
    int c = CODE[*i];
    if (is_sexp(c)) {
        cons_t *sexp = read_sexp(code, i);
        return sexp;
    } else if (is_str(c)) {
        str_t *str = read_str(code, i);
        return str;
    } else {
        // read until whitespace, comment or another sexp starts
        while (!is_eof(code, *i) && !is_ws(c) && !is_comment(c) && !is_end(c)) {
            str_append(str, c);
            *i = next_char(code, *i);
            // this will try to fetch out of boundaries
            c = CODE[*i];
        }
    }

    if (0 == strncasecmp("NIL", str->data, 3)) {
        return (token_t *) NIL;
    }

    if (is_num(str->data)) {
        // if we don't do this we can reassign numbers
        // TODO only fixnum?
        fixnum_t num = parse_num(str->data);
        // not object_t/token_t but immediate type
        return (token_t *) num;
    }

    // no more tokens
    if (str->used == 0) {
        debug("read_token=NULL (no token)\n");
        // NIL is valid token, signalize skip
        *skip = 1;
        return (token_t *) NULL;
    } else {
        token_t *token = new_token(str);
        debug("read_token=%s\n", TOKEN_DATA(token));
        FREE(str);
        return (token_t *) token;
    }
}

cons_t *read_sexp(str_t *code, size_t *i)
{
    debug("read_sexp at %zu\n" , *i); //{#{code.slice(i,10)}}"
    if (!is_sexp(CODE[*i])) {
        raise("not sexp"); //: #{code.slice(i,10)}"
        abort();
    }

    // consume (
    *i = next_char(code, *i);

    cons_t *sexp = NIL;
    while (!is_eof(code, *i) && !is_end(CODE[*i])) {
        int skip = 0;
        token_t *token = read_token(code, i, &skip);
        if (!skip) {
            // beware of this, optimization may bite again
            sexp = cons_append(sexp, (object_t *) token);
            /* or like this
            if (nilp(sexp))
                sexp = cons(token, NIL);
            else
                cons_append(sexp, (object_t *) token);
            */
        }
    }

    // consume )
    *i = next_char(code, *i);

    debug("read_sexp=%s:\n", dump_obj(sexp));
    return sexp;
}

cons_t *read_code(str_t *code)
{
    cons_t *tokens = NIL;

    size_t i = 0;
    while (!is_eof(code, i)) {
        int skip = 0;
        token_t *token = read_token(code, &i, &skip);
        // NIL is valid token
        if (skip)
            break;
        // TODO beware of this, optimization may bite again
        tokens = cons_append(tokens, (object_t *) token);
    }

    return tokens;
}
