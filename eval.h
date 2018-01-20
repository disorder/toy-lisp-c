#ifndef __EVAL_H
#define __EVAL_H

#include "common.h"
#include "cons.h"
#include "token.h"

extern cons_t *stack;
#define CTX ((cons_t *) car(stack))
#define CTX_NAME CTX->car
#define CTX_VARS (CTX->cdr)
//#define PUSH stack = cons(NIL, stack)
#define PUSH(NAME) stack = cons(cons(NAME, NIL), stack)
// TODO free and also prevent popping global?
#define POP stack = cdr(stack)

void bind(cons_t *arglist, cons_t *args);

void init_eval();
object_t *eval_token(object_t *token);
object_t *eval_sexp(cons_t *sexp);
void eval_code(cons_t *code, cons_t **results);
cons_t *def(cons_t *defs, token_t *name, object_t *obj);
void *getvar(const char *name, object_t **value);
void *getfun(const char *name);

#endif
